#!/bin/bash
gcc -g -pthread `pkg-config --cflags gtk4 libadwaita-1` -o build/app.debug src/main.c `pkg-config --libs gtk4 libadwaita-1` -l ddcutil
