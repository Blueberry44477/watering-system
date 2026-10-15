# 🌿 NodeMCU (ESP8266) 8-Channel Relay Automatic Irrigation System

A robust, sensorless multi-zone watering and irrigation controller designed for the **NodeMCU (ESP8266)** board paired with an **8-Channel Relay Module**. Features an embedded responsive Web UI, independent per-zone timer schedules, automated sequential cycling (to preserve water pressure and power), master safety controls, and boot glitch protection.

---

## ⚡ Key Features

- **🚫 100% Sensorless Operation**: Reliable timer-based and schedule-based irrigation eliminating faulty or corroding soil probes.
- **🎛️ 8 Independent Zones**:
  - Individual zone labeling (e.g. *Lawn*, *Balcony Pots*, *Greenhouse Beds*).
  - Configurable watering duration per zone (seconds or minutes).
  - Configurable interval schedule per zone (e.g. every 6h, 12h, 24h, 48h).
  - Live countdown timers showing active run duration and time until next scheduled run.
- **▶️ Sequential Cycle Mode ("Run All Zones")**:
  - Automatically waters active zones one after another with an anti-hammer safety gap (`1.5s`).
  - Solves the problem of running 8 pumps simultaneously (prevents water pressure drop and power supply brownouts).
- **🛑 Emergency Master Stop**: Single-click master kill switch to turn off all 8 channels immediately.
- **⚡ Boot Glitch Prevention**: All 8 relay outputs are held in inactive state *before* GPIO pin configuration to prevent relays from briefly triggering during NodeMCU boot/reboot.
- **🛡️ Runaway Safety Watchdogs**: Hard maximum run time limit (default 30 min) per relay prevents flooding even if browser or network disconnects.
- **🌐 Zero-Dependency Web UI**: Modern responsive single-page dashboard stored in flash (`PROGMEM`), fully usable without internet connectivity.
- **📡 Wi-Fi Manager & SoftAP Fallback**: Broadcasts setup Access Point `Watering-8Relay-Setup-XXXX` on `192.168.4.1` if home Wi-Fi credentials are not yet set.

---

## 🔌 Hardware Wiring & Pin Mapping

ESP8266 GPIOs have specific boot state requirements. The following 8 pins have been carefully mapped to ensure 100% reliable booting with standard active-low 8-channel relay breakout boards:

| Relay Channel | NodeMCU Pin | ESP8266 GPIO | Boot Behavior & Electrical Notes |
| :---: | :---: | :---: | :--- |
| **Relay 1** | `D1` | `GPIO5` | Safe boot pin; standard I2C SCL |
| **Relay 2** | `D2` | `GPIO4` | Safe boot pin; standard I2C SDA |
| **Relay 3** | `D5` | `GPIO14` | Safe boot pin; SPI CLK |
| **Relay 4** | `D6` | `GPIO12` | Safe boot pin; SPI MISO |
| **Relay 5** | `D7` | `GPIO13` | Safe boot pin; SPI MOSI |
| **Relay 6** | `D0` | `GPIO16` | Safe boot output; wake pin |
| **Relay 7** | `D3` | `GPIO0` | Safe with active-low relay (10k pullup onboard keeps pin HIGH for boot) |
| **Relay 8** | `D4` | `GPIO2` | Safe with active-low relay (10k pullup onboard keeps pin HIGH for boot) |

### ⚠️ Power Supply Notice (Critical for 8 Relays)

1. **Relay Coil Current**: An 8-channel 5V relay module draws ~560mA–600mA when all 8 coils are energized simultaneously.
2. **DO NOT power the relay board from the NodeMCU 3.3V pin** (the onboard 3.3V linear regulator will overheat and burn out).
3. **Power Architecture**:
   - Use an external **5V 2A (or 3A)** DC power supply.
   - Connect the external 5V to the relay board `VCC` and NodeMCU `VIN`.
   - Connect the external power supply `GND` to both NodeMCU `GND` and relay board `GND` (common ground).
   - If using the JD-VCC jumper on the relay board, remove the jumper and supply 5V directly to `JD-VCC` and `GND` to optically isolate the NodeMCU.

---

## 💻 Uploading Firmware with VS Code (Step-by-Step)

The project is built with **PlatformIO**, the standard professional embedded IDE in Visual Studio Code.

### Step 1: Install PlatformIO in VS Code
1. Open **Visual Studio Code**.
2. Go to the **Extensions** panel (`Cmd + Shift + X` on macOS or `Ctrl + Shift + X` on Windows).
3. Search for **PlatformIO IDE** and click **Install**.

### Step 2: Open Project
1. In VS Code, click **File > Open Folder...**.
2. Select the `watering-system` directory (`/Users/blueberry/workspace/programming/embedded/watering-system`).
3. PlatformIO will automatically recognize [`platformio.ini`](file:///Users/blueberry/workspace/programming/embedded/watering-system/platformio.ini).

### Step 3: Connect NodeMCU & Flash
1. Plug your NodeMCU into your computer using a micro-USB data cable.
2. Click the **Upload arrow (`→`)** in the bottom blue status bar of VS Code (or press `Cmd+Option+U` / `Ctrl+Alt+U`).
3. PlatformIO compiles and uploads the binary at 921,600 baud.
4. Click the **Serial Monitor plug (`🔌`)** in the status bar (115200 baud) to view boot logs.

---

## 🚀 Initial Web Setup

1. **Connect to Access Point**:
   - On first boot, connect your phone or PC to Wi-Fi network: **`Watering-8Relay-Setup-XXXX`**.
   - **Password**: `water1234`
2. **Access Web UI**:
   - Open your browser to **`http://192.168.4.1`**.
3. **Connect to Home Wi-Fi**:
   - Switch to the **Network & System** tab.
   - Enter your Wi-Fi SSID and Password and click **Save & Reconnect**.
   - The device will restart and connect to your home network at **`http://watering.local`**.
4. **Configure Zones**:
   - On the **Zones Overview** tab, click **⚙️ Edit Schedule** on each zone card to set custom names (e.g. "Balcony Tomato", "Drip Line"), run durations (e.g. 15s), and intervals (e.g. Every 12 Hours).
   - Use the **Schedule Timer** toggle on each card to enable or disable automatic runs.
   - Use **▶️ Run Sequential Cycle** whenever you want to trigger a full irrigation routine across all active zones one by one.

---

## 📡 REST API Reference

| Endpoint | Method | JSON Payload | Description |
| :--- | :--- | :--- | :--- |
| `GET /` | `GET` | - | Serves responsive single-page Web UI |
| `GET /api/status` | `GET` | - | Returns full status of all 8 relays, uptime, and IP |
| `POST /api/relay` | `POST` | `{"index": 0, "action": "on"\|"off"\|"pulse", "duration": 15}` | Controls an individual relay |
| `POST /api/cycle` | `POST` | - | Starts sequential watering cycle across all active zones |
| `POST /api/stop_all` | `POST` | - | Immediately turns off all 8 relays and halts active cycles |
| `POST /api/settings`| `POST` | `{"index": 0, "name": "Zone 1", "durationSec": 20, "intervalSec": 43200, "autoEnabled": true}` | Updates zone schedule and name |
| `POST /api/wifi` | `POST` | `{"ssid": "MyWiFi", "pass": "Secret"}` | Configures Wi-Fi credentials & reboots |
| `POST /api/restart`| `POST` | - | Reboots the NodeMCU |
