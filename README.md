<a name="readme-top"></a>

<br />
<div align="center">
  <a href="https://github.com/sidevesh/Luminance">
    <img src="icons/hicolor/scalable/apps/com.sidevesh.Luminance.svg" alt="Logo" width="120" height="120">
  </a>

<h3 align="center">Luminance</h3>

  <p align="center">
    A simple GTK application to control brightness of displays including external displays supporting DDC/CI
    <br />
    <br />
  </p>
</div>

<div align="center">
  <img src="screenshots/screenshot-faded.png" alt="Screenshot">
</div>

## Install from AUR (Arch Linux):
  
  ```
  yay -S luminance
  ```

## Install deb package (Debian and Ubuntu based distros):
Latest deb release is available on the releases page https://github.com/sidevesh/Luminance/releases

* Version 1.0.4 is the last supported version for Ubuntu before 24.04, Debian or any other Debian based distro that does not have atleast libadwaita 1.5 version

## Install rpm package (Fedora, OpenSUSE and other rpm based distros):
Latest rpm release is available on the releases page https://github.com/sidevesh/Luminance/releases

## Install from source

### Dependencies
- GTK 4.0
- Libadwaita
- ddcutil library package (typically libddcutil or libddcutil-dev if not already installed with ddcutil)
- Make (Build tool)
- Meson (Build tool)
- Ninja (Build tool)
- GCC (Compiler)

### Steps
1. Firstly, ensure that all the dependencies are installed:

    ##### Arch Linux:
    ```
    pacman -S --needed make meson glib2-devel glib2 gtk4 libadwaita ddcutil
    ```
    
    ##### Ubuntu-based distros (credit @aidengilmartin):
    ```
    sudo apt install make gcc meson libglib2.0-dev libgtk-4-dev libadwaita-1-dev libddcutil-dev
    ```
    ##### Fedora:
    ```
    sudo dnf install make gcc meson glib2-devel gtk4-devel libadwaita-devel libddcutil-devel
    ```

2. Ensure that your user has access to the i2c devices:
https://www.ddcutil.com/i2c_permissions/

3. Clone this repo:
    ```
    git clone --recurse-submodules https://github.com/sidevesh/Luminance.git
    ```

4. Change into the `Luminance` directory and build the application:
    ```
    cd Luminance
    make
    ```

5. To install this binary for all users execute install command:
    ```
    make install
    ```

    You should now be able to find Luminance in your list of apps.
    * Installing the application is necessary for it to work because it uses gsettings to store preferences and will need the schema files to be installed.
    * Note: You might need to reload udev rules or reboot for brightness control permissions to take effect.


### Development Build
To build a development version (which installs as `com.sidevesh.Luminance.Devel` and allows side-by-side installation with the release version):

```
make install-debug
```

### Flatpak

To build the application as a Flatpak:

```
make flatpak
```

To install the locally built Flatpak:

```
make install-flatpak
```

To uninstall the locally built Flatpak:

```
make uninstall-flatpak
```

To run the installed Flatpak with GDB for debugging:

```
make run-flatpak
```

To create a standalone Flatpak bundle:

```
make flatpak-bundle
```

### Uninstall

To uninstall the application, run the following from the build directory:

```bash
make uninstall
```

Or for the debug build:

```bash
make uninstall-debug
```

## Usage

Start the application normally or you can also use the app via cli interface:
```
Usage: com.sidevesh.Luminance [OPTIONS]
An application to control brightness of displays including external displays supporting DDC/CI

Options:
  -l, --list-displays                         List displays and their brightness
  -g, --get-percentage [DISPLAY NUMBER]       Get the brightness percentage of a display
  -s, --set-brightness [DISPLAY NUMBER]       Set the brightness of a display to a percentage value
  -i, --increase-brightness [DISPLAY NUMBER]  Increase the brightness of a display by a percentage value
  -d, --decrease-brightness [DISPLAY NUMBER]  Decrease the brightness of a display by a percentage value
                                              If DISPLAY NUMBER is not provided, for --set-brightness, --increase-brightness and --decrease-brightness options, the brightness of all displays will be changed
  -p  --percentage [PERCENTAGE]               Percentage value to set the brightness to in case of --set-brightness option or to increase or decrease the brightness by in case of --increase-brightness or --decrease-brightness option
  -h, --help                                  Show help information

When no arguments are provided, the application starts in GUI mode.
```

## GNOME Extension

A companion GNOME Shell extension is available for Luminance. This extension displays the native GNOME brightness OSD (On-Screen Display) when brightness is adjusted outside of the main GUI application window (e.g. via command line interface or D-Bus calls).

### Install from Release
Latest extension release is available on the releases page https://github.com/sidevesh/Luminance/releases
Download the zip file and install it using `gnome-extensions install [ZIP_FILE]`.

### Install from Source
To build and install the extension from source:
```bash
make install-gnome-extension
```

After installation, you may need to restart GNOME Shell (Alt+F2, type `r`, enter on X11, or log out/in on Wayland) and enable the extension:
```bash
gnome-extensions enable luminance-extension@sidevesh
```

## Note for Maintainers

### Release Workflow

The release process involves updating the version, tagging/pushing the main application, and then updating downstream packagers (Arch/Flathub).

1.  **Update Version & Changelog**:
    *   Update `version.txt` with the new version number.
    *   Update `releases.xml` using the script:
        ```bash
        ./scripts/generate-releases.sh
        ```
    *   **Commit these changes** to the main repository.

2.  **Tag and Push Main Repo**:
    *   **Important**: Flathub and Arch builds rely on the tag existing on GitHub.
    *   **Sync Tags**: First, ensure you have all tags from GitHub (in case releases were created there).
        ```bash
        git pull --tags
        ```
    *   **Create Tag**: You can create the release tag (e.g., `1.4.4`) either locally or by drafting a release on GitHub.
        *   **Locally**:
            ```bash
            git tag 1.4.4
            git push origin main --tags
            ```
        *   **GitHub**: Go to Releases -> Draft a new release -> Create new tag -> Publish.

3.  **Update Arch (Direct Push)**:
    *   Run packaging script: `make package-arch`
    *   Go into `arch/`, commit changes, and push to AUR (master branch is allowed).
    *   Go back to root, commit the `arch` submodule change, and push.

4.  **Update Flathub (PR Workflow)**:
    **Note**: Direct push to Flathub `master` is **not allowed**. You must use a Pull Request.
    
    *   **Preparation**:
        ```bash
        cd flathub
        git checkout master
        git pull
        git checkout -b update-to-1.4.4  # Create a new branch
        cd ..
        ```
    *   **Generate Manifest**:
        ```bash
        make package-flatpak-flathub
        ```
    *   **Commit and Push Branch**:
        ```bash
        cd flathub
        git add .
        git commit -m "Update to 1.4.4"
        git push -u origin update-to-1.4.4
        ```
    *   **Open PR**: Go to the Flathub repository URL and open a Pull Request from `update-to-1.4.4` to `master`.
    *   **Cleanup**: Once the PR is merged, you can pull the latest `master` in the `flathub` directory and commit that pointer to the main repo if you wish to keep it in sync.

## Note for AUR publishers

Switch the arch submodule push url to the ssh url before pushing to AUR for the first time:
```bash
cd arch
git remote set-url --push origin ssh://aur@aur.archlinux.org/luminance.git
```

### Packaging for Arch
1.  Ensure step 1 from "Release Workflow" is done and `version.txt` is committed.
2.  Run the packaging command:
    ```bash
    make package-arch
    ```
    This updates `PKGBUILD` and `.SRCINFO` in the `arch/` directory.
3.  Go into `arch/` and commit the changes.
4.  **Wait** until you have pushed the tag to the main repository (Step 5 of Release Workflow) before pushing this submodule to AUR.

## Note for Flatpak Packaging

### Prerequisites
1.  Install `flatpak`.
2.  Install `flatpak-builder` (It is recommended to use the flatpak version of the builder):
    ```bash
    flatpak install -y flathub org.flatpak.Builder
    ```
3.  Install the required GNOME Runtime and SDK:
    ```bash
    flatpak install org.gnome.Platform//49 org.gnome.Sdk//49
    ```

### Packaging for Flathub
1.  Ensure step 1 from "Release Workflow" is done and `version.txt` is committed.
2.  Run the packaging command:
    ```bash
    make package-flatpak-flathub
    ```
    This updates `flathub/com.sidevesh.Luminance.yml` with the URL, the new tag (which doesn't exist yet), and the commit hash of `version.txt`.
3.  Go into `flathub/` and commit the changes.
4.  **Wait** until you have pushed the tag to the main repository (Step 5 of Release Workflow) before pushing this submodule to Flathub.

### Build and Install Locally
To build and install the Flatpak locally for testing:
```bash
make install-flatpak
```

### Validate (Lint)
To validate the manifest and repository against Flathub requirements:
```bash
make lint-flatpak-flathub
```

### Run the Application (with Debugging)
```bash
make run-flatpak
```

### Creating a Bundle
To create a binary bundle `.flatpak` for distribution:
```bash
make flatpak-bundle
```


## License

Distributed under the GNU General Public License v3.0. See `LICENSE.txt` for more information.



## Contact

Swapnil Devesh - [@sid_devesh](https://twitter.com/sid_devesh) - me@sidevesh.com

Project Link: [https://github.com/sidevesh/Luminance](https://github.com/sidevesh/Luminance)



## Acknowledgments

- @ahshabbir : for ddcbc-api and ddcbc-gtk: https://github.com/ahshabbir/ddcbc-api
- @rockowitz : for ddcutil c api: https://github.com/rockowitz/ddcutil
- @jimmac : for the awesome icon: https://github.com/jimmac
