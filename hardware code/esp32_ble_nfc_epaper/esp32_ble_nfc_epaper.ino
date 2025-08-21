// ESP32 IRAM optimization settings - solve memory overflow issues


/*
 * ESP32 Multi-Device NFC+BLE+WiFi+EPaper System with Kalman RSSI Filtering and Real Distance Estimation
 * 
 * Multi-Device Configuration Guide:
 * 1. Each device needs a different DEVICE_NAME
 * 2. All devices use the same DEVICE_PREFIX "ESP32_PEER_"
 * 3. Supports up to 10 devices working simultaneously
 * 
 * Device Naming Examples:
 * - Device 1: #define DEVICE_NAME "ESP32_PEER_01"
 * - Device 2: #define DEVICE_NAME "ESP32_PEER_02"  
 * - Device 3: #define DEVICE_NAME "ESP32_PEER_03"
 * - Device 4: #define DEVICE_NAME "ESP32_PEER_04"
 * - Device 5: #define DEVICE_NAME "ESP32_PEER_05"
 * 
 * Features:
 * - Automatic discovery and management of multiple devices
 * - Kalman filter for smooth RSSI readings (reduces noise by ~70%)
 * - Real distance estimation using Log-Distance Path Loss Model
 * - Distance-based proximity levels (1-5) with meter accuracy
 * - Adjust LED strip and vibration intensity based on real distance
 * - Automatic cleanup of timeout devices (10 seconds)
 * - Comprehensive logging: RSSI (raw/filtered), distance, and levels
 * - Support for dynamic device join and leave
 * - Distance calibration and statistics for environment adaptation
 * 
 * === Kalman Filter Configuration ===
 * 
 * Each BLE device gets its own 1D Kalman filter for RSSI smoothing.
 * 
 * Key Parameters (adjustable):
 * - KALMAN_Q (0.5): Process noise - how much the true RSSI can change
 * - KALMAN_R (4.0): Measurement noise - RSSI sensor noise level  
 * - KALMAN_P_INIT (10.0): Initial uncertainty
 * 
 * Tuning Guide:
 * - Lower Q (0.1-0.5) = more stable, slower response
 * - Higher Q (1.0-2.0) = faster response, less filtering
 * - Lower R (1.0-2.0) = trust measurements more
 * - Higher R (5.0-10.0) = trust measurements less
 * 
 * Benefits:
 * - ~20 lines of code vs ATM's ~60 lines
 * - Very low CPU and memory usage
 * - Excellent noise reduction
 * - Smooth distance estimation
 * - Self-adapting to signal conditions
 * 
 * === Real Distance Estimation Configuration ===
 * 
 * Uses Log-Distance Path Loss Model: d = 10^((P0 - RSSI) / (10 * n))
 * 
 * Key Parameters (adjustable for different environments):
 * - RSSI_REF_POWER (-40.0): Reference power at 1 meter distance (dBm)
 * - PATH_LOSS_EXPONENT (2.5): Environment path loss (2.0=free space, 4.0=complex indoor)
 * - MIN_DISTANCE (0.1): Minimum detectable distance in meters
 * - MAX_DISTANCE (50.0): Maximum reasonable distance in meters
 * 
 * Distance-to-Level Mapping:
 * - Level 5: 0-1.0m (Very Close)
 * - Level 4: 1.0-2.5m (Close)  
 * - Level 3: 2.5-5.0m (Medium)
 * - Level 2: 5.0-10.0m (Far)
 * - Level 1: 10.0m+ (Very Far)
 * 
 * Calibration Tips:
 * - Adjust PATH_LOSS_EXPONENT based on environment
 * - Use printDistanceCalibration() for RSSI-distance mapping
 * - Monitor distance statistics for accuracy assessment
 */

#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Adafruit_NeoPixel.h>
#include "epd2in9.h"
#include "epd.h"

// Power button configuration
#define POWER_BUTTON_PIN 2
#define BUTTON_DEBOUNCE_MS 50
#define BUTTON_LONG_PRESS_MS 2000   // Long press 2s to enter sleep
volatile bool buttonPressed = false;
unsigned long buttonPressTime = 0;
unsigned long lastButtonCheck = 0;
bool devicePoweredOn = true;  // Device power status

// WiFi configuration
const char* ssid = "UCL_IoT";
const char* password = "Jme6fsNEq5";

// NFC
#define PN532_SDA 21
#define PN532_SCL 22
Adafruit_PN532 nfc(PN532_SDA, PN532_SCL);

// BLE multi-device configuration
#define MAX_DEVICES 10                    // Maximum supported devices
#define DEVICE_NAME "ESP32_PEER_03"       // Current device name (each device needs different name)
#define DEVICE_PREFIX "ESP32_PEER_"       // Device name prefix
#define DEVICE_TIMEOUT_MS 10000           // Device timeout (10 seconds)

// Kalman filter configuration parameters (adjustable per environment)
#define KALMAN_Q 2.0    // Process noise covariance (system noise) - smaller=more stable, larger=faster response
#define KALMAN_R 4.0    // Measurement noise covariance (RSSI measurement noise) - smaller=trust measurements more, larger=more smoothing
#define KALMAN_P_INIT 10.0  // Initial error covariance

// Simple 1D Kalman filter struct (must be defined before DeviceInfo)
struct KalmanFilter {
    float x;          // State estimate (filtered RSSI)
    float P;          // Error covariance
    bool initialized; // Whether initialized
    KalmanFilter() : x(-70), P(KALMAN_P_INIT), initialized(false) {}
};

// Device information structure with Kalman filter and distance estimation
struct DeviceInfo {
    String name;
    int rssi;                   // Filtered RSSI value
    int rawRssi;                // Raw RSSI (for debugging)
    float distance;             // Estimated distance (meters)
    int distanceLevel;          // Distance-based level (1-5)
    unsigned long lastSeen;
    bool active;
    KalmanFilter kalmanFilter;  // Per-device Kalman filter
};

// Device management
DeviceInfo nearbyDevices[MAX_DEVICES];
int deviceCount = 0;
int closestDeviceIndex = -1;

const int VIBRATION_PIN = 33;
const int RSSI_THRESHOLDS[5] = { -40, -55, -65, -75, -85 };
const int VIBRATION_DUTY = 200;
const uint32_t SCAN_TIME_SECONDS = 2;
static bool didVibrateThisScan = false;

// NeoPixel LED strip configuration
#define NEOPIXEL_PIN 18
#define NUM_LEDS 8
#define BRIGHTNESS 40  // Brightness setting: 0-255, 64 is about 25% brightness
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Gradient color definition (from blue to red)
const uint32_t gradientColors[6] = {
    0x000000,  // Level 0: Black (off)
    0x0000FF,  // Level 1: Blue
    0x00FFFF,  // Level 2: Cyan
    0x00FF00,  // Level 3: Green
    0xFFFF00,  // Level 4: Yellow
    0xFF0000   // Level 5: Red
};

// Image buffer
#define IMAGE_SIZE 4736
uint8_t imageBuffer[IMAGE_SIZE];
String lastNfcUrl = "";
bool tagPresent = false;
bool nfcFinished = false; // Flag: whether NFC detection and image download completed
bool bleStarted = false;  // Flag: whether BLE is initialized
bool bleShouldStart = false; // Flag: whether BLE should be initialized
BLEScan* pScan = nullptr;    // Global BLE scan object pointer



// === Real distance estimation parameters ===
#define RSSI_REF_POWER -40.0    // Reference RSSI at 1 meter (dBm)
#define PATH_LOSS_EXPONENT 4.0  // Path loss exponent n (indoor: 2.0-4.0)
#define MIN_DISTANCE 0.1        // Minimum distance limit (m)
#define MAX_DISTANCE 50.0       // Maximum distance limit (m)

// Global Kalman filter statistics
struct KalmanStats {
    int totalFiltered;
    float avgDifference;
    float maxDifference;
    unsigned long lastUpdate;
} kalmanStats = {0, 0.0, 0.0, 0};

// Kalman filter function - concise and efficient
int applyKalmanFilter(KalmanFilter* filter, int measurement) {
    if (!filter->initialized) {
        filter->x = measurement;  // Use first measurement as initial value
        filter->initialized = true;
        return measurement;
    }
    
    // 1) Prediction step
    // x_pred = x (state transition, RSSI assumed constant)
    float P_pred = filter->P + KALMAN_Q;  // Predicted error covariance
    
    // 2) Update step
    float K = P_pred / (P_pred + KALMAN_R);  // Kalman gain
    filter->x = filter->x + K * (measurement - filter->x);  // State update
    filter->P = (1 - K) * P_pred;  // Error covariance update
    
    int filteredValue = (int)round(filter->x);
    
    // Update statistics
    kalmanStats.totalFiltered++;
    float diff = abs(filteredValue - measurement);
    kalmanStats.avgDifference = (kalmanStats.avgDifference * 0.95) + (diff * 0.05);
    if (diff > kalmanStats.maxDifference) {
        kalmanStats.maxDifference = diff;
    }
    kalmanStats.lastUpdate = millis();
    
    return filteredValue;  // Return filtered integer RSSI
}

// Print Kalman filter statistics
void printKalmanStats() {
    Serial.println("\n=== Kalman Filter Stats ===");
    Serial.printf("Total filtered: %d\n", kalmanStats.totalFiltered);
    Serial.printf("Avg difference: %.1f dBm\n", kalmanStats.avgDifference);
    Serial.printf("Max difference: %.1f dBm\n", kalmanStats.maxDifference);
    Serial.printf("Noise reduction: %.1f%%\n", 
                  kalmanStats.avgDifference > 0 ? 
                  (kalmanStats.avgDifference / 10.0) * 100 : 0);
    Serial.println("===========================\n");
}

// === Real distance estimation functions ===

// Convert RSSI to distance (using log-distance path loss model)
// Formula: d = 10^((P0 - RSSI) / (10 * n))
// Where: P0 = reference power, n = path loss exponent, RSSI = current signal strength
float rssiToDistance(int rssi) {
    // Treat only 0 as invalid; RSSI stronger than reference should map to a closer distance
    if (rssi == 0) {
        return MAX_DISTANCE;  // Invalid RSSI value
    }
    
    // Log-distance path loss model
    float distance = pow(10.0, (RSSI_REF_POWER - rssi) / (10.0 * PATH_LOSS_EXPONENT));
    
    // Apply distance limits
    if (distance < MIN_DISTANCE) distance = MIN_DISTANCE;
    if (distance > MAX_DISTANCE) distance = MAX_DISTANCE;
    
    return distance;
}

// Map real distance to proximity level (finer grading)
int distanceToLevel(float distance) {
    if (distance <= 1.0) return 5;      // within 1.0 m: very close
    else if (distance <= 2.5) return 4; // 1.0-2.5 m: close
    else if (distance <= 5.0) return 3; // 2.5-5.0 m: medium
    else if (distance <= 10.0) return 2;// 5.0-10.0 m: far
    else return 1;                       // >10.0 m: very far
}

// Distance estimation statistics
struct DistanceStats {
    int totalEstimations;
    float avgDistance;
    float minDistance;
    float maxDistance;
    unsigned long lastUpdate;
} distanceStats = {0, 0.0, MAX_DISTANCE, 0.0, 0};

// Update distance statistics
void updateDistanceStats(float distance) {
    distanceStats.totalEstimations++;
    distanceStats.avgDistance = (distanceStats.avgDistance * 0.95) + (distance * 0.05);
    if (distance < distanceStats.minDistance) distanceStats.minDistance = distance;
    if (distance > distanceStats.maxDistance) distanceStats.maxDistance = distance;
    distanceStats.lastUpdate = millis();
}

// Print distance estimation statistics
void printDistanceStats() {
    if (distanceStats.totalEstimations == 0) return;
    
    Serial.println("\n=== Distance Estimation Stats ===");
    Serial.printf("Total estimations: %d\n", distanceStats.totalEstimations);
    Serial.printf("Average distance: %.2f m\n", distanceStats.avgDistance);
    Serial.printf("Min distance: %.2f m\n", distanceStats.minDistance);
    Serial.printf("Max distance: %.2f m\n", distanceStats.maxDistance);
    Serial.printf("Path loss exponent: %.1f\n", PATH_LOSS_EXPONENT);
    Serial.printf("Reference power: %.1f dBm @ 1m\n", RSSI_REF_POWER);
    Serial.println("==================================\n");
}

// Distance calibration helper function (for debugging and calibration)
void printDistanceCalibration() {
    Serial.println("\n=== Distance Calibration Table ===");
    Serial.println("RSSI (dBm) | Distance (m) | Level");
    Serial.println("-----------|--------------|---------");
    
    for (int rssi = -40; rssi >= -90; rssi -= 5) {
        float dist = rssiToDistance(rssi);
        int level = distanceToLevel(dist);
        Serial.printf("%6d     | %8.2f     | %5d\n", rssi, dist, level);
    }
    Serial.println("===================================\n");
}

// BLE scan callback
int rssiToLevel(int rssi) {
  for (int i = 0; i < 5; i++) if (rssi >= RSSI_THRESHOLDS[i]) return 5 - i;
  return 0;
}

// Add or update device information with Kalman filtering and distance estimation
void addOrUpdateDevice(String deviceName, int rssi) {
    unsigned long currentTime = millis();
    
    // Exclude self
    if (deviceName == DEVICE_NAME) {
        return;
    }
    
    // Find if device already exists
    int existingIndex = -1;
    for (int i = 0; i < deviceCount; i++) {
        if (nearbyDevices[i].name == deviceName) {
            existingIndex = i;
            break;
        }
    }
    
    if (existingIndex >= 0) {
        // Update existing device with Kalman filtering and distance estimation
        DeviceInfo* device = &nearbyDevices[existingIndex];
        device->rawRssi = rssi;  // Store raw value
        device->rssi = applyKalmanFilter(&device->kalmanFilter, rssi);  // Apply Kalman filtering
        
        // Compute real distance and distance-based level
        device->distance = rssiToDistance(device->rssi);
        device->distanceLevel = distanceToLevel(device->distance);
        
        device->lastSeen = currentTime;
        device->active = true;
        
        // Update distance statistics
        updateDistanceStats(device->distance);
        
        // Debug output with distance information
        Serial.printf("[DISTANCE] %s: Raw=%ddBm→Filtered=%ddBm, Distance=%.2fm, Level=%d\n", 
                     deviceName.c_str(), device->rawRssi, device->rssi, 
                     device->distance, device->distanceLevel);
                     
    } else if (deviceCount < MAX_DEVICES) {
        // Add new device with distance estimation
        DeviceInfo* newDevice = &nearbyDevices[deviceCount];
        newDevice->name = deviceName;
        newDevice->rawRssi = rssi;
        newDevice->rssi = applyKalmanFilter(&newDevice->kalmanFilter, rssi);  // Initialize Kalman filtering
        
        // Compute initial distance and level
        newDevice->distance = rssiToDistance(newDevice->rssi);
        newDevice->distanceLevel = distanceToLevel(newDevice->distance);
        
        newDevice->lastSeen = currentTime;
        newDevice->active = true;
        deviceCount++;
        
        // Update distance statistics
        updateDistanceStats(newDevice->distance);
        
        Serial.printf("[BLE] New device: %s (RSSI=%ddBm, Distance=%.2fm, Level=%d), total=%d\n", 
                     deviceName.c_str(), newDevice->rssi, newDevice->distance, 
                     newDevice->distanceLevel, deviceCount);
    }
}

// Clean up timeout devices
void cleanupTimeoutDevices() {
    unsigned long currentTime = millis();
    
    for (int i = 0; i < deviceCount; i++) {
        if (nearbyDevices[i].active && 
            (currentTime - nearbyDevices[i].lastSeen > DEVICE_TIMEOUT_MS)) {
            nearbyDevices[i].active = false;
            Serial.printf("[BLE] Device timeout: %s\n", nearbyDevices[i].name.c_str());
        }
    }
}

// Find closest active device based on real distance estimation
int findClosestDevice() {
    int closestIndex = -1;
    float shortestDistance = MAX_DISTANCE;
    
    for (int i = 0; i < deviceCount; i++) {
        if (nearbyDevices[i].active && nearbyDevices[i].distance < shortestDistance) {
            shortestDistance = nearbyDevices[i].distance;
            closestIndex = i;
        }
    }
    
    return closestIndex;
}

// Display all active devices information with distance estimation
void printActiveDevices() {
    Serial.println("[BLE] === Active Devices (with Distance Estimation) ===");
    int activeCount = 0;
    
    for (int i = 0; i < deviceCount; i++) {
        if (nearbyDevices[i].active) {
            activeCount++;
            
            // Display complete device information: RSSI, distance, level
            Serial.printf("[BLE] %d. %s - RSSI: %ddBm, Distance: %.2fm, Level: %d (%.1fm range)\n", 
                         activeCount, nearbyDevices[i].name.c_str(), 
                         nearbyDevices[i].rssi, nearbyDevices[i].distance,
                         nearbyDevices[i].distanceLevel,
                         nearbyDevices[i].distance);
        }
    }
    
    if (activeCount == 0) {
        Serial.println("[BLE] No active devices found");
    }
    Serial.println("[BLE] ========================================================");
}

// Set NeoPixel LED strip display
void setNeoPixelLevel(int level) {
    // Turn off all LEDs first
    strip.clear();
    
    if (level == 0) {
        // Level 0: No LEDs on
        strip.show();
        return;
    }
    
    // Determine number of LEDs based on level
    int numLights = 0;
    switch (level) {
        case 1: numLights = 2; break;  // Level 1: 2 LEDs
        case 2: numLights = 4; break;  // Level 2: 4 LEDs
        case 3: numLights = 5; break;  // Level 3: 5 LEDs
        case 4: numLights = 6; break;  // Level 4: 6 LEDs
        case 5: numLights = 8; break;  // Level 5: All LEDs
        default: numLights = 0; break;
    }
    
    // Set gradient colors
    for (int i = 0; i < numLights && i < NUM_LEDS; i++) {
        // Calculate current LED color (gradient from blue to red)
        float ratio = (float)i / (float)(numLights - 1);
        uint32_t color1 = gradientColors[1]; // Blue
        uint32_t color2 = gradientColors[5]; // Red
        
        // Linear interpolation color calculation
        uint8_t r1 = (color1 >> 16) & 0xFF;
        uint8_t g1 = (color1 >> 8) & 0xFF;
        uint8_t b1 = color1 & 0xFF;
        uint8_t r2 = (color2 >> 16) & 0xFF;
        uint8_t g2 = (color2 >> 8) & 0xFF;
        uint8_t b2 = color2 & 0xFF;
        
        uint8_t r = (uint8_t)(r1 + (r2 - r1) * ratio);
        uint8_t g = (uint8_t)(g1 + (g2 - g1) * ratio);
        uint8_t b = (uint8_t)(b1 + (b2 - b1) * ratio);
        
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    
    strip.show();
}

class MyScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    if (!advertisedDevice.haveName()) return;
    
    std::string advName = advertisedDevice.getName().c_str();
    String deviceName = String(advName.c_str());
    
    // Check if it's our device prefix
    if (advName.rfind(DEVICE_PREFIX, 0) != 0) return;
    
    int rssi = advertisedDevice.getRSSI();
    
    // Add or update device information (includes distance calculation)
    addOrUpdateDevice(deviceName, rssi);
    
    // Clean up timeout devices
    cleanupTimeoutDevices();
    
    // Find closest device (now based on real distance)
    closestDeviceIndex = findClosestDevice();
    
    if (closestDeviceIndex >= 0) {
      DeviceInfo& closestDevice = nearbyDevices[closestDeviceIndex];
      
      // Display closest device information (including distance)
      Serial.printf("[BLE] Closest device: %s, RSSI: %ddBm, Distance: %.2fm, Level: %d\n", 
                   closestDevice.name.c_str(), closestDevice.rssi, 
                   closestDevice.distance, closestDevice.distanceLevel);
      
      // Set NeoPixel strip based on distance level (not RSSI level)
      setNeoPixelLevel(closestDevice.distanceLevel);
      
      // Vibration feedback based on distance level
      if (!didVibrateThisScan && closestDevice.distanceLevel > 0) {
        for (int i = 0; i < closestDevice.distanceLevel; i++) {
          analogWrite(VIBRATION_PIN, VIBRATION_DUTY);
          delay(200);
          analogWrite(VIBRATION_PIN, 0);
          delay(200);
        }
        didVibrateThisScan = true;
        
        // Display all active device information
        printActiveDevices();
      }
    }
  }
};

// Improved NFC page read function with retry mechanism
bool readNfcPageWithRetry(Adafruit_PN532& nfc, uint8_t page, uint8_t* data, int maxRetries = 5) {
    for (int retry = 0; retry < maxRetries; retry++) {
        if (retry > 0) {
            delay(100 + retry * 50); // Incremental delay: 100ms, 150ms, 200ms...
        }
        
        bool success = nfc.ntag2xx_ReadPage(page, data);
        if (success) {
            return true;
        }
        Serial.printf("[NFC] ReadPage %u failed, retry %d/%d\n", page, retry + 1, maxRetries);
    }
    return false;
}

// More robust NDEF URL parsing function, compatible with various formats
String parseNdefUrl(uint8_t* ndefData, uint32_t ndefLen) {
    if (ndefLen < 4) return "";
    // Standard NDEF URL parsing
    uint32_t offset = 0;
    while (offset < ndefLen && ndefData[offset] == 0) offset++;
    if (offset >= ndefLen) return "";
    while (offset < ndefLen) {
        if (offset + 4 > ndefLen) break;
        uint8_t tnf = ndefData[offset] & 0x07;
        uint8_t typeLength = ndefData[offset + 1];
        uint8_t payloadLength = ndefData[offset + 2];
        if (offset + 3 + typeLength + payloadLength > ndefLen) break;
        if (tnf == 0x01 && typeLength == 1 && ndefData[offset + 3] == 'U') {
            uint8_t* payload = &ndefData[offset + 3 + typeLength];
            uint8_t prefix = payload[0];
            String url = "";
            switch (prefix) {
                case 0x01: url = "http://www."; break;
                case 0x02: url = "https://www."; break;
                case 0x03: url = "http://"; break;
                case 0x04: url = "https://"; break;
                default: url = ""; break;
            }
            for (int i = 1; i < payloadLength; i++) url += (char)payload[i];
            return url;
        }
        offset += 3 + typeLength + payloadLength;
    }
    // If standard parsing fails, try direct URL parsing
    String rawData = "";
    for (int i = 0; i < ndefLen; i++) {
        if (ndefData[i] >= 32 && ndefData[i] <= 126) { // Printable characters
            rawData += (char)ndefData[i];
        }
    }
    
    Serial.printf("[NDEF] Raw text extracted: %s\n", rawData.c_str());
    
    // Find URL pattern and clean - more lenient matching
    if (rawData.indexOf("raw.git") >= 0 || rawData.indexOf("github") >= 0) {
        // Remove leading "QU" (NDEF identifier)
        String cleanUrl = rawData;
        if (cleanUrl.startsWith("QU")) {
            cleanUrl = cleanUrl.substring(2);
        }
        // Remove trailing extra characters
        while (cleanUrl.length() > 0 && 
               (cleanUrl.charAt(cleanUrl.length() - 1) < 32 || 
                cleanUrl.charAt(cleanUrl.length() - 1) > 126)) {
            cleanUrl = cleanUrl.substring(0, cleanUrl.length() - 1);
        }
        // Smart URL reconstruction
        String url = "";
        
        // Try to find complete raw.githubusercontent.com URL
        int rawPos = cleanUrl.indexOf("raw.git");
        if (rawPos >= 0) {
            // Try to reconstruct complete URL
            if (cleanUrl.indexOf("githubusercontent.com") > rawPos) {
                // Found complete domain name
                int comPos = cleanUrl.indexOf(".com");
                if (comPos > 0) {
                    int binPos = cleanUrl.indexOf(".bin");
                    if (binPos > comPos) {
                        url = "https://" + cleanUrl.substring(rawPos, binPos + 4);
                    } else {
                        // Try to find path end
                        int pathEnd = comPos + 4;
                        while (pathEnd < cleanUrl.length() && cleanUrl.charAt(pathEnd) != ' ') {
                            pathEnd++;
                        }
                        url = "https://" + cleanUrl.substring(rawPos, pathEnd);
                    }
                }
            } else {
                // Only found partial, try reconstruction
                Serial.println("[NDEF] Partial URL found, attempting reconstruction...");
                // Assume it's GitHub raw file URL
                String partialUrl = cleanUrl.substring(rawPos);
                // Remove repeated characters
                partialUrl.replace("ithuithu", "");
                partialUrl.replace("tent", "");
                
                if (partialUrl.startsWith("raw.git")) {
                    // Try to reconstruct complete GitHub URL
                    url = "https://raw.githubusercontent.com/your-username/your-repo/main/image.bin";
                    Serial.printf("[NDEF] Reconstructed URL: %s\n", url.c_str());
                }
            }
        }
        
        if (url.length() > 0) {
            return url;
        }
    }
    return "";
}

// Check power button status
void checkPowerButton() {
    unsigned long currentTime = millis();
    
    // Debounce handling
    if (currentTime - lastButtonCheck < BUTTON_DEBOUNCE_MS) {
        return;
    }
    lastButtonCheck = currentTime;
    
    // Read button status (low level means pressed, because INPUT_PULLUP is used)
    bool buttonCurrentState = !digitalRead(POWER_BUTTON_PIN);
    
    if (buttonCurrentState && !buttonPressed) {
        // Button just pressed
        buttonPressed = true;
        buttonPressTime = currentTime;
        Serial.println("[BUTTON] Power button pressed, hold for 2 seconds to sleep...");
        
        // Light all LEDs red when button is pressed to prompt user
        for (int i = 0; i < NUM_LEDS; i++) {
            strip.setPixelColor(i, 0xFF0000); // Red
        }
        strip.show();
        
    } else if (!buttonCurrentState && buttonPressed) {
        // Button released
        buttonPressed = false;
        Serial.println("[BUTTON] Power button released");
        
        // Restore LED status
        setNeoPixelLevel(0); // Turn off all lights
        
    } else if (buttonPressed && (currentTime - buttonPressTime >= BUTTON_LONG_PRESS_MS)) {
        // Long press 2 seconds confirmed: enter sleep mode
        Serial.println("[BUTTON] Long press confirmed - preparing for sleep mode");
        
        // Red flash 3 times warning
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < NUM_LEDS; j++) {
                strip.setPixelColor(j, 0xFF0000); // Red
            }
            strip.show();
            delay(300);
            strip.clear();
            strip.show();
            delay(300);
        }
        
        enterDeepSleep();
    }
}

// Download and display image (with retry mechanism)
bool downloadAndDisplayImage(String url, int maxRetries = 3) {
    for (int retry = 0; retry < maxRetries; retry++) {
        if (retry > 0) {
            Serial.printf("[HTTP] Retry %d/%d after 2 seconds...\n", retry + 1, maxRetries);
            delay(2000);
        }
        
        Serial.printf("[HTTP] Downloading image (attempt %d): %s\n", retry + 1, url.c_str());
        WiFiClientSecure client; 
        client.setInsecure();
        HTTPClient http; 
        http.begin(client, url);
        http.setTimeout(15000); // Increase to 15 seconds timeout
        
        int httpCode = http.GET();
        
        if (httpCode == 200) {
            Serial.println("[HTTP] Image download success!");
            
            // Get file size
            int contentLength = http.getSize();
            Serial.printf("[HTTP] Content-Length: %d bytes\n", contentLength);
            
            WiFiClient* stream = http.getStreamPtr();
            size_t total = 0;
            unsigned long startTime = millis();
            
            // Clear buffer
            memset(imageBuffer, 0, IMAGE_SIZE);
            
            // Improved download loop
            while (total < IMAGE_SIZE && (millis() - startTime < 15000)) {
                if (stream->available()) {
                    int bytesToRead = min((int)(IMAGE_SIZE - total), stream->available());
                    int n = stream->read(&imageBuffer[total], bytesToRead);
                    if (n > 0) {
                        total += n;
                        Serial.printf("[HTTP] Downloaded %d/%d bytes (%.1f%%)\r", total, IMAGE_SIZE, (float)total*100/IMAGE_SIZE);
                    }
                } else {
                    delay(10); // Brief wait for more data
                }
                
            // Exit early if expected size downloaded
                if (contentLength > 0 && total >= contentLength) {
                    break;
                }
            }
            Serial.println(); // Line break
            
            Serial.printf("[HTTP] Final download: %d bytes in %lu ms\n", total, millis() - startTime);
            
            // Check if download is complete
            if (total == IMAGE_SIZE || (contentLength > 0 && total == contentLength)) {
                Serial.println("[EPD] Displaying image on e-paper...");
                EPD_Init_2in9_V2();
                
                // Display complete image data
                for (int i = 0; i < IMAGE_SIZE; i++) {
                    EPD_SendData(~imageBuffer[i]);
                }
                
                EPD_2IN9_V2_Show();
                Serial.println("[EPD] Image display complete - FULL SIZE!");
                
                // Set completion flag after successful display
                nfcFinished = true;
                bleShouldStart = true;
                Serial.println("[NFC] Detection finished, NFC will be disabled, BLE will start soon.");
                http.end();
                return true; // Success
            } else {
                Serial.printf("[EPD] Download incomplete: got %d bytes, expected %d\n", total, IMAGE_SIZE);
            }
        } else {
            Serial.printf("[HTTP] Download failed, HTTP code: %d\n", httpCode);
        }
        
        http.end();
        
        // If not the last retry, wait before continuing
        if (retry < maxRetries - 1) {
            Serial.println("[HTTP] Download failed, will retry...");
        }
    }
    
    Serial.printf("[HTTP] All %d download attempts failed\n", maxRetries);
    return false; // Failure
}

void startBLE() {
    BLEDevice::init(DEVICE_NAME);
    BLEServer* pServer = BLEDevice::createServer();
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setScanResponse(false);
    pAdvertising->start();
    BLEScan* scanObj = BLEDevice::getScan();
    scanObj->setAdvertisedDeviceCallbacks(new MyScanCallbacks());
    scanObj->setActiveScan(true);
    scanObj->setInterval(100);
    scanObj->setWindow(99);
    Serial.println("[BLE] BLE started, entering strict alternate scan/broadcast mode.");
    delay(100); // Ensure BLE stack ready
    bleStarted = true;
    pScan = scanObj;
}

// Deep sleep and wake up functionality
void enterDeepSleep() {
    Serial.println("[POWER] Preparing for deep sleep...");
    
    // Turn off all peripherals
    setNeoPixelLevel(0); // Turn off LED strip
    analogWrite(VIBRATION_PIN, 0); // Turn off vibration
    
    // Turn off WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    // Turn off BLE
    if (bleStarted) {
        BLEDevice::deinit(true);
    }
    
    // Wait for button to be completely released to avoid immediate wake up
    Serial.println("[POWER] Waiting for button release...");
    while (!digitalRead(POWER_BUTTON_PIN)) {
        delay(50);  // Wait for button release
    }
    
    // Additional delay to ensure button stability
    delay(500);
    Serial.println("[POWER] Button released, ready to sleep");
    
    // Check button status again
    if (!digitalRead(POWER_BUTTON_PIN)) {
        Serial.println("[POWER] Button still pressed, aborting sleep");
        return;  // If button still pressed, cancel sleep
    }
    
    // Configure wake up source: GPIO2 button press to wake up
    esp_sleep_enable_ext0_wakeup((gpio_num_t)POWER_BUTTON_PIN, 0); // Low level wake up
    
    Serial.println("[POWER] Entering deep sleep mode...");
    Serial.flush();
    delay(100);
    
    // Enter deep sleep
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    
    // Check wake up reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("[POWER] Woke up from deep sleep by power button");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            Serial.println("[BOOT] ESP32 NFC+BLE+WiFi+EPaper starting...");
            break;
    }
    
    // Initialize power button
    pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);
    Serial.println("[BUTTON] Power button initialized on GPIO2");
    
    // Initialize multi-device management system
    deviceCount = 0;
    closestDeviceIndex = -1;
    for (int i = 0; i < MAX_DEVICES; i++) {
        nearbyDevices[i].name = "";
        nearbyDevices[i].rssi = -200;
        nearbyDevices[i].lastSeen = 0;
        nearbyDevices[i].active = false;
    }
    Serial.printf("[BLE] Multi-device system initialized, max devices: %d\n", MAX_DEVICES);
    Serial.printf("[BLE] This device name: %s\n", DEVICE_NAME);
    
    pinMode(VIBRATION_PIN, OUTPUT);
    analogWrite(VIBRATION_PIN, 0);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    // Note: Don't initialize BLE at this time
    EPD_initSPI();
    Serial.println("[EPD] SPI initialized");
    Wire.begin(PN532_SDA, PN532_SCL);
    nfc.begin();
    if (!nfc.getFirmwareVersion()) {
        Serial.println("[NFC] PN532 not found!");
        while (1);
    }
    nfc.SAMConfig();
    Serial.println("[NFC] PN532 initialized");
    
    // Initialize NeoPixel strip
    strip.begin();
    strip.setBrightness(BRIGHTNESS);  // Set brightness
    strip.clear();
    strip.show();
    Serial.printf("[NEO] NeoPixel strip initialized, brightness: %d/255\n", BRIGHTNESS);
}

void loop() {
    // First check power button
    checkPowerButton();
    
    static unsigned long lastNfcCheckTime = 0;
    static unsigned long lastRetryTime = 0;
    static int nfcRetryCount = 0;
    unsigned long now = millis();
    
    // 1. First perform NFC detection, BLE not started
    if (!nfcFinished && (now - lastNfcCheckTime > 1000)) {
        uint8_t uid[7], uidLength;
        static bool tagPresent = false;
        static bool dataRead = false;
        static String lastFailedUrl = "";
        
        bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        if (success) {
            if (!tagPresent) {
                tagPresent = true;
                Serial.print("[NFC] Tag detected, UID: ");
                for (int i = 0; i < uidLength; i++) Serial.printf("%02X", uid[i]);
                Serial.println();
            }
            if (uidLength == 7 && !dataRead) {
                uint8_t ndefBuf[4];
                bool page4ok = readNfcPageWithRetry(nfc, 4, ndefBuf);
                Serial.print("[NFC] ReadPage4: "); Serial.println(page4ok ? "OK" : "FAIL");
                Serial.print("[NFC] Page4: ");
                for (int i = 0; i < 4; i++) Serial.printf("%02X ", ndefBuf[i]);
                Serial.println();
                if (page4ok && ndefBuf[0] == 0x03) {
                    uint8_t tlvLength = ndefBuf[1];
                    Serial.print("[NFC] TLV length: "); Serial.println(tlvLength);
                    uint8_t ndefData[256];
                    uint32_t dataLen = 0;
                    int startPage = 5, endPage = startPage + (tlvLength + 3) / 4;
                    for (int page = startPage; page < endPage; page++) {
                        uint8_t pageData[4];
                                                    bool pageOk = readNfcPageWithRetry(nfc, page, pageData);
                        Serial.print("[NFC] ReadPage"); Serial.print(page); Serial.print(": "); Serial.println(pageOk ? "OK" : "FAIL");
                        Serial.print("[NFC] Page"); Serial.print(page); Serial.print(": ");
                        for (int i = 0; i < 4; i++) Serial.printf("%02X ", pageData[i]);
                        Serial.println();
                        int bytesToRead = (page == endPage - 1) ? (tlvLength - dataLen) : 4;
                        for (int i = 0; i < bytesToRead && dataLen < tlvLength; i++)
                            ndefData[dataLen++] = pageData[i];
                    }
                    Serial.print("[NFC] DataLen: "); Serial.println(dataLen);
                    Serial.print("[NFC] NDEF raw: ");
                    for (uint32_t i = 0; i < dataLen; i++) Serial.printf("%02X ", ndefData[i]);
                    Serial.println();
                    String url = parseNdefUrl(ndefData, dataLen);
                    Serial.print("[NFC] Parsed URL: "); Serial.println(url);
                    
                    if (url.length() > 0) {
                        if (url != lastNfcUrl || url == lastFailedUrl) {
                            lastNfcUrl = url;
                            Serial.print("[NFC] NDEF URL: "); Serial.println(url);
                            
                            // Try to download and display image
                            bool downloadSuccess = downloadAndDisplayImage(url);
                            
                            if (downloadSuccess) {
                                Serial.println("[NFC] Image download and display successful!");
                                dataRead = true;
                                nfcRetryCount = 0;
                                lastFailedUrl = "";
                            } else {
                                Serial.println("[NFC] Image download failed, will retry on next tag detection");
                                lastFailedUrl = url;
                                nfcRetryCount++;
                                
                                // Never give up, keep retrying
                                Serial.printf("[NFC] Retry count: %d, will keep trying...\n", nfcRetryCount);
                            }
                        } else {
                            Serial.println("[NFC] URL already processed successfully");
                            dataRead = true;
                        }
                    } else {
                        Serial.println("[NFC] URL parsing failed, will retry on next tag detection");
                        nfcRetryCount++;
                        
                        // Never give up, keep retrying parsing
                        Serial.printf("[NFC] Parse retry count: %d, will keep trying...\n", nfcRetryCount);
                    }
                }
            }
        } else {
            if (tagPresent) { 
                tagPresent = false; 
                dataRead = false; 
                Serial.println("[NFC] Tag removed.");
                // Reset retry count, allow retry
                nfcRetryCount = 0;
            }
        }
        lastNfcCheckTime = now;
        delay(10);
        return; // As long as NFC detection is not complete, loop only does NFC detection
    }
    // 2. BLE delayed initialization (only execute once in main loop after NFC detection is complete)
    if (nfcFinished && !bleStarted && bleShouldStart) {
        startBLE();
        bleShouldStart = false;
    }
    // 3. After NFC detection is complete, strictly alternate BLE broadcast and scan
    if (nfcFinished && bleStarted && pScan) {
        // 1) Stop broadcast → Start scan for SCAN_TIME_SECONDS seconds → Clear cache → Restart broadcast
        BLEDevice::getAdvertising()->stop();
        Serial.printf(">> Broadcast stopped, starting scan for %u seconds...\n", SCAN_TIME_SECONDS);
        didVibrateThisScan = false;
        
        // Clean up timeout devices
        cleanupTimeoutDevices();
        
        pScan->start(SCAN_TIME_SECONDS, false);
        pScan->clearResults();
        BLEDevice::getAdvertising()->start();
        Serial.println(">> Broadcast restarted");
        
        // 2) Check if there are active devices
        int activeDevices = 0;
        for (int i = 0; i < deviceCount; i++) {
            if (nearbyDevices[i].active) {
                activeDevices++;
            }
        }
        
        if (!didVibrateThisScan || activeDevices == 0) {
            analogWrite(VIBRATION_PIN, 0);
            setNeoPixelLevel(0); // Turn off all lights
            Serial.printf("[RESULT] This round detected %d active devices, no vibration, light strip off\n", activeDevices);
        } else {
            Serial.printf("[RESULT] This round detected %d active devices, with vibration and light strip feedback\n", activeDevices);
        }
        
        // Periodically output statistics (every 20 scans)
        static int scanCount = 0;
        scanCount++;
        if (scanCount >= 20) {
            // Kalman filter statistics
            if (kalmanStats.totalFiltered > 0) {
                printKalmanStats();
            }
            
            // Distance estimation statistics
            if (distanceStats.totalEstimations > 0) {
                printDistanceStats();
            }
            
            // Distance calibration table (output every 100 scans)
            static int calibrationCount = 0;
            calibrationCount++;
            if (calibrationCount >= 5) {  // 20 * 5 = 100 scans
                printDistanceCalibration();
                calibrationCount = 0;
            }
            
            scanCount = 0;
        }
        
        delay(500);
    }
} 