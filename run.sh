#!/bin/bash

g++ -std=c++20 -I. example.cc maf/*.cc `pkg-config --cflags --libs freetype2` -o example
./example
