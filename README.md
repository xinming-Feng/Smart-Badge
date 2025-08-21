# Smart Badge Project
Design of a Smart Badge Integrating NFC Communication and E-ink Display for Digital Image Transmission and Multimodal Feedback

![01e39359a6a9f986adc8404c1c26038e](https://github.com/user-attachments/assets/bad92ba2-c0bf-440f-9fdb-a88e37561213)



## 🎯 Project Overview

This project presents a comprehensive smart badge solution that enables seamless image transmission from mobile devices to e-ink displays via NFC technology. The system is designed to foster digital empathy and enhance communication among organizational members through visual and Vibration feedback mechanisms.

![a9fa6791bef7bf54e322c5aeee878bbb](https://github.com/user-attachments/assets/ef0208fe-748b-4b5d-aafe-ac90f2e971cb)


## 🏗️ System Architecture

The Smart Badge project consists of two main components:

### 📱 Android Application (`nfcreader_version1/`)
- **Image Processing**: Advanced Floyd-Steinberg dithering algorithm for optimal e-ink display conversion
- **NFC Communication**: NDEF-compliant NFC tag writing capabilities
- **Cloud Integration**: GitHub-based image storage with automatic URL generation
- **Real-time Preview**: Live image processing preview with adjustable parameters

### ⚡ Hardware Components
- **ESP32 Development Board**
- **PN532 NFC Module**
- **2.9-inch E-ink Display**
- **NeoPixel**
- **Vibration Motor Module**

## ✨ Key Features

### Mobile Application Features
- **Image Selection**: Gallery and camera integration
- **Advanced Image Processing**: 
  - Floyd-Steinberg dithering algorithm
  - Adjustable dithering intensity (0-100%)
  - Real-time image scaling and preview
-  **NFC Integration**: 
  - Automatic NFC adapter detection
  - NDEF URI record writing
  - Support for all NFC tag types
- **Cloud Storage**: 
  - Automatic GitHub repository upload
  - Raw URL generation for hardware access
  - Secure API token management
- **User Interface**:
  - Intuitive parameter adjustment controls
  - Real-time processing feedback
  - Status notifications and error handling

### Hardware Features
- 🔄 **NFC Reception**: Automatic detection and processing of NFC tags
- 📺 **E-ink Display**: Optimized for high-contrast image rendering
- 🔋 **Power Efficiency**: Low-power design suitable for battery operation

## 🛠️ Technical Specifications

### Android Application
- **Minimum Android Version**: 8.0 (API 26)
- **Target Android Version**: 14 (API 34)
- **APK Size**: 4.2MB
- **Required Permissions**: NFC, Storage, Camera
- **Development Environment**: 
  - Android Studio
  - Gradle 7.4.2
  - Java 8
  - Android SDK 33

### Hardware Requirements
- **Microcontroller**: Waveshare E-Paper ESP32 Driver Board
- **NFC Module**: PN532 NFC Module V3
- **Display**: Waveshare 2.9-inch e-Paper
- **Power Supply**: 3.3V regulated power
- **NeoPixel**:NeoPixel Stick with 8 led beads
- **Vibration Motor Module**
- **NFC tags**


## 🚀 Getting Started

### Prerequisites
- Android device with NFC capability (Android 8.0+)
- ESP32 development board
- PN532 NFC module
- 2.9-inch e-ink display
- Arduino IDE with required libraries

### Installation

#### Android Application
1. **Method 1: Build from Source**
   ```bash
   git clone https://github.com/your-repo/Smart-Badge.git
   cd Smart-Badge/nfcreader_version1
   # Open in Android Studio and build
   ```

2. **Method 2: Direct Installation**
   - Download the latest release on Releases
   - Enable "Unknown Sources" on your Android device
   - Install the APK file

#### Hardware Setup

1. **Library Installation**:
   - PN532 Library
   - Adafruit GFX Library
   - Adafruit EPD Library

2. **Code Upload**:
   - Open Arduino IDE
   - Load the hardware sketch
   - Select ESP32 board and correct port
   - Upload the code

## 📋 Usage Workflow

1. **Image Selection**: Choose image from gallery or capture with camera
2. **Parameter Adjustment**: Set dithering intensity and scaling factor
3. **Processing**: Real-time Floyd-Steinberg dithering conversion
4. **Cloud Upload**: Automatic GitHub repository upload
5. **NFC Writing**: Write image URL to NFC tag
6. **Hardware Display**: ESP32 receives NFC data and displays image
   
   ![e8fcbfe3c9267e0b11e0bf58a116f986](https://github.com/user-attachments/assets/0bfd4926-b941-4762-aacc-50f9f8ea3fb4)


## 🔧 Development and Customization

### Android Application Structure
```
nfcreader_version1/
├── app/
│   ├── src/main/
│   │   ├── java/com/example/nfcreader/
│   │   │   ├── MainActivity.java          # Main application logic
│   │   │   └── DitherProcessor.java       # Image processing algorithms
│   │   ├── res/                           # UI resources and layouts
│   │   └── AndroidManifest.xml            # App permissions and configuration
│   └── build.gradle                       # Build configuration
```

### Key Classes and Methods
- **MainActivity**: Core application controller with NFC and image processing
- **DitherProcessor**: Floyd-Steinberg dithering implementation
- **GitHub Integration**: Cloud storage and URL generation
- **NFC Handler**: NDEF record creation and tag writing

### Hardware Code Structure
- **NFC Reception**: PN532 library integration
- **Display Control**: E-ink display driver implementation
- **Image Processing**: Bitmap handling and display formatting
- **Distance calculation in BLE mode**:Distance detection is performed by converting RSSI into distance
- **Multimodal feedback**:Feedback from both visual and tactile ways
- **Power Management**: Sleep modes and energy optimization



## 📄 Requirements and Dependencies

### Python Dependencies
```
matplotlib>=3.5.0
numpy>=1.21.0
```

### Android Dependencies
- AndroidX libraries
- NFC API (Android SDK)
- HTTP client libraries
- Image processing utilities

### Hardware Dependencies
- Arduino ESP32 Core
- PN532 NFC Library
- Adafruit display libraries
- SPI/I2C communication protocols

  
## 🎬 Demo for usage
https://www.youtube.com/watch?v=YsoVYDEWGc0

## 🛠️ Enclosures
The Enclosures folder displays the models and information for the enclosures.

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.


## 📧 Support

For technical support, bug reports, or feature requests, please open an issue on our GitHub repository.


