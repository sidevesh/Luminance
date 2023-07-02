#!/bin/bash

echo "Switching to the 'arch' directory..."
cd arch

echo "Building the package..."
makepkg --syncdeps --noconfirm --pkgdir=../build

echo "Returning to the root directory..."
cd ..
