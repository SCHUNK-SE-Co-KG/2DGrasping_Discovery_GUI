<b>This is a fork of <a href="https://github.com/roboception/rcdiscover">rcdiscover</a> at commit [ec316a8a32ae8bdf38ac2c1af7c7693ea5a0fadd](https://github.com/SCHUNK-SE-Co-KG/2DGrasping_Discovery_GUI/commit/ec316a8a32ae8bdf38ac2c1af7c7693ea5a0fadd).</b>

Changes:
- Updated company labels and icons to reflect SCHUNK.
- Clicking on a device now opens the Smart Grasping frontend instead of the default Roboception frontend.


Discovery of sensors
================================

This package contains tools for the discovery of devices via GigE Vision.

- `schunk2DGraspingDiscovery-gui`: graphical application for discovering devices and
  sending magic packets for resetting of parameters

**Installation:** Follow the compilation steps in the next sections.

Compiling on Linux
------------------

For compilation, cmake is required.

`schunk2DGraspingDiscovery-gui` additionally requires [WxWidgets](http://www.wxwidgets.org/).

To install this under Debian/Ubuntu >= 20.04.0:
```
sudo apt-get install cmake libwxgtk3.0-gtk3-dev
```

In older distributions, the package is called libwxgtk3.0-dev

### Building 

It's required to do an out-of-source build:

```
mkdir build
cd build
cmake ..
make
```

To build the gui as well, pass the CMAKE option `BUILD_SCHUNKDISCOVER_GUI`:

```
cmake -DBUILD_SCHUNKDISCOVER_GUI=ON ..
```


Afterwards, the binaries can be found in `build/tools/`.

### Installation

Installation can either be done via

```
make install
```

On Debian (and derivatives like Ubuntu) Debian packages can be built with

```
cmake -DCMAKE_INSTALL_PREFIX="/usr" ..
make package
```
which can be installed with e.g. `sudo dpkg -i schunkdiscover*.deb`


Discovering sensors in other subnets
------------------------------------

Most Linux distributions have reverse path filtering turned on, which restricts discoverability of sensor to the same subnet as the host.

Check this with
```
sysctl net.ipv4.conf.all.rp_filter
sysctl net.ipv4.conf.default.rp_filter
```

Reverse path filtering can be turned off with
```
sudo sysctl -w net.ipv4.conf.all.rp_filter=0
sudo sysctl -w net.ipv4.conf.default.rp_filter=0
```
You might also need to disable it for your specific interface, e.g.:
```
sudo sysctl -w net.ipv4.conf.eth0.rp_filter=0
```
Note: These settings are not persistent across reboots!
To persist them you can add a file in `/etc/sysctl.d/` on most distributions.
See `debian/50-schunkdiscover-rpfilter.conf` for an example.

If you built a Debian package with `make package`, it will automatically ask you if you want to disable reverse path filtering at package installation.


Compiling on Windows
--------------------
### Installing MinGW-w64 using Msys2

Use Msys2 for building, installing and running native Windows software.
Download and install `msys2-x86_64-20240507.exe` from [here](https://www.msys2.org/#:~:text=Download%20the%20installer).

Open mingw64 shell launcher under `c:\msys64`.
Run following commands to  install some tools like the mingw-w64.


```
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-cmake
```

**Note: In case certificate issue , add certificate information to  `C:\msys64\usr\ssl\certs\ca-bundle.crt` and reopen mingw64 shell launcher and run the above commands again. 


Finally, add the `bin` directory of MinGW to your PATH variable.It is normally found in`C:\msys64\mingw64\bin` for 64 bit installation .

Open windows cmd terminal and check if  `mingw32-make` is installed.

#### WxWidgets

Static libraries of WxWidgets are required for the schunkdiscover-gui. To build
them, the steps from
[here](https://wiki.wxwidgets.org/Compiling_wxWidgets_with_MinGW) have been
adapted slightly.

Open windows cmd terminal in desired folder and run below commands:

```
git clone https://github.com/wxWidgets/wxWidgets.git
cd wxWidgets
git checkout v3.2.5  # This is the stable version that worked.
git submodule update  --init # update submodule
cd build\msw
mingw32-make -f makefile.gcc SHARED=0 BUILD=release -j4 CXXFLAGS="-mtune=generic -mno-abm" CFLAGS="-mtune=generic -mno-abm"
```

#### Build GUI

Open windows cmd terminal in desired folder and run below commands:

```
cd schunkdiscover
mkdir build-mingw32
cd build-mingw32
cmake -G"MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_SCHUNKDISCOVER_GUI=ON -DwxWidgets_ROOT_DIR=<path to WxWidgets root folder> ..
mingw32-make
```
Finally,rebuild project and executable `schunk2DGraspingDiscovery-gui.exe` will be found in `\schunkdiscover\build-mingw32\tools`.


<!-- **For the 32 bit build you may encounter a 0xc000007b error when running
schunkdiscover-gui.exe.** This seems to be caused by a bug in WxWidgets build. As
a workaround, rename `rcdefs.h` in `lib\gcc_lib\mswu\wx\msw` in your WxWidgets
root directory to something different (e.g., `rcdefs.h_old`). Then, rerun
above WxWidgets build command:

