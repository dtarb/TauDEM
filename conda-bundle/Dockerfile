# Dockerfile.conda - Creates a portable conda bundle of TauDEM
# Output: taudem-conda-linux64.tar.gz containing TauDEM executables and runtime dependencies
# Usage: User extracts in custom conda env and copies binaries/libs to $PREFIX

# --- Builder Stage ---
# Use a specific version for reproducibility
FROM --platform=linux/amd64 continuumio/miniconda3:4.12.0 AS builder

LABEL maintainer="TauDEM Development Team"
LABEL description="Optimized builder for TauDEM conda bundle"

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /build

# 1. Install system dependencies first (rarely change)
RUN apt-get update && apt-get install -y --no-install-recommends \
    sudo \
    build-essential \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# 2. Install mamba and create build environment (cached unless dependencies change)
# We do this BEFORE copying the full source code to leverage caching
RUN conda install -n base -c conda-forge mamba && \
    mamba create -n taudem-build -c conda-forge -y \
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
    patchelf && \
    conda clean -afy

# Set shell to use the conda environment
SHELL ["conda", "run", "-n", "taudem-build", "/bin/bash", "-c"]

# 3. Setup compiler symlinks
RUN sudo ln -sf $(which gcc) /usr/bin/gcc && \
    sudo ln -sf $(which g++) /usr/bin/g++ && \
    sudo ln -sf $(which mpicc) /usr/bin/mpicc && \
    sudo ln -sf $(which mpicxx) /usr/bin/mpicxx

# 4. Copy only the patch script first (rarely changes, better cache reuse)
COPY conda-bundle/patch-makefile.sh /build/

# 5. Copy the rest of the source code (changes most frequently)
COPY . /build/taudem
WORKDIR /build/taudem

# 6. Build and install
RUN CONDA_PREFIX=$(conda info --base)/envs/taudem-build && \
    bash /build/patch-makefile.sh "$CONDA_PREFIX" && \
    make release COMPILER=linux PREFIX=/opt/taudem-runtime && \
    make install PREFIX=/opt/taudem-runtime

# 7. Gather bundle components into a single staging area
# This ensures version consistency between build and runtime
RUN mkdir -p /opt/taudem-bundle/bin /opt/taudem-bundle/lib /opt/taudem-bundle/share && \
    # Copy TauDEM binaries
    cp -Pr /opt/taudem-runtime/taudem/* /opt/taudem-bundle/bin/ && \
    # Copy OpenMPI executables (mpiexec, mpirun, mpicc, mpicxx, etc.)
    # Use find with explicit patterns to catch all MPI-related executables
    find /opt/conda/envs/taudem-build/bin -maxdepth 1 -type f \( -name 'mpi*' -o -name 'ompi*' -o -name 'prte*' \) | \
      xargs -I {} cp -Pr {} /opt/taudem-bundle/bin/ 2>/dev/null || true && \
    # Create mpiexec symlink to mpirun if mpirun exists but mpiexec doesn't
    if [ -f /opt/taudem-bundle/bin/mpirun ] && [ ! -f /opt/taudem-bundle/bin/mpiexec ]; then \
      ln -s mpirun /opt/taudem-bundle/bin/mpiexec; \
    fi && \
    # Create prterun symlink to prte if prte exists but prterun doesn't
    if [ -f /opt/taudem-bundle/bin/prte ] && [ ! -f /opt/taudem-bundle/bin/prterun ]; then \
      ln -s prte /opt/taudem-bundle/bin/prterun; \
    fi && \
    # Copy only essential libraries from the conda environment
    # Exclude: build tools (cmake, gcc, etc.) and unnecessary subdirectories
    # INCLUDE: OpenMPI libraries (required for TauDEM binaries)
    # INCLUDE: libstdc++, libgcc_s (C++ runtime libraries - required for GDAL, PROJ, etc.)
    find /opt/conda/envs/taudem-build/lib -maxdepth 1 \( -type f -o -type l \) | \
      grep -v -E '(libasan|libubsan|libtsan|libgomp|libgfortran|libbfd|libopcodes)' | \
      xargs -I {} cp -Pr {} /opt/taudem-bundle/lib/ 2>/dev/null || true && \
    # Copy only essential library subdirectories (pkgconfig, gdalplugins, openmpi, pmix, etc.)
    # Exclude: build tools, dev files, GUI libs, and unnecessary dependencies
    for dir in /opt/conda/envs/taudem-build/lib/*/; do \
      dirname=$(basename "$dir"); \
      if ! echo "$dirname" | grep -qE '(cmake|gcc|icu|krb5|itcl|ibacm|bfd-plugins|python|tcl|tk|ucc|ucx|udev|libibverbs|libnl|rsocket|systemd|thread|tdbc)'; then \
        cp -Pr "$dir" /opt/taudem-bundle/lib/ 2>/dev/null || true; \
      fi; \
    done && \
    # Remove CUDA-related OpenMPI plugins (they require libcuda.so.1 which is not available on CPU-only systems)
    rm -f /opt/taudem-bundle/lib/openmpi/mca_accelerator_cuda.so 2>/dev/null || true && \
    rm -f /opt/taudem-bundle/lib/openmpi/mca_btl_smcuda.so 2>/dev/null || true && \
    rm -f /opt/taudem-bundle/lib/openmpi/mca_rcache_rgpusm.so 2>/dev/null || true && \
    rm -f /opt/taudem-bundle/lib/openmpi/mca_rcache_gpusm.so 2>/dev/null || true && \
    # Copy data files: GDAL, PROJ, OpenMPI, and PMIX
    cp -Pr /opt/conda/envs/taudem-build/share/gdal /opt/taudem-bundle/share/ 2>/dev/null || true && \
    cp -Pr /opt/conda/envs/taudem-build/share/proj /opt/taudem-bundle/share/ 2>/dev/null || true && \
    cp -Pr /opt/conda/envs/taudem-build/share/openmpi /opt/taudem-bundle/share/ 2>/dev/null || true && \
    cp -Pr /opt/conda/envs/taudem-build/share/pmix /opt/taudem-bundle/share/ 2>/dev/null || true && \
    cp -Pr /opt/conda/envs/taudem-build/share/prte /opt/taudem-bundle/share/ 2>/dev/null || true && \
    # Patch RPATH so binaries find libs in the bundle
    # We do this here where patchelf is already installed
    echo "Patching RPATH for binaries..." && \
    for exe in /opt/taudem-bundle/bin/*; do \
      if [ -x "$exe" ] && [ -f "$exe" ]; then \
        patchelf --force-rpath --set-rpath '$ORIGIN/../lib:$ORIGIN/../lib/openmpi' "$exe" 2>/dev/null || echo "Warning: Failed to patch $exe"; \
      fi; \
    done && \
    # Also patch libraries themselves so they can find each other
    echo "Patching RPATH for libraries..." && \
    find /opt/taudem-bundle/lib -type f -name '*.so*' ! -type l | while read lib; do \
      patchelf --force-rpath --set-rpath '$ORIGIN:$ORIGIN/openmpi' "$lib" 2>/dev/null || true; \
    done

# --- Runtime Stage ---
FROM --platform=linux/amd64 continuumio/miniconda3:4.12.0 AS runtime

LABEL description="Minimal runtime for TauDEM conda bundle"

WORKDIR /bundle

# 1. Copy the pre-gathered bundle from builder
# This avoids version mismatches by using the exact same libraries used for building
COPY --from=builder /opt/taudem-bundle/ /bundle/taudem-conda-linux64/

# 2. Copy scripts and finalize
COPY conda-bundle/install.sh conda-bundle/README.md /bundle/taudem-conda-linux64/
RUN chmod +x /bundle/taudem-conda-linux64/install.sh && \
    # Create the final bundle archive
    tar -czf taudem-conda-linux64.tar.gz taudem-conda-linux64

# Ensure the bundle is readable by everyone
RUN chmod 644 /bundle/taudem-conda-linux64.tar.gz

VOLUME ["/output"]

# Copy the bundle to the mounted volume
CMD ["sh", "-c", "cp /bundle/taudem-conda-linux64.tar.gz /output/ && echo 'Bundle created: /output/taudem-conda-linux64.tar.gz'"]
