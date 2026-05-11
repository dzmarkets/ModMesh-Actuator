//
// File Path: components/shared_config/include/shared_config.h
// Brief:     Centralized configuration file for ModMesh Actuator Node.
// Author:    M. YOUCEF Yazid (yazid.youcef@gmail.com)
// Version:   0.8.1 (Actuator Edition)
// CreateDate: 2026-04-26
// UpdateDate: 2026-05-11
//

#ifndef SHARED_CONFIG_H
#define SHARED_CONFIG_H

// =============================================================================
// --- Node Identification ---
// =============================================================================
// Unique human-readable label for this specific physical board.
#define NODE_LABEL "ACTUATOR_01"

// =============================================================================
// --- Topic / Keyword Routing (Pub/Sub) ---
// =============================================================================
// ACTUATOR_KEYWORDS: List of topics this node listens to.
// If a sensor broadcast includes any of these, this node will process it.
#define ACTUATOR_KEYWORDS "ALL"

// DATA_KEYWORDS: List of topics this node publishes data under (e.g., status).
#define DATA_KEYWORDS "ACTUATOR_01"

// =============================================================================
// --- Device Role ---
// =============================================================================
#define ROLE_SENSOR        1 // Reads local hardware sensors/buttons
#define ROLE_ACTUATOR      2 // Controls local hardware outputs (LEDs/Relays)
#define ROLE_GATEWAY       3 // Bridge node with Modbus and Mesh coordination
#define ROLE_BOTH          4 // Sensor + Actuator functionality
#define ROLE_MESH_EXTENDER 5 // Pure multi-hop rebroadcaster

// The active role for THIS compiled binary.
#define DEVICE_ROLE ROLE_ACTUATOR

// =============================================================================
// --- Network Security ---
// =============================================================================
#define NETWORK_API_KEY "A7F9E2B4C6D8105F39A0B2C4D6E8F102"

// =============================================================================
// --- LED Configuration ---
// =============================================================================
#define USE_EXTERNAL_RGB_LED 0
#define WS2812B_LED_GPIO    48

// =============================================================================
// --- Mesh Network Parameters ---
// =============================================================================
#define MESH_MIN_DEVICES    2
#define MESH_MAX_DEVICES    25
#define MESH_NVS_NAMESPACE  "mesh_cfg"
#define MESH_NVS_KEY_COUNT  "dev_count"
#define ESPNOW_WIFI_CHANNEL 1

// Dynamic ACK Timeout: Scales with network size to handle multi-hop latency.
#define ACK_TIMEOUT_MS      (300 + (50 * MESH_MAX_DEVICES))

#define MESH_PEER_TIMEOUT_MS      6000
#define MESH_KEEPALIVE_INTERVAL_MS 1000

// =============================================================================
// --- Actuator Hardware Configuration ---
// =============================================================================
// Primary output pin for control (e.g., Relay, LED, Solenoid).
#define ACTUATOR_OUTPUT_GPIO   6

// =============================================================================
// --- Factory Reset Configuration ---
// =============================================================================
// GPIO connected to the momentary push-button used for factory resets.
#define FACTORY_RESET_GPIO  1  

// 1 = Use internal chip pull-up, 0 = External hardware pull-up present.
#define USE_INTERNAL_PULLUPS 1
#endif // SHARED_CONFIG_H
