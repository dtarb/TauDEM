# Script to setup MPICH
wget http://www.mpich.org/static/downloads/3.2.1/mpich-3.2.1.tar.gz

# Now following instructions in README
tar xzf mpich-3.2.1.tar.gz
mkdir mpich
cd mpich-3.2.1
./configure --prefix=$HOME/TauDEMDependencies/mpich/mpich-install 2>&1 | tee c.txt
make 2>&1 | tee m.txt
make install 2>&1 | tee mi.txt

# Add the following to .bashrc
PATH=$HOME/TauDEMDependencies/mpich/mpich-install/bin:$PATH ; export PATH