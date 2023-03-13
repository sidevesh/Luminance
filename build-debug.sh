#!/bin/sh
gcc -g -pthread `pkg-config --cflags gtk+-3.0` -o build/com.sidevesh.Luminance.debug src/main.c `pkg-config --libs gtk+-3.0` -l ddcutil
