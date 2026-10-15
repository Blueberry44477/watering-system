#pragma once
#include <Arduino.h>

// ==========================================
// 8-Channel Relay Module Pin Mapping for NodeMCU
// ==========================================
// ESP8266 Safe Pins for 8-Channel Relay Module:
// - D1 (GPIO5)  : Safe
// - D2 (GPIO4)  : Safe
// - D5 (GPIO14) : Safe
// - D6 (GPIO12) : Safe
// - D7 (GPIO13) : Safe
// - D0 (GPIO16) : Safe output (no pullup)
// - D3 (GPIO0)  : Safe with active-low relay (10k pullup onboard)
// - D4 (GPIO2)  : Safe with active-low relay (10k pullup onboard, built-in LED)

#define NUM_RELAYS 8

const uint8_t RELAY_PINS[NUM_RELAYS] = {
    D1, // Relay 1 (GPIO5)
    D2, // Relay 2 (GPIO4)
    D5, // Relay 3 (GPIO14)
    D6, // Relay 4 (GPIO12)
    D7, // Relay 5 (GPIO13)
    D0, // Relay 6 (GPIO16)
    D3, // Relay 7 (GPIO0)
    D4  // Relay 8 (GPIO2)
};

// ==========================================
// Relay Hardware Polarity
// ==========================================
// Most standard 8-channel relay boards are ACTIVE_LOW
#define RELAY_ACTIVE       LOW
#define RELAY_INACTIVE     HIGH

// Built-in status LED (D4 / GPIO2 or inverted)
#define PIN_STATUS_LED     LED_BUILTIN

// ==========================================
// Safety Limits & Defaults
// ==========================================
#define ABSOLUTE_MAX_RUN_SEC      1800  // Maximum allowable run time per relay (30 min watchdog)
#define DEFAULT_WATER_DURATION_SEC  10  // Default manual water duration (10 seconds)
#define DEFAULT_INTERVAL_HOURS      12  // Default schedule: every 12 hours
#define DEFAULT_INTERVAL_SEC     43200  // 12 hours in seconds
#define SEQUENTIAL_GAP_MS         1500  // Delay between switching relays in sequential cycle

// ==========================================
// Wi-Fi Configuration Defaults
// ==========================================
#define DEFAULT_AP_SSID    "Watering-8Relay-Setup"
#define DEFAULT_AP_PASS    "water1234"
#define DEFAULT_MDNS_NAME  "watering"   // http://watering.local

#define DEFAULT_STA_SSID   "YOUR_WIFI_SSID"
#define DEFAULT_STA_PASS   R"raw(YOUR_WIFI_PASSWORD)raw"