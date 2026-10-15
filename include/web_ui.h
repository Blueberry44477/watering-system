#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">
  <title>8-Relay Smart Irrigation</title>
  <style>
    :root {
      --bg: #0b1118;
      --card-bg: #141c26;
      --card-border: #222f3e;
      --primary: #10b981;
      --primary-hover: #059669;
      --accent: #38bdf8;
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --danger: #ef4444;
      --danger-hover: #dc2626;
      --warning: #f59e0b;
      --active-relay: #2563eb;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: var(--bg); color: var(--text); padding: 16px; min-height: 100vh; display: flex; justify-content: center; }
    .container { width: 100%; max-width: 900px; display: flex; flex-direction: column; gap: 16px; }
    
    /* Header */
    header { display: flex; justify-content: space-between; align-items: center; padding: 14px 18px; background: var(--card-bg); border-radius: 14px; border: 1px solid var(--card-border); flex-wrap: wrap; gap: 10px; }
    .brand { display: flex; align-items: center; gap: 10px; font-weight: 700; font-size: 1.15rem; color: #fff; }
    .brand span { color: var(--primary); }
    .status-badge { display: inline-flex; align-items: center; gap: 6px; font-size: 0.75rem; padding: 4px 10px; border-radius: 999px; background: rgba(16,185,129,0.15); color: var(--primary); }
    .status-badge.offline { background: rgba(239,68,68,0.15); color: var(--danger); }
    .status-badge.active-pulse { background: rgba(37,99,235,0.25); color: #60a5fa; animation: pulse 1.5s infinite; }
    @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }
    .status-badge .dot { width: 8px; height: 8px; border-radius: 50%; background: currentColor; }

    /* Master Action Bar */
    .master-bar { display: flex; gap: 10px; flex-wrap: wrap; }
    button { padding: 10px 16px; font-size: 0.88rem; font-weight: 600; border: none; border-radius: 8px; cursor: pointer; transition: all 0.2s; display: inline-flex; align-items: center; justify-content: center; gap: 6px; }
    .btn-primary { background: var(--primary); color: #022c22; }
    .btn-primary:hover { background: var(--primary-hover); }
    .btn-danger { background: var(--danger); color: #fff; }
    .btn-danger:hover { background: var(--danger-hover); }
    .btn-secondary { background: #222f3e; color: var(--text); }
    .btn-secondary:hover { background: #2c3e53; }
    .btn-sm { padding: 6px 10px; font-size: 0.78rem; border-radius: 6px; }

    /* Tabs */
    .tabs { display: flex; border-bottom: 1px solid var(--card-border); margin-bottom: 12px; gap: 6px; }
    .tab-btn { background: none; border: none; color: var(--text-muted); padding: 8px 16px; font-weight: 600; font-size: 0.85rem; border-bottom: 2px solid transparent; border-radius: 0; }
    .tab-btn.active { color: var(--primary); border-bottom-color: var(--primary); }
    .tab-pane { display: none; }
    .tab-pane.active { display: block; }

    /* 8-Zone Grid */
    .zone-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(280px, 1fr)); gap: 14px; }
    .zone-card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 12px; padding: 16px; display: flex; flex-direction: column; gap: 12px; position: relative; transition: border-color 0.2s; }
    .zone-card.active { border-color: #3b82f6; box-shadow: 0 0 15px rgba(59,130,246,0.15); }
    
    .zone-head { display: flex; justify-content: space-between; align-items: center; }
    .zone-title { font-weight: 700; font-size: 1rem; color: #fff; display: flex; align-items: center; gap: 8px; }
    .zone-num { font-size: 0.75rem; background: #222f3e; padding: 2px 6px; border-radius: 4px; color: var(--text-muted); }

    /* Quick water button row */
    .quick-row { display: flex; gap: 6px; }
    .quick-row button { flex: 1; }

    /* Schedule block inside zone card */
    .zone-schedule { background: rgba(255,255,255,0.02); border: 1px solid var(--card-border); border-radius: 8px; padding: 10px; font-size: 0.8rem; display: flex; flex-direction: column; gap: 6px; }
    .schedule-header { display: flex; justify-content: space-between; align-items: center; }
    .schedule-timing { display: flex; justify-content: space-between; color: var(--text-muted); font-size: 0.75rem; }

    /* Switch */
    .switch { position: relative; display: inline-block; width: 44px; height: 24px; }
    .switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #334155; transition: .25s; border-radius: 24px; }
    .slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: white; transition: .25s; border-radius: 50%; }
    input:checked + .slider { background-color: var(--primary); }
    input:checked + .slider:before { transform: translateX(20px); }

    /* Forms */
    .card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 12px; padding: 18px; }
    .form-group { margin-bottom: 12px; }
    .form-label { font-size: 0.8rem; color: var(--text-muted); margin-bottom: 6px; display: block; }
    input[type="text"], input[type="password"], input[type="number"], select {
      width: 100%; padding: 9px 12px; background: #0b1118; border: 1px solid var(--card-border); border-radius: 8px; color: #fff; font-size: 0.88rem;
    }
    .info-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; margin-top: 10px; }
    .info-item { background: rgba(255,255,255,0.02); border: 1px solid var(--card-border); border-radius: 8px; padding: 10px; font-size: 0.8rem; }
    .info-val { font-size: 0.95rem; font-weight: 600; margin-top: 2px; }

    /* Modal */
    .modal-overlay { display: none; position: fixed; inset: 0; background: rgba(0,0,0,0.7); backdrop-filter: blur(3px); z-index: 50; align-items: center; justify-content: center; padding: 16px; }
    .modal-overlay.open { display: flex; }
    .modal-box { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 12px; padding: 20px; width: 100%; max-width: 420px; display: flex; flex-direction: column; gap: 14px; }
    .modal-title { font-size: 1.05rem; font-weight: 700; color: #fff; }

    /* Toast */
    #toast { position: fixed; bottom: 20px; left: 50%; transform: translateX(-50%) translateY(100px); background: #1e293b; border: 1px solid var(--primary); color: #fff; padding: 10px 20px; border-radius: 8px; font-size: 0.85rem; box-shadow: 0 4px 14px rgba(0,0,0,0.5); transition: transform 0.3s ease; z-index: 100; pointer-events: none; }
    #toast.show { transform: translateX(-50%) translateY(0); }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div class="brand">
        🌿 <span>8-Relay</span> Irrigation Controller
      </div>
      <div style="display:flex; gap:10px; align-items:center;">
        <div id="conn-badge" class="status-badge">
          <span class="dot"></span> <span id="conn-text">Online</span>
        </div>
      </div>
    </header>

    <!-- Master Action Bar -->
    <div class="master-bar">
      <button class="btn-primary" onclick="startCycle()">
        ▶️ Run Sequential Cycle (All Zones)
      </button>
      <button class="btn-danger" onclick="stopAll()">
        🛑 Stop All Relays
      </button>
      <div style="flex:1;"></div>
      <button class="btn-secondary" onclick="openNetworkTab()">
        ⚙️ Settings / Wi-Fi
      </button>
    </div>

    <!-- Navigation Tabs -->
    <div class="tabs">
      <button class="tab-btn active" onclick="openTab('tab-zones', this)">Zones Overview</button>
      <button class="tab-btn" onclick="openTab('tab-settings', this)">Network & System</button>
    </div>

    <!-- Tab: Zones Overview -->
    <div id="tab-zones" class="tab-pane active">
      <div class="zone-grid" id="zone-cards-container">
        <!-- Rendered dynamically by JavaScript -->
      </div>
    </div>

    <!-- Tab: Settings / Network -->
    <div id="tab-settings" class="tab-pane">
      <div class="card">
        <h3 style="margin-bottom: 14px; font-size: 1rem;">Wi-Fi & System Settings</h3>
        <div class="form-group">
          <label class="form-label">Wi-Fi Network (SSID)</label>
          <input type="text" id="wifi-ssid" placeholder="Enter Wi-Fi SSID">
        </div>
        <div class="form-group">
          <label class="form-label">Wi-Fi Password</label>
          <input type="password" id="wifi-pass" placeholder="Enter Wi-Fi Password">
        </div>
        <div class="info-grid">
          <div class="info-item">
            <span style="color:var(--text-muted)">IP Address</span>
            <div id="net-ip" class="info-val">--</div>
          </div>
          <div class="info-item">
            <span style="color:var(--text-muted)">Wi-Fi Signal</span>
            <div id="net-rssi" class="info-val">-- dBm</div>
          </div>
          <div class="info-item">
            <span style="color:var(--text-muted)">System Uptime</span>
            <div id="net-uptime" class="info-val">--</div>
          </div>
          <div class="info-item">
            <span style="color:var(--text-muted)">mDNS Domain</span>
            <div id="net-mdns" class="info-val">watering.local</div>
          </div>
        </div>
        <div style="display:flex; gap:10px; margin-top:16px;">
          <button class="btn-primary" onclick="saveWiFi()">Save & Reconnect</button>
          <button class="btn-secondary" onclick="rebootESP()">Reboot Controller</button>
        </div>
      </div>
    </div>
  </div>

  <!-- Zone Schedule Config Modal -->
  <div class="modal-overlay" id="config-modal">
    <div class="modal-box">
      <div class="modal-title" id="modal-zone-title">Configure Zone</div>
      <input type="hidden" id="modal-zone-index">
      
      <div class="form-group">
        <label class="form-label">Zone Name</label>
        <input type="text" id="modal-zone-name" maxlength="20">
      </div>

      <div class="form-group">
        <label class="form-label">Watering Duration (Seconds)</label>
        <input type="number" id="modal-zone-dur" min="1" max="1800">
      </div>

      <div class="form-group">
        <label class="form-label">Watering Frequency / Interval</label>
        <select id="modal-zone-interval">
          <option value="14400">Every 4 Hours</option>
          <option value="21600">Every 6 Hours</option>
          <option value="28800">Every 8 Hours</option>
          <option value="43200">Every 12 Hours (Twice a day)</option>
          <option value="86400">Every 24 Hours (Once a day)</option>
          <option value="172800">Every 48 Hours (Every 2 days)</option>
          <option value="60">Test: Every 1 Minute</option>
        </select>
      </div>

      <div style="display:flex; gap:10px; margin-top:8px;">
        <button class="btn-primary" style="flex:1;" onclick="submitZoneConfig()">Save Settings</button>
        <button class="btn-secondary" onclick="closeModal()">Cancel</button>
      </div>
    </div>
  </div>

  <div id="toast">Saved</div>

  <script>
    let zonesData = [];
    const NUM_RELAYS = 8;

    function openTab(tabId, btn) {
      document.querySelectorAll('.tab-pane').forEach(el => el.classList.remove('active'));
      document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));
      document.getElementById(tabId).classList.add('active');
      btn.classList.add('active');
    }

    function openNetworkTab() {
      const btn = document.querySelectorAll('.tab-btn')[1];
      openTab('tab-settings', btn);
    }

    function showToast(msg) {
      const t = document.getElementById('toast');
      t.innerText = msg;
      t.classList.add('show');
      setTimeout(() => t.classList.remove('show'), 2500);
    }

    function formatTime(sec) {
      if (sec < 60) return sec + 's';
      if (sec < 3600) return Math.floor(sec / 60) + 'm ' + (sec % 60) + 's';
      const h = Math.floor(sec / 3600);
      const m = Math.floor((sec % 3600) / 60);
      return h + 'h ' + m + 'm';
    }

    function renderZoneCards() {
      const container = document.getElementById('zone-cards-container');
      if (container.children.length === 0) {
        let html = '';
        for (let i = 0; i < NUM_RELAYS; i++) {
          html += `
            <div class="zone-card" id="card-${i}">
              <div class="zone-head">
                <div class="zone-title">
                  <span class="zone-num">CH ${i+1}</span>
                  <span id="name-${i}">Zone ${i+1}</span>
                </div>
                <span id="badge-${i}" class="status-badge">Idle</span>
              </div>
              
              <div class="quick-row">
                <button class="btn-secondary btn-sm" onclick="quickWater(${i}, 5)">5s</button>
                <button class="btn-secondary btn-sm" onclick="quickWater(${i}, 10)">10s</button>
                <button class="btn-secondary btn-sm" onclick="quickWater(${i}, 30)">30s</button>
                <button class="btn-danger btn-sm" id="stop-${i}" style="display:none;" onclick="toggleRelay(${i}, false)">Stop</button>
              </div>

              <div class="zone-schedule">
                <div class="schedule-header">
                  <span style="font-weight:600;">Schedule Timer</span>
                  <label class="switch">
                    <input type="checkbox" id="sched-${i}" onchange="toggleSchedule(${i}, this.checked)">
                    <span class="slider"></span>
                  </label>
                </div>
                <div class="schedule-timing">
                  <span id="timing-${i}">Runs 10s / 12h</span>
                  <span id="next-${i}">--</span>
                </div>
              </div>

              <div style="display:flex; justify-content:space-between; align-items:center;">
                <button class="btn-secondary btn-sm" style="flex:1; margin-right:8px;" onclick="openConfigModal(${i})">
                  ⚙️ Edit Schedule
                </button>
                <button class="btn-primary btn-sm" style="flex:1;" id="btn-toggle-${i}" onclick="toggleRelay(${i})">
                  ⚡ Manual Toggle
                </button>
              </div>
            </div>
          `;
        }
        container.innerHTML = html;
      }
    }

    async function fetchStatus() {
      try {
        const res = await fetch('/api/status');
        if (!res.ok) throw new Error('Status err');
        const data = await res.json();

        document.getElementById('conn-badge').classList.remove('offline');
        document.getElementById('conn-text').innerText = 'Online';

        document.getElementById('net-ip').innerText = data.ip;
        document.getElementById('net-rssi').innerText = data.rssi + ' dBm';
        document.getElementById('net-uptime').innerText = data.uptime;

        zonesData = data.relays;
        renderZoneCards();

        zonesData.forEach((z, i) => {
          const card = document.getElementById(`card-${i}`);
          const badge = document.getElementById(`badge-${i}`);
          const stopBtn = document.getElementById(`stop-${i}`);
          const nameEl = document.getElementById(`name-${i}`);
          const schedToggle = document.getElementById(`sched-${i}`);
          const timingEl = document.getElementById(`timing-${i}`);
          const nextEl = document.getElementById(`next-${i}`);
          const toggleBtn = document.getElementById(`btn-toggle-${i}`);

          nameEl.innerText = z.name;
          schedToggle.checked = z.autoEnabled;
          timingEl.innerText = `${z.durationSec}s every ${formatTime(z.intervalSec)}`;

          if (z.autoEnabled) {
            nextEl.innerText = z.nextRunSec > 0 ? ('Next: ' + formatTime(z.nextRunSec)) : 'Ready';
          } else {
            nextEl.innerText = 'Disabled';
          }

          if (z.active) {
            card.classList.add('active');
            badge.className = 'status-badge active-pulse';
            badge.innerText = z.remainingSec > 0 ? `Pumping (${z.remainingSec}s)` : 'Active';
            stopBtn.style.display = 'inline-flex';
            toggleBtn.className = 'btn-danger btn-sm';
            toggleBtn.innerText = 'Turn OFF';
          } else {
            card.classList.remove('active');
            badge.className = 'status-badge';
            badge.innerText = 'Idle';
            stopBtn.style.display = 'none';
            toggleBtn.className = 'btn-primary btn-sm';
            toggleBtn.innerText = 'Turn ON';
          }
        });

      } catch (e) {
        document.getElementById('conn-badge').classList.add('offline');
        document.getElementById('conn-text').innerText = 'Reconnecting...';
      }
    }

    async function quickWater(index, duration) {
      try {
        await fetch('/api/relay', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({ index: index, action: 'pulse', duration: duration })
        });
        showToast(`Started Zone ${index+1} for ${duration}s`);
        fetchStatus();
      } catch (e) {
        showToast('Command failed');
      }
    }

    async function toggleRelay(index, forceState) {
      const z = zonesData[index];
      const newState = (forceState !== undefined) ? forceState : (!z.active);
      try {
        await fetch('/api/relay', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({ index: index, action: newState ? 'on' : 'off' })
        });
        showToast(`Zone ${index+1} turned ${newState ? 'ON' : 'OFF'}`);
        fetchStatus();
      } catch (e) {
        showToast('Command failed');
      }
    }

    async function toggleSchedule(index, enabled) {
      try {
        await fetch('/api/settings', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({ index: index, autoEnabled: enabled })
        });
        showToast(`Zone ${index+1} schedule ${enabled ? 'Enabled' : 'Disabled'}`);
        fetchStatus();
      } catch (e) {
        showToast('Failed to update schedule');
      }
    }

    async function startCycle() {
      if (!confirm('Run sequential watering for all active zones?')) return;
      try {
        await fetch('/api/cycle', { method: 'POST' });
        showToast('Sequential cycle started!');
        fetchStatus();
      } catch(e) {
        showToast('Failed to start cycle');
      }
    }

    async function stopAll() {
      try {
        await fetch('/api/stop_all', { method: 'POST' });
        showToast('All relays turned OFF!');
        fetchStatus();
      } catch(e) {
        showToast('Command failed');
      }
    }

    function openConfigModal(index) {
      const z = zonesData[index];
      document.getElementById('modal-zone-index').value = index;
      document.getElementById('modal-zone-title').innerText = `Configure Zone ${index+1}`;
      document.getElementById('modal-zone-name').value = z.name;
      document.getElementById('modal-zone-dur').value = z.durationSec;
      
      const sel = document.getElementById('modal-zone-interval');
      sel.value = z.intervalSec;
      if (!sel.value) sel.value = '43200';

      document.getElementById('config-modal').classList.add('open');
    }

    function closeModal() {
      document.getElementById('config-modal').classList.remove('open');
    }

    async function submitZoneConfig() {
      const index = parseInt(document.getElementById('modal-zone-index').value);
      const name = document.getElementById('modal-zone-name').value.trim() || `Zone ${index+1}`;
      const duration = parseInt(document.getElementById('modal-zone-dur').value) || 10;
      const interval = parseInt(document.getElementById('modal-zone-interval').value) || 43200;

      try {
        await fetch('/api/settings', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({
            index: index,
            name: name,
            durationSec: duration,
            intervalSec: interval
          })
        });
        closeModal();
        showToast('Zone settings saved!');
        fetchStatus();
      } catch(e) {
        showToast('Error saving zone');
      }
    }

    async function saveWiFi() {
      const ssid = document.getElementById('wifi-ssid').value.trim();
      const pass = document.getElementById('wifi-pass').value;
      if (!ssid) return alert('Please enter SSID');
      if (!confirm(`Connect to "${ssid}" and reboot?`)) return;

      try {
        await fetch('/api/wifi', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({ ssid, pass })
        });
        alert('Credentials sent. Controller restarting...');
      } catch(e) {
        alert('Failed to send Wi-Fi settings');
      }
    }

    async function rebootESP() {
      if (!confirm('Reboot controller?')) return;
      try {
        await fetch('/api/restart', { method: 'POST' });
        showToast('Rebooting...');
      } catch(e){}
    }

    window.addEventListener('load', () => {
      fetchStatus();
      setInterval(fetchStatus, 1500);
    });
  </script>
</body>
</html>
)rawliteral";
