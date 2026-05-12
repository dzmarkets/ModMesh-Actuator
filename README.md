# ⚡ ModMesh Actuator Node - Absolute Technical Documentation

* **Project:** ModMesh Industrial Framework
* **Component:** Actuator Node (ESP32-S3)
* **Version:** 1.0.0 (Full Technical Edition)
* **Author:** M. YOUCEF Yazid (yazid.youcef@gmail.com)
* **Update Date:** 2026-05-12

---

## 📖 Project Overview
The **ModMesh Actuator Node** is a specialized firmware for the ESP32-S3 that transforms it into a robust, industrial-grade wireless output controller. It operates as part of a high-reliability **Flooding Mesh Network** using the ESP-NOW protocol.

Unlike standard WiFi meshes, ModMesh is optimized for low latency and deterministic delivery in harsh industrial environments. It features a decoupled **Quad-Task RTOS Architecture**, ensuring that wireless communication, security processing, and hardware switching never block each other.

---

## 🏗️ System Architecture

### 1. The Quad-Task RTOS Model
The system is divided into four distinct threads of execution to ensure maximum responsiveness and stability.

| Task | Priority | Function | Rationale |
| :--- | :--- | :--- | :--- |
| **Sensor Task** | `10` | High-speed GPIO Polling | Ensures zero-latency detection of local triggers (if Role=Sensor). |
| **Actuator Task**| `7` | Hardware Switching | Decoupled via Queue. Ensures hardware state changes are not delayed by radio tasks. |
| **Mesh Task** | `5` | Network Management | Handles heartbeats, peer health checks, and LED status updates. |
| **Reset Task** | `1` | Safety & Maintenance | Low-priority monitor for GPIO 1 (Factory Reset) and NVS management. |

### 2. Decoupled Command Execution
When a packet is received and validated, it is not executed immediately in the network interrupt context. Instead, it is pushed into a **FreeRTOS Queue** (`s_actuator_queue`). This prevents a slow hardware switch or a long command processing loop from stalling the radio driver.

---

## 🌐 The Mesh Engine: Logic & Mechanics

The core of ModMesh is its **Self-Healing Flooding Mesh**.

### 1. Flooding Mechanism
Every node in the network acts as a repeater. When a unique message is received, the node:
1.  Processes it locally.
2.  Immediately rebroadcasts it to the entire network.
This ensures that a message can reach a node even if there is no direct line-of-sight to the original sender, as long as a chain of nodes exists.

### 2. Deduplication (The djb2 Hash Cache)
To prevent "Broadcast Storms" (where a message bounces infinitely between nodes), the system implements a deduplication cache:
*   **Hashing:** Every incoming plaintext is hashed using the **djb2 algorithm** (fast, low-collision).
*   **Cache:** A circular buffer stores the last 128 hashes.
*   **Logic:** If a received message's hash is already in the cache, it is dropped and not rebroadcasted.

### 3. Hop-by-Hop Acknowledgment (ACK)
While the mesh is broadcast-based, it uses a custom ACK layer for reliability:
*   When Node A sends a packet, it waits for an ACK from **any** neighbor.
*   Receiving Node B sends a `MSG_TYPE_ACK` back to Node A.
*   This confirms that the message has "entered" the mesh. If no ACK is received within the `ACK_TIMEOUT_MS`, the node logs a warning (Isolated Node).

---

## 🔒 Security & Data Integrity

ModMesh treats security as a first-class citizen, implementing a layered defense:

### 1. AES-128-CBC Encryption
All application data is encrypted using the industry-standard **AES-128-CBC** algorithm via the hardware-accelerated `mbedtls` library.
*   **Key:** Defined by `NETWORK_API_KEY` in `shared_config.h`.
*   **Padding:** PKCS#7 standard ensures compatibility with various payload lengths.

### 2. Random IV (Initialization Vector)
Every single packet generated has a unique **16-byte Random IV** prepended to the ciphertext. Even if the exact same sensor data is sent twice, the encrypted payload will look completely different to an eavesdropper.

### 3. Replay Protection (Sequence Numbers)
The plaintext payload contains a **Global Sequence Number** (`[NODE | MAC | SEQ]`).
*   The `SEQ` increments with every message.
*   Because the `SEQ` is inside the encrypted body, and the `djb2` hash tracks the full plaintext, the Deduplication Cache naturally prevents replay attacks.

---

## 📬 Application Layer Logic (Pub/Sub)

### 1. Keyword Routing
The node does not just listen to every command. It uses a **Keyword Subscription** model:
*   **Format:** `[NODE_NAME | MAC | SEQ] [TARGET_KEYWORDS] COMMAND`
*   The node parses the `[TARGET_KEYWORDS]` block.
*   **Match Criteria:** It triggers only if the list contains `ALL`, its specific `NODE_LABEL`, or any word in its `ACTUATOR_KEYWORDS` list.

### 2. Command Syntax
| Command | Action |
| :--- | :--- |
| `STATE:1` / `CMD:LED_ON` | Set `ACTUATOR_OUTPUT_GPIO` to HIGH. |
| `STATE:0` / `CMD:LED_OFF` | Set `ACTUATOR_OUTPUT_GPIO` to LOW. |
| `CMD:NETWORK_RESET` | Emergency Kill: Sets output to LOW immediately. |

---

## 🚥 Visual Diagnostics (RGB Status LED)

The node uses a WS2812B (NeoPixel) to communicate its internal state:

| Color | State | Meaning |
| :--- | :--- | :--- |
| 🔴 **Solid Red** | `DISCONNECTED` | No peers known or no heartbeats received. |
| 🟢 **Solid Green** | `CONNECTED` | Healthy mesh. All expected peers are active. |
| 🟢 **Blinking Green**| `PARTIAL` | One or more peers from NVS are missing. |
| 🔵 **Blue Flash** | `SENDING` | Active transmission of sensor/command data. |
| 🟡 **Yellow Flash** | `FACTORY_RESET`| Factory reset in progress (3s hold). |

---

## ⚙️ Configuration Deep-Dive

Configuration is centralized in `components/shared_config/include/shared_config.h`.

### Hardware Settings
*   `ACTUATOR_OUTPUT_GPIO (6)`: The pin connected to your relay or LED.
*   `FACTORY_RESET_GPIO (1)`: The pin for the physical reset button.
*   `WS2812B_LED_GPIO (48)`: The pin for the RGB status indicator.

### Network Settings
*   `NODE_LABEL`: Human-readable name (e.g., "PUMP_ROOM_01").
*   `ACTUATOR_KEYWORDS`: Subscribed topics (e.g., "MOTORS,FANS").
*   `MESH_KEEPALIVE_INTERVAL_MS (1000)`: Frequency of heartbeat broadcasts.
*   `ACK_TIMEOUT_MS`: Dynamically calculated as `300ms + (50ms * MESH_MAX_DEVICES)`.

---

## 🛠️ Maintenance: Factory Reset

If the `FACTORY_RESET_GPIO` is held for **3 seconds**:
1.  **Visual Warning:** The LED pulses Yellow.
2.  **Broadcast:** The node sends an `[ALL]CMD:NETWORK_RESET` to clear other actuators.
3.  **Wipe:** It erases its local Peer Tracking Table in NVS.
4.  **Reboot:** The system restarts in a clean state.

---

## 🚀 Build & Deployment

### Prerequisites
*   ESP-IDF v5.x
*   ESP32-S3 Hardware

### Commands
```powershell
# 1. Setup Environment
. $HOME/esp/esp-idf/export.ps1

# 2. Configure (Optional)
idf.py menuconfig

# 3. Build & Flash
idf.py build flash monitor
```

---

## 📄 License & Attribution
Developed by **dzmarkets** as part of the ModMesh Industrial Suite. 
Lead Architect: **M. YOUCEF Yazid**.
All rights reserved.
