#!/bin/bash
gcc -pthread `pkg-config --cflags gtk4 libadwaita-1` -o build/app src/main.c `pkg-config --libs gtk4 libadwaita-1` -l ddcutil
