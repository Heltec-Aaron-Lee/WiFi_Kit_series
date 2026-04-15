#!/bin/bash
set -euo pipefail

if [ "${GITHUB_EVENT_NAME:-release}" != "release" ] && [ -z "${RELEASE_TAG:-}" ]; then
    echo "This script expects to run in GitHub Actions release context or with RELEASE_TAG set."
    exit 1
fi

EVENT_JSON="${GITHUB_EVENT_PATH:-}"
if [ -n "$EVENT_JSON" ] && [ -f "$EVENT_JSON" ]; then
    action=$(jq -r '.action // empty' "$EVENT_JSON")
    draft=$(jq -r '.release.draft // false' "$EVENT_JSON")
    release_tag_from_event=$(jq -r '.release.tag_name // empty' "$EVENT_JSON")
    if [ "$action" != "published" ]; then
        echo "Release action is '$action', skipping."
        exit 0
    fi
    if [ "$draft" = "true" ]; then
        echo "Draft release, skipping."
        exit 0
    fi
else
    release_tag_from_event=""
fi

RELEASE_TAG="${RELEASE_TAG:-$release_tag_from_event}"
if [ -z "$RELEASE_TAG" ]; then
    echo "RELEASE_TAG is empty."
    exit 1
fi

REPO_ROOT="${GITHUB_WORKSPACE:-$(pwd)}"
OUTPUT_DIR="$REPO_ROOT/build"
PACKAGE_JSON_TEMPLATE="$REPO_ROOT/package/package_esp32_index.template.json"
PACKAGE_JSON_NAME="package_heltec_esp32_index.json"

PACKAGE_NAME="heltec-esp32-arduino-$RELEASE_TAG"
PACKAGE_DIR="$OUTPUT_DIR/$PACKAGE_NAME"
PACKAGE_ZIP="$OUTPUT_DIR/$PACKAGE_NAME.zip"
PACKAGE_URL="https://github.com/${GITHUB_REPOSITORY}/releases/download/${RELEASE_TAG}/${PACKAGE_NAME}.zip"

mkdir -p "$OUTPUT_DIR"
rm -rf "$PACKAGE_DIR" "$PACKAGE_ZIP"

echo "Preparing package directory..."
mkdir -p "$PACKAGE_DIR/tools"

cp -f  "$REPO_ROOT/boards.txt"                "$PACKAGE_DIR/"
cp -f  "$REPO_ROOT/CMakeLists.txt"            "$PACKAGE_DIR/"
cp -f  "$REPO_ROOT/idf_component.yml"         "$PACKAGE_DIR/"
cp -f  "$REPO_ROOT/Kconfig.projbuild"         "$PACKAGE_DIR/"
cp -f  "$REPO_ROOT/package.json"              "$PACKAGE_DIR/"
cp -f  "$REPO_ROOT/programmers.txt"           "$PACKAGE_DIR/"

cp -Rf "$REPO_ROOT/cores"                     "$PACKAGE_DIR/"
cp -Rf "$REPO_ROOT/libraries"                 "$PACKAGE_DIR/"
cp -Rf "$REPO_ROOT/variants"                  "$PACKAGE_DIR/"

cp -f  "$REPO_ROOT/tools/espota.py"           "$PACKAGE_DIR/tools/"
[ -f "$REPO_ROOT/tools/espota.exe" ] && cp -f "$REPO_ROOT/tools/espota.exe" "$PACKAGE_DIR/tools/" || true
cp -f  "$REPO_ROOT/tools/gen_esp32part.py"    "$PACKAGE_DIR/tools/"
[ -f "$REPO_ROOT/tools/gen_esp32part.exe" ] && cp -f "$REPO_ROOT/tools/gen_esp32part.exe" "$PACKAGE_DIR/tools/" || true
cp -f  "$REPO_ROOT/tools/gen_insights_package.py" "$PACKAGE_DIR/tools/"
[ -f "$REPO_ROOT/tools/gen_insights_package.exe" ] && cp -f "$REPO_ROOT/tools/gen_insights_package.exe" "$PACKAGE_DIR/tools/" || true
[ -f "$REPO_ROOT/tools/pioarduino-build.py" ] && cp -f "$REPO_ROOT/tools/pioarduino-build.py" "$PACKAGE_DIR/tools/" || true
cp -Rf "$REPO_ROOT/tools/partitions"          "$PACKAGE_DIR/tools/"
cp -Rf "$REPO_ROOT/tools/ide-debug"           "$PACKAGE_DIR/tools/"

find "$PACKAGE_DIR" -name '*.DS_Store' -delete
find "$PACKAGE_DIR" -name '.git*' -type f -delete

echo "Generating platform.txt..."
sed \
  -e "s/^version=.*/version=${RELEASE_TAG}/" \
  -e 's|{runtime\.platform\.path}/tools/esp32-arduino-libs|{runtime.tools.esp32-arduino-libs.path}|g' \
  -e 's|{runtime\.platform\.path}\\tools\\esp32-arduino-libs|{runtime.tools.esp32-arduino-libs.path}|g' \
  -e 's|^tools\.xtensa-esp-elf-gcc\.path=.*|tools.xtensa-esp-elf-gcc.path={runtime.tools.xtensa-esp-elf-gcc.path}|g' \
  -e 's|^tools\.xtensa-esp-elf-gdb\.path=.*|tools.xtensa-esp-elf-gdb.path={runtime.tools.xtensa-esp-elf-gdb.path}|g' \
  -e 's|^tools\.riscv32-esp-elf-gcc\.path=.*|tools.riscv32-esp-elf-gcc.path={runtime.tools.riscv32-esp-elf-gcc.path}|g' \
  -e 's|^tools\.riscv32-esp-elf-gdb\.path=.*|tools.riscv32-esp-elf-gdb.path={runtime.tools.riscv32-esp-elf-gdb.path}|g' \
  -e 's|^tools\.esptool_py\.path=.*|tools.esptool_py.path={runtime.tools.esptool_py.path}|g' \
  "$REPO_ROOT/platform.txt" > "$PACKAGE_DIR/platform.txt"

echo "Generating core_version.h..."
GIT_VER=$(git -C "$REPO_ROOT" rev-parse --short=8 HEAD 2>/dev/null || echo 00000000)
GIT_DESC=$(git -C "$REPO_ROOT" describe --tags --always 2>/dev/null || echo "$RELEASE_TAG")
VER_DEFINE=$(echo "$RELEASE_TAG" | tr '[:lower:].-' '[:upper:]__')

cat > "$PACKAGE_DIR/cores/esp32/core_version.h" <<EOF
#define ARDUINO_ESP32_GIT_VER 0x${GIT_VER}
#define ARDUINO_ESP32_GIT_DESC "${GIT_DESC}"
#define ARDUINO_ESP32_RELEASE_${VER_DEFINE}
#define ARDUINO_ESP32_RELEASE "${VER_DEFINE}"
EOF

echo "Creating ZIP..."
(
  cd "$OUTPUT_DIR"
  zip -qr "$(basename "$PACKAGE_ZIP")" "$PACKAGE_NAME"
)

PACKAGE_SIZE=$(stat -c%s "$PACKAGE_ZIP")
PACKAGE_SHA=$(sha256sum "$PACKAGE_ZIP" | awk '{print $1}')

echo "Generating ${PACKAGE_JSON_NAME}..."
jq \
  --arg version "$RELEASE_TAG" \
  --arg url "$PACKAGE_URL" \
  --arg archive "$(basename "$PACKAGE_ZIP")" \
  --arg size "$PACKAGE_SIZE" \
  --arg checksum "SHA-256:$PACKAGE_SHA" \
  '
  .packages[0].platforms[0].version = $version |
  .packages[0].platforms[0].url = $url |
  .packages[0].platforms[0].archiveFileName = $archive |
  .packages[0].platforms[0].size = $size |
  .packages[0].platforms[0].checksum = $checksum
  ' \
  "$PACKAGE_JSON_TEMPLATE" > "$OUTPUT_DIR/$PACKAGE_JSON_NAME"

echo
echo "Build complete:"
echo "  $PACKAGE_ZIP"
echo "  $OUTPUT_DIR/$PACKAGE_JSON_NAME"