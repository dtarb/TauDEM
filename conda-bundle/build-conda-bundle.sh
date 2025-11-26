#!/bin/bash
# Build script for TauDEM conda bundle
# Creates a portable Linux x86_64 bundle with all dependencies

set -e  # Exit on error

echo "========================================"
echo "Building TauDEM Conda Bundle"
echo "========================================"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$REPO_ROOT/conda-bundle-output"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

echo "Script directory: $SCRIPT_DIR"
echo "Repository root: $REPO_ROOT"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Build Docker image
echo "Building Docker image..."
cd "$REPO_ROOT"
docker build --platform=linux/amd64 -f conda-bundle/Dockerfile -t taudem-conda-builder .

echo ""
echo "Extracting bundle from Docker image..."
# Run container and copy the bundle
docker run --rm -v "$OUTPUT_DIR:/output" taudem-conda-builder

echo ""
echo "========================================"
echo "Build Complete!"
echo "========================================"
echo "Bundle location: $OUTPUT_DIR/taudem-conda-linux64.tar.gz"
echo "Bundle size: $(du -h "$OUTPUT_DIR/taudem-conda-linux64.tar.gz" | cut -f1)"
echo ""
echo "To test the bundle:"
echo "  1. Extract: tar -xzf $OUTPUT_DIR/taudem-conda-linux64.tar.gz"
echo "  2. Follow instructions in taudem-conda-linux64/README.md"
echo "========================================"
