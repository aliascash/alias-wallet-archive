#!/bin/bash

wget -O boost_1_67_0.tar.gz http://sourceforge.net/projects/boost/files/boost/1.67.0/boost_1_67_0.tar.gz/download --no-check-certificate
tar xzvf boost_1_67_0.tar.gz
cd boost_1_67_0/

sudo apt-get update
sudo apt-get install build-essential g++ python-dev autotools-dev libicu-dev libbz2-dev

./bootstrap.sh --prefix=/usr/local
sudo ./b2 --with=all -j 2 install
