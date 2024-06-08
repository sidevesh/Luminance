#!/bin/bash
gcc -g -pthread `pkg-config --cflags gtk4` -o build/app.debug src/main.c `pkg-config --libs gtk4` -l ddcutil
