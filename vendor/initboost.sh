#!/bin/sh
if [ ! -d "boost" ]; then
    if [ ! -f "boost.tar.gz" ]; then
        wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz -O boost.tar.gz
    fi
    tar xvf boost.tar.gz
    mv boost_1_66_0 boost
fi
cd boost
if [ ! -f "b2" ]; then
./bootstrap.sh
fi
./b2 headers
./b2 release link=static -j4