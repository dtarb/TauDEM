# TauDEM Portable Conda Bundle

This package contains TauDEM (Terrain Analysis Using Digital Elevation Models) executables and all required runtime dependencies bundled together for easy installation in any conda environment.

## What's Included

- All TauDEM command-line tools (pitremove, d8flowdir, aread8, etc.)
- Runtime libraries (OpenMPI, GDAL, PROJ, SQLite, libspatialite, etc.)
- Environment configuration scripts
- Installation script

## System Requirements

- Linux x86_64 system
- Conda or Miniconda installed
- Python 3.11+ (for conda environment)

## Installation

1. **Create a fresh conda environment (recommended):**

   ```bash
   conda create -n taudem python=3.11
   conda activate taudem
   ```

   **If you encounter permission errors** (common in shared JupyterHub environments), use a local prefix instead:

   ```bash
   conda create --prefix ~/.local/taudem-env python=3.11
   conda activate ~/.local/taudem-env
   ```

2. **(Optional) Create a Jupyter kernel for running TauDEM in notebooks:**

   If you want to run TauDEM commands from Jupyter notebooks, install `ipykernel` and register the environment as a Jupyter kernel:

   ```bash
   # Make sure your taudem environment is active
   conda install -n taudem ipykernel
   
   # Register the kernel (for named environment)
   python -m ipykernel install --user --name=taudem --display-name="TauDEM"
   
   # Or for prefix-based environment
   conda install --prefix ~/.local/taudem-env ipykernel
   conda activate ~/.local/taudem-env
   python -m ipykernel install --user --name=taudem --display-name="TauDEM"
   ```

   After this, you'll see "TauDEM" as an available kernel when creating or opening notebooks in JupyterHub.

3. Extract the archive:

   ```bash
   tar -xzf taudem-conda-linux64.tar.gz
   cd taudem-conda-linux64
   ```

4. Run the installation script:

   ```bash
   ./install.sh
   ```

   This will:
   - Copy TauDEM executables to your conda environment's bin directory
   - Copy all runtime libraries to your conda environment's lib directory
   - Set up necessary environment variables in your conda activate.d scripts
   - Configure library paths

   **All dependencies are bundled - no additional conda packages needed!**

5. Restart your shell or run:

   ```bash
   conda deactivate
   conda activate taudem
   OR
   conda activate ~/.local/taudem-env
   ```

## Usage

After installation, all TauDEM tools are available in your PATH:

```bash
# Remove pits from DEM
mpiexec -n 8 pitremove -z dem.tif -fel demfel.tif

# Calculate D8 flow directions
mpiexec -n 8 d8flowdir -fel demfel.tif -p flowdir.tif -sd8 slope.tif

# Calculate contributing area
mpiexec -n 8 aread8 -p flowdir.tif -ad8 area.tif
```

See the [TauDEM documentation](http://hydrology.usu.edu/taudem/taudem5/documentation.html) for complete usage information.

## Bundle Contents

```
taudem-conda-linux64/
├── bin/              # TauDEM executable binaries
├── lib/              # Runtime shared libraries
├── share/            # GDAL and PROJ data files
├── install.sh        # Installation script
└── README.md         # This file
```

## Uninstallation

To remove TauDEM, simply delete the conda environment:

```bash
# Deactivate if currently active
conda deactivate

# Remove named environment
conda env remove -n taudem

# Or remove prefix-based environment
conda env remove --prefix ~/.local/taudem-env
```

To reinstall TauDEM later, follow the installation instructions again.

## Troubleshooting

### GDAL version mismatch warning

If you see `gdalinfo --version` showing a different version than 3.12.0, this is usually harmless. It typically means `gdalinfo` is coming from another conda environment or system installation in your PATH (common in JupyterHub environments).

**To verify TauDEM is using the correct GDAL libraries:**

```bash
ldd $(which pitremove) | grep libgdal
```

You should see it points to your taudem environment: `libgdal.so.38 => /path/to/your/taudem/lib/libgdal.so.38`

**If TauDEM is using the wrong GDAL libraries** (different path), then create a fresh environment:

```bash
conda deactivate
conda env remove -n taudem
conda create -n taudem python=3.11
conda activate taudem
# Then reinstall TauDEM bundle
```

### Command not found after installation

Make sure you've reactivated your conda environment:

```bash
conda deactivate
conda activate taudem
```

### Library errors when running TauDEM tools

Check that the environment variables are set:

```bash
echo $LD_LIBRARY_PATH
echo $GDAL_DATA
echo $PROJ_LIB
```

These should include paths to your conda environment.

### MPI errors

This bundle uses OpenMPI. If you have other MPI implementations installed system-wide, there may be conflicts. The bundled OpenMPI libraries should take precedence due to the LD_LIBRARY_PATH configuration.

## License

TauDEM is distributed under the GNU General Public License v3.0. See the main TauDEM repository for complete license information.

## Support

For TauDEM issues and documentation:

- Website: http://hydrology.usu.edu/taudem/
- GitHub: https://github.com/dtarb/TauDEM

For bundle-specific issues, please contact the bundle maintainer or file an issue on the TauDEM GitHub repository.
