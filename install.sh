#!/bin/bash

# This script builds and installs Luminance for all users. Root permissions are needed

if [ `whoami` != "root" ]
then
    echo "Not root. Exiting..."
    exit
fi

echo "Building com.sidevesh.Luminance..."
./build.sh
echo "Copying com.sidevesh.Luminance to /usr/bin..."
cp ./build/com.sidevesh.Luminance /usr/bin/com.sidevesh.Luminance
echo "Copying desktop file to /usr/share/applications..."
cp ./install_files/com.sidevesh.Luminance.desktop /usr/share/applications/com.sidevesh.Luminance.desktop
echo "Done!"
