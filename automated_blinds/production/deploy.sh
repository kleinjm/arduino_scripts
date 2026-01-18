#!/bin/bash
# Deploy blinds firmware - compiles and uploads to all blinds in parallel
#
# Usage:
#   ./deploy.sh              # Upload all blinds via OTA
#   ./deploy.sh compile      # Compile only (no upload)
#   ./deploy.sh generate     # Generate resolved YAML files to generated/
#   ./deploy.sh blind-right-1 blind-left-2  # Upload specific blinds only

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# All blind configs
ALL_BLINDS=(
  "blind-right-1"
  "blind-right-2"
  "blind-right-3"
  "blind-left-1"
  "blind-left-2"
)

# Parse arguments
COMPILE_ONLY=false
GENERATE_ONLY=false
SELECTED_BLINDS=()

for arg in "$@"; do
  if [ "$arg" = "compile" ]; then
    COMPILE_ONLY=true
  elif [ "$arg" = "generate" ]; then
    GENERATE_ONLY=true
  else
    SELECTED_BLINDS+=("$arg")
  fi
done

# If no specific blinds selected, use all
if [ ${#SELECTED_BLINDS[@]} -eq 0 ]; then
  SELECTED_BLINDS=("${ALL_BLINDS[@]}")
fi

echo "==================================="
echo "Blind Deployment Script"
echo "==================================="
echo "Blinds: ${SELECTED_BLINDS[*]}"
if [ "$GENERATE_ONLY" = true ]; then
  echo "Mode: Generate resolved YAML files"
elif [ "$COMPILE_ONLY" = true ]; then
  echo "Mode: Compile only"
else
  echo "Mode: Compile + Upload"
fi
echo ""

# Handle generate mode
if [ "$GENERATE_ONLY" = true ]; then
  mkdir -p generated
  for blind in "${SELECTED_BLINDS[@]}"; do
    config="${blind}.yaml"
    output="generated/${blind}.yaml"

    if [ ! -f "$config" ]; then
      echo "[$blind] ERROR: Config file not found: $config"
      continue
    fi

    echo "[$blind] Generating resolved config..."
    if esphome config "$config" 2>/dev/null | grep -v "^INFO" > "$output"; then
      # Fix absolute paths to relative (for HA ESPHome compatibility)
      sed -i '' 's|/Users/.*/production/||g' "$output"

      # Replace verbose wifi section with simple version
      python3 - "$output" << 'PYTHON_SCRIPT'
import sys
import re

with open(sys.argv[1], 'r') as f:
    content = f.read()

# Replace wifi section (everything from 'wifi:' up to but not including 'api:')
wifi_simple = """wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  power_save_mode: NONE

"""
content = re.sub(r'^wifi:.*?(?=^api:)', wifi_simple, content, flags=re.MULTILINE | re.DOTALL)

with open(sys.argv[1], 'w') as f:
    f.write(content)
PYTHON_SCRIPT

      echo "[$blind] Generated: $output"
    else
      echo "[$blind] FAILED to generate"
    fi
  done

  echo ""
  echo "==================================="
  echo "Generated files in: $(pwd)/generated/"
  ls -la generated/
  echo "==================================="
  exit 0
fi

# Function to deploy a single blind
deploy_blind() {
  local blind=$1
  local config="${blind}.yaml"

  if [ ! -f "$config" ]; then
    echo "[$blind] ERROR: Config file not found: $config"
    return 1
  fi

  echo "[$blind] Compiling..."
  if ! esphome compile "$config" > /tmp/esphome_${blind}.log 2>&1; then
    echo "[$blind] COMPILE FAILED - see /tmp/esphome_${blind}.log"
    return 1
  fi
  echo "[$blind] Compile OK"

  if [ "$COMPILE_ONLY" = true ]; then
    return 0
  fi

  echo "[$blind] Uploading via OTA..."
  if ! esphome upload "$config" --device "${blind}.local" >> /tmp/esphome_${blind}.log 2>&1; then
    echo "[$blind] UPLOAD FAILED - device may be offline. See /tmp/esphome_${blind}.log"
    return 1
  fi
  echo "[$blind] Upload OK"
}

# Export function and variables for parallel execution
export -f deploy_blind
export COMPILE_ONLY
export SCRIPT_DIR

# Run deployments in parallel
echo "Starting parallel deployment..."
echo ""

# Use GNU parallel if available, otherwise fall back to background jobs
if command -v parallel &> /dev/null; then
  printf '%s\n' "${SELECTED_BLINDS[@]}" | parallel -j 5 deploy_blind {}
  EXIT_CODE=$?
else
  # Fallback: run in background with wait
  pids=()
  for blind in "${SELECTED_BLINDS[@]}"; do
    deploy_blind "$blind" &
    pids+=($!)
  done

  # Wait for all and collect exit codes
  EXIT_CODE=0
  for pid in "${pids[@]}"; do
    if ! wait $pid; then
      EXIT_CODE=1
    fi
  done
fi

echo ""
echo "==================================="
if [ $EXIT_CODE -eq 0 ]; then
  echo "All deployments completed successfully!"
else
  echo "Some deployments failed. Check logs in /tmp/esphome_*.log"
fi
echo "==================================="

exit $EXIT_CODE
