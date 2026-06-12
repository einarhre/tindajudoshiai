# Tindajudoshiai

This is a customized version of the JudoShiai software for Júdódeild Tindastóls.

Upstream JudoShiai project: https://sourceforge.net/projects/judoshiai/

This version contains Icelandic translations and local naming/customization used by the club.

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

The result is written under `../build/release-linux/`.

The setup file has a name like:

```text
judoshiai-setup-4.1-alpha3.bin
```

Install the current Linux release directly with:

```bash
sudo make TARGETOS=LINUX install
```

The generated `.bin` installer and `make install` use the same installer logic.

The Linux install places the application under `/opt/judoshiai`.

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

The current Windows build setup uses the CRS custom cross-compiler environment, not the old MXE instructions.

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

## Windows../build/release-win32-static/`

* `../build/release-win32-shared/`

Installer names include the target and build kind, for example:

````text
judoshiai-4. package targets

For Windows builds:

- `make TARGETOS=WIN64 BUILD_KIND=static windows-installer` creates an NSIS installer.
- `make TARGETOS=WIN64 BUILD_KIND=static windows-zip` creates a portable ZIP package.
- `make TARGETOS=WIN64 BUILD_KIND=static windows-dist` creates both the installer and the ZIP package.

Use the same pattern with `TARGETOS=WIN32`, `BUILD_KIND=shared`, or both as needed.

## Cleaning

Clean the current selected target with:

```bash
make clean
````

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

## Notes

Linux builds are dynamically linked.

Windows `static` and `shared` refer to how the Windows dependency/runtime set is linked and packaged.

The Linux installer deliberately installs desktop files, MIME files, icons and pixmaps as real files under `/usr/local/share`, not as symlinks into `/opt/judoshiai`. This avoids broken desktop integration if `/opt/judoshiai` is removed manually.

