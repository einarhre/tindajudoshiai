#!/bin/sh
set -eu

SRC=${1:-judoshiai}
DIR=${2:-/opt}
LOCAL=${3:-/usr/local}

APPDIR="$DIR/judoshiai"

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: install.sh must be run as root" >&2
    exit 1
fi

if [ ! -d "$SRC/bin" ]; then
    echo "ERROR: invalid JudoShiai release tree: $SRC" >&2
    exit 1
fi

echo "Installing JudoShiai"
echo "Source:      $SRC"
echo "Install dir: $APPDIR"
echo "Local dir:   $LOCAL"

mkdir -p "$DIR"
rm -rf "$APPDIR"
mkdir -p "$APPDIR"
cp -R "$SRC"/. "$APPDIR"/

mkdir -p "$LOCAL/bin"

for prog in judoshiai judotimer judoinfo judoweight judojudogi judoproxy judohttpd
do
    if [ -x "$APPDIR/bin/$prog" ]; then
        ln -sfn "$APPDIR/bin/$prog" "$LOCAL/bin/$prog"
    fi
done

if [ -d "$APPDIR/share/applications" ]; then
    mkdir -p "$LOCAL/share/applications"
    rm -f "$LOCAL/share/applications"/judo*.desktop
    rm -f "$LOCAL/share/applications"'/*.desktop'

    for f in "$APPDIR"/share/applications/judo*.desktop
    do
        [ -f "$f" ] || continue
        if command -v desktop-file-install >/dev/null 2>&1; then
            desktop-file-install --dir="$LOCAL/share/applications" "$f"
        else
            install -m 644 "$f" "$LOCAL/share/applications/"
        fi
    done

    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "$LOCAL/share/applications" || true
    fi
fi

if [ -d "$APPDIR/share/pixmaps" ]; then
    mkdir -p "$LOCAL/share/pixmaps"
    rm -f "$LOCAL/share/pixmaps"/judo*.png

    for f in "$APPDIR"/share/pixmaps/judo*.png
    do
        [ -f "$f" ] || continue
        install -m 644 "$f" "$LOCAL/share/pixmaps/"
    done
fi

if [ -d "$APPDIR/share/icons/hicolor" ]; then
    for appdir in "$APPDIR"/share/icons/hicolor/*/apps
    do
        [ -d "$appdir" ] || continue

        size=$(basename "$(dirname "$appdir")")
        dest="$LOCAL/share/icons/hicolor/$size/apps"

        mkdir -p "$dest"
        rm -f "$dest"/judo*.png

        for f in "$appdir"/judo*.png
        do
            [ -f "$f" ] || continue
            install -m 644 "$f" "$dest/"
        done
    done

    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache --force --ignore-theme-index "$LOCAL/share/icons/hicolor" || true
    fi
fi

if [ -f "$APPDIR/share/mime/packages/judoshiai.xml" ]; then
    mkdir -p "$LOCAL/share/mime/packages"
    rm -f "$LOCAL/share/mime/packages/judoshiai.xml"
    install -m 644 "$APPDIR/share/mime/packages/judoshiai.xml" "$LOCAL/share/mime/packages/"

    if command -v update-mime-database >/dev/null 2>&1; then
        update-mime-database "$LOCAL/share/mime" || true
    fi
fi

echo "JudoShiai installed successfully."
