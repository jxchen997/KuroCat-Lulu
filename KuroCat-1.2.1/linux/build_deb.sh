#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
VERSION="${1:-1.1.0}"
ARCH="$(dpkg --print-architecture)"
PKG_DIR="$(mktemp -d)"
trap 'rm -rf "$PKG_DIR"' EXIT

if [ ! -x "$ROOT/kurocat" ]; then
    "$ROOT/build.sh"
fi

mkdir -p \
    "$PKG_DIR/DEBIAN" \
    "$PKG_DIR/usr/bin" \
    "$PKG_DIR/usr/share/applications" \
    "$PKG_DIR/usr/share/icons/hicolor/scalable/apps"

cp "$ROOT/kurocat" "$PKG_DIR/usr/bin/kurocat"
chmod 755 "$PKG_DIR/usr/bin/kurocat"
cp "$ROOT/kurocat.desktop" "$PKG_DIR/usr/share/applications/kurocat.desktop"
cp "$ROOT/kurocat.svg" "$PKG_DIR/usr/share/icons/hicolor/scalable/apps/kurocat.svg"

cat > "$PKG_DIR/DEBIAN/control" <<EOF
Package: kurocat
Version: ${VERSION}
Section: utils
Priority: optional
Architecture: ${ARCH}
Depends: libgtk-3-0, libcairo2, libx11-6, libxi6
Maintainer: chanke17 <https://github.com/chanke17/KuroCat>
Description: KuroCat native desktop keyboard & mouse cat pet
 Lightweight 60fps transparent desktop pet supporting full-state keyboard and
 mouse capture for typing and gaming (XInput2 + evdev), 2.5D head tracking,
 hunting dilated eyes, petting interactions, and Black/Siamese skins.
 Compatible with Ubuntu, Debian, Deepin, UnionTech UOS, and Kylin OS.
EOF

OUT_DEB="$ROOT/KuroCat-Linux-${ARCH}.deb"
dpkg-deb --build --root-owner-group "$PKG_DIR" "$OUT_DEB"
echo "==> Packaged Debian/UOS installer: $OUT_DEB ($(du -h "$OUT_DEB" | cut -f1))"
