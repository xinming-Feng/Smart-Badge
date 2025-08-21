# Smart Badge Project
Design of a Smart Badge Integrating NFC Communication and E-ink Display for Digital Image Transmission and Multimodal Feedback

## 🎯 Project Overview

This project presents a comprehensive smart badge solution that enables seamless image transmission from mobile devices to e-ink displays via NFC technology. The system is designed to foster digital empathy and enhance communication among organizational members through visual feedback mechanisms.

## 🏗️ System Architecture

The Smart Badge project consists of two main components:

### 📱 Android Application (`nfcreader_version1/`)
- **Image Processing**: Advanced Floyd-Steinberg dithering algorithm for optimal e-ink display conversion
- **NFC Communication**: NDEF-compliant NFC tag writing capabilities
- **Cloud Integration**: GitHub-based image storage with automatic URL generation
- **Real-time Preview**: Live image processing preview with adjustable parameters

### ⚡ Hardware Components
- **ESP32 Development Board**: Main microcontroller for NFC reception and display control
- **PN532 NFC Module**: Handles NFC data reception from mobile devices
- **2.9-inch E-ink Display**: High-contrast monochrome display (296x128 pixels)
- **Supporting Electronics**: Power management and connection interfaces

## ✨ Key Features

### Mobile Application Features
- 📸 **Image Selection**: Gallery and camera integration
- 🎨 **Advanced Image Processing**: 
  - Floyd-Steinberg dithering algorithm
  - Adjustable dithering intensity (0-100%)
  - Real-time image scaling and preview
- 📡 **NFC Integration**: 
  - Automatic NFC adapter detection
  - NDEF URI record writing
  - Support for all NFC tag types
- ☁️ **Cloud Storage**: 
  - Automatic GitHub repository upload
  - Raw URL generation for hardware access
  - Secure API token management
- 🖥️ **User Interface**:
  - Intuitive parameter adjustment controls
  - Real-time processing feedback
  - Status notifications and error handling

### Hardware Features
- 🔄 **NFC Reception**: Automatic detection and processing of NFC tags
- 📺 **E-ink Display**: Optimized for high-contrast image rendering
- 🔋 **Power Efficiency**: Low-power design suitable for battery operation
- 🌐 **Connectivity**: WiFi capabilities for extended functionality

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
- **Microcontroller**: ESP32 (WiFi + Bluetooth capable)
- **NFC Module**: PN532 (I2C/SPI interface)
- **Display**: 2.9" E-ink display (296x128 pixels)
- **Power Supply**: 3.3V regulated power
- **Memory**: Minimum 4MB flash, 520KB RAM

### Image Processing Specifications
- **Input Formats**: JPEG, PNG, BMP
- **Output Format**: 1-bit monochrome PNG
- **Maximum Input Size**: 1024x1024 pixels
- **Target Output Size**: 296x128 pixels (e-ink optimized)
- **Processing Algorithm**: Floyd-Steinberg error diffusion dithering
- **Compression**: PNG with optimized palette

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
   - Download `smart-badge-v1.0-signed.apk`
   - Enable "Unknown Sources" on your Android device
   - Install the APK file

#### Hardware Setup
1. **Wiring Connections**:
   ```
   ESP32    PN532
   3.3V  -> VCC
   GND   -> GND
   D5    -> CS
   D2    -> IRQ
   D3    -> RST
   D23   -> MOSI
   D19   -> MISO
   D18   -> SCK
   ```

2. **Library Installation**:
   - PN532 Library
   - Adafruit GFX Library
   - Adafruit EPD Library

3. **Code Upload**:
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
- **Power Management**: Sleep modes and energy optimization

## 🎨 Image Processing Pipeline

1. **Input Validation**: Format and size verification
2. **Grayscale Conversion**: RGB to luminance transformation
3. **Floyd-Steinberg Dithering**: Error diffusion algorithm
4. **Scaling**: Adaptive resizing for e-ink display
5. **Compression**: PNG optimization for NFC transmission
6. **Quality Assurance**: Output validation and error handling

## 🔍 Advanced Features

### Dithering Algorithm Details
- **Error Diffusion Pattern**:
  ```
      X   7/16
  3/16 5/16 1/16
  ```
- **Adjustable Intensity**: 0-100% error propagation control
- **Real-time Processing**: Optimized for mobile performance
- **Quality Optimization**: Balanced speed vs. quality algorithms

### NFC Communication Protocol
- **Data Format**: NDEF URI records
- **Payload**: GitHub raw URL pointing to processed image
- **Compatibility**: Universal NFC tag support
- **Error Handling**: Retry mechanisms and status feedback

## 🐛 Troubleshooting

### Common Issues
- **NFC Not Working**: Verify device compatibility and permissions
- **Image Processing Slow**: Reduce image size or lower dithering intensity
- **Upload Failures**: Check network connectivity and GitHub token
- **Hardware Unresponsive**: Verify wiring and power supply

### Debug Information
- **Android**: Logcat output with detailed error messages
- **Hardware**: Serial monitor output (115200 baud rate)
- **Network**: HTTP response codes and error handling

## 📊 Performance Metrics

- **Image Processing Time**: < 3 seconds for 512x512 images
- **NFC Write Speed**: < 1 second for typical payloads
- **Memory Usage**: < 50MB RAM during processing
- **Battery Impact**: Minimal with optimized algorithms

## 🔮 Future Enhancements

### Planned Features
- **Multiple Dithering Algorithms**: Jarvis-Judice-Ninke, Stucki
- **Batch Processing**: Multiple image support
- **Advanced Filters**: Contrast, brightness, gamma correction
- **Wireless Alternatives**: Bluetooth and WiFi transmission options

### Hardware Expansions
- **Larger Displays**: Support for various e-ink sizes
- **Color Displays**: RGB e-ink compatibility
- **Local Storage**: SD card integration for offline operation
- **Sensor Integration**: Environmental data display

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

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

We welcome contributions to the Smart Badge project! Please read our contributing guidelines and submit pull requests for any improvements.

## 📧 Support

For technical support, bug reports, or feature requests, please open an issue on our GitHub repository.

## 🏆 Acknowledgments

- Floyd-Steinberg dithering algorithm implementation
- Android NFC development community
- ESP32 and Arduino ecosystem contributors
- E-ink display technology providers

---

**Project Status**: Active Development | **Version**: 1.0 | **Last Updated**: 2024
