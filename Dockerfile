FROM ubuntu:18.04

RUN apt update
RUN apt install -y git python3-gdal gdal-bin libgdal-dev python3-pip python3-numpy python3-scipy libspatialindex-dev python python-gdal python-pip mpich
RUN apt auto-remove

RUN pip3 install numpy scipy scikit-learn pandas geopandas tqdm numba gdal
RUN pip install netcdf4 tqdm

RUN mkdir /projects /data

RUN git clone https://github.com/dtarb/TauDEM.git /projects/TauDEM

RUN mkdir /projects/TauDEM/bin

RUN cd /projects/TauDEM/src && make
