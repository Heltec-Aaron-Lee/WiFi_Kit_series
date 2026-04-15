#!/bin/bash
set -euo pipefail

SCRIPTS_DIR="./.github/scripts"
OUTPUT_DIR="${GITHUB_WORKSPACE}/build"
PACKAGE_JSON="package_heltec_esp32_index.json"
PACKAGE_ZIP=$(find "$OUTPUT_DIR" -maxdepth 1 -name 'heltec-esp32-core-*.zip' | head -n 1)

if [ -z "${PACKAGE_ZIP:-}" ] || [ ! -f "$PACKAGE_ZIP" ]; then
    echo "Package zip not found."
    exit 1
fi

function get_file_url {
    local file_path="$1"
    echo "file://$(cd "$(dirname "$file_path")" && pwd)/$(basename "$file_path")"
}

echo "Installing Arduino CLI..."
bash "${SCRIPTS_DIR}/install-arduino-cli.sh"
export PATH="$HOME/bin:$PATH"

LOCAL_PACKAGE_URL=$(get_file_url "$PACKAGE_ZIP")
LOCAL_JSON="$OUTPUT_DIR/package_heltec_esp32_index.local.json"

echo "Creating local test JSON..."
jq \
  --arg url "$LOCAL_PACKAGE_URL" \
  '.packages[0].platforms[0].url = $url' \
  "$OUTPUT_DIR/$PACKAGE_JSON" > "$LOCAL_JSON"

LOCAL_JSON_URL=$(get_file_url "$LOCAL_JSON")

echo "Installing core from local JSON..."
arduino-cli core install esp32:esp32 --additional-urls "$LOCAL_JSON_URL"

EXAMPLE="$GITHUB_WORKSPACE/libraries/ESP32/examples/CI/CIBoardsTest/CIBoardsTest.ino"

echo "Compiling Heltec WiFi LoRa 32(V4)..."
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V4 "$EXAMPLE"

echo "Compiling Heltec Wireless Tracker(V2)..."
arduino-cli compile --fqbn esp32:esp32:heltec_wireless_tracker_v2 "$EXAMPLE"

echo "Cleaning up..."
arduino-cli core uninstall esp32:esp32 || true

echo "All tests passed."