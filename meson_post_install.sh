#!/bin/sh

if [ -z $DESTDIR ]; then
  glib-compile-schemas $MESON_INSTALL_PREFIX/share/glib-2.0/schemas
  gtk-update-icon-cache -qtf $MESON_INSTALL_PREFIX/share/icons/hicolor
fi
