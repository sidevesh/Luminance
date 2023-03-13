#!/bin/sh
gcc -pthread `pkg-config --cflags gtk+-3.0` -o build/com.sidevesh.Luminance src/main.c `pkg-config --libs gtk+-3.0` -l ddcutil
