#!/bin/sh

gcc ddcbc-api.c `pkg-config --cflags --libs glib-2.0 ddcutil` -o ddcbc-api