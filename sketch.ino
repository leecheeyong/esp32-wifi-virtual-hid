#include "WiFi.h"
#include "WebServer.h"
#include "USB.h"
#include "USBHIDMouse.h"
#include "USBHIDKeyboard.h"
#include <Preferences.h>

USBHIDMouse Mouse;
USBHIDKeyboard Keyboard;
WebServer server(80);
Preferences preferences;

const char* ssid = "ESP32 Virtual HID";
const char* password = "password123";

void sendOK() {
    server.sendHeader("Connection", "keep-alive");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200);
}

void safeMouseMove(int x, int y, int wheel = 0) {
    while (x != 0 || y != 0 || wheel != 0) {
        int mx = constrain(x, -127, 127);
        int my = constrain(y, -127, 127);
        int mw = constrain(wheel, -127, 127);
        Mouse.move(mx, my, mw);
        x -= mx; y -= my; wheel -= mw;
    }
}

uint8_t resolveKey(int code) {
    switch(code) {
        case 0xB1: return KEY_ESC;
        case 0xB2: return KEY_TAB;
        case 0xB3: return KEY_RETURN;
        case 0xB0: return KEY_BACKSPACE;
        case 0xD1: return KEY_INSERT;
        case 0xD4: return KEY_DELETE;
        case 0xD3: return KEY_HOME;
        case 0xD5: return KEY_END;
        case 0xD6: return KEY_PAGE_UP;
        case 0xD7: return KEY_PAGE_DOWN;
        case 0xDA: return KEY_UP_ARROW;
        case 0xD9: return KEY_DOWN_ARROW;
        case 0xD8: return KEY_LEFT_ARROW;
        case 0xD7 + 1: return KEY_RIGHT_ARROW; // 0xD8 conflicts, use explicit
        case 0x80: return KEY_LEFT_CTRL;
        case 0x81: return KEY_LEFT_SHIFT;
        case 0x82: return KEY_LEFT_ALT;
        case 0x83: return KEY_LEFT_GUI;
        case 0x84: return KEY_RIGHT_CTRL;
        case 0x85: return KEY_RIGHT_SHIFT;
        case 0x86: return KEY_RIGHT_ALT;
        case 0x87: return KEY_RIGHT_GUI;
        case 0xC1: return KEY_CAPS_LOCK;
        case 0xC2: return KEY_F1;
        case 0xC3: return KEY_F2;
        case 0xC4: return KEY_F3;
        case 0xC5: return KEY_F4;
        case 0xC6: return KEY_F5;
        case 0xC7: return KEY_F6;
        case 0xC8: return KEY_F7;
        case 0xC9: return KEY_F8;
        case 0xCA: return KEY_F9;
        case 0xCB: return KEY_F10;
        case 0xCC: return KEY_F11;
        case 0xCD: return KEY_F12;
        default: return (uint8_t)code;
    }
}

void executeCommand(String line) {
    line.trim();
    if (line.length() == 0 || line.startsWith("REM")) return;
    
    int spaceIdx = line.indexOf(' ');
    String cmd = spaceIdx != -1 ? line.substring(0, spaceIdx) : line;
    String arg = spaceIdx != -1 ? line.substring(spaceIdx + 1) : "";
    
    if (cmd == "STRING") Keyboard.print(arg);
    else if (cmd == "ENTER") Keyboard.write(KEY_RETURN);
    else if (cmd == "TAB") Keyboard.write(KEY_TAB);
    else if (cmd == "SPACE") Keyboard.write(' ');
    else if (cmd == "UP") Keyboard.write(KEY_UP_ARROW);
    else if (cmd == "DOWN") Keyboard.write(KEY_DOWN_ARROW);
    else if (cmd == "LEFT") Keyboard.write(KEY_LEFT_ARROW);
    else if (cmd == "RIGHT") Keyboard.write(KEY_RIGHT_ARROW);
    else if (cmd == "GUI" || cmd == "WINDOWS") {
        Keyboard.press(KEY_LEFT_GUI);
        if(arg.length() > 0) Keyboard.press(arg.charAt(0));
        Keyboard.releaseAll();
    }
    else if (cmd == "CTRL" || cmd == "CONTROL") {
        Keyboard.press(KEY_LEFT_CTRL);
        if(arg.length() > 0) Keyboard.press(arg.charAt(0));
        Keyboard.releaseAll();
    }
    else if (cmd == "ALT") {
        Keyboard.press(KEY_LEFT_ALT);
        if(arg.length() > 0) Keyboard.press(arg.charAt(0));
        Keyboard.releaseAll();
    }
    else if (cmd == "SHIFT") {
        Keyboard.press(KEY_LEFT_SHIFT);
        if(arg.length() > 0) Keyboard.press(arg.charAt(0));
        Keyboard.releaseAll();
    }
    else if (cmd == "DELAY") {
        delay(arg.toInt());
    }
    else if (cmd == "MOUSE_MOVE") {
        int comma = arg.indexOf(' ');
        if (comma != -1) {
            safeMouseMove(arg.substring(0, comma).toInt(), arg.substring(comma + 1).toInt(), 0);
        }
    }
    else if (cmd == "MOUSE_SCROLL") {
        safeMouseMove(0, 0, arg.toInt());
    }
    else if (cmd == "MOUSE_CLICK") {
        if (arg == "LEFT") Mouse.click(MOUSE_LEFT);
        else if (arg == "RIGHT") Mouse.click(MOUSE_RIGHT);
    }
    else if (cmd == "MOUSE_DOWN") {
        Mouse.press(MOUSE_LEFT);
    }
    else if (cmd == "MOUSE_UP") {
        Mouse.release(MOUSE_LEFT);
    }
    else if (cmd == "MOUSE_RESET") {
        for(int i=0; i<30; i++) safeMouseMove(-127, -127, 0);
    }
}

const char* html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>ESP32 Virtual HID</title>
    <style>
        :root { 
            --bg: #09090b; --surface: rgba(255, 255, 255, 0.05); --surface-active: rgba(255, 255, 255, 0.12); 
            --border: rgba(255, 255, 255, 0.08); --accent: #0ea5e9; --accent-glow: rgba(14, 165, 233, 0.4);
            --danger: #ef4444; --text: #f8fafc; --text-muted: #94a3b8;
        }
        * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
        body { margin: 0; background: var(--bg); color: var(--text); font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; touch-action: none; height: 100vh; display: flex; flex-direction: column; overflow: hidden; overscroll-behavior: none; }
        
        .container { display: flex; flex-direction: column; height: 100%; width: 100%; max-width: 1400px; margin: 0 auto; padding: 12px; gap: 12px; }
        
        .top-panel { background: var(--surface); border: 1px solid var(--border); border-radius: 20px; padding: 16px; display: flex; flex-direction: column; gap: 12px; backdrop-filter: blur(12px); -webkit-backdrop-filter: blur(12px); box-shadow: 0 4px 30px rgba(0, 0, 0, 0.1); }
        
        .panel-header { display: flex; justify-content: space-between; align-items: center; }
        .panel-title { font-size: 13px; color: var(--text-muted); font-weight: 700; text-transform: uppercase; letter-spacing: 1.2px; }
        
        input, select, textarea { width: 100%; border-radius: 12px; border: 1px solid var(--border); background: rgba(0,0,0,0.3); color: white; font-size: 15px; outline: none; padding: 14px; transition: border 0.2s; }
        input:focus, textarea:focus { border-color: var(--accent); }
        textarea { height: 140px; resize: none; font-family: 'Courier New', Courier, monospace; display: none; }
        
        .btn-group { display: flex; gap: 8px; flex-wrap: wrap; display: none; }
        .btn { flex: 1; min-width: calc(33% - 8px); background: var(--surface); border: 1px solid var(--border); color: white; padding: 10px; border-radius: 10px; font-size: 13px; font-weight: 600; cursor: pointer; transition: all 0.15s ease; }
        .btn:active { transform: scale(0.96); background: var(--surface-active); }
        .btn-primary { background: var(--accent); border-color: var(--accent); }
        .btn-danger { background: rgba(239, 68, 68, 0.15); border-color: var(--danger); color: var(--danger); }
        .btn-record.recording { background: var(--danger); color: white; border-color: var(--danger); animation: pulse 1.5s infinite cubic-bezier(0.4, 0, 0.6, 1); }
        
        @keyframes pulse { 0%, 100% { opacity: 1; box-shadow: 0 0 0 0 rgba(239, 68, 68, 0.4); } 50% { opacity: 0.7; box-shadow: 0 0 0 10px rgba(239, 68, 68, 0); } }

        .trackpad-area { flex: 1; display: flex; flex-direction: column; gap: 12px; min-height: 0; }
        
        #pad { flex: 1; background: var(--surface); border-radius: 24px; display: flex; flex-direction: column; align-items: center; justify-content: center; color: var(--text-muted); user-select: none; border: 1px solid var(--border); transition: border 0.2s, background 0.2s, box-shadow 0.2s; position: relative; overflow: hidden; }
        #pad.active { background: var(--surface-active); color: var(--text); }
        #pad.dragging { border-color: var(--accent); box-shadow: 0 0 30px var(--accent-glow) inset; color: var(--accent); }
        
        .mouse-buttons { display: flex; gap: 12px; height: 80px; flex-shrink: 0; }
        .mouse-btn { flex: 1; border-radius: 20px; border: 1px solid var(--border); background: var(--surface); color: var(--text); font-weight: 600; font-size: 14px; cursor: pointer; transition: all 0.1s; display: flex; align-items: center; justify-content: center; backdrop-filter: blur(8px); }
        .mouse-btn:active { background: rgba(255,255,255,0.15); transform: scale(0.98); }

        /* ── Virtual Keyboard ── */
        .kb-toggle { position: fixed; bottom: 12px; right: 12px; z-index: 100; width: 48px; height: 48px; border-radius: 14px; border: 1px solid var(--border); background: var(--surface); color: var(--text); font-size: 22px; cursor: pointer; backdrop-filter: blur(12px); -webkit-backdrop-filter: blur(12px); display: flex; align-items: center; justify-content: center; transition: all 0.2s; box-shadow: 0 4px 20px rgba(0,0,0,0.3); }
        .kb-toggle:active { transform: scale(0.92); }
        .kb-toggle.active { background: var(--accent); border-color: var(--accent); }

        .kb-panel { position: fixed; bottom: 0; left: 0; right: 0; z-index: 99; background: rgba(20, 20, 22, 0.97); border-top: 1px solid var(--border); backdrop-filter: blur(16px); -webkit-backdrop-filter: blur(16px); transform: translateY(100%); transition: transform 0.25s cubic-bezier(0.4, 0, 0.2, 1); padding: 6px; padding-bottom: max(6px, env(safe-area-inset-bottom)); }
        .kb-panel.open { transform: translateY(0); }

        .kb-row { display: flex; gap: 3px; margin-bottom: 3px; justify-content: center; }
        .kb-row:last-child { margin-bottom: 0; }

        .k { min-width: 0; flex: 1; height: 42px; max-width: 56px; border-radius: 8px; border: 1px solid rgba(255,255,255,0.1); background: rgba(255,255,255,0.07); color: var(--text); font-size: 12px; font-weight: 600; cursor: pointer; display: flex; align-items: center; justify-content: center; transition: background 0.08s, transform 0.08s; user-select: none; padding: 0 2px; white-space: nowrap; overflow: hidden; }
        .k:active, .k.held { background: rgba(255,255,255,0.2); transform: scale(0.94); }
        .k.mod { background: rgba(14, 165, 233, 0.15); border-color: rgba(14, 165, 233, 0.25); color: #7dd3fc; }
        .k.mod.held { background: var(--accent); color: #fff; border-color: var(--accent); transform: scale(1); }
        .k.wide { flex: 1.5; max-width: 84px; }
        .k.wider { flex: 2; max-width: 112px; }
        .k.space { flex: 6; max-width: none; }
        .k.fn { font-size: 10px; background: rgba(255,255,255,0.04); }

        .kb-section { display: flex; gap: 3px; margin-bottom: 3px; }
        .kb-fn-row { display: flex; gap: 2px; margin-bottom: 5px; justify-content: center; }
        .kb-fn-row .k { height: 32px; font-size: 10px; flex: 1; max-width: 48px; background: rgba(255,255,255,0.04); }

        @media (max-width: 767px) { 
            .mouse-buttons { display: none; }
            .k { height: 38px; font-size: 11px; }
            .kb-fn-row .k { height: 28px; font-size: 9px; }
            .kb-toggle { bottom: 10px; right: 10px; width: 44px; height: 44px; font-size: 20px; }
        }
        @media (min-width: 768px), (orientation: landscape) {
            .container { flex-direction: row; padding: 20px; gap: 20px; }
            .top-panel { width: 420px; flex: none; }
            .trackpad-area { flex: 1; }
            .k { height: 44px; font-size: 13px; max-width: 64px; }
            .k.space { max-width: none; }
            .kb-fn-row .k { max-width: 56px; }
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Top Panel -->
        <div class="top-panel">
            <div class="panel-header">
                <div class="panel-title" id="modeTitle">Live Keyboard</div>
                <button class="btn" style="width: auto; padding: 6px 14px; flex: none;" onclick="toggleMode()">Toggle Ducky</button>
            </div>
            
            <input type="text" id="kInput" placeholder="Tap here to type..." autocomplete="off" autocorrect="off" spellcheck="false">
            
            <select id="scriptSelect" style="display:none;" onchange="loadScript()"></select>
            <textarea id="duckyArea" placeholder="Write Ducky Script here..."></textarea>
            
            <div class="btn-group" id="btnGroup1">
                <button class="btn btn-primary" id="runBtn" onclick="runDucky()">▶ Run</button>
                <button class="btn btn-record" id="recordBtn" onclick="toggleRecord()">Record</button>
                <button class="btn" onclick="saveScript()">Save</button>
            </div>
            <div class="btn-group" id="btnGroup2">
                <button class="btn" onclick="setStartup()">Set Startup</button>
                <button class="btn btn-danger" onclick="delStartup()">Clear Startup</button>
                <button class="btn btn-danger" onclick="deleteScript()">Delete</button>
            </div>
        </div>

        <!-- Trackpad Area -->
        <div class="trackpad-area">
            <div id="pad">
                <div style="font-size:26px; font-weight:800; letter-spacing:6px;" id="padText">TOUCH</div>
                <div style="font-size:13px; margin-top:12px; opacity: 0.6; font-weight: 500;">Double-Tap &amp; Hold to Drag</div>
            </div>
            <div class="mouse-buttons">
                <button class="mouse-btn" onclick="fastFetch('/cl')">LEFT CLICK</button>
                <button class="mouse-btn" onclick="fastFetch('/cr')">RIGHT CLICK</button>
            </div>
        </div>
    </div>

    <!-- Keyboard Toggle FAB -->
    <button class="kb-toggle" id="kbToggle" onclick="toggleKB()">⌨</button>

    <!-- Virtual Keyboard Panel -->
    <div class="kb-panel" id="kbPanel"></div>

    <script>
        const kInput = document.getElementById('kInput');
        const duckyArea = document.getElementById('duckyArea');
        const scriptSelect = document.getElementById('scriptSelect');
        const padText = document.getElementById('padText');
        let isDuckyMode = false;

        const fetchOpts = { keepalive: true, cache: 'no-store' };
        function fastFetch(url) { return fetch(url, fetchOpts).catch(()=>{}); }

        function toggleMode() {
            isDuckyMode = !isDuckyMode;
            document.getElementById('modeTitle').innerText = isDuckyMode ? "Ducky Editor" : "Live Keyboard";
            kInput.style.display = isDuckyMode ? 'none' : 'block';
            duckyArea.style.display = isDuckyMode ? 'block' : 'none';
            scriptSelect.style.display = isDuckyMode ? 'block' : 'none';
            document.getElementById('btnGroup1').style.display = isDuckyMode ? 'flex' : 'none';
            document.getElementById('btnGroup2').style.display = isDuckyMode ? 'flex' : 'none';
            if (isDuckyMode) refreshScriptList();
        }

        let scripts = JSON.parse(localStorage.getItem('airpad_scripts')) || { "Default": "STRING Hello World\nENTER" };
        function refreshScriptList() {
            scriptSelect.innerHTML = '<option value="">-- Create New / Select Script --</option>';
            for (let name in scripts) scriptSelect.innerHTML += `<option value="${name}">${name}</option>`;
        }
        function loadScript() { duckyArea.value = scripts[scriptSelect.value] || ''; }
        function saveScript() {
            let name = scriptSelect.value || prompt("Enter name for this script:");
            if (!name) return;
            scripts[name] = duckyArea.value;
            localStorage.setItem('airpad_scripts', JSON.stringify(scripts));
            refreshScriptList(); scriptSelect.value = name;
        }
        function deleteScript() {
            let name = scriptSelect.value;
            if (name && confirm(`Delete ${name}?`)) {
                delete scripts[name];
                localStorage.setItem('airpad_scripts', JSON.stringify(scripts));
                duckyArea.value = ''; refreshScriptList();
            }
        }

        async function setStartup() {
            let res = await fetch('/startup', { method: 'POST', body: duckyArea.value });
            if (res.ok) alert("Startup Script saved to ESP32 Flash!");
        }
        async function delStartup() {
            let res = await fetch('/startup', { method: 'POST', body: "" });
            if (res.ok) alert("Startup Script cleared from ESP32.");
        }

        let isRecording = false, lastActionTime = 0;
        function toggleRecord() {
            isRecording = !isRecording;
            const btn = document.getElementById('recordBtn');
            if (isRecording) {
                btn.classList.add('recording'); btn.innerText = "Stop";
                duckyArea.value = "MOUSE_RESET\n"; lastActionTime = Date.now();
                padText.innerText = "RECORDING";
            } else {
                btn.classList.remove('recording'); btn.innerText = "Record";
                padText.innerText = "TOUCH";
            }
        }

        function logAction(cmd) {
            if (!isRecording) return;
            let delay = Date.now() - lastActionTime;
            if (delay > 40) duckyArea.value += `DELAY ${delay}\n`;
            duckyArea.value += `${cmd}\n`;
            lastActionTime = Date.now();
            duckyArea.scrollTop = duckyArea.scrollHeight; // Auto-scroll
        }

        kInput.addEventListener('input', () => {
            let val = kInput.value;
            if (val.length > 0) {
                fastFetch('/k?text=' + encodeURIComponent(val));
                logAction(`STRING ${val}`); kInput.value = ''; 
            }
        });
        kInput.addEventListener('keydown', (e) => {
            if (e.key === 'Backspace') { fastFetch('/kb'); logAction('DELAY 50\nkb'); }
            if (e.key === 'Enter') { fastFetch('/en'); logAction('ENTER'); }
        });

        async function runDucky() {
            const btn = document.getElementById('runBtn'); btn.innerText = "⏳...";
            const lines = duckyArea.value.split('\n');
            for (let line of lines) {
                line = line.trim();
                if (!line || line.startsWith('REM')) continue;
                if (line.startsWith('DELAY')) {
                    await new Promise(r => setTimeout(r, parseInt(line.split(' ')[1]) || 500));
                } else {
                    await fetch('/ducky?cmd=' + encodeURIComponent(line));
                    await new Promise(r => setTimeout(r, 15));
                }
            }
            btn.innerText = "Run";
        }

        // --- TRACKPAD LOGIC WITH DRAG ---
        const pad = document.getElementById('pad');
        let startX = 0, startY = 0, lastX = 0, lastY = 0;
        let accumX = 0, accumY = 0, accumScroll = 0;
        
        let isMoving = false, hasSwiped = false, maxTouches = 0;
        let touchStartTime = 0, lastTapTime = 0;
        let potentialDrag = false, isDragging = false;
        let gestureFired = false, isSending = false;

        pad.addEventListener('touchstart', e => {
            e.preventDefault();
            pad.classList.add('active');
            maxTouches = Math.max(maxTouches, e.touches.length);
            gestureFired = false;
            
            let avgX = 0, avgY = 0;
            for(let i=0; i<e.touches.length; i++){ avgX += e.touches[i].clientX; avgY += e.touches[i].clientY; }
            avgX /= e.touches.length; avgY /= e.touches.length;
            
            lastX = startX = avgX; lastY = startY = avgY;
            isMoving = true; hasSwiped = false; touchStartTime = Date.now();

            if (e.touches.length === 1) {
                if (Date.now() - lastTapTime < 300) potentialDrag = true;
                else potentialDrag = false;
            } else {
                potentialDrag = false;
            }
        }, { passive: false });

        pad.addEventListener('touchmove', e => {
            e.preventDefault();
            if (!isMoving) return;
            
            let avgX = 0, avgY = 0;
            for(let i=0; i<e.touches.length; i++){ avgX += e.touches[i].clientX; avgY += e.touches[i].clientY; }
            avgX /= e.touches.length; avgY /= e.touches.length;
            
            let dx = avgX - lastX; let dy = avgY - lastY;
            
            if (Math.abs(avgX - startX) > 5 || Math.abs(avgY - startY) > 5) {
                hasSwiped = true;
                // Transition to Dragging State
                if (potentialDrag && !isDragging && e.touches.length === 1) {
                    isDragging = true;
                    pad.classList.add('dragging');
                    fastFetch('/md'); logAction('MOUSE_DOWN');
                    padText.innerText = "DRAGGING";
                }
            }

            if (e.touches.length === 1) { 
                // Kinetic Acceleration: Fast swipes move cursor further
                let speed = Math.sqrt(dx*dx + dy*dy);
                let accel = speed > 10 ? 1.8 : 1.2; 
                accumX += dx * accel; accumY += dy * accel; 
            } 
            else if (e.touches.length === 2) { accumScroll -= dy * 0.3; }
            else if (e.touches.length === 3 && !gestureFired) {
                if (Math.abs(avgX - startX) > 40 || Math.abs(avgY - startY) > 40) {
                    gestureFired = true;
                    if (Math.abs(avgX - startX) > Math.abs(avgY - startY)) {
                        if (avgX - startX > 0) { fastFetch('/g?v=3R'); logAction('ALT TAB'); }
                        else { fastFetch('/g?v=3L'); logAction('ALT TAB'); }
                    } else {
                        if (avgY - startY > 0) { fastFetch('/g?v=3D'); logAction('GUI d'); }
                        else { fastFetch('/g?v=3U'); logAction('GUI TAB'); }
                    }
                }
            }
            lastX = avgX; lastY = avgY;
        }, { passive: false });

        pad.addEventListener('touchend', e => {
            e.preventDefault();
            pad.classList.remove('active');
            
            if (e.touches.length === 0) {
                if (isDragging) {
                    isDragging = false;
                    pad.classList.remove('dragging');
                    fastFetch('/mu'); logAction('MOUSE_UP');
                    padText.innerText = isRecording ? "RECORDING" : "TOUCH";
                    potentialDrag = false;
                } 
                else if (!hasSwiped && Date.now() - touchStartTime < 250) {
                    if (maxTouches === 1) { 
                        fastFetch('/cl'); logAction('MOUSE_CLICK LEFT'); 
                        lastTapTime = Date.now(); 
                    }
                    else if (maxTouches === 2) { fastFetch('/cr'); logAction('MOUSE_CLICK RIGHT'); }
                }
                isMoving = false; maxTouches = 0; hasSwiped = false;
            }
        }, { passive: false });

        setInterval(() => {
            if (isSending) return;
            let sendX = Math.round(accumX), sendY = Math.round(accumY), sendS = Math.round(accumScroll);
            
            if (sendX !== 0 || sendY !== 0 || sendS !== 0) {
                isSending = true;
                accumX -= sendX; accumY -= sendY; accumScroll -= sendS;
                
                if(sendX !== 0 || sendY !== 0) logAction(`MOUSE_MOVE ${sendX} ${sendY}`);
                if(sendS !== 0) logAction(`MOUSE_SCROLL ${sendS}`);
                
                fastFetch(`/m?dx=${sendX}&dy=${sendY}&s=${sendS}`).finally(() => {
                    isSending = false; 
                });
            }
        }, 15);

        // ── VIRTUAL KEYBOARD ──
        const kbPanel = document.getElementById('kbPanel');
        const kbToggle = document.getElementById('kbToggle');
        let kbOpen = false;
        // Modifier state: tracks which modifiers are currently held
        let heldMods = new Set();

        function toggleKB() {
            kbOpen = !kbOpen;
            kbPanel.classList.toggle('open', kbOpen);
            kbToggle.classList.toggle('active', kbOpen);
        }

        // Key definitions: [label, keycode_or_char, css_class]
        // Keycodes use the ESP32 USBHIDKeyboard hex values
        // Printable chars are sent as text via /k endpoint
        // Special keys/modifiers use /kp (press) and /kr (release) endpoints
        function buildKB() {
            // F-key row
            const fnRow = [
                ['Esc',0xB1],['F1',0xC2],['F2',0xC3],['F3',0xC4],['F4',0xC5],
                ['F5',0xC6],['F6',0xC7],['F7',0xC8],['F8',0xC9],
                ['F9',0xCA],['F10',0xCB],['F11',0xCC],['F12',0xCD]
            ];
            // Main keyboard rows: [label, value, extraClass]
            // value: string = printable char, number = HID keycode
            const rows = [
                [['`','`'],['1','1'],['2','2'],['3','3'],['4','4'],['5','5'],['6','6'],['7','7'],['8','8'],['9','9'],['0','0'],['-','-'],['=','='],['⌫',0xB0,'wide']],
                [['Tab',0xB2,'wide'],['Q','q'],['W','w'],['E','e'],['R','r'],['T','t'],['Y','y'],['U','u'],['I','i'],['O','o'],['P','p'],['[','['],[']',']'],['\\','\\']],
                [['Caps',0xC1,'wide mod'],['A','a'],['S','s'],['D','d'],['F','f'],['G','g'],['H','h'],['J','j'],['K','k'],['L','l'],[';',';'],["'","'"],['Enter',0xB3,'wider']],
                [['Shift',0x81,'wider mod'],['Z','z'],['X','x'],['C','c'],['V','v'],['B','b'],['N','n'],['M','m'],[',',','],['.','.'],['/','/'],['Shift',0x85,'wide mod']],
                [['Ctrl',0x80,'wide mod'],['Win',0x83,'mod'],['Alt',0x82,'mod'],['',0x20,'space'],['Alt',0x86,'mod'],['Del',0xD4],['←',0xD8],['↑',0xDA],['↓',0xD9],['→',0xD7]]
            ];

            let h = '<div class="kb-fn-row">';
            for (const [l,c] of fnRow) h += `<button class="k fn" data-c="${c}">${l}</button>`;
            h += '</div>';

            for (const row of rows) {
                h += '<div class="kb-row">';
                for (const [l, v, cls] of row) {
                    const dc = typeof v === 'number' ? `data-c="${v}"` : `data-ch="${v}"`;
                    h += `<button class="k ${cls||''}" ${dc}>${l}</button>`;
                }
                h += '</div>';
            }
            h += '<div class="kb-fn-row">';
            const navKeys = [['Ins',0xD1],['Home',0xD3],['End',0xD5],['PgUp',0xD6],['PgDn',0xD7+1]];
            for (const [l,c] of navKeys) h += `<button class="k fn" data-c="${c}">${l}</button>`;
            h += '</div>';

            kbPanel.innerHTML = h;

            const modCodes = new Set([0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87]);

            kbPanel.addEventListener('pointerdown', (e) => {
                const btn = e.target.closest('.k');
                if (!btn) return;
                e.preventDefault();

                const code = btn.dataset.c ? parseInt(btn.dataset.c) : null;
                const ch = btn.dataset.ch;

                if (code !== null && modCodes.has(code)) {
                    // Toggle modifier
                    if (heldMods.has(code)) {
                        heldMods.delete(code);
                        btn.classList.remove('held');
                        fastFetch('/kr?c=' + code);
                    } else {
                        heldMods.add(code);
                        btn.classList.add('held');
                        fastFetch('/kp?c=' + code);
                    }
                } else if (ch !== undefined) {
                    btn.classList.add('held');
                    let sendCh = ch;
                    // If shift is held, apply shift to printable
                    if (heldMods.has(0x81) || heldMods.has(0x85)) {
                        sendCh = ch.toUpperCase();
                    }
                    fastFetch('/k?text=' + encodeURIComponent(sendCh));
                    setTimeout(() => btn.classList.remove('held'), 100);
                    releaseAllMods();
                } else if (code !== null) {
                    btn.classList.add('held');
                    fastFetch('/kp?c=' + code);
                    setTimeout(() => {
                        fastFetch('/kr?c=' + code);
                        btn.classList.remove('held');
                        releaseAllMods();
                    }, 60);
                }
            });
        }

        function releaseAllMods() {
            if (heldMods.size === 0) return;
            heldMods.forEach(c => fastFetch('/kr?c=' + c));
            heldMods.clear();
            kbPanel.querySelectorAll('.k.mod.held').forEach(b => b.classList.remove('held'));
        }

        buildKB();
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Mouse.begin(); 
    Keyboard.begin(); 
    USB.begin();
    
    preferences.begin("airpad", false);
    WiFi.softAP(ssid, password);
    
    // STARTUP SCRIPT EXECUTION
    String bootScript = preferences.getString("boot_script", "");
    if (bootScript.length() > 0) {
        delay(3000); 
        int str_len = bootScript.length() + 1;
        char char_array[str_len];
        bootScript.toCharArray(char_array, str_len);
        
        char *line = strtok(char_array, "\n");
        while(line != NULL) {
            executeCommand(String(line));
            line = strtok(NULL, "\n");
        }
    }
    server.on("/", [](){ server.send(200, "text/html", html); });
    
    server.on("/startup", HTTP_POST, [](){
        String payload = server.arg("plain");
        if(payload.length() == 0) preferences.remove("boot_script");
        else preferences.putString("boot_script", payload);
        sendOK();
    });
    
    server.on("/m", [](){ safeMouseMove(server.arg("dx").toInt(), server.arg("dy").toInt(), server.arg("s").toInt()); sendOK(); });
    server.on("/cl", [](){ Mouse.click(MOUSE_LEFT); sendOK(); });
    server.on("/cr", [](){ Mouse.click(MOUSE_RIGHT); sendOK(); });
    
    server.on("/md", [](){ Mouse.press(MOUSE_LEFT); sendOK(); });
    server.on("/mu", [](){ Mouse.release(MOUSE_LEFT); sendOK(); });
    
    server.on("/k", [](){ Keyboard.print(server.arg("text")); sendOK(); });
    server.on("/kb", [](){ Keyboard.write(KEY_BACKSPACE); sendOK(); });
    server.on("/en", [](){ Keyboard.write(KEY_RETURN); sendOK(); });

    server.on("/kp", [](){ 
        uint8_t key = resolveKey(server.arg("c").toInt());
        Keyboard.press(key); 
        sendOK(); 
    });
    server.on("/kr", [](){ 
        uint8_t key = resolveKey(server.arg("c").toInt());
        Keyboard.release(key); 
        sendOK(); 
    });

    server.on("/g", [](){
        String v = server.arg("v");
        if (v == "3U") { Keyboard.press(KEY_LEFT_GUI); Keyboard.press(KEY_TAB); Keyboard.releaseAll(); }
        else if (v == "3D") { Keyboard.press(KEY_LEFT_GUI); Keyboard.press('d'); Keyboard.releaseAll(); }
        else if (v == "3L" || v == "3R") { Keyboard.press(KEY_LEFT_ALT); Keyboard.press(KEY_TAB); Keyboard.releaseAll(); }
        sendOK();
    });

    server.on("/ducky", [](){
        executeCommand(server.arg("cmd"));
        sendOK();
    });

    server.begin();
}

void loop() { 
    server.handleClient(); 
}
