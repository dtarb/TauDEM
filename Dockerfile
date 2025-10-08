# This Dockerfile builds and installs TauDEM on Ubuntu Linux

# Using Ubuntu as the base image
FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary dependencies to compile TauDEM
RUN apt update && apt install -y \
    build-essential \
    cmake \
    openmpi-bin \
    libopenmpi-dev \
    gdal-bin \
    libgdal-dev \
    libproj-dev \
    libtiff-dev \
    libgeotiff-dev \
    git \
    nano \
    sudo \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*  # Clean up to reduce image size

# Create a non-root user
RUN useradd -m -s /bin/bash taudem

# Set the password for the taudem user
RUN echo "taudem:taudem" | chpasswd

# Grant the taudem user sudo privileges without password
RUN usermod -aG sudo taudem && \
    echo "taudem ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Create workspace directory
RUN mkdir -p /home/taudem/workspace && \
    chown -R taudem:taudem /home/taudem/workspace

# Switch to taudem user
USER taudem
WORKDIR /home/taudem/workspace

# Clone the TauDEM repository (clones the Develop branch)
RUN git clone --depth 1 https://github.com/dtarb/TauDEM.git

# Set working directory to TauDEM
WORKDIR /home/taudem/workspace/TauDEM

# Build and install TauDEM
RUN make clean && \
    make release COMPILER=linux && \
    sudo make install PREFIX=/usr/local && \
    echo "=== Validating installation ===" && \
    test -f /usr/local/taudem/pitremove || (echo "ERROR: pitremove not found in /usr/local/taudem" && exit 1) && \
    echo "SUCCESS: TauDEM installation validated - pitremove found"

# Set up environment variables - add /usr/local/taudem to PATH
RUN echo "export PATH=/usr/local/taudem:\$PATH" >> /home/taudem/.bashrc

# Switch back to root for final setup
USER root

# Set the PATH environment variable globally to include taudem directory
ENV PATH="/usr/local/taudem:${PATH}"

# Set the working directory
WORKDIR /home/taudem/workspace/TauDEM

# Set the default command to run an interactive shell
CMD ["/bin/bash"]
