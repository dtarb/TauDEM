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
cp -v "$BUNDLE_DIR/bin/"* "$CONDA_PREFIX/bin/"
chmod +x "$CONDA_PREFIX/bin/"*

# ------------------------------------------------------------------
# Install runtime libraries
# ------------------------------------------------------------------
echo ""
echo "Installing runtime libraries..."
mkdir -p "$CONDA_PREFIX/lib"
cp -v "$BUNDLE_DIR/lib/"* "$CONDA_PREFIX/lib/"

# ------------------------------------------------------------------
# Install shared data (GDAL / PROJ)
# ------------------------------------------------------------------
if [ -d "$BUNDLE_DIR/share" ]; then
    echo ""
    echo "Installing shared data files..."
    mkdir -p "$CONDA_PREFIX/share"
    cp -rv "$BUNDLE_DIR/share/"* "$CONDA_PREFIX/share/"
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
echo "IMPORTANT: You must install OpenMPI separately if not already present:"
echo "  conda install -c conda-forge openmpi"
echo ""
echo "Test with:"
echo "  mpiexec -n 4 pitremove -z input.tif -fel output.tif"
echo ""
echo "If MPI fails, verify:"
echo "  ldd \$(which pitremove) | grep libmpi"
echo ""
echo "See README.md for usage examples and documentation."
echo ""
