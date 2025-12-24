# This Dockerfile builds and installs TauDEM using a multi-stage build for optimization

# --- Builder Stage ---
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary dependencies to compile TauDEM
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    libopenmpi-dev \
    libgdal-dev \
    libproj-dev \
    libtiff-dev \
    libgeotiff-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /src

# Copy source code (leveraging .dockerignore to exclude unnecessary files)
COPY . .

# Build and install TauDEM to a temporary directory
# We use PREFIX=/install so it installs to /install/taudem
RUN make release COMPILER=linux && \
    make install PREFIX=/install && \
    echo "=== Validating build ===" && \
    test -f /install/taudem/pitremove || (echo "ERROR: pitremove not found" && exit 1)

# --- Runtime Stage ---
FROM ubuntu:22.04

LABEL maintainer="TauDEM Developers"
LABEL description="Parallel C++ suite for terrain analysis using Digital Elevation Models"

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install only runtime dependencies
# libgdal30 is the runtime for GDAL on Ubuntu 22.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    openmpi-bin \
    libgdal30 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Copy the installed TauDEM binaries from the builder stage
COPY --from=builder /install/taudem /usr/local/taudem

# Set the PATH environment variable globally to include taudem directory
ENV PATH="/usr/local/taudem:${PATH}"

# Create a non-root user for security
RUN useradd -m -s /bin/bash taudem
USER taudem
WORKDIR /home/taudem

# Set the default command to run an interactive shell
CMD ["/bin/bash"]
