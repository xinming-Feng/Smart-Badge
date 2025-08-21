# NFC Image Uploader User Guide

## Project Overview

This is a complete NFC image transmission solution that includes:
- **Android Application**: Select, process, and send images to NFC chip
- **Hardware Code**: Receive NFC image data and display on 2.9-inch e-ink screen

## Quick Start

### 1. Android Application Installation

#### Method 1: Using Android Studio
1. Open Android Studio
2. Select "Open an existing project"
3. Select the project root directory
4. Connect an NFC-enabled Android device
5. Click the run button

#### Method 2: Direct APK Installation
1. Enable "Unknown Sources" installation on Android device
2. Transfer the generated APK file to the device
3. Install the APK file

### 2. Hardware Setup

#### Required Hardware
- ESP32 development board
- PN532 NFC module
- 2.9-inch e-ink screen (optional)
- Connection wires

#### Wiring Diagram
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

#### Code Upload
1. Open Arduino IDE
2. Install necessary libraries:
   - PN532
   - Adafruit GFX Library
   - Adafruit EPD Library
3. Open `hardware/simple_nfc_receiver.ino`
4. Select the correct board and port
5. Upload the code

## Usage Steps

### 1. Launch Application
- Open the NFC Image Uploader application
- Ensure the device's NFC function is enabled
- Grant camera and storage permissions

### 2. Select Image
- Click the "Select Image" button
- Choose an image from the gallery to process
- The application will automatically load and display a preview

### 3. Adjust Parameters
- **Dithering Intensity**: Controls the black and white conversion effect
  - 0-30: Low dithering, suitable for simple images
  - 30-70: Medium dithering, suitable for most images
  - 70-100: High dithering, suitable for complex images
- **Scale Image**: Adjust image size
  - 0-50: Reduce image size
  - 50-100: Enlarge image size

### 4. Send to NFC
- Click the "Send to NFC" button
- Bring your phone close to the NFC chip
- Wait for transmission completion notification

## Technical Details

### Dithering Algorithm
Uses the Floyd-Steinberg dithering algorithm to convert color images to high-quality black and white images:

```
Error diffusion pattern:
    X   7/16
3/16 5/16 1/16
```

### NFC Data Format
- **MIME Type**: image/png
- **Data Format**: PNG compressed image
- **Maximum Size**: 4KB (hardware limitation)

### Image Processing Pipeline
1. Load original image
2. Convert to grayscale
3. Apply dithering algorithm
4. Scale to target dimensions
5. Compress to PNG format
6. Send via NFC

## Troubleshooting

### Common Issues

#### 1. NFC Unavailable
- Check if the device supports NFC
- Ensure NFC function is enabled
- Restart the application

#### 2. Image Sending Failed
- Ensure the NFC chip is within effective range
- Check if the chip supports NDEF format
- Try reducing image size

#### 3. Slow Image Processing
- Select smaller original images
- Reduce dithering intensity
- Close other applications to free memory

#### 4. Hardware Unresponsive
- Check if wiring is correct
- Confirm PN532 module is working properly
- Check serial monitor output

### Debug Information

#### Android Side
- Check Logcat output
- Verify permission status
- Validate NFC adapter status

#### Hardware Side
- Open serial monitor (115200 baud rate)
- Check reception status information
- Verify data integrity

## Performance Optimization

### Image Size Recommendations
- **Original Image**: No more than 1024x1024 pixels
- **Processed Image**: Adapted for 2.9-inch screen (296x128 pixels)
- **File Size**: No more than 2KB

### Processing Speed Optimization
- Use smaller preview images
- Process images in background threads
- Release Bitmap resources promptly

## Extended Features

### Possible Improvements
1. **Multiple Dithering Algorithms**: Add algorithms like Jarvis-Judice-Ninke
2. **Image Cropping**: Add touch cropping functionality
3. **Batch Transmission**: Support continuous transmission of multiple images
4. **Transmission Progress**: Display real-time transmission progress
5. **Image Filters**: Add contrast and brightness adjustment

### Hardware Extensions
1. **Larger Storage**: Support for larger image files
2. **Multiple Screens**: Support different sizes of e-ink screens
3. **Wireless Transmission**: Add WiFi/Bluetooth transmission options
4. **Local Storage**: Save received images to SD card

## Technical Support

If you encounter problems, please check:
1. Device compatibility
2. Permission settings
3. NFC function status
4. Hardware connections

For more technical details, please refer to code comments and README documentation. 