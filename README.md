# ESP32 WiFi Virtual Human Interface Device
Turn an ESP32 into a wireless plug and play keyboard & mouse

## Tested Devices
- [x] ESP32-S2 Mini
- [x] ESP32-S3 N16R8

## Features
- **Native USB HID:** Acts as a standard plug-and-play USB mouse and keyboard. No client software required on the host computer.
- **Standalone Network:** Creates its own Wi-Fi network (Access Point), meaning it works anywhere without needing an existing router.
- **On-Screen Trackpad:** Smooth and optimized deadzone filtering to prevent cursor drift, network flooding, and jitter.
- **Web UI:** A minimalist interface that scales perfectly across portrait and landscape orientations on mobile/tablets.

## Setup and Installation
1. **Configure Arduino IDE:**
    - Board: Select your ESP32-S2 board model.
    - USB CDC On Boot: Enabled (if applicable to your board variant).
    - Dependencies: No external libraries required. 
2. **Upload:**
    - Connect the board, select the correct COM port, and upload the sketch.

## Usage
1. Plug the ESP32 into the computer you want to control.
2. Connect to the Wi-Fi network named `ESP32 Virtual Hid`
3. Open a web browser and navigate to: [http://192.168.4.1](http://192.168.4.1)
4. Swipe the trackpad area to move the mouse, tap to click, and use the input field to type text.
