const char* html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>ESP Virtual HID</title>
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
        .header-actions { display: flex; gap: 8px; }

        input, textarea { width: 100%; border-radius: 12px; border: 1px solid var(--border); background: rgba(0,0,0,0.3); color: white; font-size: 15px; outline: none; padding: 14px; transition: border 0.2s; }
        input:focus, textarea:focus { border-color: var(--accent); }
        select { width: 100%; border-radius: 12px; border: 1px solid var(--border); background: rgba(0,0,0,0.5); color: white; font-size: 15px; outline: none; padding: 14px; appearance: none; }
        textarea { height: 140px; resize: none; font-family: 'Courier New', Courier, monospace; display: none; }
        
        .btn-group { display: flex; gap: 8px; flex-wrap: wrap; display: none; }
        .btn { flex: 1; min-width: calc(33% - 8px); background: var(--surface); border: 1px solid var(--border); color: white; padding: 10px; border-radius: 10px; font-size: 13px; font-weight: 600; cursor: pointer; transition: all 0.15s ease; display: flex; align-items: center; justify-content: center; }
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

        .kb-toggle { position: fixed; bottom: 12px; right: 12px; z-index: 2000; width: 48px; height: 48px; border-radius: 14px; border: 1px solid var(--border); background: var(--surface); color: var(--text); font-size: 22px; cursor: grab; backdrop-filter: blur(12px); -webkit-backdrop-filter: blur(12px); display: flex; align-items: center; justify-content: center; box-shadow: 0 4px 20px rgba(0,0,0,0.3); touch-action: none; }
        .kb-toggle:active { cursor: grabbing; }
        .kb-toggle.active { background: var(--accent); border-color: var(--accent); }

        .kb-panel { position: fixed; bottom: 0; left: 0; right: 0; z-index: 1999; background: rgba(20, 20, 22, 0.97); border-top: 1px solid var(--border); backdrop-filter: blur(16px); -webkit-backdrop-filter: blur(16px); transform: translateY(100%); transition: transform 0.25s cubic-bezier(0.4, 0, 0.2, 1); padding: 6px; padding-bottom: max(6px, env(safe-area-inset-bottom)); }
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

        .modal-overlay { position: fixed; top: 0; left: 0; right: 0; bottom: 0; background: rgba(0,0,0,0.6); backdrop-filter: blur(8px); z-index: 1000; display: none; align-items: center; justify-content: center; padding: 20px; }
        .modal-overlay.open { display: flex; }
        .modal { background: rgba(20, 20, 22, 0.95); border: 1px solid var(--border); border-radius: 20px; width: 100%; max-width: 360px; padding: 20px; box-shadow: 0 10px 40px rgba(0,0,0,0.5); }
        .setting-row { display: flex; justify-content: space-between; align-items: center; margin-top: 16px; padding: 12px; background: rgba(255,255,255,0.03); border-radius: 12px; border: 1px solid var(--border); }
        
        .setting-toggle-btn { background: var(--surface); border: 1px solid var(--border); color: var(--text-muted); padding: 8px 16px; border-radius: 10px; font-size: 13px; font-weight: 600; cursor: pointer; min-width: 90px; text-align: center; transition: all 0.2s; user-select: none; }
        .setting-toggle-btn.active { background: rgba(14, 165, 233, 0.2); border-color: var(--accent); color: var(--accent); }
        .setting-toggle-btn:active { transform: scale(0.95); }

        @media (max-width: 767px) { 
            .mouse-buttons { display: none; }
            .k { height: 38px; font-size: 11px; }
            .kb-fn-row .k { height: 28px; font-size: 9px; }
            .kb-toggle { width: 44px; height: 44px; font-size: 20px; }
        }
        
        @media (max-height: 500px) and (orientation: landscape) {
            .kb-panel { top: 0; bottom: 0; height: 100vh; height: 100dvh; display: flex; flex-direction: column; justify-content: stretch; padding: max(6px, env(safe-area-inset-top)) max(6px, env(safe-area-inset-right)) max(6px, env(safe-area-inset-bottom)) max(6px, env(safe-area-inset-left)); }
            .kb-row, .kb-fn-row { flex: 1; margin-bottom: 4px; }
            .kb-row:last-child { margin-bottom: 0; }
            .k, .kb-fn-row .k { height: auto; max-width: none; flex-basis: 0; border-radius: 6px; }
        }

        @media (min-width: 768px), (orientation: landscape) {
            .container { flex-direction: row; padding: 20px; gap: 20px; }
            .top-panel { width: 420px; flex: none; }
            .trackpad-area { flex: 1; }
            .k { font-size: 13px; } 
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="top-panel">
            <div class="panel-header">
                <div class="panel-title" id="modeTitle">Live Keyboard</div>
                <div class="header-actions">
                    <button class="btn" style="width: auto; padding: 6px 14px; flex: none;" onclick="toggleMode()">Toggle Ducky</button>
                    <button class="btn" style="width: auto; padding: 6px 10px; flex: none; font-size: 16px;" onclick="toggleSettingsModal()">⚙</button>
                </div>
            </div>
            
            <input type="text" id="kInput" placeholder="Tap here to type..." autocomplete="off" autocorrect="off" spellcheck="false">
            
            <select id="scriptSelect" style="display:none;" onchange="loadScript()"></select>
            <textarea id="duckyArea" placeholder="Write Ducky Script here..."></textarea>
            
            <div class="btn-group" id="btnGroup1">
                <button class="btn btn-primary" id="runBtn" onclick="runDucky()">Run</button>
                <button class="btn btn-record" id="recordBtn" onclick="toggleRecord()">Record</button>
                <button class="btn" onclick="saveScript()">Save</button>
            </div>
            <div class="btn-group" id="btnGroup2">
                <button class="btn" onclick="setStartup()">Set Startup</button>
                <button class="btn btn-danger" onclick="delStartup()">Clear</button>
                <button class="btn btn-danger" onclick="deleteScript()">Delete</button>
            </div>
        </div>

        <div class="trackpad-area">
            <div id="pad">
                <div style="font-size:26px; font-weight:800; letter-spacing:6px;" id="padText">TOUCH</div>
                <div style="font-size:13px; margin-top:12px; opacity: 0.6; font-weight: 500;">Double-Tap &amp; Hold to Drag</div>
            </div>
            <div class="mouse-buttons">
                <button class="mouse-btn" onclick="fastFetch('/cl'); triggerVibrate();">LEFT CLICK</button>
                <button class="mouse-btn" onclick="fastFetch('/cr'); triggerVibrate();">RIGHT CLICK</button>
            </div>
        </div>
    </div>

    <button class="kb-toggle" id="kbToggle">⌨</button>
    <div class="kb-panel" id="kbPanel"></div>

    <div class="modal-overlay" id="settingsModal">
        <div class="modal">
            <div class="panel-header">
                <div class="panel-title">Settings</div>
                <div class="header-actions">
                    <a href="https://github.com/leecheeyong/esp32-wifi-virtual-hid" target="_blank" class="btn" style="width: auto; flex: none; padding: 6px 12px; font-size: 13px; text-decoration: none; display: flex; align-items: center; justify-content: center;">GitHub</a>
                    <button class="btn" onclick="toggleSettingsModal()" style="width: auto; flex: none; padding: 6px 12px; font-size: 13px;">Close</button>
                </div>
            </div>
            <div class="setting-row">
                <span style="font-weight: 600; font-size: 14px;">Scroll Direction</span>
                <button id="btnScroll" class="setting-toggle-btn" onclick="toggleScrollDir()">Standard</button>
            </div>
            <div class="setting-row">
                <span style="font-weight: 600; font-size: 14px;">Click Vibration</span>
                <button id="btnVibrate" class="setting-toggle-btn" onclick="toggleVibrate()">Off</button>
            </div>
            <div class="setting-row">
                <span style="font-weight: 600; font-size: 14px;">Key Repeat</span>
                <button id="btnKeyRepeat" class="setting-toggle-btn" onclick="toggleKeyRepeat()">Off</button>
            </div>
        </div>
    </div>

    <script>
        const kInput = document.getElementById('kInput');
        const duckyArea = document.getElementById('duckyArea');
        const scriptSelect = document.getElementById('scriptSelect');
        const padText = document.getElementById('padText');
        const kbToggle = document.getElementById('kbToggle');
        let isDuckyMode = false;

        let scrollMultiplier = parseFloat(localStorage.getItem('airpad_scroll_mult')) || -0.3;
        let vibrateEnabled = localStorage.getItem('airpad_vibrate') === 'true';
        let keyRepeatEnabled = localStorage.getItem('airpad_key_repeat') === 'true';

        const btnScroll = document.getElementById('btnScroll');
        const btnVibrate = document.getElementById('btnVibrate');
        const btnKeyRepeat = document.getElementById('btnKeyRepeat');

        function syncSettingsUI() {
            if (scrollMultiplier > 0) { btnScroll.innerText = "Natural"; btnScroll.classList.add('active'); } 
            else { btnScroll.innerText = "Standard"; btnScroll.classList.remove('active'); }
            if (vibrateEnabled) { btnVibrate.innerText = "On"; btnVibrate.classList.add('active'); } 
            else { btnVibrate.innerText = "Off"; btnVibrate.classList.remove('active'); }
            if (keyRepeatEnabled) { btnKeyRepeat.innerText = "On"; btnKeyRepeat.classList.add('active'); } 
            else { btnKeyRepeat.innerText = "Off"; btnKeyRepeat.classList.remove('active'); }
        }

        function toggleScrollDir() { scrollMultiplier = scrollMultiplier > 0 ? -0.3 : 0.3; localStorage.setItem('airpad_scroll_mult', scrollMultiplier); syncSettingsUI(); }
        function toggleVibrate() { vibrateEnabled = !vibrateEnabled; localStorage.setItem('airpad_vibrate', vibrateEnabled); syncSettingsUI(); if (vibrateEnabled) triggerVibrate(25); }
        function toggleKeyRepeat() { keyRepeatEnabled = !keyRepeatEnabled; localStorage.setItem('airpad_key_repeat', keyRepeatEnabled); syncSettingsUI(); }
        function triggerVibrate(duration = 15) { if (vibrateEnabled && navigator.vibrate) { try { navigator.vibrate(duration); } catch(e){} } }
        function toggleSettingsModal() { document.getElementById('settingsModal').classList.toggle('open'); }
        syncSettingsUI();

        const fetchOpts = { keepalive: true, cache: 'no-store' };
        
        function fastFetch(url) { return fetch(url, fetchOpts).catch(function(){}); }
        
        let keyPromise = Promise.resolve();
        function keyFetch(url) {
            keyPromise = keyPromise.then(() => fetch(url, fetchOpts).catch(function(){})).then(() => new Promise(r => setTimeout(r, 8)));
        }

        function toggleMode() {
            if (isRecording) toggleRecord(); 
            
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
            let res = await fetch('/startup', { method: 'POST', body: duckyArea.value }).catch(function(){});
            if (res && res.ok) alert("Startup Script saved to ESP32 Flash!");
        }
        async function delStartup() {
            let res = await fetch('/startup', { method: 'POST', body: "" }).catch(function(){});
            if (res && res.ok) alert("Startup Script cleared from ESP32.");
        }

        let isRecording = false, lastActionTime = 0;
        
        async function toggleRecord() {
            const btn = document.getElementById('recordBtn');
            
            if (!isRecording) {
                btn.innerText = "⏳...";
                await fetch('/ducky?cmd=MOUSE_RESET').catch(function(){});
                
                isRecording = true;
                btn.classList.add('recording'); 
                btn.innerText = "⏹ Stop";
                
                kInput.style.display = 'block';
                kInput.placeholder = "Recording... Tap here to use native keyboard";
                
                duckyArea.value = "MOUSE_RESET\n"; 
                lastActionTime = Date.now();
                padText.innerText = "RECORDING";
            } else {
                isRecording = false;
                btn.classList.remove('recording'); 
                btn.innerText = "⏺ Record";
                padText.innerText = "TOUCH";
                
                kInput.style.display = 'none';
                kInput.placeholder = "Tap here to type...";
            }
        }

        function logAction(cmd) {
            if (!isRecording) return;
            let delay = Date.now() - lastActionTime;
            if (delay > 40) duckyArea.value += `DELAY ${delay}\n`;
            duckyArea.value += `${cmd}\n`;
            lastActionTime = Date.now();
            duckyArea.scrollTop = duckyArea.scrollHeight;
        }

        kInput.addEventListener('input', () => {
            let val = kInput.value;
            if (val.length > 0) {
                keyFetch('/k?text=' + encodeURIComponent(val));
                logAction(`STRING ${val}`); kInput.value = ''; 
            }
        });
        kInput.addEventListener('keydown', (e) => {
            if (e.key === 'Backspace') { keyFetch('/kb'); logAction('BACKSPACE'); }
            if (e.key === 'Enter') { keyFetch('/en'); logAction('ENTER'); }
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
                    await fetch('/ducky?cmd=' + encodeURIComponent(line)).catch(function(){});
                    await new Promise(r => setTimeout(r, 15));
                }
            }
            btn.innerText = "▶ Run";
        }

        const pad = document.getElementById('pad');
        let startX = 0, startY = 0, lastX = 0, lastY = 0;
        let accumX = 0, accumY = 0, accumScroll = 0;
        
        let isMoving = false, hasSwiped = false, maxTouches = 0;
        let touchStartTime = 0, lastTapTime = 0;
        let potentialDrag = false, isDraggingPad = false;
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
                if (potentialDrag && !isDraggingPad && e.touches.length === 1) {
                    isDraggingPad = true;
                    pad.classList.add('dragging');
                    fastFetch('/md'); logAction('MOUSE_DOWN');
                    padText.innerText = "DRAGGING";
                }
            }

            if (e.touches.length === 1) { 
                let speed = Math.sqrt(dx*dx + dy*dy);
                let accel = speed > 10 ? 1.8 : 1.2; 
                accumX += dx * accel; accumY += dy * accel; 
            } 
            else if (e.touches.length === 2) { 
                accumScroll += dy * scrollMultiplier; 
            }
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
                if (isDraggingPad) {
                    isDraggingPad = false;
                    pad.classList.remove('dragging');
                    fastFetch('/mu'); logAction('MOUSE_UP');
                    padText.innerText = isRecording ? "RECORDING" : "TOUCH";
                    potentialDrag = false;
                } 
                else if (!hasSwiped && Date.now() - touchStartTime < 250) {
                    if (maxTouches === 1) { 
                        fastFetch('/cl'); logAction('MOUSE_CLICK LEFT'); 
                        triggerVibrate();
                        lastTapTime = Date.now(); 
                    }
                    else if (maxTouches === 2) { 
                        fastFetch('/cr'); logAction('MOUSE_CLICK RIGHT'); 
                        triggerVibrate();
                    }
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
                
                fastFetch(`/m?dx=${sendX}&dy=${sendY}&s=${sendS}`).finally(function() {
                    isSending = false; 
                });
            }
        }, 15);

        const kbPanel = document.getElementById('kbPanel');
        const modCodes = new Set([0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87]);
        let kbOpen = false, heldMods = new Set();
        let kbTStartX = 0, kbTStartY = 0, kbTInitLeft = 0, kbTInitTop = 0;
        let isDraggingKB = false, clickCount = 0, clickTimer = null;
        let repeatTimeout = null, repeatInterval = null, activeRepeatBtn = null;

        const shiftPairs = {
            '`': '~', '1': '!', '2': '@', '3': '#', '4': '$', '5': '%',
            '6': '^', '7': '&', '8': '*', '9': '(', '0': ')', '-': '_', '=': '+',
            '[': '{', ']': '}', '\\': '|', ';': ':', "'": '"', ',': '<', '.': '>', '/': '?'
        };

        function escapeHtml(str) {
            return String(str)
                .replace(/&/g, '&amp;')
                .replace(/"/g, '&quot;')
                .replace(/'/g, '&#39;')
                .replace(/</g, '&lt;')
                .replace(/>/g, '&gt;');
        }

        function stopKeyRepeat() {
            if (repeatTimeout) { clearTimeout(repeatTimeout); repeatTimeout = null; }
            if (repeatInterval) { clearInterval(repeatInterval); repeatInterval = null; }
            if (activeRepeatBtn) { activeRepeatBtn.classList.remove('held'); activeRepeatBtn = null; }
        }

        window.addEventListener('pointerup', stopKeyRepeat);
        window.addEventListener('pointercancel', stopKeyRepeat);
        window.addEventListener('resize', () => {
            if (!kbToggle.style.left && !kbToggle.style.top) return;
            const rect = kbToggle.getBoundingClientRect();
            kbToggle.style.left = Math.min(Math.max(0, rect.left), window.innerWidth - kbToggle.offsetWidth) + 'px';
            kbToggle.style.top = Math.min(Math.max(0, rect.top), window.innerHeight - kbToggle.offsetHeight) + 'px';
        });

        function toggleKB() {
            kbOpen = !kbOpen;
            kbPanel.classList.toggle('open', kbOpen);
            kbToggle.classList.toggle('active', kbOpen);
            if (!kbOpen) releaseAllMods();
        }

        function toggleFullscreen() {
            if (!document.fullscreenElement) { document.documentElement.requestFullscreen().catch(function(){}); } 
            else { if (document.exitFullscreen) document.exitFullscreen(); }
        }

        kbToggle.addEventListener('pointerdown', (e) => {
            e.preventDefault(); kbToggle.setPointerCapture(e.pointerId);
            kbTStartX = e.clientX; kbTStartY = e.clientY;
            const rect = kbToggle.getBoundingClientRect();
            kbTInitLeft = rect.left; kbTInitTop = rect.top;
            kbToggle.style.left = kbTInitLeft + 'px'; kbToggle.style.top = kbTInitTop + 'px';
            kbToggle.style.bottom = 'auto'; kbToggle.style.right = 'auto'; kbToggle.style.transition = 'none';
            isDraggingKB = false;
        });

        kbToggle.addEventListener('pointermove', (e) => {
            if (!kbToggle.hasPointerCapture(e.pointerId)) return;
            const dx = e.clientX - kbTStartX, dy = e.clientY - kbTStartY;
            if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                isDraggingKB = true;
                kbToggle.style.left = Math.max(0, Math.min(kbTInitLeft + dx, window.innerWidth - kbToggle.offsetWidth)) + 'px';
                kbToggle.style.top = Math.max(0, Math.min(kbTInitTop + dy, window.innerHeight - kbToggle.offsetHeight)) + 'px';
            }
        });

        kbToggle.addEventListener('pointerup', (e) => {
            kbToggle.releasePointerCapture(e.pointerId);
            kbToggle.style.transition = 'all 0.2s, background 0.2s';
            if (!isDraggingKB) {
                clickCount++;
                if (clickCount === 1) clickTimer = setTimeout(() => { clickCount = 0; }, 500);
                if (clickCount === 3) { clearTimeout(clickTimer); toggleFullscreen(); clickCount = 0; }
                toggleKB(); 
            }
        });

        function updateKeyLabels() {
            const isShift = heldMods.has(0x81) || heldMods.has(0x85);
            
            kbPanel.querySelectorAll('.k[data-unch]').forEach(btn => {
                if (isShift) {
                    btn.innerText = btn.dataset.shlabel;
                    btn.dataset.ch = btn.dataset.shch;
                } else {
                    btn.innerText = btn.dataset.unlabel;
                    btn.dataset.ch = btn.dataset.unch;
                }
            });

            kbPanel.querySelectorAll('.k.mod').forEach(b => {
                const c = b.dataset.c ? parseInt(b.dataset.c) : null;
                if (c !== null && heldMods.has(c)) {
                    b.classList.add('held');
                } else {
                    b.classList.remove('held');
                }
            });
        }

        function processKeyPress(btn) {
            if (btn.dataset.winInstant === 'true') {
                triggerVibrate(15);
                btn.classList.add('held');
                keyFetch('/kp?c=128');
                keyFetch('/kp?c=177');
                logAction('CTRL ESC');
                setTimeout(() => {
                    keyFetch('/kr?c=177');
                    keyFetch('/kr?c=128');
                    btn.classList.remove('held');
                }, 60);
                releaseAllMods();
                return;
            }

            const code = btn.dataset.c ? parseInt(btn.dataset.c) : null;
            const ch = btn.dataset.ch;

            triggerVibrate(15);

            if (code !== null && modCodes.has(code)) {
                if (heldMods.has(code)) {
                    heldMods.delete(code);
                    keyFetch('/kr?c=' + code);
                } else {
                    heldMods.add(code);
                    keyFetch('/kp?c=' + code);
                }
                updateKeyLabels();
                return;
            }

            const getDuckyKey = (btnText, keyCode) => {
                if (keyCode === 0x20) return 'SPACE'; 
                if (btnText === '⌫') return 'BACKSPACE';
                if (btnText === 'Enter') return 'ENTER';
                if (btnText === 'Tab') return 'TAB';
                if (btnText === 'Esc') return 'ESCAPE';
                if (btnText === 'Caps') return 'CAPSLOCK';
                if (btnText === 'Del') return 'DELETE';
                if (btnText === '↑') return 'UPARROW';
                if (btnText === '↓') return 'DOWNARROW';
                if (btnText === '←') return 'LEFTARROW';
                if (btnText === '→') return 'RIGHTARROW';
                if (btnText === 'PgUp') return 'PAGEUP';
                if (btnText === 'PgDn') return 'PAGEDOWN';
                if (btnText === 'Ins') return 'INSERT';
                return btnText.toUpperCase();
            };

            const sendKey = () => {
                let modPrefix = "";
                if (heldMods.has(0x80)) modPrefix += "CTRL ";
                if (heldMods.has(0x82) || heldMods.has(0x86)) modPrefix += "ALT ";
                if (heldMods.has(0x83)) modPrefix += "GUI ";

                if (ch !== undefined) {
                    btn.classList.add('held');
                    let sendCh = ch;
                    let charModPrefix = modPrefix;
                    if ((heldMods.has(0x81) || heldMods.has(0x85)) && charModPrefix !== "") {
                        charModPrefix += "SHIFT ";
                    }

                    keyFetch('/k?text=' + encodeURIComponent(sendCh));
                    
                    if (charModPrefix) {
                        logAction((charModPrefix + sendCh.toLowerCase()).trim());
                    } else if (sendCh === ' ') {
                        logAction('SPACE');
                    } else {
                        logAction(`STRING ${sendCh}`);
                    }

                    setTimeout(() => { if (!repeatInterval) btn.classList.remove('held'); }, 80);
                    releaseAllMods();
                } else if (code !== null) {
                    btn.classList.add('held');
                    let keyModPrefix = modPrefix;
                    if (heldMods.has(0x81) || heldMods.has(0x85)) {
                        keyModPrefix += "SHIFT ";
                    }

                    keyFetch('/kp?c=' + code);
                    let keyName = getDuckyKey(btn.innerText, code);
                    
                    if (keyModPrefix) {
                        logAction((keyModPrefix + keyName).trim());
                    } else {
                        logAction(keyName);
                    }

                    setTimeout(() => {
                        keyFetch('/kr?c=' + code);
                        if (!repeatInterval) btn.classList.remove('held');
                    }, 60);
                    releaseAllMods();
                }
            };

            sendKey();

            if (keyRepeatEnabled) {
                stopKeyRepeat();
                activeRepeatBtn = btn;
                repeatTimeout = setTimeout(() => {
                    repeatInterval = setInterval(() => { sendKey(); triggerVibrate(8); }, 50);
                }, 300);
            }
        }

        function buildKB() {
            const fnRow = [['Esc',0xB1],['F1',0xC2],['F2',0xC3],['F3',0xC4],['F4',0xC5],['F5',0xC6],['F6',0xC7],['F7',0xC8],['F8',0xC9],['F9',0xCA],['F10',0xCB],['F11',0xCC],['F12',0xCD]];
            const rows = [
                [['`','`'],['1','1'],['2','2'],['3','3'],['4','4'],['5','5'],['6','6'],['7','7'],['8','8'],['9','9'],['0','0'],['-','-'],['=','='],['⌫',0xB2,'wide']],
                [['Tab',0xB3,'wide'],['Q','q'],['W','w'],['E','e'],['R','r'],['T','t'],['Y','y'],['U','u'],['I','i'],['O','o'],['P','p'],['[','['],[']',']'],['\\','\\']],
                [['Caps',0xC1,'wide'],['A','a'],['S','s'],['D','d'],['F','f'],['G','g'],['H','h'],['J','j'],['K','k'],['L','l'],[';',';'],["'","'"],['Enter',0xB0,'wider']],
                [['Shift',0x81,'wider mod'],['Z','z'],['X','x'],['C','c'],['V','v'],['B','b'],['N','n'],['M','m'],[',',','],['.','.'],['/','/'],['Shift',0x85,'wide mod']],
                [['Ctrl',0x80,'wide mod'],['Win',0x83,'mod'],['Alt',0x82,'mod'],['',0x20,'space'],['Alt',0x86,'mod'],['Del',0xD4],['←',0xD8],['↑',0xDA],['↓',0xD9],['→',0xD7]]
            ];

            let h = '<div class="kb-fn-row">';
            for (const [l,c] of fnRow) h += `<button class="k fn" data-c="${c}">${escapeHtml(l)}</button>`;
            h += '</div>';

            for (const row of rows) {
                h += '<div class="kb-row">';
                for (const [l, v, cls] of row) {
                    if (typeof v === 'string') {
                        const unch = v;
                        const shch = shiftPairs[v] || v.toUpperCase();
                        const unlabel = l;
                        const shlabel = shiftPairs[l] || (l.length === 1 ? l.toUpperCase() : l);

                        h += `<button class="k ${cls||''}" data-unch="${escapeHtml(unch)}" data-shch="${escapeHtml(shch)}" data-unlabel="${escapeHtml(unlabel)}" data-shlabel="${escapeHtml(shlabel)}" data-ch="${escapeHtml(unch)}">${escapeHtml(unlabel)}</button>`;
                    } else {
                        h += `<button class="k ${cls||''}" data-c="${v}">${escapeHtml(l)}</button>`;
                    }
                }
                h += '</div>';
            }

            h += '<div class="kb-fn-row">';
            const navKeys = [['Win','win-instant'],['Ins',0xD1],['Home',0xD2],['End',0xD5],['PgUp',0xD3],['PgDn',0xD6]];
            for (const [l,c,cls] of navKeys) {
                if (c === 'win-instant') {
                    h += `<button class="k fn" data-win-instant="true">${escapeHtml(l)}</button>`;
                } else {
                    h += `<button class="k fn ${cls||''}" data-c="${c}">${escapeHtml(l)}</button>`;
                }
            }
            h += '</div>';

            kbPanel.innerHTML = h;

            kbPanel.addEventListener('pointerdown', (e) => {
                const btn = e.target.closest('.k');
                if (!btn) return;
                e.preventDefault();
                processKeyPress(btn);
            });
        }

        function releaseAllMods() {
            if (heldMods.size === 0) return;
            heldMods.forEach(c => keyFetch('/kr?c=' + c));
            heldMods.clear();
            updateKeyLabels();
        }

        buildKB();
    </script>
</body>
</html>
)rawliteral";
