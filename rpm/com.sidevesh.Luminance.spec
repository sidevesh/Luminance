Name:           com.sidevesh.Luminance
Version:        1.0.1
Release:        1%{?dist}
Summary:        A simple GTK application to control brightness of displays including external displays supporting DDC/CI

License:        GPL-3.0
URL:            https://github.com/sidevesh/Luminance
BuildArch:      x86_64

# Define the BuildRequires and Requires packages
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gtk+-3.0)

Requires:       libgtk-3-0
Requires:       ddcutil

%description
Luminance is a simple GTK application that allows you to control the brightness of displays, including external displays supporting DDC/CI.

%prep
%setup -q

%build
# No build steps are required

%install
# Install the binary
install -D -m 755 com.sidevesh.Luminance %{buildroot}/%{_bindir}/com.sidevesh.Luminance

# Install the desktop file
install -D -m 644 com.sidevesh.Luminance.desktop %{buildroot}/%{_datadir}/applications/com.sidevesh.Luminance.desktop

# Install the GSettings schema
install -D -m 644 com.sidevesh.Luminance.gschema.xml %{buildroot}/%{_datadir}/glib-2.0/schemas/com.sidevesh.Luminance.gschema.xml

# Install the udev rules
install -D -m 644 44-backlight-permissions.rules %{buildroot}/%{_libdir}/udev/rules.d/44-backlight-permissions.rules

# Install the icon
install -D -m 644 com.sidevesh.Luminance.svg %{buildroot}/%{_datadir}/icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg

%files
%{_bindir}/com.sidevesh.Luminance
%{_datadir}/applications/com.sidevesh.Luminance.desktop
%{_datadir}/glib-2.0/schemas/com.sidevesh.Luminance.gschema.xml
%{_libdir}/udev/rules.d/44-backlight-permissions.rules
%{_datadir}/icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg
