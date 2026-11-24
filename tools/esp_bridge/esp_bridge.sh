#!/bin/bash
#
# ESP bridge: lets a remote agent (with access only to this folder) build, flash and monitor an ESP32.
#
# Usage (macOS/Linux terminal, ESP-IDF environment active):
#   . $IDF_PATH/export.sh
#   tools/esp_bridge/esp_bridge.sh /dev/cu.usbserial-XXXX
#
# Protocol (files inside <repo>/.bridge/):
#   request   written by the agent. First line is a command:
#               flash <example path relative to examples/>   build + flash, then restart serial log
#               reset                                         reset the board (serial log keeps running)
#               send <text>                                   send text + CR to the serial port
#   status    written by the bridge: "busy <cmd>" or "idle <cmd> rc=<n>"
#   build.log output of the last build/flash
#   monitor.log  serial output (timestamped), truncated at every flash/reset
#
# Stop with Ctrl+C.

PORT="${1:?serial port required, ie /dev/cu.usbserial-0201E290}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
B="${ROOT}/.bridge"
mkdir -p "${B}"
LOGGER_PID=""

# copy all output (and errors) to .bridge/bridge.log
exec > >(tee -a "${B}/bridge.log") 2>&1
echo "=== bridge start $(date)  IDF_PATH=${IDF_PATH:-<not set>}  python3=$(command -v python3)"
command -v idf.py > /dev/null || echo "WARNING: idf.py not found, activate ESP-IDF first (. \$IDF_PATH/export.sh)"
python3 -c "import serial" 2>/dev/null || echo "WARNING: pyserial not available for python3 (activate ESP-IDF environment)"
[ -e "${PORT}" ] || echo "WARNING: serial port ${PORT} not found"

start_logger() {
  python3 "${ROOT}/tools/esp_bridge/serial_logger.py" "${PORT}" "${B}" "$1" &
  LOGGER_PID=$!
}

stop_logger() {
  if [ -n "${LOGGER_PID}" ]; then
    kill "${LOGGER_PID}" 2>/dev/null
    wait "${LOGGER_PID}" 2>/dev/null
    LOGGER_PID=""
  fi
}

trap 'stop_logger; echo "stopped" > "${B}/status"; echo "=== bridge stopped (signal)"; exit 0' INT TERM HUP

echo "idle start rc=0" > "${B}/status"
start_logger noreset
echo "ESP bridge running on ${PORT} (folder ${B}). Ctrl+C to stop."

while true; do
  if [ -f "${B}/request" ]; then
    CMD="$(head -n 1 "${B}/request")"
    rm -f "${B}/request"
    echo "busy ${CMD}" > "${B}/status"
    echo "[$(date +%H:%M:%S)] ${CMD}"
    set -- ${CMD}
    case "$1" in
      flash)
        EX="$2"
        NAME="$(echo "${EX}" | tr '/' '_')"
        stop_logger
        idf.py -C "${ROOT}/examples/${EX}" -B "${B}/build_${NAME}" -D SDKCONFIG="${B}/build_${NAME}/sdkconfig" -p "${PORT}" build flash > "${B}/build.log" 2>&1
        RC=$?
        start_logger reset
        ;;
      reset)
        stop_logger
        start_logger reset
        RC=0
        ;;
      send)
        shift
        echo "$*" > "${B}/send"
        RC=0
        ;;
      *)
        RC=99
        ;;
    esac
    echo "idle ${CMD} rc=${RC}" > "${B}/status"
    echo "[$(date +%H:%M:%S)] done rc=${RC}"
  fi
  sleep 1
done
