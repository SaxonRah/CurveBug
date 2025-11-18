# CurveBug - raylib Edition

A cross-platform curve tracer viewer for the vintageTEK CurveBug hardware. Rewritten in C with raylib for minimal dependencies and maximum performance.

![Windows](https://github.com/SaxonRah/CurveBug/actions/workflows/build-windows.yml/badge.svg)
![Linux](https://github.com/SaxonRah/CurveBug/actions/workflows/build-linux.yml/badge.svg)
![macOS](https://github.com/SaxonRah/CurveBug/actions/workflows/build-macos.yml/badge.svg)

## Features

- **Real-time I-V curve plotting** with dual DUT support
- **Multiple excitation modes**: 4.7K (T), 100K Weak (W), and Alternating
- **Interactive plotting**: Pan, zoom, and auto-scale
- **Customizable interface**: Dark/Light themes with full color customization
- **Configurable keybinds** and window settings
- **Auto-detection** of CurveBug hardware (VID: 16D0, PID: 13F9)
- **Tiny binary**: ~1-2MB executable with no runtime dependencies

## Usage

### Quick Start

1. Connect your CurveBug device via USB
2. Run the executable:
   - **Windows**: `curvebug.exe`
   - **Linux/macOS**: `./curvebug`
3. Press `F1` or click `Settings` in the bottom right to open settings and configure your serial port
4. Use the `Auto Find` button to automatically detect CurveBug hardware

### Keyboard Controls

| Key | Action |
|-----|--------|
| `SPACE` | Cycle excitation mode (4.7K -> 100K Weak -> Alternating) |
| `P` | Pause/Resume data acquisition |
| `S` | Toggle single channel mode |
| `A` | Toggle auto-scale |
| `F` | Fit view to data |
| `R` | Reset view (zoom and pan) |
| `F1` | Open settings |
| `ESC` | Quit (or close settings without saving) |

### Mouse Controls

- **Scroll wheel**: Zoom in/out
- **Click and drag**: Pan the view (when not in auto-scale mode)

### Settings

Access the settings window via `F1` or the Settings button:

- **General Tab**: Serial port configuration, window size, auto-find
- **Colors Tab**: Full RGB customization with Dark/Light mode presets
- **Key Bindings Tab**: Customize all keyboard shortcuts

Settings are automatically saved to `curvebug.cfg` in the same directory.

## Development

### Prerequisites

- **CMake** 3.15 or higher
- **C11 compatible compiler**:
  - Windows: MSVC 2019+, MinGW-w64, or Clang
  - Linux: GCC 7+ or Clang 5+
  - macOS: Xcode Command Line Tools (Clang)

### Platform-Specific Dependencies

**Linux:**
```bash
sudo apt-get install build-essential cmake git \
    libx11-dev libxrandr-dev libxi-dev libxcursor-dev \
    libxinerama-dev mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev
```

**macOS:**
```bash
brew install cmake git
```

**Windows:**
- Install [Visual Studio 2019+](https://visualstudio.microsoft.com/) with C++ desktop development, or
- Install [MinGW-w64](https://www.mingw-w64.org/)

### Project Structure

```
curvebug/
├── src/
│   ├── main.c          # Main application and UI
│   ├── serial.c/h      # Serial port communication
│   ├── config.c/h      # Configuration management
│   └── plotter.h       # Plot data structures
├── external/
│   ├── raylib/         # Cloned raylib library
│   └── raygui/         # Cloned raygui UI library
├── CMakeLists.txt      # Build configuration
└── README.md
```

## Building Locally

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/curvebug.git
cd curvebug
```

### 2. Clone Dependencies

```bash
# Clone raylib
git clone --depth 1 --branch 5.0 https://github.com/raysan5/raylib.git external/raylib

# Clone raygui
git clone --depth 1 https://github.com/raysan5/raygui.git external/raygui
```

### 3. Build with CMake

**Windows (Visual Studio):**
```cmd
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

**Windows (MinGW):**
```cmd
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Linux:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**macOS:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 4. Run the Executable

**Windows:**
```cmd
build\Release\curvebug.exe
```

**Linux/macOS:**
```bash
./build/curvebug
```

## Serial Port Configuration

### Default Ports

- **Windows**: `COM4`
- **Linux**: `/dev/ttyUSB0` or `/dev/ttyACM0`
- **macOS**: `/dev/cu.usbserial` or `/dev/cu.usbmodem`

### Linux Permissions

On Linux, you may need to add your user to the `dialout` group to access serial ports:

```bash
sudo usermod -a -G dialout $USER
# Log out and log back in for changes to take effect
```

### Auto-Detection

The application can automatically detect CurveBug devices by:
- **VID/PID**: `VID_16D0&PID_13F9`
- **Device description**: Contains "CurveBug"

Use the "Auto Find" button in Settings -> General to automatically locate your device.

## Troubleshooting

### No Data Displayed
- Check serial port connection in Settings -> General
- Try the "Auto Find" button
- Verify CurveBug is powered and connected
- On Linux, check permissions (see above)

### Settings Not Saving
- Ensure the application has write permissions in its directory
- Check that `curvebug.cfg` can be created/modified

### Build Errors
- Ensure all dependencies are cloned into `external/`
- Verify CMake version is 3.15 or higher: `cmake --version`
- Check that compiler is properly installed and in PATH

## License
MIT License
This project uses:
- [raylib](https://github.com/raysan5/raylib) - zlib License
- [raygui](https://github.com/raysan5/raygui) - zlib License

## Credits

Original C++ version: [CurveBug](https://github.com/rbep/CurveBug) by Bob Puckette
Python version: [PyCurveBug](https://github.com/SaxonRah/PyCurveBug) by Robert Valentine

C/raylib port with features and cross-platform support.

---

**Hardware**: Designed for vintageTEK CurveBug hardware  
**Tested**: Windows 10 - Needs Windows 11, Linux, and macOS testing
