# Dockerfile.conda - Creates a portable conda bundle of TauDEM
# Output: taudem-conda-linux64.tar.gz containing TauDEM executables and runtime dependencies
# Usage: User extracts in custom conda env and copies binaries/libs to $PREFIX

# Use x86_64 architecture for maximum compatibility with cloud environments
FROM --platform=linux/amd64 continuumio/miniconda3:latest

LABEL maintainer="TauDEM Development Team"
LABEL description="Build portable conda bundle for TauDEM with runtime dependencies"

# Set working directory
WORKDIR /build

# Update conda and install mamba for faster dependency resolution
RUN apt-get update && apt-get install -y sudo && \
    conda update -n base -c defaults conda && \
    conda install -n base -c conda-forge mamba

# Create build environment with dependencies
# Pin GDAL version to ensure consistency between build and runtime
# Note: GDAL 3.11 avoids ERROR 6 (AddBand) warnings seen in 3.12+
# These warnings are cosmetic and don't affect output, but 3.11 provides cleaner logs
RUN mamba create -n taudem-build -c conda-forge -y \
    cmake \
    make \
    cxx-compiler \
    openmpi \
    openmpi-mpicc \
    openmpi-mpicxx \
    gdal=3.11 \
    proj \
    sqlite \
    libspatialite \
    libtiff \
    geotiff \
    patchelf

# Activate environment for subsequent RUN commands
SHELL ["conda", "run", "-n", "taudem-build", "/bin/bash", "-c"]

# Create symlinks so Makefile can find compilers at expected paths
RUN sudo ln -sf $(which gcc) /usr/bin/gcc && \
    sudo ln -sf $(which g++) /usr/bin/g++ && \
    sudo ln -sf $(which mpicc) /usr/bin/mpicc && \
    sudo ln -sf $(which mpicxx) /usr/bin/mpicxx

# Copy TauDEM source code and patch script
COPY . /build/taudem
COPY conda-bundle/patch-makefile.sh /build/

# Patch Makefile to use conda MPI paths
WORKDIR /build/taudem
RUN CONDA_PREFIX=$(conda info --base)/envs/taudem-build && \
    bash /build/patch-makefile.sh "$CONDA_PREFIX"

# Build TauDEM using patched Makefile
RUN rm -rf build && \
    make clean && \
    make release COMPILER=linux PREFIX=/opt/taudem-runtime && \
    make install PREFIX=/opt/taudem-runtime

# Create runtime-only conda environment
# Use same GDAL version as build environment
RUN mamba create -n taudem-runtime -c conda-forge -y \
    openmpi \
    gdal=3.11 \
    proj \
    sqlite \
    libspatialite \
    libtiff \
    geotiff

# Package the runtime bundle
WORKDIR /bundle
RUN mkdir -p taudem-conda-linux64/bin && \
    mkdir -p taudem-conda-linux64/lib && \
    mkdir -p taudem-conda-linux64/share

# Copy TauDEM binaries
RUN cp -r /opt/taudem-runtime/taudem/* taudem-conda-linux64/bin/

# Copy TauDEM libraries (if any)
RUN if [ -d /opt/taudem-runtime/lib ]; then \
        cp -r /opt/taudem-runtime/lib/* taudem-conda-linux64/lib/; \
    fi

# Get conda environment path
RUN conda info --base | xargs -I {} echo {}/envs/taudem-runtime > /tmp/conda_prefix

# Copy required runtime libraries from conda environment
RUN CONDA_PREFIX=$(cat /tmp/conda_prefix) && \
    echo "Copying all shared libraries from conda environment..." && \
    # Copy all .so* files (both regular files and symlinks) to ensure we get all dependencies
    find ${CONDA_PREFIX}/lib -maxdepth 1 -name "*.so*" -type f -exec cp -L {} taudem-conda-linux64/lib/ \; 2>/dev/null || true && \
    find ${CONDA_PREFIX}/lib -maxdepth 1 -name "*.so*" -type l -exec cp -L {} taudem-conda-linux64/lib/ \; 2>/dev/null || true && \
    echo "Library copy complete. Total libraries:" && \
    ls taudem-conda-linux64/lib/*.so* 2>/dev/null | wc -l

# Copy GDAL data files
RUN CONDA_PREFIX=$(cat /tmp/conda_prefix) && \
    if [ -d ${CONDA_PREFIX}/share/gdal ]; then \
        cp -r ${CONDA_PREFIX}/share/gdal taudem-conda-linux64/share/; \
    fi

# Copy PROJ data files
RUN CONDA_PREFIX=$(cat /tmp/conda_prefix) && \
    if [ -d ${CONDA_PREFIX}/share/proj ]; then \
        cp -r ${CONDA_PREFIX}/share/proj taudem-conda-linux64/share/; \
    fi

# Copy installation script and README from repository
COPY conda-bundle/install.sh taudem-conda-linux64/
COPY conda-bundle/README.md taudem-conda-linux64/
RUN chmod +x taudem-conda-linux64/install.sh

# Patch RPATH (runtime library search path) for all executables
# TauDEM executables will automatically find their dependencies in ../lib/ relative to their location
RUN for exe in taudem-conda-linux64/bin/*; do \
      if [ -x "$exe" ] && [ -f "$exe" ]; then \
          patchelf --set-rpath '$ORIGIN/../lib' "$exe"; \
      fi; \
    done

# Create tarball
RUN tar -czf taudem-conda-linux64.tar.gz taudem-conda-linux64

# Display build information
RUN echo "======================================" && \
    echo "TauDEM Conda Bundle Build Information" && \
    echo "======================================" && \
    echo "Build Date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')" && \
    echo "Platform: linux-64" && \
    echo "Bundle: taudem-conda-linux64.tar.gz" && \
    echo "Bundle Size: $(du -h taudem-conda-linux64.tar.gz | cut -f1)" && \
    echo "======================================" && \
    conda run -n taudem-runtime mpirun --version | head -n1 && \
    echo "GDAL: $(conda run -n taudem-runtime gdal-config --version)" && \
    conda run -n taudem-runtime proj 2>&1 | head -n1 && \
    echo "======================================"

# Set the output directory as volume
VOLUME ["/output"]

# Copy the bundle to /output when container runs
CMD ["sh", "-c", "cp /bundle/taudem-conda-linux64.tar.gz /output/ && echo 'TauDEM conda bundle created successfully. Check /output/taudem-conda-linux64.tar.gz'"]
