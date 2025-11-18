#!/bin/bash
# TauDEM Conda Bundle Installer
# Installs TauDEM executables and runtime dependencies into the active conda environment

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if conda environment is active
if [ -z "$CONDA_PREFIX" ]; then
    echo -e "${RED}Error: No conda environment is active.${NC}"
    echo "Please activate a conda environment first:"
    echo "  conda create -n taudem python=3.9"
    echo "  conda activate taudem"
    exit 1
fi

echo -e "${GREEN}Installing TauDEM to: $CONDA_PREFIX${NC}"
echo ""

# Check for existing GDAL installation that might conflict
if command -v gdalinfo &> /dev/null; then
    EXISTING_GDAL=$(gdalinfo --version 2>&1 | grep -oP 'GDAL \K[0-9]+\.[0-9]+\.[0-9]+' || echo "unknown")
    BUNDLE_GDAL="3.12.0"
    if [ "$EXISTING_GDAL" != "$BUNDLE_GDAL" ]; then
        echo -e "${YELLOW}Note: Found GDAL $EXISTING_GDAL in PATH (bundle contains $BUNDLE_GDAL)${NC}"
        echo "This is usually harmless (e.g., from JupyterHub notebook environment)."
        echo "TauDEM will use the bundled GDAL $BUNDLE_GDAL libraries via LD_LIBRARY_PATH."
        echo ""
        echo "After installation, you can verify with:"
        echo "  ldd \$(which pitremove) | grep libgdal"
        echo ""
    fi
fi

# Get the directory where this script is located
BUNDLE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check if bundle directories exist
if [ ! -d "$BUNDLE_DIR/bin" ] || [ ! -d "$BUNDLE_DIR/lib" ]; then
    echo -e "${RED}Error: Bundle directories not found.${NC}"
    echo "Expected to find bin/ and lib/ directories in: $BUNDLE_DIR"
    exit 1
fi

# Copy executables
echo "Installing TauDEM executables..."
cp -v "$BUNDLE_DIR"/bin/* "$CONDA_PREFIX/bin/"
chmod +x "$CONDA_PREFIX"/bin/*

# Copy libraries
echo ""
echo "Installing runtime libraries..."
mkdir -p "$CONDA_PREFIX/lib"
cp -v "$BUNDLE_DIR"/lib/* "$CONDA_PREFIX/lib/"

# Copy data files
if [ -d "$BUNDLE_DIR/share" ]; then
    echo ""
    echo "Installing data files..."
    mkdir -p "$CONDA_PREFIX/share"
    cp -rv "$BUNDLE_DIR"/share/* "$CONDA_PREFIX/share/"
fi

# Set up environment activation scripts
echo ""
echo "Configuring environment variables..."

mkdir -p "$CONDA_PREFIX/etc/conda/activate.d"
mkdir -p "$CONDA_PREFIX/etc/conda/deactivate.d"

# Create activation script
cat > "$CONDA_PREFIX/etc/conda/activate.d/taudem_env.sh" << 'EOF'
#!/bin/bash
# TauDEM environment activation script

# Save original values
export TAUDEM_OLD_LD_LIBRARY_PATH="$LD_LIBRARY_PATH"
export TAUDEM_OLD_GDAL_DATA="$GDAL_DATA"
export TAUDEM_OLD_PROJ_LIB="$PROJ_LIB"
export TAUDEM_OLD_OPAL_PREFIX="$OPAL_PREFIX"
export TAUDEM_OLD_PMIX_PREFIX="$PMIX_PREFIX"
export TAUDEM_OLD_PRTE_PREFIX="$PRTE_PREFIX"

# Set up library paths
export LD_LIBRARY_PATH="$CONDA_PREFIX/lib:$LD_LIBRARY_PATH"

# Set up GDAL and PROJ data paths
if [ -d "$CONDA_PREFIX/share/gdal" ]; then
    export GDAL_DATA="$CONDA_PREFIX/share/gdal"
fi

if [ -d "$CONDA_PREFIX/share/proj" ]; then
    export PROJ_LIB="$CONDA_PREFIX/share/proj"
fi

# Set up OpenMPI prefix (tells MPI where to find help files, etc.)
export OPAL_PREFIX="$CONDA_PREFIX"

# Set PMIx and PRTE prefix explicitly
export PMIX_PREFIX="$CONDA_PREFIX"
export PRTE_PREFIX="$CONDA_PREFIX"

# Configure GDAL to suppress AddBand() error and other warnings
# CPL_DEBUG=OFF suppresses debug messages
# GDAL_PAM_ENABLED=NO prevents auxiliary metadata files
export CPL_DEBUG="OFF"
export GDAL_PAM_ENABLED="NO"
export GDAL_DRIVER_PATH="$CONDA_PREFIX/lib/gdalplugins"
EOF

# Create deactivation script
cat > "$CONDA_PREFIX/etc/conda/deactivate.d/taudem_env.sh" << 'EOF'
#!/bin/bash
# TauDEM environment deactivation script

# Restore original values
if [ -n "$TAUDEM_OLD_LD_LIBRARY_PATH" ]; then
    export LD_LIBRARY_PATH="$TAUDEM_OLD_LD_LIBRARY_PATH"
    unset TAUDEM_OLD_LD_LIBRARY_PATH
else
    unset LD_LIBRARY_PATH
fi

if [ -n "$TAUDEM_OLD_GDAL_DATA" ]; then
    export GDAL_DATA="$TAUDEM_OLD_GDAL_DATA"
    unset TAUDEM_OLD_GDAL_DATA
else
    unset GDAL_DATA
fi

if [ -n "$TAUDEM_OLD_PROJ_LIB" ]; then
    export PROJ_LIB="$TAUDEM_OLD_PROJ_LIB"
    unset TAUDEM_OLD_PROJ_LIB
else
    unset PROJ_LIB
fi

if [ -n "$TAUDEM_OLD_OPAL_PREFIX" ]; then
    export OPAL_PREFIX="$TAUDEM_OLD_OPAL_PREFIX"
    unset TAUDEM_OLD_OPAL_PREFIX
else
    unset OPAL_PREFIX
fi

if [ -n "$TAUDEM_OLD_PMIX_PREFIX" ]; then
    export PMIX_PREFIX="$TAUDEM_OLD_PMIX_PREFIX"
    unset TAUDEM_OLD_PMIX_PREFIX
else
    unset PMIX_PREFIX
fi

if [ -n "$TAUDEM_OLD_PRTE_PREFIX" ]; then
    export PRTE_PREFIX="$TAUDEM_OLD_PRTE_PREFIX"
    unset TAUDEM_OLD_PRTE_PREFIX
else
    unset PRTE_PREFIX
fi
EOF

chmod +x "$CONDA_PREFIX/etc/conda/activate.d/taudem_env.sh"
chmod +x "$CONDA_PREFIX/etc/conda/deactivate.d/taudem_env.sh"

echo ""
echo -e "${GREEN}Installation complete!${NC}"
echo ""
echo "To activate the environment variables, run:"
echo "  conda deactivate"
echo "  conda activate $(basename $CONDA_PREFIX)"
echo ""
echo "Then you can run TauDEM tools, for example:"
echo "  mpiexec -n 4 pitremove -z input.tif -fel output.tif"
echo ""
echo "See README.md for usage examples and documentation."
