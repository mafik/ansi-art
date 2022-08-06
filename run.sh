#!/bin/bash

g++ -pthread -std=c++2a -I. example.cc maf/*.cc `pkg-config --cflags --libs freetype2` -o example
./example
