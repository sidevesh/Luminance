#!/bin/bash

# This script builds an RPM package for Luminance

echo "Building the app..."
build.sh

echo "Creating 'build/rpm-package-files' directory..."
if [ -d "build/rpm-package-files" ]; then
  rm -rf ./build/rpm-package-files
fi
mkdir -p ./build/rpm-package-files/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

echo "Copying files to the package structure..."
cp ./build/app ./build/rpm-package-files/rpmbuild/SOURCES/com.sidevesh.Luminance
cp ./rpm/com.sidevesh.Luminance.spec ./build/rpm-package-files/rpmbuild/SPECS/com.sidevesh.Luminance.spec
cp ./install_files/com.sidevesh.Luminance.desktop ./build/rpm-package-files/rpmbuild/SOURCES/com.sidevesh.Luminance.desktop
cp ./install_files/com.sidevesh.Luminance.gschema.xml ./build/rpm-package-files/rpmbuild/SOURCES/com.sidevesh.Luminance.gschema.xml
cp ./install_files/44-backlight-permissions.rules ./build/rpm-package-files/rpmbuild/SOURCES/44-backlight-permissions.rules
cp ./icons/icon.svg ./build/rpm-package-files/rpmbuild/SOURCES/com.sidevesh.Luminance.svg

echo "Switching to the 'rpm-package-files' directory..."
cd ./build/rpm-package-files

echo "Creating RPM package..."
rpmbuild -bb ./rpmbuild/SPECS/com.sidevesh.Luminance.spec

echo "Moving the RPM package to the root build directory..."
mv ./rpmbuild/RPMS/*/*.rpm ../../luminance.rpm

echo "Returning to the root directory..."
cd ../..

echo "RPM package built successfully"
