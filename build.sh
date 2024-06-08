#!/bin/bash
gcc -pthread `pkg-config --cflags gtk4` -o build/app src/main.c `pkg-config --libs gtk4` -l ddcutil
