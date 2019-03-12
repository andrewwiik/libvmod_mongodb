#!/bin/bash

yum install -y cmake3 openssl-devel cyrus-sasl-devel wget varnish-plus varnish-plus-devel --nogpgcheck 
ln -s /usr/bin/cmake3 /bin/cmake
yum group install -y "Development Tools" --nogpgcheck 
yum install -y glib* --nogpgcheck 
yum install -y python-docutils --nogpgcheck 
wget https://github.com/mongodb/mongo-c-driver/releases/download/1.13.1/mongo-c-driver-1.13.1.tar.gz
tar xzf mongo-c-driver-1.13.1.tar.gz
cd mongo-c-driver-1.13.1
mkdir cmake-build
cd cmake-build
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
make
make install
ldconfig