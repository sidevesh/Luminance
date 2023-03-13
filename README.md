# Luminance
A GTK Interface for controlling brightness through the DDC/CI protocol. It includes support for controlling the brightness of multiple displays.

# Dependencies
- ddcutil library package (typically libddcutil or libddcutil-dev if not already installed with ddcutil)
- GTK 3.0

# Setup

1. Firstly, ensure that all the dependencies are installed.

#### Arch Linux:
```
pacman -S --needed ddcutil gtk3
```

#### Ubuntu-based distros (credit @aidengilmartin):
```
sudo apt install libgtk3-dev gcc
sudo add-apt-repository ppa:rockowitz/ddcutil # add ddcutil repo
sudo apt install ddcutil libddcutil-dev
```
#### Fedora:
```
sudo dnf install ddcutil libddcutil libddcutil-devel gtk3-devel gcc
```
TODO: Add other distributions to this! [#3](../../issues/3)

2. Ensure that your user has access to the i2c devices:
https://www.ddcutil.com/i2c_permissions/

3. Clone this repo along with its submodules:
```
git clone --recurse-submodules https://github.com/sidevesh/Luminance.git
```

4. Change into the `Luminance` directory and execute build.sh to build this application:
```
cd Luminance
./build.sh
```

5. This should result in a `com.sidevesh.Luminance` binary that you can execute to contol the brightness:
```
./com.sidevesh.Luminance
```

To install this binary for all users execute 'install.sh' as root:
```
sudo ./install.sh
```

You should now be able to find Luminance in your application menu under utilities.

# Credits
- @ahshabbir : for ddcbc-api and ddcbc-gtk: https://github.com/ahshabbir/ddcbc-api
- @rockowitz : for ddcutil c api: https://github.com/rockowitz/ddcutil
