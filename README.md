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
    pacman -S --needed make meson glib2 gtk4 libadwaita ddcutil
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
  -o, --show-osd                              Show OSD popup when brightness is changed for specified environment:
                                                g: GNOME, experimental, only works with https://extensions.gnome.org/extension/5952/eval-gjs/ extension installed
  -h, --help                                  Show help information

When no arguments are provided, the application starts in GUI mode.
```

## Note for AUR publishers

Switch the arch submodule push url to the ssh url before pushing to AUR for the first time:
```
cd arch
git remote set-url --push origin ssh://aur@aur.archlinux.org/luminance.git
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
