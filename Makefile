UNAME_S := $(shell uname -s)

# Default build type
BUILD_TYPE ?= Debug

# Directories
SRC_DIR = src
BUILD_DIR = $(SRC_DIR)/build
CONFIG_FILE = config.cmake
INSTALL_DIR = $(shell sed -n 's/.*CMAKE_INSTALL_PREFIX "\([^"]*\)".*/\1/p' $(CONFIG_FILE))

# Build commands
.PHONY: all debug release clean install uninstall help

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
$(if $(filter macos,$1),-DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-14 -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-14 \
	-DMPI_C_COMPILER=/opt/homebrew/opt/open-mpi/bin/mpicc \
	-DMPI_CXX_COMPILER=/opt/homebrew/opt/open-mpi/bin/mpicxx \
	-DMPI_C_LIBRARIES=/opt/homebrew/opt/open-mpi/lib/libmpi.dylib \
	-DMPI_CXX_LIBRARIES=/opt/homebrew/opt/open-mpi/lib/libmpi.dylib \
	-DMPI_C_INCLUDE_PATH=/opt/homebrew/opt/open-mpi/include \
	-DMPI_CXX_INCLUDE_PATH=/opt/homebrew/opt/open-mpi/include \
	-DMPI_CXX_FOUND=TRUE,\
$(if $(filter clang,$1),-DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++,\
$(if $(filter gcc-apple,$1),-DCMAKE_C_COMPILER=/opt/homebrew/bin/aarch64-apple-darwin24-gcc-14 -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/aarch64-apple-darwin24-g++-14 \
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
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -C ../../$(CONFIG_FILE) -DCMAKE_BUILD_TYPE=Debug $(CMAKE_COMPILER_OPTIONS) -G $(CMAKE_GENERATOR)
	@cd $(BUILD_DIR) && make

release:
	$(warning COMPILER is $(COMPILER))
	$(warning UNAME_S is $(UNAME_S))
	$(call check_os_compatibility)
	$(call check_compiler)
	@echo "Building in Release mode with $(COMPILER) compiler..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -C ../../$(CONFIG_FILE) -DCMAKE_BUILD_TYPE=Release $(CMAKE_COMPILER_OPTIONS) -G $(CMAKE_GENERATOR)
	@cd $(BUILD_DIR) && make

dk-release:
	$(if $(filter Linux,$(UNAME_S)),,$(error dk-release is only supported on Linux))
	$(warning COMPILER is $(COMPILER))
	$(warning UNAME_S is $(UNAME_S))
	$(call check_compiler)
	@echo "Building in Release mode with $(COMPILER) compiler..."
	@mkdir -p $(BUILD_DIR)
	@cd $(SRC_DIR) && cmake -S . -B build
	@cd $(SRC_DIR)/build && make

dk-run-tests:
	@echo "Running tests..."
	@cd /home/taudem-docker/workspace/TauDEM-Test-Data/Input && \
	./taudem-tests.sh

clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

install:	
	@echo "Installing TauDEM..."
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		echo "Error: Build directory not found. Please run 'make release' first."; \
		exit 1; \
	fi
	@cd $(BUILD_DIR) && make install

uninstall:
	@echo "Uninstalling TauDEM..."
	@rm -rf $(INSTALL_DIR)

help:
	@echo "Makefile usage:"
	@echo "  make debug COMPILER=<compiler>    - Build in Debug mode with specified compiler (linux, macos, clang, gcc-apple)"
	@echo "  make release COMPILER=<compiler>  - Build in Release mode with specified compiler (linux, macos, clang, gcc-apple)"
	@echo "  make dk-release                   - Build in Release mode for Docker (Linux only)"
	@echo "  make dk-run-tests                 - Run TauDEM tests in Docker environment"
	@echo "  make clean                        - Clean build directory"
	@echo "  make install                      - Install TauDEM"
	@echo "  make uninstall                    - Remove TauDEM installation"
	@echo "  make help                         - Show this help message"
	@echo ""
	@echo "Note: For Windows builds, use build-windows.bat instead"