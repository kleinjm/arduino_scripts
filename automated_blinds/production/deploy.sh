#!/bin/bash
# Deploy blinds firmware - compiles and uploads to all blinds in parallel
#
# Usage:
#   ./deploy.sh              # Upload all blinds via OTA
#   ./deploy.sh compile      # Compile only (no upload)
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
SELECTED_BLINDS=()

for arg in "$@"; do
  if [ "$arg" = "compile" ]; then
    COMPILE_ONLY=true
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
echo "Mode: $([ "$COMPILE_ONLY" = true ] && echo "Compile only" || echo "Compile + Upload")"
echo ""

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
