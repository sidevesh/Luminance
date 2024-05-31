#!/bin/bash

echo "Building the app..."
./build.sh

echo "Creating 'build/debian-package-files' directory..."
if [ -d "build/debian-package-files" ]; then
  rm -rf ./build/debian-package-files
fi
mkdir -p ./build/debian-package-files
mkdir -p ./build/debian-package-files/DEBIAN
mkdir -p ./build/debian-package-files/usr/bin
mkdir -p ./build/debian-package-files/usr/share/applications
mkdir -p ./build/debian-package-files/usr/share/glib-2.0/schemas
mkdir -p ./build/debian-package-files/usr/lib/udev/rules.d
mkdir -p ./build/debian-package-files/usr/share/icons/hicolor/scalable/apps

echo "Copying files to the package structure..."
cp ./debian/control ./build/debian-package-files/DEBIAN/control
cp ./build/app ./build/debian-package-files/usr/bin/com.sidevesh.Luminance
cp ./install_files/com.sidevesh.Luminance.desktop ./build/debian-package-files/usr/share/applications/com.sidevesh.Luminance.desktop
cp ./install_files/com.sidevesh.Luminance.gschema.xml ./build/debian-package-files/usr/share/glib-2.0/schemas/com.sidevesh.Luminance.gschema.xml
cp ./install_files/44-backlight-permissions.rules ./build/debian-package-files/usr/lib/udev/rules.d/44-backlight-permissions.rules
cp ./icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg ./build/debian-package-files/usr/share/icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg
cp ./icons/hicolor/symbolic/apps/com.sidevesh.Luminance-symbolic.svg ./build/debian-package-files/usr/share/icons/hicolor/symbolic/apps/com.sidevesh.Luminance-symbolic.svg

echo "Creating Debian package..."
dpkg-deb --build ./build/debian-package-files ./build/luminance.deb

echo "DEB package built successfully"
