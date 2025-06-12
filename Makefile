UNAME_S := $(shell uname -s)

# Default build type
BUILD_TYPE ?= Debug

# Directories
SRC_DIR = src
BUILD_DIR_DEBUG = $(SRC_DIR)/build-debug
BUILD_DIR_RELEASE = $(SRC_DIR)/build-release
CONFIG_FILE = config.cmake
INSTALL_DIR = $(shell sed -n 's/.*CMAKE_INSTALL_PREFIX "\([^"]*\)".*/\1/p' $(CONFIG_FILE))

# Build commands
.PHONY: all debug release dk-debug dk-release dk-install dk-run-tests clean install uninstall help

# Default target
all: debug

# Set default installation directory if not found in CONFIG_FILE
ifeq ($(INSTALL_DIR),)
	INSTALL_DIR = /usr/local
endif

# Function to check compiler and set options
check_compiler = $(if $(COMPILER),\
	$(if $(filter $(COMPILER),linux macos clang gcc-apple),\
		$(eval CMAKE_GENERATOR="Unix Makefiles")\
		$(eval CMAKE_COMPILER_OPTIONS=$(call get_compiler_options,$(COMPILER))),\
		$(error Unknown compiler specified. Use COMPILER=linux, macos, clang, gcc-apple)\
	),\
	$(error COMPILER is required for build targets. Use COMPILER=linux, macos, clang, gcc-apple)\
)

# Function to get compiler options
define get_compiler_options
$(if $(filter linux,$1),\
	-DCMAKE_C_COMPILER=/usr/bin/gcc \
	-DCMAKE_CXX_COMPILER=/usr/bin/g++ \
	-DMPI_C_COMPILER=/usr/bin/mpicc \
	-DMPI_CXX_COMPILER=/usr/bin/mpicxx \
	-DMPI_C_LIBRARIES=/usr/lib/aarch64-linux-gnu/libmpi.so \
	-DMPI_CXX_LIBRARIES=/usr/lib/aarch64-linux-gnu/libmpi.so \
	-DMPI_C_INCLUDE_PATH=/usr/lib/aarch64-linux-gnu/openmpi/include \
	-DMPI_CXX_INCLUDE_PATH=/usr/lib/aarch64-linux-gnu/openmpi/include \
	-DMPI_CXX_FOUND=TRUE,\
$(if $(filter macos,$1),-DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-15 -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-15 \
	-DMPI_C_COMPILER=/opt/homebrew/opt/open-mpi/bin/mpicc \
	-DMPI_CXX_COMPILER=/opt/homebrew/opt/open-mpi/bin/mpicxx \
	-DMPI_C_LIBRARIES=/opt/homebrew/opt/open-mpi/lib/libmpi.dylib \
	-DMPI_CXX_LIBRARIES=/opt/homebrew/opt/open-mpi/lib/libmpi.dylib \
	-DMPI_C_INCLUDE_PATH=/opt/homebrew/opt/open-mpi/include \
	-DMPI_CXX_INCLUDE_PATH=/opt/homebrew/opt/open-mpi/include \
	-DMPI_CXX_FOUND=TRUE,\
$(if $(filter clang,$1),-DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++,\
$(if $(filter gcc-apple,$1),-DCMAKE_C_COMPILER=/opt/homebrew/bin/aarch64-apple-darwin24-gcc-15 -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/aarch64-apple-darwin24-g++-15 \
	-DMPI_C_COMPILER=/opt/homebrew/opt/open-mpi/bin/mpicc \
	-DMPI_CXX_COMPILER=/opt/homebrew/opt/open-mpi/bin/mpicxx \
	-DMPI_C_LIBRARIES=/opt/homebrew/opt/open-mpi/lib/libmpi.dylib \
	-DMPI_CXX_LIBRARIES=/opt/homebrew/opt/open-mpi/lib/libmpi.dylib \
	-DMPI_C_INCLUDE_PATH=/opt/homebrew/opt/open-mpi/include \
	-DMPI_CXX_INCLUDE_PATH=/opt/homebrew/opt/open-mpi/include \
	-DMPI_CXX_FOUND=TRUE,))))
endef

# Function to check OS compatibility
check_os_compatibility = $(if $(COMPILER),\
	$(if $(and $(filter linux,$(COMPILER)),$(filter Darwin,$(UNAME_S))),\
		$(error Linux compiler is not suitable on macOS. Use COMPILER=clang, macos, or gcc-apple),\
	$(if $(and $(filter macos clang gcc-apple,$(COMPILER)),$(filter Linux,$(UNAME_S))),\
		$(error macOS compilers are not suitable on Linux. Use COMPILER=linux))),\
	$(error COMPILER is required for build targets. Use COMPILER=linux, macos, clang, gcc-apple)\
)

# Build targets
debug:
	$(warning COMPILER is $(COMPILER))
	$(warning UNAME_S is $(UNAME_S))
	$(call check_os_compatibility)
	$(call check_compiler)
	@echo "Building in Debug mode with $(COMPILER) compiler..."
	@mkdir -p $(BUILD_DIR_DEBUG)
	@cd $(BUILD_DIR_DEBUG) && cmake .. -C ../../$(CONFIG_FILE) -DCMAKE_BUILD_TYPE=Debug $(CMAKE_COMPILER_OPTIONS) -G $(CMAKE_GENERATOR)
	$(if $(TARGET),\
		@echo "Building specific target: $(TARGET)" && cd $(BUILD_DIR_DEBUG) && make $(TARGET),\
		@echo "Building all targets" && cd $(BUILD_DIR_DEBUG) && make\
	)

release:
	$(warning COMPILER is $(COMPILER))
	$(warning UNAME_S is $(UNAME_S))
	$(call check_os_compatibility)
	$(call check_compiler)
	@echo "Building in Release mode with $(COMPILER) compiler..."
	@mkdir -p $(BUILD_DIR_RELEASE)
	@cd $(BUILD_DIR_RELEASE) && cmake .. -C ../../$(CONFIG_FILE) -DCMAKE_BUILD_TYPE=Release $(CMAKE_COMPILER_OPTIONS) -G $(CMAKE_GENERATOR)
	$(if $(TARGET),\
		@echo "Building specific target: $(TARGET)" && cd $(BUILD_DIR_RELEASE) && make $(TARGET),\
		@echo "Building all targets" && cd $(BUILD_DIR_RELEASE) && make\
	)

dk-debug:
	$(if $(filter Linux,$(UNAME_S)),,$(error dk-debug is only supported on Linux))
	$(warning COMPILER is $(COMPILER))
	$(warning UNAME_S is $(UNAME_S))
	$(call check_compiler)
	@echo "Building in Debug mode with $(COMPILER) compiler..."
	@mkdir -p $(BUILD_DIR_DEBUG)
	@cd $(SRC_DIR) && cmake -S . -B build-debug
	$(if $(TARGET),\
		@echo "Building specific target: $(TARGET)" && cd $(BUILD_DIR_DEBUG) && make $(TARGET),\
		@echo "Building all targets" && cd $(BUILD_DIR_DEBUG) && make\
	)

dk-release:
	$(if $(filter Linux,$(UNAME_S)),,$(error dk-release is only supported on Linux))
	$(warning COMPILER is $(COMPILER))
	$(warning UNAME_S is $(UNAME_S))
	$(call check_compiler)
	@echo "Building in Release mode with $(COMPILER) compiler..."
	@mkdir -p $(BUILD_DIR_RELEASE)
	@cd $(SRC_DIR) && cmake -S . -B build-release
	$(if $(TARGET),\
		@echo "Building specific target: $(TARGET)" && cd $(BUILD_DIR_RELEASE) && make $(TARGET),\
		@echo "Building all targets" && cd $(BUILD_DIR_RELEASE) && make\
	)

dk-install:
	$(if $(filter Linux,$(UNAME_S)),,$(error dk-install is only supported on Linux))
	@echo "Installing TauDEM to $(or $(PREFIX),/usr/local)..."
	@if [ ! -d "$(BUILD_DIR_RELEASE)" ]; then \
		echo "Error: Build directory not found. Please run 'make dk-release' first."; \
		exit 1; \
	fi
	@cd $(BUILD_DIR_RELEASE) && cmake . -DCMAKE_INSTALL_PREFIX=$(or $(PREFIX),/usr/local)
	@cd $(BUILD_DIR_RELEASE) && make install
	@echo "=== Validating installation ==="
	@test -f "$(or $(PREFIX),/usr/local)/taudem/pitremove" || (echo "ERROR: pitremove not installed" && exit 1)
	@echo "SUCCESS: TauDEM installation validated"

dk-run-tests:
	@echo "Running tests..."
	@cd /home/taudem-docker/workspace/TauDEM-Test-Data/Input && \
	./taudem-tests.sh

clean:
	@echo "Cleaning build directories..."
	@rm -rf $(BUILD_DIR_DEBUG) $(BUILD_DIR_RELEASE)

install:	
	@echo "Installing TauDEM to $(or $(PREFIX),/usr/local/taudem)..."
	@if [ ! -d "$(BUILD_DIR_RELEASE)" ]; then \
		echo "Error: Release build directory not found. Please run 'make release' first."; \
		exit 1; \
	fi
	@if [ -n "$(PREFIX)" ]; then \
		cd $(BUILD_DIR_RELEASE) && cmake . -DCMAKE_INSTALL_PREFIX=$(PREFIX); \
	fi
	@cd $(BUILD_DIR_RELEASE) && make install

uninstall:
	@echo "Uninstalling TauDEM..."
	@rm -rf $(INSTALL_DIR)

help:
	@echo "Makefile usage:"
	@echo "  make debug COMPILER=<compiler>    - Build in Debug mode with specified compiler (linux, macos, clang, gcc-apple)"
	@echo "  make debug COMPILER=<compiler> TARGET=<target> - Build specific target in Debug mode with specified compiler (linux, macos, clang, gcc-apple)"
	@echo "  make release COMPILER=<compiler>  - Build in Release mode with specified compiler (linux, macos, clang, gcc-apple)"
	@echo "  make release COMPILER=<compiler> TARGET=<target> - Build specific target in Release mode with specified compiler (linux, macos, clang, gcc-apple)"
	@echo "  make dk-debug COMPILER=<compiler> - Build in Debug mode for Docker (Linux only)"
	@echo "  make dk-debug COMPILER=<compiler> TARGET=<target> - Build specific target in Debug mode for Docker (Linux only)"
	@echo "  make dk-release COMPILER=<compiler> - Build in Release mode for Docker (Linux only)"
	@echo "  make dk-release COMPILER=<compiler> TARGET=<target> - Build specific target in Release mode for Docker (Linux only)"
	@echo "  make dk-install [PREFIX=<path>]   - Install TauDEM to specified directory (default: /usr/local, Linux only)"
	@echo "  make dk-run-tests                 - Run TauDEM tests in Docker environment"
	@echo "  make clean                        - Clean build directories"
	@echo "  make install [PREFIX=<path>]      - Install TauDEM (default: /usr/local)"
	@echo "  make uninstall                    - Remove TauDEM installation"
	@echo "  make help                         - Show this help message"
	@echo ""
	@echo "Note: For Windows builds, use build-windows.bat instead"