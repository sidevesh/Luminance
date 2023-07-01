#!/bin/bash
gcc -pthread `pkg-config --cflags gtk+-3.0` -o build/app src/main.c `pkg-config --libs gtk+-3.0` -l ddcutil
