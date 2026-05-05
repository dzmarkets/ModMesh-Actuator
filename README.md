# ESP-NOW Multi-Node Mesh Network (ESP32-S3)

* **File Path:** `README.md`
* **Author:** M. YOUCEF Yazid (yazid.youcef@gmail.com)
* **Version:** 0.6.0 (Multi-Node Sync Edition)
* **Update Date:** 2026-05-04

---

## 📖 Overview
This project provides a production-ready, highly modular framework for building **Encrypted ESP-NOW Mesh Networks** using the **ESP32-S3**. While standard ESP-NOW is limited by point-to-point range, this implementation utilizes a **Multi-Hop Flooding** architecture, allowing messages to be rebroadcasted across an entire building or industrial site without requiring a central router or Wi-Fi AP.

Designed for engineers and IoT students, the codebase is strictly developed using **ESP-IDF V5.4.1**. It addresses the biggest challenges in ESP-NOW networking:
1.  **Security:** How to prevent eavesdropping and replay attacks.
2.  **Reliability:** How to guarantee message delivery without a router.
3.  **Observability:** How to know the exact health and status of every node in real-time.
4.  **Performance (New in 0.6.0):** Utilizing a **Tri-Task RTOS Architecture** to ensure real-time sensor polling (50ms) and hardware execution are never blocked by network operations.

By combining hardware-accelerated AES-128 encryption with a custom application-layer ACK system and a background health monitor, this project turns the ESP32-S3 into a powerful, secure mesh communicator.

---

## 🚀 Key Features

### ⚡ Tri-Task Real-Time Engine (v0.6.0)
*   **Decoupled Architecture:** The system uses three fully independent FreeRTOS tasks to guarantee zero blocking across the stack.
*   **High-Priority Sensor Task (Priority 10):** Polls hardware sensors every **50ms**. Since it never performs networking, sensor readings are truly "Real-Time" and never lag.
*   **Dedicated Actuator Task (Priority 7):** Uses a high-speed **FreeRTOS Queue** to decouple physical output commands (like slow Modbus UART writes) from the ESP-NOW driver context. This prevents hardware delays from blocking the radio and causing packet loss.
*   **Background Mesh Task (Priority 5):** Manages the health registry, heartbeats, and ACKs without interfering with the physical hardware layer.

### 🛡️ Secure-Mesh Architecture
*   **Military-Grade Encryption:** Every packet is encrypted using **AES-128-CBC** via the `mbedtls` library. 
*   **Randomized IVs:** Each transmission generates a unique **16-byte Initialization Vector (IV)**. This ensures that even identical sensor readings produce completely different ciphertext, making the network immune to packet identification and **Replay Attacks**.
*   **Private Mesh:** Only nodes with the matching **16-character API Key** can join the conversation, creating a secure, isolated communication environment.

### 📊 Real-Time Health & Connectivity
*   **Original MAC Identification:** Nodes track the **True Origin** of a message (extracted from the payload) rather than the immediate sender. This ensures your mesh map remains accurate even if messages hop through multiple routers.
*   **Active Peer Registry:** Each node maintains an internal "RAM Table" tracking the **Device Name**, **MAC Address**, and **Online Status** of every peer it has discovered.
*   **High-Frequency Heartbeats:** Implements a **5-second Keep-Alive** mechanism. Nodes check in frequently, ensuring the mesh stays highly synchronized.
*   **Autonomous Timeout (30s):** If a peer is silent for more than 30 seconds (6 missed check-ins), the `mesh_manager` automatically marks them as **OFFLINE**, triggers a serial warning, and updates the local LED status.
*   **NVS MAC-List Persistence:** Unlike standard mesh networks that "forget" their neighbors after a power cut, this system stores the **full Known Peer List** (MACs and Names) in NVS. Upon reboot, the node immediately knows exactly who should be in its mesh, allowing it to detect a "Partial Mesh" state the moment it powers on.

### 🚥 Intelligent Visual Diagnostics (RGB LED)
The system uses a high-priority background task to provide instant visual feedback on the state of the entire network:
*   🔴 **RED (Isolated):** The node is powered on but has **zero active peers** in range.
*   🟢 **GREEN SOLID (Perfect):** The node is connected, and **100% of all registered peers** are currently online.
*   🟢 **GREEN BLINKING (Warning):** The node is connected, but **some peers have gone offline**. This provides an immediate alert if a device in another room has lost power.
*   🔵 **BLUE (Activity):** Indicates a transmission is in progress and the node is waiting for a delivery receipt (ACK).

### 🧹 Factory Reset (How to use the Reset)
Because nodes remember their peers permanently in NVS, you must perform a **Factory Reset** if you permanently remove a device from your network. Otherwise, the remaining nodes will permanently show a "Partial Mesh" (Blinking Green) state because they are still waiting for the missing device.

**To perform a reset:**
1.  Press and **hold your Primary Control Button (GPIO 1)** on your ESP32-S3.
2.  While holding **GPIO 1**, press the **EN (Reset) button** or power the device on.
3.  Continue holding the **button for 3 seconds**.
4.  The RGB LED will **flash RED at medium speed (300ms)** for several cycles to confirm the mesh identity has been erased.
5.  The node will now restart discovery and register the first new peers it hears as its new mesh family.

> [!IMPORTANT]
> To decommission a node, you should ideally run this reset on **all remaining nodes** so they stop looking for the removed MAC address.

### ⚡ Smart Flooding & Efficiency
*   **Sequence Number Flooding:** Every packet includes a unique 32-bit sequence number. This allows all traffic (including identical heartbeats) to be rebroadcasted across multiple hops, making the network truly self-healing and dynamic.
*   **Deduplication Cache:** A high-speed **128-slot `djb2` hash cache** remembers recently seen packets, ensuring that flooded messages do not loop infinitely between nodes.
*   **Permanent STA Architecture:** Nodes operate exclusively in `STA` mode. This eliminates the 50-100ms "radio dropout" that occurs during mode switching, ensuring that no incoming mesh messages are missed while a node is busy transmitting.

---

## 📂 Component Architecture

The project is strictly modular. Each component is a standalone directory under `components/`, ensuring a clean separation of concerns:

### 1. `espnow_control` (The Networking Layer)
*   **Wi-Fi Management:** Initializes the ESP32-S3 Wi-Fi stack in permanent `STA` mode.
*   **Stability Optimization:** Maintains a fixed channel (Channel 1) to ensure all nodes are always synchronized without the overhead of Access Point beaconing.
*   **Peer Management:** Wraps the low-level `esp_now_add_peer` API to simplify broadcast and unicast registrations.

### 2. `mesh_manager` (The Network Accountant)
*   **Real-Time Tracking:** Maintains a `peer_info_t` array in RAM that stores the name, original MAC address, and "last seen" timestamp of every device in the network.
*   **MAC-List Persistence:** Instead of just a count, it persists the **full binary blob of MAC addresses and Names** to Flash memory (NVS). This ensures that rebooted nodes do not lose their identity within the mesh.
*   **Health Monitoring:** Runs a background check to calculate which peers are still active (responded within 30s) vs. those that have gone offline.

### 3. `message_processor` (The Logic Engine)
*   **Packet Parsing & MAC Tracking:** Decodes the binary wire format. Parses the original sender's MAC from the encrypted payload to accurately track peers over multiple hops.
*   **Deduplication:** Implements a 128-slot `djb2` hash cache. It hashes the decrypted plaintext (which includes the unique sequence number) to detect if a message received via a multi-hop flood has already been processed.
*   **Dynamic Rebroadcasting:** Rebroadcasts unique messages to the rest of the mesh, allowing the network to dynamically reroute traffic if an intermediate node goes offline.

### 4. `message_provider` (The Data Packager)
*   **Payload Formatting:** Builds the unique plaintext string in the format: `[NAME | MAC | SEQ] SENSOR_DATA`.
*   **Encryption Wrapper:** Interfaces with the `security_manager` to generate a random 16-byte IV for every single packet, ensuring that even identical sensor readings look unique over the air.

### 5. `blink_led` (The Visual Driver)
*   **Dual Hardware Support:** Supports both standard **3-pin RGB LEDs** (GPIO toggling) and **WS2812B Addressable LEDs** (using the RMT/led_strip driver).
*   **Asynchronous Blinking:** Uses the `esp_timer` API to toggle the LED at 500ms intervals for the "Partial Mesh" state, ensuring the main application loop is never blocked.

### 6. `security_manager` (The Security Core)
*   **AES-128-CBC:** Implements encryption and decryption using the `mbedtls` library.
*   **Padding:** Handles PKCS#7 padding to ensure all payloads are multiples of the 16-byte AES block size.
*   **Key Security:** Uses the project's API Key as the Pre-Shared Key (PSK) for the mesh.

### 7. `sensors` & `actuators` (Hardware Interface)
*   **Sensors:** Provides a unified `sensors_read()` API. Depending on the `ACTIVE_APP_SAMPLE`, it reads simulated values, physical button states, or manages a 4-button array for multi-node control.
*   **Actuators:** Implements `actuators_execute()`. It can trigger simulated responses, toggle physical relays, synchronize a 4-LED array across the mesh, or bridge data to MOD-BUS (MAX485).

### 8. `device_reset` (The System Reset Utility)
*   **Encapsulated Logic:** Centralizes the boot-time factory reset trigger.
*   **Generic Capability:** Currently wipes mesh identity, but designed to be expanded for WiFi or NVS resets.
*   **Visual Feedback:** Manages the medium-speed Red blink pattern (300ms) to confirm successful data erasure.

### 9. `shared_config` (Global Configuration)
*   **Single Source of Truth:** Centralizes all GPIO assignments, Wi-Fi channels, timeout constants, and application mode toggles. Modifying this one file reconfigures the entire project.

---

## 🛠️ Hardware Setup

Before running the samples, ensure your hardware is wired correctly according to the `ACTIVE_APP_SAMPLE` you plan to use. All GPIO pins are easily configurable in `shared_config.h`.

* **Minimum Requirement:** 2 to 25 ESP32-S3 development boards.

### 1. Base Setup (Required for all samples)
You need a visual indicator for the mesh health monitor. You have two choices:
*   **Internal WS2812B (Default):** Most ESP32-S3 boards have a built-in addressable LED on **GPIO 48**. Set `USE_EXTERNAL_RGB_LED` to `0`.
*   **External RGB LED:** If using a physical 3-pin common-cathode LED, connect the anodes to **Red (GPIO 10)**, **Green (GPIO 11)**, and **Blue (GPIO 12)** through 220Ω resistors. Connect the cathode to GND. Set `USE_EXTERNAL_RGB_LED` to `1`.

### 2. Sample 2 & 3 Wiring (Remote Hardware Control)
To test physical mesh interactions, wire the following components on **each** board:
*   **Push Button (Sensor):** Connect one side of a momentary push-button to **GPIO 1** and the other side to **GND**. **Note:** Use an external **10kΩ pull-up resistor** between GPIO 1 and 3.3V for maximum protection.
*   **External LED / Relay (Actuator):** Connect the positive pin of an LED (via a 220Ω resistor) or the signal pin of a Relay module to **GPIO 6**. Connect the negative pin to GND.

### 3. Sample 4 Wiring (4-Node Sync Control)
To test the 4-channel synchronization across multiple nodes:
*   **4 Push Buttons:** Connect to **GPIO 2, 3, 4, 5** and **GND**. **Note:** Connect each GPIO to 3.3V through a **10kΩ pull-up resistor** for maximum protection.
*   **4 LEDs:** Connect to **GPIO 6, 7, 8, 9** (anodes) and **GND**.

### 4. Sample 5 Wiring (Industrial MOD-BUS Bridge)
To test the RS485 wireless bridge, connect an ESP32 to a **MAX485 TTL-to-RS485 module** on **each** node:
*   **DI (TX):** Connect to **GPIO 17**.
*   **RO (RX):** Connect to **GPIO 18**.
*   **RE & DE:** Tie both pins together and connect them to **GPIO 16**.
*   **VCC & GND:** Connect to the ESP32's 3.3V/5V and GND.

---

## 🧪 Application Samples

The project includes pre-integrated samples. Toggle them by changing `#define ACTIVE_APP_SAMPLE` in `components/shared_config/include/shared_config.h`.

### Sample 1: Intelligent Climate Simulation (`ACTIVE_APP_SAMPLE 1`)
*   **Purpose:** Software-only simulation of environmental monitoring.
*   **Behavior:** Cycles temperature (24.0°C - 25.5°C). Alerts mesh nodes at 25.0°C.

### Sample 2: Remote Powering LED (Momentary) (`ACTIVE_APP_SAMPLE 2`)
*   **Behavior:** The LED on GPIO 6 follows the button on GPIO 1. It is a "momentary" switch—the LED is ON only while the button is held down.

### Sample 3: Remote Powering LED (Toggle) (`ACTIVE_APP_SAMPLE 3`)
*   **Behavior:** Pressing the button on GPIO 1 toggles the LED on GPIO 6. The LED stays in its new state until the button is pressed again.

### Sample 4: 4-Node Sync Control (`ACTIVE_APP_SAMPLE 4`)
*   **Purpose:** Synchronizes 4 independent channels across 4 nodes.
*   **Logic:** Pressing Button 1 on Node A toggles LED 1 on **all nodes** in the mesh.
*   **Sync Mechanism:** Uses absolute state broadcasting (`CMD:LEDS:1,0,0,1`) to ensure eventual consistency. If any node misses a toggle, the next heartbeat or press will synchronize it.

### Sample 5: Industrial MOD-BUS Bridge (`ACTIVE_APP_SAMPLE 5`)
*   **Purpose:** Wireless extension for RS485/MOD-BUS industrial networks.
*   **Operation:** Receives mesh packets and forwards them to the MAX485 UART (GPIO 17/16). Broadcasts any data received on the RS485 bus back to the mesh.

---

## ⚙️ Configuration & Installation

### 1. Environment Prerequisites
*   **ESP-IDF Version:** Strictly **V5.4.1**.
*   **Operating System:** Windows, Linux, or macOS.
*   **Setup:** Follow the [official Espressif guide](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/get-started/index.html) to install the tools.
*   **Environment Variables:** Always ensure the IDF tools are in your path. Run:
    *   **Windows:** `C:\Espressif\frameworks\esp-idf-v5.4.1\export.bat`
    *   **Linux/macOS:** `. $HOME/esp/esp-idf/export.sh`

### 2. SDK Configuration (Advanced)
The project is optimized for the **ESP32-S3 N16R8** (16MB Flash, 8MB PSRAM). To adjust for different S3 variants:
1.  Run the configuration menu:
    ```bash
    idf.py menuconfig
    ```
2.  Navigate to **Serial Flasher Config** to change the Flash size.
3.  Navigate to **Component Config → ESP32S3-Specific** to enable/disable PSRAM.
4.  Navigate to **Component Config → Wi-Fi** to adjust the ESP-NOW buffer sizes.

### 3. Hardware & Timing Customization
All application constants are centralized in `components/shared_config/include/shared_config.h`. You must review these before flashing:
*   **LED Selection:** Set `USE_EXTERNAL_RGB_LED` to `1` for discrete LEDs or `0` for integrated WS2812B.
*   **GPIO Mapping:** Update `RGB_LED_*_GPIO`, `BUTTON_SENSOR_GPIO`, and `MAX485_*_GPIO` to match your specific wiring.
*   **Mesh Health Timing:**
    *   `MESH_PEER_TIMEOUT_MS` (Default 6,000ms): Time before a node is declared offline.
    *   `MESH_KEEPALIVE_INTERVAL_MS` (Default 1,000ms): Frequency of heartbeat pings.

### 4. Security Configuration
The network uses AES-128 encryption. To secure your private mesh:
1.  Locate `NETWORK_API_KEY` in `shared_config.h`.
2.  Change this to a unique **16-character string**.
3.  **Important:** All nodes in your mesh MUST share the exact same key, or they will be unable to decrypt each other's data.

### 5. Deployment Identity
In `main/main.c`, assign a unique label to each physical board to enable individual tracking in the serial logs:
```c
#define NODE_NAME "NODE_A" // NODE_B, NODE_C, etc.
```

---

## 💻 Usage Guide

### Step 1: Flashing Individual Nodes
1.  Connect your first ESP32-S3 board to your computer.
2.  Open `main/main.c` and set `#define NODE_NAME "NODE_A"`.
3.  Build and flash the board:
    ```bash
    idf.py -p COM3 build flash monitor
    ```
4.  Connect your second board, change the name to `"NODE_B"`, and flash it (using its specific COM port).

### Step 2: Real-Time Network Monitoring
Once flashed, open the serial monitor for each node. You should observe the following sequence:
1.  **Initialisation:** The node logs its MAC address and starts in `STA` mode.
2.  **Discovery:** When `NODE_B` sends a packet, `NODE_A` will log:
    `[I] ESPNOW_CTRL: Registered peer 40:22:D8:XX:XX:XX (total peers: 1)`
3.  **Data Exchange:** You will see incoming data logs:
    `[I] msg_proc: Payload from NODE_B (40:22:D8:XX): TEMP:24.5C`

### Step 3: Visual Diagnosis (The LED Guide)
The RGB LED provides instant feedback on the health of your network:

| LED Color | Pattern | Meaning | Action Needed |
| :--- | :--- | :--- | :--- |
| 🔴 **Red** | Solid | **Disconnected.** This node sees 0 active peers. | Check if other nodes are powered on and in range. |
| 🟢 **Green** | Solid | **Full Health.** All peers ever registered are currently online. | None. The mesh is 100% operational. |
| 🟢 **Green** | Blinking | **Partial Mesh.** At least one peer is online, but some are missing. | Check if one of your nodes has lost power or crashed. |
| 🔵 **Blue** | Brief | **Transmitting.** The node is sending data and waiting for an ACK. | None. Normal operation during data changes. |

### Step 4: Testing Mesh Robustness
To verify the health monitoring system:
1.  Start with 3 nodes (NODE_A, B, and C). All LEDs should be **Solid Green**.
2.  Unplug **NODE_C**.
3.  Within **30 seconds**, the LEDs on **NODE_A** and **NODE_B** will start **Blinking Green**.
4.  The serial monitor will log: `[W] mesh_mgr: Device NODE_C (MAC) is OFFLINE`.
5.  Plug **NODE_C** back in. The LEDs will return to **Solid Green** within 5 seconds.

---

## 📚 Educational Notes

This project is designed as a learning platform for advanced mesh networking. Here are the core concepts students should focus on:

### 1. The "Flooding Loop" Problem & Deduplication
In a broadcast-based mesh, when Node A sends a message, Node B receives it and rebroadcasts it. Node A then receives its own message back from Node B. Without protection, Node A would rebroadcast it again, creating an infinite loop that crashes the network.
*   **The Solution:** We use a `djb2` hash of the decrypted message content. Every node stores the last 128 hashes it has seen. If it sees a hash already in its memory, it **silently drops** the message instead of rebroadcasting it.

### 2. Encryption & The "Identifiability" Risk
If you encrypt "TEMP:25" with a static key, the resulting binary data is always the same. An attacker might not know what it means, but they can see you are sending the *same* message repeatedly. They could even "record" that signal and play it back later (a **Replay Attack**) to trick your actuators.
*   **The Solution:** Every packet uses a **random 16-byte Initialization Vector (IV)**. This ensures that even if you send "TEMP:25" a thousand times, the data over the air looks completely different every single time.

### 3. Why ACKs are Required for Mesh Stability
Standard ESP-NOW broadcasts are "Fire and Forget"—there is no guarantee the receiver actually heard the message. In a critical mesh (like a fire alarm), this is unacceptable.
*   **The Solution:** Our application-layer **ACK system** forces the receiver to send a small confirmation packet back to the sender. If the sender doesn't get this ACK within 300ms, it knows the transmission failed and logs a warning.

### 4. Permanent STA Mode Benefits
Previously, this project used dynamic `STA ↔ AP` switching. However, benchmarking showed that `esp_wifi_set_mode` causes a brief radio silence. 
*   **The Solution:** All nodes now stay in `STA` mode permanently. This results in a cleaner radio environment (no phantom SSIDs), lower latency, and 100% receiver uptime, which is critical for a high-traffic flooding mesh.

### 5. The "Multi-Hop Heartbeat" Problem
In a standard mesh, nodes often drop identical heartbeats to save bandwidth. This creates a "1-hop only" heartbeat where distant nodes falsely believe each other are offline. 
*   **The Solution:** We solved this by adding an incrementing **Sequence Number** to every packet. Because the sequence number makes every heartbeat unique, intermediate nodes repeat them, allowing heartbeats to flow dynamically through the whole mesh.

### 6. Identifying True Origin vs. Immediate Sender
In ESP-NOW, hardware only tells you who *last* sent the packet. If Node B forwards a message from Node C to Node A, Node A thinks the message came from Node B. 
*   **The Solution:** We parse the **Original MAC Address** directly from the encrypted payload so that nodes can "see" peers that are multiple hops away, maintaining accurate network maps without centralized routing.
