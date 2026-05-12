# ModMesh: Industrial Actuator Node Documentation

![ModMesh Actuator Banner](assets/actuator_banner.png)

## 📌 Introduction

The **ModMesh Actuator Node** is a specialized firmware for the ESP32-S3 that transforms it into a robust, industrial-grade wireless output controller. It operates as part of a high-reliability **Managed Flooding Mesh** using the ESP-NOW protocol.

Unlike standard Wi-Fi meshes, ModMesh is optimized for low latency and deterministic delivery in harsh industrial environments. It features a decoupled **Quad-Task RTOS Architecture**, ensuring that wireless communication, security processing, and hardware switching never block each other.

---

## 🏗️ System Architecture

The Actuator Node utilizes the Quad-Task model to ensure that hardware control is executed with high priority without being stalled by network activity.

### 📊 RTOS Task Model
| Task Name | Priority | Stack Size | Responsibility |
| :--- | :---: | :---: | :--- |
| **Actuator Task**| **7** | 4KB | **Hardware Switching**. Decoupled via Queue. Triggers GPIOs based on commands. |
| **Mesh Task** | **5** | 4KB | **Network Management**. Handles heartbeats and peer health monitoring. |
| **Sensor Task** | **10** (Max) | 4KB | **Local Overrides**. Monitors local buttons for manual hardware control. |
| **Reset Task** | **5** | 8KB | **Safety & Maintenance**. Monitors GPIO 1 for factory reset. |

### Decoupled Command Execution
When a packet is received, it is pushed into a **FreeRTOS Queue** (`s_actuator_queue`). This prevents a slow hardware switch or a long command processing loop from stalling the radio driver or delaying ACK responses.

---

## 📡 The Mesh Engine: Managed Flooding

The core of ModMesh is its **Self-Healing Flooding Mesh**.

### 1. Deduplication (djb2 Hash Cache)
To prevent "Broadcast Storms," the system implements a deduplication cache:
- **Hashing**: Every incoming plaintext is hashed using the **djb2 algorithm** (fast, low-collision).
- **Cache**: A circular buffer stores the last **128 hashes**.
- **Logic**: If a message's hash is already in the cache, it is dropped and not rebroadcasted.

### 2. Multi-Hop & Hop-by-Hop ACK
Every node acts as a repeater.
- **ACK Layer**: When a node receives a message, it sends a `MSG_TYPE_ACK` back to the immediate sender to confirm the message has "entered" the mesh.
- **Rebroadcast**: The node then immediately re-encrypts and rebroadcasts the message to extend its reach.

---

## 📬 Communication Protocol (Pub/Sub)

The node uses a **Keyword Subscription** model for routing.

### Message Syntax
`[SENDER_LABEL | MAC | SEQ] [TARGET_KEYWORDS] COMMAND`

### Command Execution Logic
The node triggers its physical outputs only if the `[TARGET_KEYWORDS]` block contains:
- `ALL` (Global Broadcast).
- Its specific `NODE_LABEL`.
- Any word in its `ACTUATOR_KEYWORDS` list.

### Standard Commands
| Command | Action |
| :--- | :--- |
| `STATE:1` / `CMD:LED_ON` | Set `ACTUATOR_OUTPUT_GPIO` to **HIGH**. |
| `STATE:0` / `CMD:LED_OFF` | Set `ACTUATOR_OUTPUT_GPIO` to **LOW**. |
| `CMD:NETWORK_RESET` | **Emergency Kill**: Sets output to LOW immediately. |

---

## 🔐 Enterprise-Grade Security

ModMesh implements a layered defense to ensure industrial integrity:

- **AES-128-CBC Encryption**: All application data is hardware-encrypted via `mbedTLS`.
- **Random IV (16-byte)**: Every single packet generated has a unique random IV prepended to the ciphertext. This ensures that even identical commands produce unique encrypted noise.
- **Replay Protection**: The Global Sequence Number inside the encrypted body prevents attackers from re-sending captured packets.
- **Padding Integrity**: PKCS#7 standard validation ensures data has not been tampered with.

---

## 🚨 Safety & Maintenance (Factory Reset)

If the `FACTORY_RESET_GPIO` is held for **3 seconds**:
1. **Visual Warning**: The LED pulses **Rapid Red**.
2. **Global Broadcast**: The node sends an `[ALL]CMD:NETWORK_RESET` to clear other actuators.
3. **Wipe**: It erases its local Peer Tracking Table in NVS.
4. **Reboot**: The system restarts in a clean state.

---

## ⚙️ Configuration (shared_config.h)

Key Actuator settings:

| Parameter | Default | Description |
| :--- | :--- | :--- |
| `DEVICE_ROLE` | `ROLE_ACTUATOR`| Optimizes the binary for output control. |
| `ACTUATOR_OUTPUT_GPIO`| `6` | Pin connected to the Relay or LED. |
| `NODE_LABEL` | `ACT_01` | Unique name for targeting commands. |
| `ACTUATOR_KEYWORDS`| `MOTORS,FANS` | Subscribed topics. |

---

## 🚦 Visual Diagnostics (RGB LED)

| Color | Pattern | Meaning |
| :--- | :--- | :--- |
| 🟢 **Solid Green** | Static | **Healthy**: All expected peers are active. |
| 🟢 **Blinking Green**| 500ms | **Partial Mesh**: One or more peers from NVS are missing. |
| 🔴 **Solid Red** | Static | **Disconnected**: No peers known or heartbeats lost. |
| 🔵 **Blue Pulse** | Pulse | **Sending**: Transmitting local status / Waiting for ACK. |
| 🔴 **Rapid Red** | 100ms | **Safety Reset**: 3s hold warning. |

---

## 🛠️ Build & Deployment

### Prerequisites
- ESP-IDF v5.x
- ESP32-S3 Hardware

### Commands
```bash
# Configure Role & Keywords in shared_config.h
idf.py build flash monitor
```

---

*Developed by M. YOUCEF Yazid | v1.0.0 Actuator Edition*
