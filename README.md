# Tindajudoshiai

This is a customised version of the JudoShiai software with Icelandic translations and local naming conventions for the Icelandic judo club Júdódeild Tindastóls.

Upstream JudoShiai project: <https://sourceforge.net/projects/judoshiai/>

This project is hosted at: <https://github.com/einarhre/tindajudoshiai>

This version contains Icelandic translations and local naming/customisation used by the club.

## Supported targets

Supported build targets:

* Linux
* Windows 32-bit
* Windows 64-bit

Linux builds are normal dynamically linked Linux builds. Do not use `BUILD_KIND=static` or `BUILD_KIND=shared` for Linux.

Windows builds support two build kinds:

* `BUILD_KIND=static`
* `BUILD_KIND=shared`

The full Windows build matrix is:

```bash
make TARGETOS=WIN32 BUILD_KIND=static
make TARGETOS=WIN32 BUILD_KIND=shared
make TARGETOS=WIN64 BUILD_KIND=static
make TARGETOS=WIN64 BUILD_KIND=shared
```

## Repository submodules

This repository uses a Git submodule for the CRS_BLD Windows cross-build environment:

```text
tools/CRS_BLD
```

CRS_BLD is a small MinGW-w64 cross-compiler and Windows dependency build environment based on ideas from MXE: <https://mxe.cc/>

The submodule contains the CRS_BLD build scripts, package configuration files and patches. It does not contain the generated compiler or package output directories.

Important generated CRS_BLD directories such as these are intentionally not tracked:

```text
tools/CRS_BLD/build/
tools/CRS_BLD/install/
tools/CRS_BLD/src/cpl/
tools/CRS_BLD/src/pkg/tar/
```

Clone this repository with submodules using:

```bash
git clone --recurse-submodules https://github.com/einarhre/tindajudoshiai.git
```

For an already existing clone, initialise the submodule with:

```bash
git submodule update --init --recursive
```

Check the current submodule commit with:

```bash
git submodule status
```

## Linux build

Development is normally done on Linux.

Required packages vary slightly between distributions. On openSUSE, the required development packages are roughly:

```bash
sudo zypper install \
  gcc gcc-c++ make \
  bison flex gettext-tools pkgconf-pkg-config \
  gtk3-devel cairo-devel librsvg-devel glib2-devel \
  libwebsockets-devel libcurl-devel libssh2-devel \
  libao-devel mpg123-devel \
  libxml2-devel xz-devel \
  desktop-file-utils shared-mime-info
```

On Debian/Ubuntu systems, the old dependency list is approximately:

```bash
sudo apt-get install \
  build-essential bison flex gettext \
  libgtk-3-dev libcairo2-dev librsvg2-dev \
  libao-dev libmpg123-dev \
  libcurl4-openssl-dev libssh2-1-dev \
  cmake libuv1-dev libcap-dev \
  liblzma-dev libxml2-dev \
  desktop-file-utils shared-mime-info
```

Build the normal Linux release with either `make TARGETOS=LINUX` or simply `make` on a normal 64-bit Linux system.

Create the Linux self-extracting setup file with:

```bash
make TARGETOS=LINUX setup
```

The result is written under:

```text
../build/release-linux/
```

The setup file has a name like:

```text
judoshiai-setup-4.1-alpha3.bin
```

Install the current Linux release directly with:

```bash
sudo make TARGETOS=LINUX install
```

The generated `.bin` installer and `make install` use the same installer logic.

The Linux install places the application under:

```text
/opt/judoshiai
```

Desktop integration is installed under:

* `/usr/local/bin`
* `/usr/local/share/applications`
* `/usr/local/share/icons`
* `/usr/local/share/mime`
* `/usr/local/share/pixmaps`

Executable command links are installed into `/usr/local/bin`.

## Linux 32-bit and ARM builds

The Makefiles still contain support for:

```bash
make TARGETOS=LINUX32
make TARGETOS=LINUXARM
```

These targets require the corresponding cross or multi-arch development libraries to be installed on the build system.

Do not use `BUILD_KIND` with Linux targets.

## Windows cross builds

Windows builds are cross-compiled from Linux.

The current Windows build setup uses the CRS_BLD custom cross-compiler environment, not the old MXE setup directly.

The CRS_BLD source/build environment is included as a submodule under:

```text
tools/CRS_BLD
```

Before building Windows packages, the CRS_BLD toolchains and dependency packages must exist locally. These generated files are not committed to Git.

The generated CRS_BLD cross-compilers are expected below the CRS_BLD `install/` tree, for example:

```text
tools/CRS_BLD/install/cross/i686-w64-mingw32/
tools/CRS_BLD/install/cross/x86_64-w64-mingw32/
```

Supported Windows targets:

* `TARGETOS=WIN32`
* `TARGETOS=WIN64`

Supported Windows build kinds:

* `BUILD_KIND=static`
* `BUILD_KIND=shared`

Examples:

```bash
make TARGETOS=WIN64 BUILD_KIND=static
make TARGETOS=WIN64 BUILD_KIND=shared
make TARGETOS=WIN32 BUILD_KIND=static
make TARGETOS=WIN32 BUILD_KIND=shared
```

Create a Windows installer with:

```bash
make TARGETOS=WIN64 BUILD_KIND=static windows-installer
make TARGETOS=WIN64 BUILD_KIND=shared windows-installer
make TARGETOS=WIN32 BUILD_KIND=static windows-installer
make TARGETOS=WIN32 BUILD_KIND=shared windows-installer
```

The shorter `setup` target also creates a Windows installer when building a Windows target:

```bash
make TARGETOS=WIN64 BUILD_KIND=static setup
```

Windows installers are created with NSIS, so `makensis` must be available in `PATH`.

The generated Windows installers are written under target-specific release directories:

* `../build/release-win64-static/`
* `../build/release-win64-shared/`
* `../build/release-win32-static/`
* `../build/release-win32-shared/`

Installer names include the target and build kind, for example:

```text
judoshiai-4.1-alpha3-win64-static-setup.exe
judoshiai-4.1-alpha3-win64-shared-setup.exe
judoshiai-4.1-alpha3-win32-static-setup.exe
judoshiai-4.1-alpha3-win32-shared-setup.exe
```

## Windows package targets

For Windows builds:

* `make TARGETOS=WIN64 BUILD_KIND=static windows-installer` creates an NSIS installer.
* `make TARGETOS=WIN64 BUILD_KIND=static windows-zip` creates a portable ZIP package.
* `make TARGETOS=WIN64 BUILD_KIND=static windows-dist` creates both the installer and the ZIP package.

Use the same pattern with `TARGETOS=WIN32`, `BUILD_KIND=shared`, or both as needed.

## Cleaning

Clean the current selected target with:

```bash
make clean
```

Or explicitly:

```bash
make TARGETOS=LINUX clean
make TARGETOS=WIN64 BUILD_KIND=static clean
make TARGETOS=WIN64 BUILD_KIND=shared clean
make TARGETOS=WIN32 BUILD_KIND=static clean
make TARGETOS=WIN32 BUILD_KIND=shared clean
```

The broader cleanup of all old build directories is intentionally not part of the normal `clean` target.

## Typical maintainer build commands

Build Linux release and setup file:

```bash
make TARGETOS=LINUX clean
make TARGETOS=LINUX all setup
```

Build all Windows installers:

```bash
for target in WIN64 WIN32
do
  for kind in static shared
  do
    make TARGETOS="${target}" BUILD_KIND="${kind}" clean
    make TARGETOS="${target}" BUILD_KIND="${kind}" all windows-installer
  done
done
```

## Updating the CRS_BLD submodule pointer

When CRS_BLD itself has been changed, committed and pushed, update the submodule pointer in this repository:

```bash
cd tools/CRS_BLD
git pull
```

Then return to the Tindajudoshiai repository and commit the updated submodule pointer:

```bash
cd ../..

git status
git submodule status
git add tools/CRS_BLD
git commit -m "build: update CRS build environment submodule"
git push
```

This does not copy CRS_BLD files into Tindajudoshiai. It only updates the exact CRS_BLD commit that this repository points to.

## Notes

Linux builds are dynamically linked.

Windows `static` and `shared` refer to how the Windows dependency/runtime set is linked and packaged.

The Linux installer deliberately installs desktop files, MIME files, icons and pixmaps as real files under `/usr/local/share`, not as symlinks into `/opt/judoshiai`. This avoids broken desktop integration if `/opt/judoshiai` is removed manually.

