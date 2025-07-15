# TauDEM

**TauDEM (Terrain Analysis Using Digital Elevation Models)** is a suite of Digital Elevation Model (DEM) tools for the extraction and analysis of hydrologic information from topography as represented by a DEM.

![TauDEM Logo](WindowsInstaller/taudem.svg)

## 📋 Table of Contents

- [What's TauDEM](#-whats-taudem)
- [Setup Visual Studio Code for TauDEM Development](#setup-vs-code-for-taudem-development)
- Building/Compiling TauDEM
  - [Using Command Line](#-building-taudem-from-command-line)
  - [Using VS Code Tasks](#configure-tasks-for-buildingcompiling)
- [Dependencies](#-dependencies)
- [Project Structure](#-project-structure)
- [Testing](#-testing)
- [Contributing](#-contributing)
- [Resources](#-resources)
- [Support](#support)
- [License](#-license)

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

## Setup VS Code for TauDEM Development

### Clone the TauDEM Github Repository

#### Installing Git

Before cloning the repository, you'll need to install Git if you don't have it already:

**macOS**:
```bash
# Using Homebrew
brew install git

# Or download the installer from
# https://git-scm.com/download/mac
```

**Windows**:
```bash
# Download and run the installer from
# https://git-scm.com/download/win
```

**Linux (Debian/Ubuntu)**:
```bash
sudo apt update
sudo apt install git
```

Verify your installation:
```bash
git --version
```

#### Cloning the Repository

After installing Git, clone the TauDEM repository at a location of your choice:

```bash
git clone https://github.com/dtarb/TauDEM.git
cd TauDEM
git status
# Should show 'On branch Develop'
# You can then checkout the branch you want to work on or create a new branch
```

### Install Visual Studio Code (VS Code)

Download and install Visual Studio Code for your operating system using this link:
https://code.visualstudio.com/download

### Prerequisites

Before setting up VS Code, ensure you have the required dependencies installed (see [Dependencies](#-dependencies) section).

### VS Code Extensions

Install the following essential extensions for TauDEM development:

```bash
# Core C++ development
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cpptools-extension-pack

# CMake integration
code --install-extension ms-vscode.cmake-tools

# Git integration
code --install-extension eamodio.gitlens

# Additional helpful extensions (better C++ syntax highlighting)
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
   ```

3. **Configure IntelliSense**: The `.vscode/c_cpp_properties.json` is already configured for macOS with proper include paths.

#### Windows Setup

1. **Copy VS Code settings template**:
   ```cmd
   copy .vscode\settings-windows.json.template .vscode\settings.json
   ```

2. **Install vcpkg dependencies**: See [section](#windows-vcpkg) for detailed instructions.

3. **Configure paths**: Update the include paths in `.vscode/c_cpp_properties.json` if your vcpkg installation is in a different location.

4. **Install CMake**: Download and install CMake for Windows (look for the binary distribution for platform 'Windows x64 Installer') from the following link:

   https://cmake.org/download/

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

#### Configure Launch Configuration (for debugging TauDEM tools)

**NOTE**: Here is an example of a `launch.json` configuration for debugging shown to explain the configuration. You do not need to create this file. The `launch-*.json.template` files are already configured for each platform.

Example `.vscode/launch.json`:

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
  - `${workspaceFolder}` = `path/to/TauDEM local repository`
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

**🚀 How to Use the Above Example Configuration**

1. **Place test files**: Ensure `input.tif` exists in your workspace root

2. **Set breakpoints**: Click in the margin next to line numbers in C++ source files

3. **Start debugging**: Press `F5` or Run → Start Debugging to debug the first configuration. To debug a different target, first press `cmd+shift+d` (macos) or `ctrl+shift+d` (Windows/Linux) to open the Debug panel, or click on the **Run and Debug** icon (icon with a bug symbol) in VS Code's activity bar (vertical toolbar on the left side of vscode). Then select a configuration from the debug dropdown and press `F5` to start debugging.

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

This debugging setup is helpful for understanding TauDEM's flow direction algorithms, watershed delineation logic, and spatial data processing workflows!

#### Configure Tasks for Building/Compiling

Tasks configurations make it possible to run build scripts, compile code, and perform other project-related tasks directly from VS Code.

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
- **Clean** Delete all build directories and binaries
- **Interactive tool runner** with user input prompts

**Note**: The tasks defined in the `tasks.json` can be executed from the Command Palette (`cmd+shift+p` or `ctrl+shift+p`) and then typing `Tasks: Run Task` and selecting the task to run.

#### Configure Launch Configuration for Debugging

**For macOS development**, copy the macOS-specific launch template:
```bash
cp .vscode/launch-macos.json.template .vscode/launch.json
```

The macOS launch configuration includes:
- **Debug configurations** for all TauDEM tools (PitRemove, D8FlowDir, StreamNet, etc.)
- **LLDB debugger integration** optimized for macOS
- **Pre-launch build tasks** that automatically compile before debugging
- **Interactive debugging** with user input prompts for file paths and parameters
- **MPI debugging support** for parallel processing development
- **Process attachment** capabilities for debugging running instances


**For Windows development**, copy the Windows-specific launch template:
```cmd
copy .vscode\launch-windows.json.template .vscode\launch.json
```

**Key debugging configurations available:**
- Pre-configured debugging setups for some TauDEM tools. For the remaining TauDEM tools, there is a generic debugging setup (`Build Generic Target (Debug) - Windows`/`Build Generic Target (Debug, Clang) - macOS`) that would prompt the user to provide the name of the TauDEM tool.

**NOTE**: Input data files needs to be placed in the `test_data/input` folder and output data files will be placed in the `test_data/output` folder. The `test_data` folder needs to be created manually at the root of the project. The subfolders `input` and `output` also need to be created manually. Name the input files as named in the `launch.json` or adjust the input file names in `launch.json`.

**Usage**: First press `cmd+shift+d` (macOS) or `ctrl+shift+d` (Windows/Linux) to open the Debug panel, or click on the **Run and Debug** icon (icon with a bug symbol) in VS Code's activity bar (vertical toolbar on the left side of vscode). Then select a configuration from the debug dropdown and press `F5` to start debugging..

## Building TauDEM from VS Code Command Palette

VS Code provides a convenient Command Palette interface to execute build tasks without using the command line. This section shows how to use the pre-configured tasks for building TauDEM.

### Windows

1. **Open Command Palette**:
   - Press `Ctrl+Shift+P` to open the Command Palette
   - Or use the menu: `View → Command Palette...`

2. **Access Build Tasks**:
   - Type `Tasks: Run Task` and press Enter
   - Or type `task` and select `Tasks: Run Task` from the dropdown

3. **Select a Build Task**:
   Choose from available Windows tasks:
   - **`Create Build Directories - Windows`**: Create necessary build directories if they don't exist
   - **`Build TauDEM (Debug) - Windows`**: Build all TauDEM tools in Debug mode
   - **`Build TauDEM (Release) - Windows`**: Build all TauDEM tools in Release mode
   - **`Build pitremove (Debug) - Windows`**: Build only the pitremove tool
   - **`Build d8flowdir (Debug) - Windows`**: Build only the d8flowdir tool
   - **`Build Generic Target (Debug) - Windows`**: Build any specific target (prompts for target name)
   - **`Clean Build - Windows`**: Remove all build artifacts

4. **Example Workflow**:
   ```
   Ctrl+Shift+P → type "Tasks: Run Task" → select "Build TauDEM (Debug) - Windows"
   ```

### macOS

1. **Open Command Palette**:
   - Press `Cmd+Shift+P` to open the Command Palette
   - Or use the menu: `View → Command Palette...`

2. **Access Build Tasks**:
   - Type `Tasks: Run Task` and press Enter
   - Or type `task` and select `Tasks: Run Task` from the dropdown

3. **Select a Build Task**:
   Choose from available macOS tasks:   
   - **`Build TauDEM (Debug, Clang) - macOS`**: Build all TauDEM tools using Clang (default) in Debug mode
   - **`Build TauDEM (Release, Clang) - macOS`**: Build all TauDEM tools in Release mode
   - **`Build pitremove (Debug, Clang) - macOS`**: Build only the pitremove tool in Debug mode
   - **`Build d8flowdir (Debug, Clang) - macOS`**: Build only the d8flowdir tool in Debug mode
   - **`Build Generic Target (Debug, Clang) - macOS`**: Build any specific target (prompts for target name)
   - **`Install TauDEM - macOS`**: Install TauDEM to `$HOME/local/taudem`
   - **`Clean Build - macOS`**: Remove all build artifacts

4. **Example Workflow**:
   ```
   Cmd+Shift+P → type "Tasks: Run Task" → select "Build TauDEM (Debug, Clang) - macOS"
   ```

**💡 Tips for VS Code Command Palette**:
- Tasks are executed in VS Code's integrated terminal
- Build progress and errors are displayed in real-time
- Use `Ctrl+`` (Windows) or `Cmd+`` (macOS) to show/hide the terminal panel
- Failed builds will show error messages with clickable file locations
- The default build task can be run quickly with `Ctrl+Shift+B` (Windows) or `Cmd+Shift+B` (macOS)

## 🔨 Building TauDEM from Command Line

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
# Release build (optimized) all targets
build-windows.bat release

# Debug build (with debugging symbols) all targets
build-windows.bat

# Release build (optimized) specific target
build-windows.bat release pitremove

# Debug build (with debugging symbols) specific target
build-windows.bat debug pitremove
```

### Build Options

#### Using Make (Linux/macOS)

```bash
# Debug build - all targets
make debug COMPILER=clang    # macOS with Clang
make debug COMPILER=linux    # Linux with GCC

# Release build - all targets
make release COMPILER=clang  # macOS with Clang
make release COMPILER=linux  # Linux with GCC

# Alternative compilers (macOS) - all targets
make release COMPILER=macos      # Homebrew GCC
make release COMPILER=gcc-apple  # Apple GCC

# Debug build - specific target
make debug COMPILER=clang TARGET=pitremove

# Release build - specific target
make release COMPILER=clang TARGET=pitremove

# Delete build directories
make clean
```

#### Using CMake Directly

```bash
cd src
mkdir build && cd build
cmake .. -C ../../config.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```
#### Build Directories

TauDEM uses separate build directories for Debug and Release builds. The build directories are:
- `src/build-debug` for Debug builds
- `src/build-release` for Release builds

#### Docker Build (Building/Testing TauDEM for Linux)

```bash
# Build the Docker image for testing (from the root of the project)
docker build -f Dockerfile-run.tests -t taudem-linux-run-tests .

# Run the Docker container with volume mounting
docker run --rm -it -v $(pwd):/app taudem-linux-run-tests

# Inside the container - clean and build
make clean
make dk-release COMPILER=linux
make dk-install PREFIX=/usr/local

# Run tests in Docker (as user taudem-docker)
su - taudem-docker
cd /app
make dk-run-tests
exit

# Exit the container
exit
```

### Installation

```bash
# Install to default location (/usr/local/taudem)
make install

# Install to custom location
make install PREFIX=/custom/path

# Uninstall
make uninstall
```

## 📦 Dependencies

TauDEM requires the following dependencies:

### Core Dependencies

- **C++17 Compiler**: GCC 7+, Clang 5+, or Visual Studio 2022
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
│   └── tasks-*.json.template       # Platform-specific build and utility tasks
├── config.cmake            # CMake configuration for different platforms
├── Makefile                # Main build system
├── build-windows.bat       # Windows build script
├── Dockerfile              # Docker configuration (running TauDEM in docker container)
├── Dockerfile-run.tests    # Docker configuration (building TauDEM for testing on Ubuntu)
├── README.md               # This file
└── README.txt              # Original documentation (TODO: delete this file)
```

### Key Components

- **`src/`**: Contains all C++ source code for TauDEM algorithms
- **`pyfiles/`**: Python wrappers and ArcGIS toolbox integration
- **`config.cmake`**: Platform-specific CMake configurations
- **`.vscode/`**: VS Code development environment setup

## 🧪 Testing

TauDEM includes comprehensive testing capabilities:

### Running Tests

For running tests for TauDEM Linux build, see section [Docker Build](#docker-build-buildingtesting-taudem-for-linux) for Docker build instructions.

```bash
# Manual testing with sample data (assumes TauDEM installed path is in PATH)
cd /path/to/test/data
pitremove -z input.tif -fel output.tif
# Testing in parallel
mpiexec -n 2 pitremove -z input.tif -fel output.tif
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

We welcome contributions to TauDEM! TODO: Add more details here.

### Development Workflow

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/new-algorithm`
3. **Setup development environment**: Follow the VS Code setup instructions
4. **Make changes**: Follow [coding standards](#coding-standards)
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
