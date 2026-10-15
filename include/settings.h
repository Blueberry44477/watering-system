#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"

#define EEPROM_SIZE        1024
#define SETTINGS_MAGIC     0x38524C59 // "8RLY" in ASCII
#define SETTINGS_VERSION   2

struct ZoneSettings {
    char     name[24];
    bool     enabled;
    uint32_t intervalSec;
    uint16_t durationSec;
};

struct SystemSettings {
    uint32_t     magic;
    uint8_t      version;
    char         wifiSsid[33];
    char         wifiPass[65];
    char         apPass[33];
    char         hostname[33];
    ZoneSettings zones[NUM_RELAYS];
    uint8_t      checksum;
};

class SettingsManager {
public:
    static SystemSettings data;

    static void init() {
        EEPROM.begin(EEPROM_SIZE);
        if (!load()) {
            Serial.println(F("[EEPROM] Initializing defaults."));
            factoryReset();
        } else {
            Serial.println(F("[EEPROM] Settings loaded successfully."));
            // Auto-update Wi-Fi if credentials in config.h were modified
            if (strcmp(DEFAULT_STA_SSID, "YOUR_WIFI_SSID") != 0 &&
                (strcmp(data.wifiSsid, DEFAULT_STA_SSID) != 0 || strcmp(data.wifiPass, DEFAULT_STA_PASS) != 0)) {
                Serial.println(F("[EEPROM] New Wi-Fi credentials detected in config.h. Updating..."));
                strncpy(data.wifiSsid, DEFAULT_STA_SSID, sizeof(data.wifiSsid) - 1);
                data.wifiSsid[sizeof(data.wifiSsid) - 1] = '\0';
                strncpy(data.wifiPass, DEFAULT_STA_PASS, sizeof(data.wifiPass) - 1);
                data.wifiPass[sizeof(data.wifiPass) - 1] = '\0';
                save();
            }
        }
    }

    static void factoryReset() {
        data.magic = SETTINGS_MAGIC;
        data.version = SETTINGS_VERSION;
        strncpy(data.wifiSsid, DEFAULT_STA_SSID, sizeof(data.wifiSsid) - 1);
        data.wifiSsid[sizeof(data.wifiSsid) - 1] = '\0';

        strncpy(data.wifiPass, DEFAULT_STA_PASS, sizeof(data.wifiPass) - 1);
        data.wifiPass[sizeof(data.wifiPass) - 1] = '\0';

        strncpy(data.apPass, DEFAULT_AP_PASS, sizeof(data.apPass) - 1);
        data.apPass[sizeof(data.apPass) - 1] = '\0';

        strncpy(data.hostname, DEFAULT_MDNS_NAME, sizeof(data.hostname) - 1);
        data.hostname[sizeof(data.hostname) - 1] = '\0';

        for (uint8_t i = 0; i < NUM_RELAYS; i++) {
            snprintf(data.zones[i].name, sizeof(data.zones[i].name), "Zone %d", i + 1);
            data.zones[i].enabled = true;
            data.zones[i].intervalSec = DEFAULT_INTERVAL_SEC;     // 12 hours
            data.zones[i].durationSec = DEFAULT_WATER_DURATION_SEC; // 10 seconds
        }

        save();
    }

    static bool save() {
        data.magic = SETTINGS_MAGIC;
        data.version = SETTINGS_VERSION;
        data.checksum = calculateChecksum();

        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);
        for (size_t i = 0; i < sizeof(SystemSettings); i++) {
            EEPROM.write(i, ptr[i]);
        }
        bool success = EEPROM.commit();
        if (success) {
            Serial.println(F("[EEPROM] 8-Relay settings saved."));
        } else {
            Serial.println(F("[EEPROM] Error saving settings."));
        }
        return success;
    }

    static bool load() {
        SystemSettings temp;
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&temp);
        for (size_t i = 0; i < sizeof(SystemSettings); i++) {
            ptr[i] = EEPROM.read(i);
        }

        if (temp.magic != SETTINGS_MAGIC || temp.version != SETTINGS_VERSION) {
            return false;
        }

        uint8_t computed = 0;
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&temp);
        for (size_t i = 0; i < sizeof(SystemSettings) - 1; i++) {
            computed ^= p[i];
        }

        if (computed != temp.checksum) {
            Serial.println(F("[EEPROM] Checksum mismatch."));
            return false;
        }

        data = temp;
        return true;
    }

private:
    static uint8_t calculateChecksum() {
        uint8_t cs = 0;
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&data);
        for (size_t i = 0; i < sizeof(SystemSettings) - 1; i++) {
            cs ^= p[i];
        }
        return cs;
    }
};

__attribute__((unused)) inline SystemSettings SettingsManager::data;
