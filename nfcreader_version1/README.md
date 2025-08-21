# NFC Image Uploader

A simple Android application that can transmit images via NFC to a 2.9-inch e-ink display.

## Features

- 📱 Image selection (from gallery or camera)
- 🎨 Floyd-Steinberg dithering algorithm processing
- 📏 Image scaling functionality
- 📡 NFC data transmission
- 👀 Real-time preview effects

## Technical Implementation

### Dithering Algorithm
Uses the Floyd-Steinberg dithering algorithm to convert color images to high-quality black and white images, particularly suitable for e-ink display.

### NFC Communication
- Support for NDEF format NFC tags
- Automatic NFC device detection
- Compressed image data transmission

### Image Processing
- Real-time grayscale conversion
- Adjustable dithering intensity
- Image scaling to fit screen

## Usage

1. Launch the application
2. Click the "Select Image" button to choose an image to process
3. Adjust dithering intensity and scaling parameters
4. Click the "Send to NFC" button
5. Bring your phone close to the NFC chip to complete the transmission

## System Requirements

- Android 5.0 (API 21) or higher
- NFC-enabled Android device
- Camera and storage permissions

## Development Environment

- Android Studio
- Gradle 7.4.2
- Android SDK 33
- Java 8

## Project Structure

```
app/
├── src/main/
│   ├── java/com/example/nfcreader/
│   │   ├── MainActivity.java          # Main activity
│   │   └── DitherProcessor.java       # Dithering algorithm processor
│   ├── res/
│   │   ├── layout/activity_main.xml   # Main interface layout
│   │   ├── values/strings.xml         # String resources
│   │   └── xml/nfc_tech_filter.xml    # NFC technology filter
│   └── AndroidManifest.xml            # Application manifest
└── build.gradle                       # Module build configuration
```

## Build and Run

1. Clone the project locally
2. Open the project in Android Studio
3. Connect an NFC-enabled Android device
4. Click the run button

## Notes

- Ensure the device's NFC function is enabled
- Camera and storage permissions are required
- It is recommended to use smaller images for better processing results
- NFC transmission may take a few seconds

## License

MIT License 