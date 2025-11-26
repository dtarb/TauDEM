#!/bin/bash
# Patch Makefile to use conda environment paths for MPI

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <conda-prefix-path>"
    exit 1
fi

CONDA_PREFIX="$1"
MAKEFILE="Makefile"

echo "Patching Makefile for conda environment at: $CONDA_PREFIX"

# Create a temporary file
TMP_FILE=$(mktemp)

# Replace the linux compiler section with conda-aware paths
awk -v conda_prefix="$CONDA_PREFIX" '
/^\$\(if \$\(filter linux,\$1\),/ {
    print "$(if $(filter linux,$1),\\"
    print "\t-DCMAKE_C_COMPILER=/usr/bin/gcc \\"
    print "\t-DCMAKE_CXX_COMPILER=/usr/bin/g++ \\"
    print "\t-DMPI_C_COMPILER=" conda_prefix "/bin/mpicc \\"
    print "\t-DMPI_CXX_COMPILER=" conda_prefix "/bin/mpicxx \\"
    print "\t-DMPI_C_LIBRARIES=" conda_prefix "/lib/libmpi.so \\"
    print "\t-DMPI_CXX_LIBRARIES=" conda_prefix "/lib/libmpi.so \\"
    print "\t-DMPI_C_INCLUDE_PATH=" conda_prefix "/include \\"
    print "\t-DMPI_CXX_INCLUDE_PATH=" conda_prefix "/include \\"
    print "\t-DMPI_CXX_FOUND=TRUE,\\"
    # Skip the original lines until we hit the next $(if
    in_linux_block = 1
    next
}

# Skip lines that are part of the linux block
in_linux_block && /^\$\(if \$\(filter macos,/ {
    in_linux_block = 0
}

# Print lines that are not part of the linux block
!in_linux_block { print }
' "$MAKEFILE" > "$TMP_FILE"

# Replace the original file
mv "$TMP_FILE" "$MAKEFILE"

echo "Makefile patched successfully"
