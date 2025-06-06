# TauDEM

**TauDEM (Terrain Analysis Using Digital Elevation Models)** is a suite of Digital Elevation Model (DEM) tools for the extraction and analysis of hydrologic information from topography as represented by a DEM.

![TauDEM Logo](WindowsInstaller/taudem.svg)

## 📋 Table of Contents

- [What's TauDEM](#-whats-taudem)
- [Setup VS Code for TauDEM Development](#️-setup-vs-code-for-taudem-development)
- [Building TauDEM](#-building-taudem)
- [Dependencies](#-dependencies)
- [Project Structure](#-project-structure)
- [Testing](#-testing)
- [Contributing](#-contributing)
- [Resources](#-resources)

## 🌍 What's TauDEM

TauDEM is a comprehensive suite of tools designed for terrain analysis using Digital Elevation Models. It provides algorithms for:

- **Flow Direction Analysis**: D8 and D-Infinity flow direction algorithms
- **Contributing Area Calculation**: Watershed delineation and catchment area computation
- **Stream Network Extraction**: Automatic identification and characterization of stream networks
- **Topographic Analysis**: Slope, aspect, curvature, and wetness index calculations
- **Hydrologic Modeling**: Tools for runoff analysis and watershed characterization

### Key Features

- **Parallel Processing**: Built with MPI support for high-performance computing
- **Multiple Algorithms**: Supports both D8 and D-Infinity flow routing methods
- **ArcGIS Integration**: Python toolbox for seamless integration with ArcGIS
- **Cross-Platform**: Runs on Windows, Linux, and macOS
- **Open Source**: Licensed under GPL v3

For more information, visit the [official website](http://hydrology.usu.edu/taudem) and the [project wiki](https://github.com/dtarb/TauDEM/wiki).

## 🛠️ Setup VS Code for TauDEM Development

### Prerequisites

Before setting up VS Code, ensure you have the required dependencies installed (see [Dependencies](#dependencies) section).

### VS Code Extensions

Install the following essential extensions for TauDEM development:

```bash
# Core C++ development
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools

# IntelliSense and code completion
code --install-extension llvm-vs-code-extensions.vscode-clangd

# Git integration
code --install-extension eamodio.gitlens

# Additional helpful extensions
code --install-extension jeff-hykin.better-cpp-syntax

# Optional extensions for specialized tasks
code --install-extension ms-vscode.hexeditor  # For examining binary DEM files (optional)
```

### Platform-Specific Setup

#### macOS Setup

1. **Copy VS Code settings template**:
   ```bash
   cp .vscode/settings-macos.json.template .vscode/settings.json
   ```

2. **Install dependencies via Homebrew**:
   ```bash
   # Core dependencies
   brew install gdal
   brew install open-mpi
   brew install cmake
   
   # Development tools
   brew install llvm  # for clangd
   ```

3. **Configure IntelliSense**: The `.vscode/c_cpp_properties.json` is already configured for macOS with proper include paths.

#### Windows Setup

1. **Copy VS Code settings template**:
   ```cmd
   copy .vscode\settings-windows.json.template .vscode\settings.json
   ```

2. **Install vcpkg dependencies**: Follow the Windows build instructions in [Building TauDEM](#building-taudem).

3. **Configure paths**: Update the include paths in `.vscode/c_cpp_properties.json` if your vcpkg installation is in a different location.

#### Linux Setup

1. **Install dependencies**:
   ```bash
   sudo apt update
   sudo apt install -y build-essential cmake openmpi-bin libopenmpi-dev \
                        gdal-bin libgdal-dev libproj-dev libtiff-dev libgeotiff-dev
   ```

2. **Configure VS Code**: Create `.vscode/settings.json` with appropriate Linux-specific settings.

### VS Code Configuration

The project includes pre-configured settings for:

- **IntelliSense**: Proper C++ standard (C++17) and include paths
- **CMake Integration**: Source directory set to `src/`
- **Code Formatting**: C++ formatting with `ms-vscode.cpptools`
- **Compiler Configuration**: Platform-specific compiler settings

### Debugging Setup

#### Configure Launch Configuration (for debugging TauDEM tools - TODO: Need to test this configuration)

Create `.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug TauDEM Tool",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/bin/pitremove",
            "args": ["-z", "input.tif", "-fel", "output.tif"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ]
        }
    ]
}
```

#### Understanding launch.json Configuration

The `launch.json` file configures VS Code's **debugging system** for TauDEM development. Here's what each setting does:

**🔧 Core Configuration**
- **`"version": "0.2.0"`** - VS Code debugging configuration format version
- **`"configurations": []`** - Array containing different debug configurations (you can have multiple)

**🎯 Debug Target Settings**
- **`"name": "Debug TauDEM Tool"`** - Display name in VS Code's debug dropdown menu
- **`"type": "cppdbg"`** - Specifies C++ debugger type
- **`"request": "launch"`** - Launch a new process (alternative: `"attach"` to running process)

**📂 Program and Arguments**
- **`"program": "${workspaceFolder}/bin/pitremove"`** - Path to executable to debug
  - `${workspaceFolder}` = `/Users/a01841079/Workspace/SoftwareProjects/TauDEM`
  - Targets the `pitremove` tool (pit removal algorithm)
- **`"args": ["-z", "input.tif", "-fel", "output.tif"]`** - Command line arguments
  - Simulates running: `./bin/pitremove -z input.tif -fel output.tif`
  - `-z input.tif`: Input DEM file with pits
  - `-fel output.tif`: Output filled DEM file

**🔍 Execution Environment**
- **`"stopAtEntry": false`** - Don't automatically break at `main()` function
- **`"cwd": "${workspaceFolder}"`** - Working directory for the debugged program
- **`"environment": []`** - Environment variables (empty array = inherit from VS Code)
- **`"externalConsole": false`** - Use VS Code's integrated terminal instead of external console

**🛠️ Debugger Configuration**
- **`"MIMode": "gdb"`** - Use GDB debugger (GNU Debugger for macOS/Linux)
  - On Windows, this would be `"vsdbg"` for Visual Studio debugger
- **`"setupCommands"`** - GDB initialization commands
  - `"-enable-pretty-printing"` makes variable display more readable during debugging

**🚀 How to Use This Configuration**

1. **Place test files**: Ensure `input.tif` exists in your workspace root

2. **Set breakpoints**: Click in the margin next to line numbers in C++ source files

3. **Start debugging**: Press `F5` or Run → Start Debugging (VS Code will automatically build the debug version via the preLaunchTask)

5. **Debug controls available**:
   - **Step Over** (`F10`): Execute current line, don't enter function calls
   - **Step Into** (`F11`): Enter function calls to debug inside them
   - **Step Out** (`Shift+F11`): Exit current function
   - **Continue** (`F5`): Run until next breakpoint
   - **Variables panel**: Inspect variable values and memory
   - **Call stack**: See function call hierarchy
   - **Watch expressions**: Monitor specific variables/expressions

**🔄 Customizing for Other Tools**

To debug different TauDEM tools, create additional configurations:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug PitRemove",
            "program": "${workspaceFolder}/bin/pitremove",
            "args": ["-z", "dem.tif", "-fel", "filled.tif"]
        },
        {
            "name": "Debug AreaD8",
            "program": "${workspaceFolder}/bin/aread8",
            "args": ["-p", "flowdir.tif", "-ad8", "area.tif"]
        },
        {
            "name": "Debug D8FlowDir",
            "program": "${workspaceFolder}/bin/d8flowdir",
            "args": ["-fel", "filled.tif", "-p", "flowdir.tif", "-sd8", "slope.tif"]
        }
    ]
}
```

This debugging setup is invaluable for understanding TauDEM's flow direction algorithms, watershed delineation logic, and spatial data processing workflows!

#### Configure Tasks for Building/Compiling


**For macOS development**, copy the macOS-specific tasks template:
```bash
cp .vscode/tasks-macos.json.template .vscode/tasks.json
```

The macOS tasks configuration includes:
- **Build tasks** with multiple compiler options (Clang, Apple GCC, Homebrew GCC)
- **Debug and Release builds** for each compiler
- **Install/uninstall tasks** with custom path support
- **Interactive tool runner** with user input prompts

**For Windows development**, copy the Windows-specific tasks template:
```cmd
copy .vscode\tasks-windows.json.template .vscode\tasks.json
```

The Windows tasks configuration includes:
- **Build tasks** using `build-windows.bat` script
- **CMake tasks** with vcpkg integration
- **Clean** tasks
- **Interactive tool runner** with user input prompts

#### Configure Launch Configuration for Debugging

**For macOS development**, copy the macOS-specific launch template:
```bash
cp .vscode/launch-macos.json.template .vscode/launch.json
```

The macOS launch configuration includes:
- **Debug configurations** for all major TauDEM tools (PitRemove, D8FlowDir, StreamNet, etc.)
- **LLDB debugger integration** optimized for macOS
- **Pre-launch build tasks** that automatically compile before debugging
- **Interactive debugging** with user input prompts for file paths and parameters
- **MPI debugging support** for parallel processing development
- **Process attachment** capabilities for debugging running instances

**Key debugging configurations available:**
- **PitRemove**: Debug pit filling algorithms with DEM input/output
- **D8FlowDir**: Debug D8 flow direction computation
- **DinfFlowDir**: Debug D-Infinity flow direction algorithms
- **AreaD8**: Debug contributing area calculations
- **StreamNet**: Debug stream network extraction
- **GageWatershed**: Debug watershed delineation
- **DropAnalysis**: Debug threshold analysis tools
- **Distance Tools**: Debug horizontal/vertical distance calculations

**For Windows development**, copy the Windows-specific launch template:
```cmd
copy .vscode\launch-windows.json.template .vscode\launch.json
```

**Usage**: Select a configuration from the debug dropdown and press F5 to start debugging.

## 🔨 Building TauDEM

TauDEM supports multiple build methods and platforms.

### Quick Start

#### macOS
```bash
# Release build (optimized)
make release COMPILER=clang

# Debug build (with debugging symbols)
make debug COMPILER=clang
```

#### Linux
```bash
# Release build (optimized)
make release COMPILER=linux

# Debug build (with debugging symbols)
make debug COMPILER=linux
```

#### Windows
```cmd
# Release build (optimized)
build-windows.bat release

# Debug build (with debugging symbols)
build-windows.bat debug
```

### Build Options

#### Using Make (Linux/macOS)

```bash
# Debug build
make debug COMPILER=clang    # macOS with Clang
make debug COMPILER=linux    # Linux with GCC

# Release build
make release COMPILER=clang  # macOS with Clang
make release COMPILER=linux  # Linux with GCC

# Alternative compilers (macOS)
make release COMPILER=macos      # Homebrew GCC
make release COMPILER=gcc-apple  # Apple GCC

# Clean build
make clean
```

#### Using CMake Directly

```bash
cd src
mkdir build && cd build
cmake .. -C ../../config.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

#### Docker Build (Building/Testing TauDEN for Linux)

```bash
# Build the Docker image for testing (from the root of the project)
docker build -f Dockerfile-run.tests -t taudem-linux-run-tests .

# Run the Docker container with volume mounting
docker run --rm -it -v $(pwd):/app taudem-linux-run-tests

# Inside the container - clean and build
make clean
make dk-release COMPILER=linux
make dk-install PREFIX=/usr/local

# Run tests in Docker
su - taudem-docker
cd /app
make dk-run-tests

# Exit the container
exit
exit
```

### Installation

```bash
# Install to default location (/usr/local/taudem)
make install

# Install to custom location (now supported!)
make install PREFIX=/custom/path

# Uninstall
make uninstall
```

## 📦 Dependencies

TauDEM requires the following dependencies:

### Core Dependencies

- **C++17 Compiler**: GCC 7+, Clang 5+, or Visual Studio 2019+
- **MPI**: Message Passing Interface for parallel processing
- **GDAL**: Geospatial Data Abstraction Library for raster/vector I/O
- **CMake**: Build system (version 3.10+)

### Platform-Specific Installation

#### macOS (Homebrew)

```bash
brew install gdal open-mpi cmake
```

#### Linux (Ubuntu/Debian)

```bash
sudo apt install -y build-essential cmake openmpi-bin libopenmpi-dev \
                    gdal-bin libgdal-dev libproj-dev libtiff-dev libgeotiff-dev
```

#### Windows (vcpkg)

```cmd
# 1. Install vcpkg (C++ package manager)
mkdir C:\dev
cd C:\dev
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 2. Integrate vcpkg with Visual Studio (optional but recommended)
.\vcpkg integrate install

# 3. Add vcpkg to PATH (optional - allows running vcpkg from anywhere)
# Add the vcpkg directory to your system PATH environment variable
# Or run vcpkg commands from the vcpkg directory

# 4. Install TauDEM dependencies - for x64 builds (recommended), specify the triplet
vcpkg install gdal[core,tools,sqlite3,libkml]:x64-windows mpi:x64-windows
```

### Optional Dependencies (Windows)

- **Python 3.10+**: For ArcGIS integration and Python tools
- **ArcGIS**: For using TauDEM toolbox in ArcGIS

## 📁 Project Structure

```
TauDEM/
├── src/                    # Source code
│   ├── *.cpp, *.h          # TauDEM algorithms implementation
│   ├── CMakeLists.txt      # CMake build configuration
│   └── build/              # Build directory (generated)
├── pyfiles/                # Python tools and ArcGIS integration
│   ├── *.py                # Python wrappers for TauDEM tools
│   └── *.tbx               # ArcGIS toolbox files
├── .vscode/                # VS Code configuration
│   ├── c_cpp_properties.json       # IntelliSense configuration
│   ├── settings-*.json.template    # Platform-specific VS Code settings
│   |── launch-*.json.template      # Platform-specific debugging configurations
│   └── tasks-*.json.template       # Platform-specific build tasks
├── config.cmake            # CMake configuration for different platforms
├── Makefile                # Main build system
├── build-windows.bat       # Windows build script
├── Dockerfile              # Docker configuration (running TauDEM in docker container)
├── Dockerfile-run.tests    # Docker configuration (building TauDEM for testing on Ubuntu)
├── README.md               # This file
└── README.txt              # Original documentation
```

### Key Components

- **`src/`**: Contains all C++ source code for TauDEM algorithms
- **`pyfiles/`**: Python wrappers and ArcGIS toolbox integration
- **`config.cmake`**: Platform-specific CMake configurations
- **`.vscode/`**: VS Code development environment setup

## 🧪 Testing

TauDEM includes comprehensive testing capabilities:

### Running Tests

For running tests for TauDEM Linux build, see section [Docker Build](#docker-build) for Docker build instructions.

```bash
# Manual testing with sample data
cd /path/to/test/data
pitremove -z input.tif -fel output.tif
```

#### Test Data and Test Scripts for Running TauDEM Tests

Download test data from the [TauDEM-Test-Data](https://github.com/dtarb/TauDEM-Test-Data) repository:

```bash
git clone https://github.com/dtarb/TauDEM-Test-Data.git
cd TauDEM-Test-Data/Input
# Run tests on macOS/Linux
# Setup bats testing framework (one-time setup)
brew install bats-core
git clone https://github.com/bats-core/bats-support.git /tmp/bats-support
git clone https://github.com/bats-core/bats-assert.git /tmp/bats-assert
git clone https://github.com/bats-core/bats-file.git /tmp/bats-file
# Run tests assuming TauDEM installed path is in PATH
./taudem-tests.sh
# Run tests assuming TauDEM installed path is in TAUDEM_PATH
TAUDEM_PATH=/path/to/taudem ./taudem-tests.sh
# Run tests on Windows (bats testing framework is not necessary)
testall.bat
```

### Validation

The test suite validates:
- Algorithm correctness
- Memory management
- Parallel processing functionality
- Cross-platform compatibility
- Performance benchmarks

## 🤝 Contributing

We welcome contributions to TauDEM! Please see our [contribution guidelines](https://github.com/dtarb/TauDEM/wiki/Contributing).

### Development Workflow

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/new-algorithm`
3. **Setup development environment**: Follow the VS Code setup instructions
4. **Make changes**: Follow our coding standards (see `.github/copilot-instructions-WIP.md`)
5. **Test thoroughly**: Ensure all tests pass
6. **Submit a pull request**

### Coding Standards

- **C++17**: Use modern C++ features appropriately
- **Code Style**: Follow existing conventions
- **Documentation**: Document all public functions
- **Testing**: Include tests for new functionality

## 📚 Resources

- **Website**: [http://hydrology.usu.edu/taudem](http://hydrology.usu.edu/taudem)
- **Wiki**: [https://github.com/dtarb/TauDEM/wiki](https://github.com/dtarb/TauDEM/wiki)
- **Documentation**: [http://hydrology.usu.edu/taudem/taudem5/documentation.html](http://hydrology.usu.edu/taudem/taudem5/documentation.html)
- **Issues**: [https://github.com/dtarb/TauDEM/issues](https://github.com/dtarb/TauDEM/issues)
- **Test Data**: [https://github.com/dtarb/TauDEM-Test-Data](https://github.com/dtarb/TauDEM-Test-Data)

### Academic References

- Tarboton, D. G. (1997). "A new method for the determination of flow directions and upslope areas in grid digital elevation models." Water Resources Research, 33(2), 309-319.
- Tarboton, D. G. (2003). "Terrain Analysis Using Digital Elevation Models in Hydrology." 23rd ESRI International Users Conference, San Diego, California.

### Support

- **Community Forum**: Use GitHub Discussions for questions
- **Bug Reports**: Submit issues on GitHub
- **Feature Requests**: Open enhancement issues on GitHub

---

## 📄 License

TauDEM is licensed under the GNU General Public License v3.0. See [GPLv3license.txt](GPLv3license.txt) for details.

---

**Developed by**: Utah State University  
**Maintainer**: David Tarboton  
**Contributors**: TODO: create a CONTRIBUTORS.md file
