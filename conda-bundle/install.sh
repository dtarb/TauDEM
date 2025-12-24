#!/bin/bash
# TauDEM Conda Bundle Installer
# Installs TauDEM executables and runtime dependencies into the active conda environment
# Assumes TauDEM binaries are RPATH-patched to find libmpi and other libs

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# ------------------------------------------------------------------
# Verify conda environment
# ------------------------------------------------------------------
if [ -z "$CONDA_PREFIX" ]; then
    echo -e "${RED}Error: No conda environment is active.${NC}"
    echo "Please activate a conda environment first:"
    echo "  conda create -n taudem python=3.9"
    echo "  conda activate taudem"
    exit 1
fi

echo -e "${GREEN}Installing TauDEM into: $CONDA_PREFIX${NC}"
echo ""

# ------------------------------------------------------------------
# Locate bundle
# ------------------------------------------------------------------
BUNDLE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ ! -d "$BUNDLE_DIR/bin" ] || [ ! -d "$BUNDLE_DIR/lib" ]; then
    echo -e "${RED}Error: Bundle directories not found.${NC}"
    echo "Expected bin/ and lib/ under: $BUNDLE_DIR"
    exit 1
fi

# ------------------------------------------------------------------
# Install binaries
# ------------------------------------------------------------------
echo "Installing TauDEM executables..."
cp -Pr "$BUNDLE_DIR/bin/"* "$CONDA_PREFIX/bin/"
chmod +x "$CONDA_PREFIX/bin/"*

# ------------------------------------------------------------------
# Install runtime libraries
# ------------------------------------------------------------------
echo ""
echo "Installing runtime libraries..."
mkdir -p "$CONDA_PREFIX/lib"
cp -Pr "$BUNDLE_DIR/lib/"* "$CONDA_PREFIX/lib/"

# ------------------------------------------------------------------
# Install shared data (GDAL / PROJ)
# ------------------------------------------------------------------
if [ -d "$BUNDLE_DIR/share" ]; then
    echo ""
    echo "Installing shared data files..."
    mkdir -p "$CONDA_PREFIX/share"
    cp -Pr "$BUNDLE_DIR/share/"* "$CONDA_PREFIX/share/"
fi

# ------------------------------------------------------------------
# Conda activation hook (SAFE ONLY)
# ------------------------------------------------------------------
echo ""
echo "Configuring conda activation environment..."

mkdir -p "$CONDA_PREFIX/etc/conda/activate.d"

cat > "$CONDA_PREFIX/etc/conda/activate.d/taudem_env.sh" << 'EOF'
#!/bin/bash
# TauDEM environment activation (MPI-safe)

# GDAL data
if [ -d "$CONDA_PREFIX/share/gdal" ]; then
    export GDAL_DATA="$CONDA_PREFIX/share/gdal"
fi

# PROJ data
if [ -d "$CONDA_PREFIX/share/proj" ]; then
    export PROJ_LIB="$CONDA_PREFIX/share/proj"
fi

# OpenMPI data (help files, etc.)
# OPAL_PREFIX tells OpenMPI to look inside this conda environment for its
# runtime help files and configuration data, ensuring bundled mpiexec/mpirun
# behave correctly without relying on system-wide OpenMPI installs.
if [ -d "$CONDA_PREFIX/share/openmpi" ]; then
    export OPAL_PREFIX="$CONDA_PREFIX"
fi

# PRTE (PMIx Reference RunTime Environment) is part of the modern OpenMPI
# runtime stack. Setting PRTE_PREFIX ensures PRTE can locate its data/help
# files within this conda environment.
if [ -d "$CONDA_PREFIX/share/prte" ]; then
    export PRTE_PREFIX="$CONDA_PREFIX"
fi

# PMIx (Process Management Interface for Exascale) is used by modern OpenMPI
# for process management; PMIX_PREFIX ensures PMIx/OpenMPI can find their
# runtime data in this conda environment.
if [ -d "$CONDA_PREFIX/share/pmix" ]; then
    export PMIX_PREFIX="$CONDA_PREFIX"
fi

# GDAL plugins
export GDAL_DRIVER_PATH="$CONDA_PREFIX/lib/gdalplugins"

# Reduce noise / side effects
export CPL_DEBUG="OFF"
export GDAL_PAM_ENABLED="NO"
EOF

chmod +x "$CONDA_PREFIX/etc/conda/activate.d/taudem_env.sh"

# ------------------------------------------------------------------
# Final notes
# ------------------------------------------------------------------
echo ""
echo -e "${GREEN}TauDEM installation complete.${NC}"
echo ""
echo "Next steps:"
echo "  1. conda deactivate"
echo "  2. conda activate \"$CONDA_PREFIX\""
echo ""
echo "Test TauDEM:"
echo "  pitremove -h"
echo ""
echo "Test with MPI (parallel processing):"
echo "  mpiexec -n 4 pitremove -z input.tif -fel output.tif"
echo ""
echo "Note: OpenMPI is included in this bundle and configured to work with"
echo "the TauDEM binaries. The binaries use RPATH to locate all required"
echo "libraries within the conda environment."
echo ""
echo "See README.md for usage examples and documentation."
echo ""
