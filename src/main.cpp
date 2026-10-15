#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

#include "config.h"
#include "settings.h"
#include "web_ui.h"

ESP8266WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// ==========================================
// Relay Channel Runtime State
// ==========================================
struct RelayRuntime {
    bool          active;
    unsigned long startedAtMs;
    unsigned long targetDurationMs; // 0 = indefinite (capped by safety watchdog)
    unsigned long lastRunTimestamp;
};

RelayRuntime relayStates[NUM_RELAYS];

// Sequential Cycle State
bool          cycleRunning = false;
int8_t        cycleCurrentZone = -1;
bool          cycleInGap = false;
unsigned long cycleGapStartedAt = 0;

bool isAccessPointMode = false;

// Forward declarations
void handleRoot();
void handleStatus();
void handleControlRelay();
void handleStartCycle();
void handleStopAll();
void handleUpdateSettings();
void handleWiFiConfig();
void handleRestart();
void setupWiFi();
void setRelay(uint8_t index, bool state, uint32_t durationSec = 0);
void stopAllRelays();
void processCycle();
String getUptimeString();

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("   NodeMCU 8-Channel Relay Irrigation   "));
    Serial.println(F("========================================"));

    // CRITICAL: Initialize output state before setting pinMode
    // to prevent relay chattering/pulsing on ESP8266 boot
    for (uint8_t i = 0; i < NUM_RELAYS; i++) {
        digitalWrite(RELAY_PINS[i], RELAY_INACTIVE);
        pinMode(RELAY_PINS[i], OUTPUT);

        relayStates[i].active = false;
        relayStates[i].startedAtMs = 0;
        relayStates[i].targetDurationMs = 0;
        relayStates[i].lastRunTimestamp = millis(); // Offset initial automated trigger
    }

    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, HIGH); // LED OFF

    // Load EEPROM configuration
    SettingsManager::init();

    // Connect to Wi-Fi or create SoftAP
    setupWiFi();

    // Setup HTTP routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/relay", HTTP_POST, handleControlRelay);
    server.on("/api/cycle", HTTP_POST, handleStartCycle);
    server.on("/api/stop_all", HTTP_POST, handleStopAll);
    server.on("/api/settings", HTTP_POST, handleUpdateSettings);
    server.on("/api/wifi", HTTP_POST, handleWiFiConfig);
    server.on("/api/restart", HTTP_POST, handleRestart);

    server.enableCORS(true);
    server.begin();
    Serial.println(F("[HTTP] 8-Relay Web Server started on port 80"));
}

void loop() {
    if (isAccessPointMode) {
        dnsServer.processNextRequest();
    }
    server.handleClient();
    MDNS.update();

    unsigned long currentMillis = millis();

    // 1. Process Sequential Cycle if running
    if (cycleRunning) {
        processCycle();
    }

    // 2. Individual Relay Durations & Safety Watchdog
    for (uint8_t i = 0; i < NUM_RELAYS; i++) {
        if (relayStates[i].active) {
            unsigned long elapsed = currentMillis - relayStates[i].startedAtMs;

            // Check if timed run expired
            if (relayStates[i].targetDurationMs > 0 && elapsed >= relayStates[i].targetDurationMs) {
                Serial.printf("[RELAY %d] Timed duration completed (%lu ms)\n", i + 1, relayStates[i].targetDurationMs);
                setRelay(i, false);
            }
            // Absolute safety watchdog cutoff
            else if (elapsed >= (ABSOLUTE_MAX_RUN_SEC * 1000UL)) {
                Serial.printf("[SAFETY] Relay %d exceeded max safety limit. Forcing OFF.\n", i + 1);
                setRelay(i, false);
            }
        }
    }

    // 3. Automated Per-Zone Interval Schedules (only if not in a sequential cycle)
    if (!cycleRunning) {
        for (uint8_t i = 0; i < NUM_RELAYS; i++) {
            if (SettingsManager::data.zones[i].enabled && !relayStates[i].active) {
                unsigned long intervalMs = (unsigned long)SettingsManager::data.zones[i].intervalSec * 1000UL;
                if (currentMillis - relayStates[i].lastRunTimestamp >= intervalMs) {
                    Serial.printf("[AUTO] Zone %d timer triggered (%ds run)\n", i + 1, SettingsManager::data.zones[i].durationSec);
                    setRelay(i, true, SettingsManager::data.zones[i].durationSec);
                }
            }
        }
    }
}

// ==========================================
// Relay Actuation & Sequential Cycle
// ==========================================
void setRelay(uint8_t index, bool state, uint32_t durationSec) {
    if (index >= NUM_RELAYS) return;

    if (state) {
        digitalWrite(RELAY_PINS[index], RELAY_ACTIVE);
        relayStates[index].active = true;
        relayStates[index].startedAtMs = millis();
        relayStates[index].targetDurationMs = durationSec > 0 ? ((unsigned long)durationSec * 1000UL) : 0;
        digitalWrite(PIN_STATUS_LED, LOW); // LED ON
        Serial.printf("[RELAY %d ON] Duration: %u sec\n", index + 1, durationSec);
    } else {
        digitalWrite(RELAY_PINS[index], RELAY_INACTIVE);
        relayStates[index].active = false;
        relayStates[index].targetDurationMs = 0;
        relayStates[index].lastRunTimestamp = millis();

        // Check if any relay is still active to decide status LED
        bool anyActive = false;
        for (uint8_t j = 0; j < NUM_RELAYS; j++) {
            if (relayStates[j].active) anyActive = true;
        }
        if (!anyActive) {
            digitalWrite(PIN_STATUS_LED, HIGH); // LED OFF
        }
        Serial.printf("[RELAY %d OFF]\n", index + 1);
    }
}

void stopAllRelays() {
    cycleRunning = false;
    cycleCurrentZone = -1;
    cycleInGap = false;

    for (uint8_t i = 0; i < NUM_RELAYS; i++) {
        setRelay(i, false);
    }
    Serial.println(F("[RELAY] All 8 relays forced OFF."));
}

void processCycle() {
    unsigned long currentMillis = millis();

    // If waiting between zones (gap)
    if (cycleInGap) {
        if (currentMillis - cycleGapStartedAt >= SEQUENTIAL_GAP_MS) {
            cycleInGap = false;
            // Advance to next zone
            cycleCurrentZone++;
            while (cycleCurrentZone < NUM_RELAYS && !SettingsManager::data.zones[cycleCurrentZone].enabled) {
                cycleCurrentZone++;
            }

            if (cycleCurrentZone >= NUM_RELAYS) {
                // Cycle complete
                cycleRunning = false;
                cycleCurrentZone = -1;
                Serial.println(F("[CYCLE] Sequential watering cycle finished!"));
            } else {
                // Activate next zone
                uint16_t dur = SettingsManager::data.zones[cycleCurrentZone].durationSec;
                setRelay(cycleCurrentZone, true, dur);
            }
        }
        return;
    }

    // Active zone running in cycle
    if (cycleCurrentZone >= 0 && cycleCurrentZone < NUM_RELAYS) {
        if (!relayStates[cycleCurrentZone].active) {
            // Zone finished, start gap
            cycleInGap = true;
            cycleGapStartedAt = currentMillis;
        }
    }
}

// ==========================================
// Network Setup
// ==========================================
static const char* wifiStatusToString(wl_status_t status) {
    switch (status) {
        case WL_IDLE_STATUS:      return "IDLE (0)";
        case WL_NO_SSID_AVAIL:    return "NO_SSID_AVAIL (1 - Network not found / 5GHz only)";
        case WL_SCAN_COMPLETED:   return "SCAN_COMPLETED (2)";
        case WL_CONNECTED:        return "CONNECTED (3)";
        case WL_CONNECT_FAILED:   return "CONNECT_FAILED (4)";
        case WL_CONNECTION_LOST:  return "CONNECTION_LOST (5)";
        case WL_WRONG_PASSWORD:   return "WRONG_PASSWORD (6 - Incorrect Wi-Fi Password!)";
        case WL_DISCONNECTED:     return "DISCONNECTED (7)";
        default:                  return "UNKNOWN_STATUS";
    }
}

void setupWiFi() {
    WiFi.disconnect();
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(SettingsManager::data.hostname);

    bool hasCredentials = (strlen(SettingsManager::data.wifiSsid) > 0 &&
                          strcmp(SettingsManager::data.wifiSsid, "YOUR_WIFI_SSID") != 0);

    if (hasCredentials) {
        Serial.print(F("[WIFI] Connecting to SSID: \""));
        Serial.print(SettingsManager::data.wifiSsid);
        Serial.println(F("\" ..."));

        WiFi.begin(SettingsManager::data.wifiSsid, SettingsManager::data.wifiPass);

        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 20000) {
            delay(500);
            Serial.print(F("."));
        }
        Serial.println();
    } else {
        Serial.println(F("[WIFI] No configured Wi-Fi credentials found."));
    }

    if (WiFi.status() == WL_CONNECTED) {
        isAccessPointMode = false;
        Serial.println(F("[WIFI] Connected successfully!"));
        Serial.print(F("[WIFI] IP Address : "));
        Serial.println(WiFi.localIP());
        Serial.print(F("[WIFI] Gateway    : "));
        Serial.println(WiFi.gatewayIP());
        Serial.print(F("[WIFI] Signal RSSI: "));
        Serial.print(WiFi.RSSI());
        Serial.println(F(" dBm"));

        if (MDNS.begin(SettingsManager::data.hostname)) {
            Serial.print(F("[MDNS] Host: http://"));
            Serial.print(SettingsManager::data.hostname);
            Serial.println(F(".local"));
            MDNS.addService("http", "tcp", 80);
        }
    } else {
        isAccessPointMode = true;
        if (hasCredentials) {
            Serial.print(F("[WIFI] Connection failed. Reason: "));
            Serial.println(wifiStatusToString(WiFi.status()));
        }

        WiFi.mode(WIFI_AP);
        String apName = String(DEFAULT_AP_SSID) + "-" + String(ESP.getChipId(), HEX);
        WiFi.softAP(apName.c_str(), SettingsManager::data.apPass);

        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

        MDNS.begin(SettingsManager::data.hostname);
        MDNS.addService("http", "tcp", 80);

        Serial.println(F("[WIFI] Started fallback SoftAP mode!"));
        Serial.print(F("[WIFI] AP SSID: "));
        Serial.println(apName);
        Serial.print(F("[WIFI] AP IP  : "));
        Serial.println(WiFi.softAPIP());
    }
}

// ==========================================
// HTTP Route Handlers
// ==========================================
void handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatus() {
    JsonDocument doc;
    doc["ip"] = isAccessPointMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = getUptimeString();
    doc["cycleRunning"] = cycleRunning;

    JsonArray relaysArr = doc["relays"].to<JsonArray>();
    unsigned long currentMillis = millis();

    for (uint8_t i = 0; i < NUM_RELAYS; i++) {
        JsonObject r = relaysArr.add<JsonObject>();
        r["name"] = SettingsManager::data.zones[i].name;
        r["active"] = relayStates[i].active;
        r["autoEnabled"] = SettingsManager::data.zones[i].enabled;
        r["intervalSec"] = SettingsManager::data.zones[i].intervalSec;
        r["durationSec"] = SettingsManager::data.zones[i].durationSec;

        // Remaining active seconds
        if (relayStates[i].active && relayStates[i].targetDurationMs > 0) {
            unsigned long elapsed = currentMillis - relayStates[i].startedAtMs;
            if (elapsed < relayStates[i].targetDurationMs) {
                r["remainingSec"] = (relayStates[i].targetDurationMs - elapsed) / 1000 + 1;
            } else {
                r["remainingSec"] = 0;
            }
        } else {
            r["remainingSec"] = 0;
        }

        // Seconds until next auto run
        if (SettingsManager::data.zones[i].enabled) {
            unsigned long intervalMs = (unsigned long)SettingsManager::data.zones[i].intervalSec * 1000UL;
            unsigned long elapsedSinceLast = currentMillis - relayStates[i].lastRunTimestamp;
            if (elapsedSinceLast < intervalMs) {
                r["nextRunSec"] = (intervalMs - elapsedSinceLast) / 1000UL;
            } else {
                r["nextRunSec"] = 0;
            }
        } else {
            r["nextRunSec"] = 0;
        }
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleControlRelay() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    uint8_t index = doc["index"] | 255;
    if (index >= NUM_RELAYS) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid relay index\"}");
        return;
    }

    String action = doc["action"] | "";
    if (action == "on") {
        setRelay(index, true, 0); // Indefinite, capped by watchdog
    } else if (action == "off") {
        setRelay(index, false);
    } else if (action == "pulse") {
        uint32_t dur = doc["duration"] | SettingsManager::data.zones[index].durationSec;
        setRelay(index, true, dur);
    } else {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Unknown action\"}");
        return;
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleStartCycle() {
    stopAllRelays(); // Turn off any currently running relays

    // Find first enabled zone
    cycleCurrentZone = 0;
    while (cycleCurrentZone < NUM_RELAYS && !SettingsManager::data.zones[cycleCurrentZone].enabled) {
        cycleCurrentZone++;
    }

    if (cycleCurrentZone >= NUM_RELAYS) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No zones enabled for cycle\"}");
        return;
    }

    cycleRunning = true;
    cycleInGap = false;
    uint16_t dur = SettingsManager::data.zones[cycleCurrentZone].durationSec;
    setRelay(cycleCurrentZone, true, dur);

    Serial.printf("[CYCLE] Started sequential cycle starting at Zone %d\n", cycleCurrentZone + 1);
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Cycle started\"}");
}

void handleStopAll() {
    stopAllRelays();
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"All relays stopped\"}");
}

void handleUpdateSettings() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    uint8_t index = doc["index"] | 255;
    if (index >= NUM_RELAYS) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid index\"}");
        return;
    }

    if (doc["name"].is<const char*>()) {
        const char* newName = doc["name"];
        strncpy(SettingsManager::data.zones[index].name, newName, sizeof(SettingsManager::data.zones[index].name) - 1);
        SettingsManager::data.zones[index].name[sizeof(SettingsManager::data.zones[index].name) - 1] = '\0';
    }
    if (doc["durationSec"].is<uint16_t>()) {
        SettingsManager::data.zones[index].durationSec = constrain(doc["durationSec"].as<uint16_t>(), 1, ABSOLUTE_MAX_RUN_SEC);
    }
    if (doc["intervalSec"].is<uint32_t>()) {
        SettingsManager::data.zones[index].intervalSec = constrain(doc["intervalSec"].as<uint32_t>(), 30, 2592000UL); // min 30s, max 30 days
    }
    if (doc["autoEnabled"].is<bool>()) {
        SettingsManager::data.zones[index].enabled = doc["autoEnabled"].as<bool>();
    }

    SettingsManager::save();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleWiFiConfig() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    const char* newSsid = doc["ssid"] | "";
    const char* newPass = doc["pass"] | "";

    if (strlen(newSsid) == 0) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID empty\"}");
        return;
    }

    strncpy(SettingsManager::data.wifiSsid, newSsid, sizeof(SettingsManager::data.wifiSsid) - 1);
    SettingsManager::data.wifiSsid[sizeof(SettingsManager::data.wifiSsid) - 1] = '\0';

    strncpy(SettingsManager::data.wifiPass, newPass, sizeof(SettingsManager::data.wifiPass) - 1);
    SettingsManager::data.wifiPass[sizeof(SettingsManager::data.wifiPass) - 1] = '\0';

    SettingsManager::save();
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Reconnecting...\"}");
    delay(1000);
    ESP.restart();
}

void handleRestart() {
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Restarting...\"}");
    delay(1000);
    ESP.restart();
}

String getUptimeString() {
    unsigned long sec = millis() / 1000;
    unsigned long min = sec / 60;
    unsigned long hr = min / 60;
    unsigned long days = hr / 24;

    char buf[32];
    if (days > 0) {
        snprintf(buf, sizeof(buf), "%lud %luh %lum", days, hr % 24, min % 60);
    } else if (hr > 0) {
        snprintf(buf, sizeof(buf), "%luh %lum %lus", hr, min % 60, sec % 60);
    } else {
        snprintf(buf, sizeof(buf), "%lum %lus", min % 60, sec % 60);
    }
    return String(buf);
}
