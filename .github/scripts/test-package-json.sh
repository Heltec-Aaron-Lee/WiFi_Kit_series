#!/bin/bash
set -euo pipefail

SCRIPTS_DIR="./.github/scripts"
OUTPUT_DIR="${GITHUB_WORKSPACE}/build"
PACKAGE_JSON="package_heltec_esp32_index.json"
PACKAGE_ZIP=$(find "$OUTPUT_DIR" -maxdepth 1 -name 'heltec-esp32-arduino-*.zip' | head -n 1)

if [ -z "${PACKAGE_ZIP:-}" ] || [ ! -f "$PACKAGE_ZIP" ]; then
    echo "Package zip not found."
    exit 1
fi

echo "Installing Arduino CLI..."
bash "${SCRIPTS_DIR}/install-arduino-cli.sh"
export PATH="$HOME/bin:$PATH"

PORT=8123
BASE_URL="http://127.0.0.1:${PORT}"
ZIP_NAME="$(basename "$PACKAGE_ZIP")"
LOCAL_JSON="$OUTPUT_DIR/package_heltec_esp32_index.local.json"

cleanup() {
    if [ -n "${HTTP_PID:-}" ] && kill -0 "$HTTP_PID" 2>/dev/null; then
        kill "$HTTP_PID" 2>/dev/null || true
        wait "$HTTP_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

echo "Creating local test JSON..."
jq \
  --arg url "${BASE_URL}/${ZIP_NAME}" \
  '.packages[0].platforms[0].url = $url' \
  "$OUTPUT_DIR/$PACKAGE_JSON" > "$LOCAL_JSON"

echo "Starting local HTTP server..."
python3 -m http.server "$PORT" --directory "$OUTPUT_DIR" >/tmp/heltec-http.log 2>&1 &
HTTP_PID=$!

sleep 2

echo "Checking local server..."
curl -I "${BASE_URL}/${ZIP_NAME}"

echo "Installing core from local JSON..."
arduino-cli core install Heltec-esp32:esp32 --additional-urls "${BASE_URL}/$(basename "$LOCAL_JSON")"

EXAMPLE="$GITHUB_WORKSPACE/libraries/ESP32/examples/CI/CIBoardsTest/CIBoardsTest.ino"

echo "Compiling Heltec WiFi LoRa 32(V4)..."
arduino-cli compile --fqbn Heltec-esp32:esp32:heltec_wifi_lora_32_V4 "$EXAMPLE"

echo "Compiling Heltec Wireless Tracker(V2)..."
arduino-cli compile --fqbn Heltec-esp32:esp32:heltec_wireless_tracker_v2 "$EXAMPLE"

echo "Cleaning up..."
arduino-cli core uninstall Heltec-esp32:esp32 || true

echo "All tests passed."