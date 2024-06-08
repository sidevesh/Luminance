Name:           com.sidevesh.Luminance
Version:        1.0.3
Release:        1%{?dist}
Summary:        A simple GTK application to control brightness of displays including external displays supporting DDC/CI

License:        GPL-3.0
URL:            https://github.com/sidevesh/Luminance
BuildArch:      x86_64

# Define the BuildRequires and Requires packages
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gtk4)

Requires:       gtk3
Requires:       ddcutil

%description
Luminance is a simple GTK application that allows you to control the brightness of displays, including external displays supporting DDC/CI.

%prep
# No source archive, no unpacking required

%build
# No build steps are required

%install
# Install the binary
install -D -m 755 %{_sourcedir}/com.sidevesh.Luminance %{buildroot}/%{_bindir}/com.sidevesh.Luminance

# Install the desktop file
install -D -m 644 %{_sourcedir}/com.sidevesh.Luminance.desktop %{buildroot}/%{_datadir}/applications/com.sidevesh.Luminance.desktop

# Install the GSettings schema
install -D -m 644 %{_sourcedir}/com.sidevesh.Luminance.gschema.xml %{buildroot}/%{_datadir}/glib-2.0/schemas/com.sidevesh.Luminance.gschema.xml

# Install the udev rules
install -D -m 644 %{_sourcedir}/44-backlight-permissions.rules %{buildroot}/%{_libdir}/udev/rules.d/44-backlight-permissions.rules

# Install the icons
install -D -m 644 %{_sourcedir}/com.sidevesh.Luminance.svg %{buildroot}/%{_datadir}/icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg
install -D -m 644 %{_sourcedir}/com.sidevesh.Luminance-symbolic.svg %{buildroot}/%{_datadir}/icons/hicolor/symbolic/apps/com.sidevesh.Luminance-symbolic.svg

%files
%{_bindir}/com.sidevesh.Luminance
%{_datadir}/applications/com.sidevesh.Luminance.desktop
%{_datadir}/glib-2.0/schemas/com.sidevesh.Luminance.gschema.xml
%{_libdir}/udev/rules.d/44-backlight-permissions.rules
%{_datadir}/icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg
%{_datadir}/icons/hicolor/symbolic/apps/com.sidevesh.Luminance-symbolic.svg
