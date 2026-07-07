#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# kg8k.sh — KitchenGadget8000 build / flash / monitor helper
#
# Usage:
#   ./kg8k.sh [command...]   (default: build flash monitor)
#
# Commands:
#   deploy          ★ Full cycle: clean → build → flash → monitor
#   build           Compile the firmware
#   flash           Flash firmware to connected device
#   monitor         Open serial monitor (Ctrl-] or Ctrl-T Ctrl-X to exit)
#   spiffs          Build and flash the SPIFFS recipe image
#   clean           Delete the build directory and regenerate sdkconfig
#   set-target      Force idf.py set-target esp32p4 (run after clean)
#   help            Show this message
#
# Commands can be combined:  ./kg8k.sh build flash monitor
# Port override:             PORT=/dev/cu.usbmodem123 ./kg8k.sh flash
#
# Examples:
#   ./kg8k.sh deploy               # clean slate full deploy
#   ./kg8k.sh build flash          # rebuild and flash (keep existing sdkconfig)
#   ./kg8k.sh spiffs               # update recipes on device
#   ./kg8k.sh monitor              # open serial console only
# ─────────────────────────────────────────────────────────────────────────────

set -euo pipefail

# ── Configuration ─────────────────────────────────────────────────────────────
IDF_PATH_DEFAULT="$HOME/esp/esp-idf"
TARGET="esp32p4"
RECIPES_DIR="recipes"
SPIFFS_PARTITION_OFFSET="0x620000"   # must match partitions.csv
SPIFFS_PARTITION_SIZE="10092544"     # 0x9E0000 bytes

# ── Colours: black + orange theme ────────────────────────────────────────────
OR='\033[38;5;208m'        # orange
OR2='\033[38;5;172m'       # amber (dimmer orange)
OR3='\033[38;5;130m'       # dark orange (borders/dividers)
WHT='\033[1;37m'           # bright white
GRY='\033[2;37m'           # dim grey
GRN='\033[38;5;114m'       # muted green (success)
RED='\033[38;5;203m'       # red (error)
YLW='\033[38;5;221m'       # yellow (warning)
BLD='\033[1m'
NC='\033[0m'

SCRIPT_START=$(date +%s)

# ── Logging helpers ───────────────────────────────────────────────────────────
_ts()      { date '+%H:%M:%S'; }
_elapsed() { echo $(( $(date +%s) - SCRIPT_START ))s; }

log_step() {   # orange label + white message
    echo -e "${OR}  ●  ${BLD}$1${NC}${WHT}  $2${NC}"
}
log_info() {
    echo -e "${GRY}[$(_ts)]${NC}  ${OR2}▸${NC}  $*"
}
log_ok() {
    echo -e "${GRY}[$(_ts)]${NC}  ${GRN}✔${NC}  $*"
}
log_warn() {
    echo -e "${GRY}[$(_ts)]${NC}  ${YLW}⚠${NC}  ${YLW}$*${NC}"
}
log_die() {
    echo -e "${GRY}[$(_ts)]${NC}  ${RED}✘  $*${NC}" >&2
    divider_thin
    echo -e "${RED}  FAILED  ${GRY}(after $(_elapsed))${NC}" >&2
    exit 1
}

divider() {
    echo -e "${OR3}  $(printf '─%.0s' {1..68})${NC}"
}
divider_thin() {
    echo -e "${OR3}  $(printf '╌%.0s' {1..68})${NC}"
}

banner() {
    echo
    divider
    echo -e "${OR}${BLD}  KitchenGadget8000${NC}${GRY}  ·  ESP32-P4  ·  ESP-IDF v5.4${NC}"
    echo -e "${OR2}  $*${NC}"
    divider
}

section() {   # section title
    echo
    echo -e "${OR}${BLD}  ┌─  $*${NC}"
}

section_done() {
    local elapsed=$(( $(date +%s) - ${SECTION_START:-$(date +%s)} ))
    echo -e "${OR3}  └─${NC}  ${GRN}done${NC}  ${GRY}(${elapsed}s)${NC}"
}
SECTION_START=$(date +%s)
_section_start() { SECTION_START=$(date +%s); }

# ── Locate ESP-IDF ────────────────────────────────────────────────────────────
setup_idf() {
    local idf_path="${IDF_PATH:-$IDF_PATH_DEFAULT}"
    section "ESP-IDF environment"; _section_start
    [[ -f "$idf_path/export.sh" ]] \
        || log_die "ESP-IDF not found at $idf_path  (set IDF_PATH or edit IDF_PATH_DEFAULT)"
    log_info "Sourcing $idf_path/export.sh"
    # shellcheck source=/dev/null
    source "$idf_path/export.sh" 2>&1 \
        | grep -v "^$\|WARNING\|Checking\|Python\|Activating\|Setting\|Detecting\|Establishing\|Shell\|Done\|autocompletion\|outdated\|Go to" \
        || true
    local ver; ver=$(idf.py --version 2>/dev/null | head -1)
    log_ok "Ready  —  $ver  →  target: $TARGET"
    section_done
}

# ── Detect serial port ────────────────────────────────────────────────────────
detect_port() {
    local candidates
    candidates=$(ls /dev/cu.usbmodem* /dev/cu.wchusb* /dev/cu.CP* /dev/cu.SLAB* 2>/dev/null || true)
    [[ -n "$candidates" ]] || log_die "No USB serial device found. Plug in the board and try again."
    local count; count=$(echo "$candidates" | wc -l | tr -d ' ')
    if [[ "$count" -gt 1 ]]; then
        log_warn "Multiple serial ports detected — using the first one (set PORT=<dev> to override):"
        echo "$candidates" | while read -r p; do echo -e "          ${GRY}$p${NC}"; done
    fi
    PORT=$(echo "$candidates" | head -1)
    log_ok "Serial port  →  ${WHT}$PORT${NC}"
}

# ── Check secrets.h ───────────────────────────────────────────────────────────
check_secrets() {
    if [[ ! -f "main/secrets.h" ]]; then
        log_warn "main/secrets.h not found"
        if [[ -f "secrets.h.example" ]]; then
            cp secrets.h.example main/secrets.h
            log_warn "Copied from secrets.h.example — edit it with your WiFi credentials!"
        else
            log_die "secrets.h.example not found. Create main/secrets.h manually."
        fi
    fi
}

# ── Commands ──────────────────────────────────────────────────────────────────
cmd_clean() {
    section "Clean"; _section_start
    log_info "Removing build/ and sdkconfig"
    rm -rf build sdkconfig
    log_ok "Build directory cleared"
    section_done
}

cmd_set_target() {
    section "Set target → $TARGET"; _section_start
    log_info "Running idf.py set-target $TARGET"
    idf.py set-target "$TARGET"
    log_ok "Target configured"
    section_done
}

cmd_build() {
    check_secrets
    section "Build"; _section_start
    if [[ ! -f "sdkconfig" ]]; then
        log_info "No sdkconfig found — running set-target first"
        idf.py set-target "$TARGET" 2>&1 | grep -v "^--\|CMake\|Executing\|Adding\|Running\|Found\|Detecting\|Performing\|Check\|Build files\|Configuring done\|Generating done\|Components\|Component paths" || true
    fi
    log_info "Compiling firmware…"
    idf.py build
    local size; size=$(du -sh build/KitchenGadget8000.bin 2>/dev/null | cut -f1)
    log_ok "Firmware  →  ${WHT}build/KitchenGadget8000.bin${NC}  ${GRY}(${size})${NC}"
    section_done
}

cmd_flash() {
    [[ -f "build/KitchenGadget8000.bin" ]] || log_die "No firmware binary found — run build first."
    [[ -z "${PORT:-}" ]] && detect_port
    section "Flash  →  $PORT"; _section_start
    log_info "Writing firmware to device…"
    idf.py -p "$PORT" flash
    log_ok "Flash complete"
    section_done
}

cmd_monitor() {
    [[ -z "${PORT:-}" ]] && detect_port
    section "Monitor  →  $PORT"; _section_start
    echo -e "  ${GRY}Press ${WHT}Ctrl-]${GRY} or ${WHT}Ctrl-T Ctrl-X${GRY} to exit the monitor${NC}"
    divider_thin
    idf.py -p "$PORT" monitor
}

cmd_spiffs() {
    [[ -d "$RECIPES_DIR" ]] || log_die "Recipes directory '$RECIPES_DIR' not found (run from project root)."
    [[ -z "${PORT:-}" ]] && detect_port
    section "SPIFFS image"; _section_start

    local spiffs_bin="build/spiffs.bin"
    local spiffsgen="$IDF_PATH/components/spiffs/spiffsgen.py"
    [[ -f "$spiffsgen" ]] || log_die "spiffsgen.py not found at $spiffsgen"

    local recipe_count; recipe_count=$(find "$RECIPES_DIR" -name "*.md" | wc -l | tr -d ' ')
    log_info "Building SPIFFS image  →  ${recipe_count} recipes  (${SPIFFS_PARTITION_SIZE} bytes)"
    python3 "$spiffsgen" "$SPIFFS_PARTITION_SIZE" "$RECIPES_DIR" "$spiffs_bin"
    local size; size=$(du -sh "$spiffs_bin" | cut -f1)
    log_ok "Image built  →  ${WHT}$spiffs_bin${NC}  ${GRY}(${size})${NC}"

    log_info "Flashing SPIFFS to $PORT at offset $SPIFFS_PARTITION_OFFSET"
    esptool.py -p "$PORT" write_flash "$SPIFFS_PARTITION_OFFSET" "$spiffs_bin"
    log_ok "SPIFFS partition updated"
    section_done
}

cmd_help() {
    head -23 "$0" | tail -20
    echo
}

# ── Main ──────────────────────────────────────────────────────────────────────
cd "$(dirname "$0")"

# Parse PORT=... override
for arg in "$@"; do
    case "$arg" in PORT=*) export PORT="${arg#PORT=}"; set -- "${@/$arg/}" ;; esac
done

# Default invocation
if [[ $# -eq 0 ]]; then set -- build flash monitor; fi

# Expand shorthand
expanded=()
for cmd in "$@"; do
    case "$cmd" in
        deploy) expanded+=(clean build flash monitor) ;;
        *)      expanded+=("$cmd") ;;
    esac
done
set -- "${expanded[@]}"

# Print banner with the resolved command list
banner "$(echo "$@" | tr ' ' '  →  ')"

# Set up IDF once, then run all requested commands
IDF_READY=0
for cmd in "$@"; do
    case "$cmd" in
        help)       cmd_help ;;
        clean)      [[ $IDF_READY -eq 0 ]] && { setup_idf; IDF_READY=1; }; cmd_clean ;;
        set-target) [[ $IDF_READY -eq 0 ]] && { setup_idf; IDF_READY=1; }; cmd_set_target ;;
        build)      [[ $IDF_READY -eq 0 ]] && { setup_idf; IDF_READY=1; }; cmd_build ;;
        flash)      [[ $IDF_READY -eq 0 ]] && { setup_idf; IDF_READY=1; }; cmd_flash ;;
        monitor)    [[ $IDF_READY -eq 0 ]] && { setup_idf; IDF_READY=1; }; cmd_monitor ;;
        spiffs)     [[ $IDF_READY -eq 0 ]] && { setup_idf; IDF_READY=1; }; cmd_spiffs ;;
        "")         ;; # skip blanks left by PORT= removal
        *)          log_die "Unknown command: '$cmd'  (run ./kg8k.sh help)" ;;
    esac
done

echo
divider
echo -e "${OR}${BLD}  ALL DONE${NC}  ${GRY}(total: $(_elapsed))${NC}"
divider
echo

