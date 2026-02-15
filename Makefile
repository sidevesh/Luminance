.PHONY: all release debug install uninstall install-debug uninstall-debug package-deb package-rpm package-arch package-arch-test install-arch uninstall-arch flatpak flatpak-bundle install-flatpak uninstall-flatpak run-flatpak package-flatpak-flathub lint-flatpak-flathub package-gnome-extension install-gnome-extension uninstall-gnome-extension clean

# Build directories
BUILD_RELEASE_DIR = build/release
BUILD_DEBUG_DIR = build/debug
BUILD_PACKAGING_DIR = build/release-packaging

# Packaging variables
PKG_DIR = build/packaging
DEB_DIR = $(PKG_DIR)/debian
RPM_DIR = $(PKG_DIR)/rpm
RPMBUILD_DIR = $(RPM_DIR)/rpmbuild
OUTPUTS_DIR = $(PKG_DIR)/outputs
FLATPAK_DIR = $(PKG_DIR)/flatpak
FLATPAK_REPO_DIR = $(FLATPAK_DIR)/repo
FLATPAK_MANIFEST_IN = $(CURDIR)/packaging/flatpak/com.sidevesh.Luminance.yml.in
FLATPAK_APP_ID = com.sidevesh.Luminance

VERSION = $(shell cat version.txt)
DESCRIPTION = $(shell cat description.txt)

# Default target is release
all: release

# Release build
release:
	@if [ ! -d "$(BUILD_RELEASE_DIR)" ]; then \
		meson setup $(BUILD_RELEASE_DIR) --buildtype=release; \
	fi
	meson compile -C $(BUILD_RELEASE_DIR)

# Debug build
debug:
	@if [ ! -d "$(BUILD_DEBUG_DIR)" ]; then \
		meson setup $(BUILD_DEBUG_DIR) --buildtype=debug; \
	fi
	meson compile -C $(BUILD_DEBUG_DIR)

# Install release version
install: release
	sudo meson install -C $(BUILD_RELEASE_DIR)
	# Fix permissions of the install log so subsequent user builds don't fail
	-sudo chown $(USER):$(shell id -gn) $(BUILD_RELEASE_DIR)/meson-logs/install-log.txt

# Uninstall release version
uninstall:
	sudo ninja -C $(BUILD_RELEASE_DIR) uninstall

# Install debug version
install-debug: debug
	sudo meson install -C $(BUILD_DEBUG_DIR)
	# Fix permissions of the install log
	-sudo chown $(USER):$(shell id -gn) $(BUILD_DEBUG_DIR)/meson-logs/install-log.txt

# Uninstall debug version
uninstall-debug:
	sudo ninja -C $(BUILD_DEBUG_DIR) uninstall


# Packaging builds (configured with /usr prefix)
packaging-build:
	@if [ ! -d "$(BUILD_PACKAGING_DIR)" ]; then \
		meson setup $(BUILD_PACKAGING_DIR) --buildtype=release --prefix=/usr; \
	fi
	meson compile -C $(BUILD_PACKAGING_DIR)

package-deb: packaging-build
	@echo "Building Debian package (Version: $(VERSION))..."
	rm -rf $(DEB_DIR)
	mkdir -p $(DEB_DIR)/DEBIAN
	mkdir -p $(OUTPUTS_DIR)
	# Generate control file from template
	sed -e "s|@VERSION@|$(VERSION)|g" -e "s|@DESCRIPTION@|$(DESCRIPTION)|g" packaging/debian/control.in > $(DEB_DIR)/DEBIAN/control
	# Install to packaging directory using the packaging build (prefix=/usr)
	meson install -C $(BUILD_PACKAGING_DIR) --destdir $(CURDIR)/$(DEB_DIR)
	dpkg-deb --build $(DEB_DIR) $(OUTPUTS_DIR)/luminance.deb
	@echo "Debian package created at $(OUTPUTS_DIR)/luminance.deb"

package-rpm: packaging-build
	@echo "Building RPM package (Version: $(VERSION))..."
	rm -rf $(RPM_DIR)
	mkdir -p $(RPMBUILD_DIR)/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
	mkdir -p $(OUTPUTS_DIR)
	# Copy binary (from packaging build)
	cp $(BUILD_PACKAGING_DIR)/com.sidevesh.Luminance $(RPMBUILD_DIR)/SOURCES/
	# Copy resources
	# Use generated desktop file from build
	cp $(BUILD_PACKAGING_DIR)/com.sidevesh.Luminance.desktop $(RPMBUILD_DIR)/SOURCES/
	cp $(BUILD_PACKAGING_DIR)/com.sidevesh.Luminance.gschema.xml $(RPMBUILD_DIR)/SOURCES/
	cp $(BUILD_PACKAGING_DIR)/com.sidevesh.Luminance.metainfo.xml $(RPMBUILD_DIR)/SOURCES/
	cp $(BUILD_PACKAGING_DIR)/com.sidevesh.Luminance.service $(RPMBUILD_DIR)/SOURCES/
	cp install_files/44-backlight-permissions.rules $(RPMBUILD_DIR)/SOURCES/
	cp icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg $(RPMBUILD_DIR)/SOURCES/
	cp icons/hicolor/symbolic/apps/com.sidevesh.Luminance-symbolic.svg $(RPMBUILD_DIR)/SOURCES/
	# Generate spec file from template
	sed -e "s|@VERSION@|$(VERSION)|g" -e "s|@DESCRIPTION@|$(DESCRIPTION)|g" packaging/rpm/com.sidevesh.Luminance.spec.in > $(RPMBUILD_DIR)/SPECS/com.sidevesh.Luminance.spec
	# Build
	rpmbuild --define "_topdir $(CURDIR)/$(RPMBUILD_DIR)" -bb $(RPMBUILD_DIR)/SPECS/com.sidevesh.Luminance.spec
	# Copy result
	cp $(RPMBUILD_DIR)/RPMS/*/*.rpm $(OUTPUTS_DIR)/luminance.rpm
	@echo "RPM package created at $(OUTPUTS_DIR)/luminance.rpm"

# Generate arch/PKGBUILD from template
package-arch: packaging/arch/PKGBUILD.in version.txt description.txt
	@echo "Generating arch/PKGBUILD..."
	sed -e "s|@VERSION@|$(VERSION)|g" -e "s|@DESCRIPTION@|$(DESCRIPTION)|g" packaging/arch/PKGBUILD.in > arch/PKGBUILD
	# Update .SRCINFO
	cd arch && makepkg --printsrcinfo > .SRCINFO

# Test Arch package locally
package-arch-test: package-arch
	@echo "Building Arch package from local source..."
	# Create a local PKGBUILD that points to local filesystem instead of git tag
	# Use $$_pkgname:: to force the source directory name to match what prepare() expects (Luminance)
	sed 's|git+https://github.com/sidevesh/[^ "]*|$$_pkgname::git+file://$(CURDIR)|' arch/PKGBUILD > arch/PKGBUILD.local
	cd arch && makepkg -p PKGBUILD.local -f
	rm arch/PKGBUILD.local

install-arch: package-arch-test
	@echo "Installing Arch package..."
	sudo pacman -U arch/luminance-$(VERSION)-*.pkg.tar.zst

uninstall-arch:
	@echo "Uninstalling Arch package..."
	sudo pacman -R luminance

flatpak: $(FLATPAK_MANIFEST_IN)
	@echo "Building Flatpak..."
	mkdir -p $(FLATPAK_DIR)
	# Generate manifest for local build directly to .flatpak-manifest.yml
	sed -e "s|@LUMINANCE_SOURCE_TYPE@|dir|g" \
	    -e "s|@LUMINANCE_SOURCE_DETAILS@|path: $(CURDIR)|g" \
	    $(FLATPAK_MANIFEST_IN) > $(CURDIR)/.flatpak-manifest.yml
	
	# Run flathub-build from root. This creates 'repo' and 'builddir' in $(CURDIR)
	flatpak run --filesystem=$(CURDIR) --command=flathub-build org.flatpak.Builder --force-clean $(CURDIR)/.flatpak-manifest.yml
	
	# Move artifacts to clean up root
	rm -rf $(FLATPAK_DIR)/repo $(FLATPAK_DIR)/builddir
	mv repo $(FLATPAK_DIR)/
	mv builddir $(FLATPAK_DIR)/
	rm $(CURDIR)/.flatpak-manifest.yml

flatpak-bundle: flatpak
	@echo "Creating Flatpak bundle..."
	mkdir -p $(OUTPUTS_DIR)
	flatpak build-bundle $(FLATPAK_REPO_DIR) $(OUTPUTS_DIR)/$(FLATPAK_APP_ID).flatpak $(FLATPAK_APP_ID)
	@echo "Flatpak bundle created at $(OUTPUTS_DIR)/$(FLATPAK_APP_ID).flatpak"

install-flatpak: flatpak
	@echo "Installing Flatpak..."
	flatpak remote-add --user --if-not-exists --no-gpg-verify luminance-local-repo $(CURDIR)/$(FLATPAK_REPO_DIR)
	flatpak install --user -y --reinstall luminance-local-repo $(FLATPAK_APP_ID)

uninstall-flatpak:
	@echo "Uninstalling Flatpak..."
	-flatpak uninstall --user -y $(FLATPAK_APP_ID)
	-flatpak remote-delete --user luminance-local-repo

run-flatpak: install-flatpak
	@echo "Running Flatpak with GDB..."
	flatpak run --devel --command=sh $(FLATPAK_APP_ID) -c "gdb -batch -ex run -ex \"bt full\" --args /app/bin/$(FLATPAK_APP_ID)"

package-flatpak-flathub: $(FLATPAK_MANIFEST_IN)
	@echo "Checking version.txt status..."
	@if git status --porcelain version.txt | grep -q 'M'; then \
		echo "Error: version.txt has uncommitted changes. Please commit changes."; \
		exit 1; \
	fi
	@COMMIT=$$(git log -n 1 --pretty=format:%H -- version.txt); \
	echo "Updating flathub manifest for version $(VERSION) (commit $$COMMIT)..."; \
	sed -e "s|@LUMINANCE_SOURCE_TYPE@|git|g" \
	    -e "s|@LUMINANCE_SOURCE_DETAILS@|url: https://github.com/sidevesh/Luminance.git\\n        tag: v$(VERSION)\\n        commit: $$COMMIT|g" \
	    $(FLATPAK_MANIFEST_IN) > flathub/com.sidevesh.Luminance.yml

lint-flatpak-flathub: package-flatpak-flathub
	@echo "Linting Flatpak manifest..."
	# Lint the manifest generated for flathub
	-flatpak run --filesystem=$(CURDIR) --command=flatpak-builder-lint org.flatpak.Builder manifest flathub/com.sidevesh.Luminance.yml

# GNOME Extension
EXTENSION_UUID = luminance-extension@sidevesh
SRC_EXTENSION_DIR = gnome-extension/$(EXTENSION_UUID)
EXTENSION_BUILD_DIR = $(PKG_DIR)/gnome-extension
EXTENSION_ZIP = $(OUTPUTS_DIR)/$(EXTENSION_UUID).shell-extension.zip

package-gnome-extension:
	@echo "Packing GNOME extension..."
	rm -rf $(EXTENSION_BUILD_DIR)
	mkdir -p $(EXTENSION_BUILD_DIR)
	cp -r $(SRC_EXTENSION_DIR)/* $(EXTENSION_BUILD_DIR)/
	mkdir -p $(OUTPUTS_DIR)
	gnome-extensions pack $(EXTENSION_BUILD_DIR) --force --out-dir=$(OUTPUTS_DIR)
	@echo "Extension packed at $(EXTENSION_ZIP)"

install-gnome-extension: package-gnome-extension
	@echo "Installing GNOME extension..."
	gnome-extensions install --force $(EXTENSION_ZIP)
	@echo "GNOME extension installed. You may need to enable it with: gnome-extensions enable $(EXTENSION_UUID)"
	@echo "You may need to restart GNOME Shell (Alt+F2, 'r') or log out/in."

uninstall-gnome-extension:
	@echo "Uninstalling GNOME extension..."
	-gnome-extensions uninstall $(EXTENSION_UUID)
	@echo "Extension uninstalled."

clean:
	rm -rf build
