# TauDEM Conda Bundle Builder

This directory contains the tools to create a portable conda bundle of TauDEM for Linux x86_64.

## Contents

- `Dockerfile` - Multi-stage Docker build that compiles TauDEM and packages it with runtime dependencies
- `build-conda-bundle.sh` - Script to build the Docker image and create the bundle
- `patch-makefile.sh` - Patches the Makefile to use conda environment MPI paths
- `install.sh` - Installation script included in the bundle (for end users)
- `README.md` - User-facing documentation included in the bundle

## Building the Bundle

To create the portable conda bundle:

```bash
./build-conda-bundle.sh
```

This will:

1. Build a Docker image with TauDEM compiled from source
2. Bundle all runtime dependencies (OpenMPI, GDAL, PROJ, SQLite, etc.)
3. Create `taudem-conda-linux64.tar.gz` in `../conda-bundle-output/`

Build time: ~5-10 minutes depending on your system.

## Output

The build produces:

- `../conda-bundle-output/taudem-conda-linux64.tar.gz` (~122MB) - The portable bundle with all dependencies

## Bundle Structure

The resulting tar.gz contains:

```text
taudem-conda-linux64/
├── bin/              # TauDEM executables (37 tools)
├── lib/              # Runtime shared libraries (MPI, GDAL, PROJ, etc.)
├── share/            # GDAL and PROJ data files
├── install.sh        # Installation script
└── README.md         # User documentation
```

## Distribution

Users can extract and install the bundle in any conda environment without requiring system-wide installation or admin privileges:

```bash
# Create/activate conda environment
conda create -n taudem python=3.11
conda activate taudem

# Extract and install
tar -xzf taudem-conda-linux64.tar.gz
cd taudem-conda-linux64
./install.sh
```

All dependencies are bundled - no additional conda packages needed!

## Testing in JupyterHub

To test the bundle in a cloud-based JupyterHub environment:

1. **Upload the bundle**: Upload `taudem-conda-linux64.tar.gz` to your JupyterHub workspace

2. **Open a terminal** in JupyterHub

3. **Create a conda environment**:

   ```bash
   # Create a named environment
   conda create -n taudem python=3.11
   conda activate taudem

   # Or use a local prefix if you have permission issues
   conda create --prefix ~/.local/taudem-env python=3.11
   conda activate ~/.local/taudem-env
   ```

4. **Install the bundle**:

   ```bash
   # Extract the bundle
   tar -xzf taudem-conda-linux64.tar.gz
   cd taudem-conda-linux64

   # Run the installer
   ./install.sh
   ```

5. **Test a TauDEM tool**:

   ```bash
   # Check if tools are available
   which pitremove

   # Run a simple command to verify
   pitremove
   ```

6. **Test MPI** (if your JupyterHub supports it):

   ```bash
   mpiexec
   ```

**Note**: The bundle is compiled for Linux x86_64. For ARM64 systems, you'll need to modify the Dockerfile to remove the `--platform=linux/amd64` flag.

## Requirements for Building

- Docker installed and running
- ~2-3 GB disk space for build process
- Internet connection to download base images and dependencies
- Can be built on any platform (macOS, Linux, Windows with Docker)
- Uses platform emulation if building x86_64 on ARM64 host (e.g., Apple Silicon)

## Architecture Notes

- The Dockerfile builds for **Linux x86_64** architecture using `--platform=linux/amd64`
- This is suitable for most cloud platforms, servers, and JupyterHub instances
- Can be built on any host platform (macOS, Linux, Windows) using Docker's platform emulation

## Technical Details

- Executables are **dynamically linked** to included libraries
- Libraries included: GDAL 3.11, PROJ, SQLite, libspatialite, OpenMPI, PMIX, PRTE, and dependencies
- **OpenMPI, PMIX, and PRTE are fully bundled** - no additional conda install needed
- All MPI components (mpirun, mpiexec, prte, prterun) are included with help files and configuration
- CUDA-related plugins are excluded to avoid warnings on CPU-only systems
- No Python TauDEM tools (pyfiles) are included - executables only
- Users do not need admin/root privileges to install
- The `install.sh` script copies binaries and libraries to the active conda environment
- Binaries are RPATH-patched to find libraries in the conda environment's lib directory
- Environment variables (OPAL_PREFIX, PRTE_PREFIX, PMIX_PREFIX) are set during conda activation
