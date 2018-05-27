# Install gcc-7 compiler from repository.  Has to be run sudo.
# This was not run as a script, instead these were done by hand

add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt update
apt install -y gcc-7 g++-7

# To set these as the default compilers (code from Tony https://github.com/Castronova/docker-image-build/blob/master/cuahsi/singleuser/install-taudem.sh) 
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-7 --slave /usr/bin/gcc-nm gcc-nm /usr/bin/gcc-nm-7 --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-7
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 10

# Instructions from Tony that could be used to revert or toggle between alternatives
#sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-7 --slave /usr/bin/gcc-nm gcc-nm /usr/bin/gcc-nm-7 --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-7
#
#sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 60 --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-5 --slave /usr/bin/gcc-nm gcc-nm /usr/bin/gcc-nm-5 --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-5
#
#sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 10
#sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 10
#
#Now, gcc-7 is the default compiler. To change back to gcc-5, you need to run :
#sudo update-alternatives --config gcc
#Then select gcc-5.
# e.g. 
# echo 1 | update-alternatives --config gcc
# echo 2 | update-alternatives --config gcc
