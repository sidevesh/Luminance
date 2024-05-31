#!/bin/bash

# This script builds an RPM package for Luminance

echo "Building the app..."
./build.sh

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
cp ./icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg ./build/rpm-package-files/rpmbuild/SOURCES/com.sidevesh.Luminance.svg
cp ./icons/hicolor/symbolic/apps/com.sidevesh.Luminance-symbolic.svg ./build/rpm-package-files/rpmbuild/SOURCES/com.sidevesh.Luminance-symbolic.svg

echo "Creating RPM package..."
rpmbuild --define "_topdir `pwd`/build/rpm-package-files/rpmbuild" -bb ./build/rpm-package-files/rpmbuild/SPECS/com.sidevesh.Luminance.spec

echo "Moving the RPM package to the root build directory..."
cp ./build/rpm-package-files/rpmbuild/RPMS/*/*.rpm ./build/luminance.rpm

echo "RPM package built successfully"
