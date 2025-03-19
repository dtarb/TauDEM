# Use Ubuntu as the base image
FROM ubuntu:22.04 

# Install necessary dependencies
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
    && rm -rf /var/lib/apt/lists/*  # Clean up to reduce image size

# Set the working directory inside the container
WORKDIR /app

# Set the default command to run an interactive shell
CMD ["/bin/bash"]