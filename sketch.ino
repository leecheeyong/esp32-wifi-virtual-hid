/**
* ESP32 WiFi Virtual HID
* Copyright (C) 2026-present  Lee Chee Yong
* @license GNU Affero General Public License v3.0
**/
#include "WiFi.h"
#include "WebServer.h"
#include "USB.h"
#include "USBHIDMouse.h"
#include "USBHIDKeyboard.h"
#include "html.h"
#include <Preferences.h>

USBHIDMouse Mouse;
USBHIDKeyboard Keyboard;
WebServer server(80);
Preferences preferences;

String ssid = "ESP32 Virtual HID";
String password = "password123";

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
        case 0xB0: return KEY_RETURN;      // 176
        case 0xB1: return KEY_ESC;         // 177
        case 0xB2: return KEY_BACKSPACE;   // 178
        case 0xB3: return KEY_TAB;         // 179
        case 0xD1: return KEY_INSERT;      // 209
        case 0xD2: return KEY_HOME;        // 210
        case 0xD3: return KEY_PAGE_UP;     // 211
        case 0xD4: return KEY_DELETE;      // 212
        case 0xD5: return KEY_END;         // 213
        case 0xD6: return KEY_PAGE_DOWN;   // 214
        case 0xD7: return KEY_RIGHT_ARROW; // 215
        case 0xD8: return KEY_LEFT_ARROW;  // 216
        case 0xD9: return KEY_DOWN_ARROW;  // 217
        case 0xDA: return KEY_UP_ARROW;    // 218
        case 0x80: return KEY_LEFT_CTRL;   // 128
        case 0x81: return KEY_LEFT_SHIFT;  // 129
        case 0x82: return KEY_LEFT_ALT;    // 130
        case 0x83: return KEY_LEFT_GUI;    // 131
        case 0x84: return KEY_RIGHT_CTRL;  // 132
        case 0x85: return KEY_RIGHT_SHIFT; // 133
        case 0x86: return KEY_RIGHT_ALT;   // 134
        case 0x87: return KEY_RIGHT_GUI;   // 135
        case 0xC1: return KEY_CAPS_LOCK;   // 193
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

void setup() {
    Mouse.begin(); 
    Keyboard.begin(); 
    USB.begin();
    
    preferences.begin("airpad", false);
    ssid = preferences.getString("wifi_ssid", ssid);
    password = preferences.getString("wifi_pass", password);
    
    WiFi.softAP(ssid.c_str(), password.c_str());
    
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
    
    // Drag Support Endpoints
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

    server.on("/wifi", HTTP_POST, [](){
        if (server.hasArg("ssid") || server.hasArg("pass")) {
            if (server.hasArg("ssid")) {
                String newSsid = server.arg("ssid");
                if (newSsid.length() > 0) {
                    preferences.putString("wifi_ssid", newSsid);
                }
            }
            if (server.hasArg("pass")) {
                String newPass = server.arg("pass");
                preferences.putString("wifi_pass", newPass);
            }
            sendOK();
            delay(500);
            ESP.restart(); 
        } else {
            server.send(400, "text/plain", "Bad Request");
        }
    });

    server.begin();
}

void loop() { 
    server.handleClient(); 
}
