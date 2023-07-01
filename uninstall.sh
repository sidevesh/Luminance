#!/bin/bash

# This script uninstall Luminance for all users. Root permissions are needed

if [ `whoami` != "root" ]
then
  echo "Not root. Exiting..."
  exit
fi

if [ ! -f /usr/bin/com.sidevesh.Luminance ]
then
  echo "App not installed. Exiting..."
  exit
fi

echo "Removing app from /usr/bin..."
rm /usr/bin/com.sidevesh.Luminance

echo "Removing desktop file from /usr/share/applications..."
rm /usr/share/applications/com.sidevesh.Luminance.desktop

echo "Removing GSettings schema..."
rm ./install_files/com.sidevesh.Luminance.gschema.xml

echo "Removing udev rules..."
rm /usr/lib/udev/rules.d/44-backlight-permissions.rules

echo "Removing icon..."
rm /usr/share/icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg

echo "Compiling schemas and updating icon cache..."
glib-compile-schemas /usr/share/glib-2.0/schemas/
gtk-update-icon-cache /usr/share/icons/hicolor/

echo "Uninstalled successfully"
