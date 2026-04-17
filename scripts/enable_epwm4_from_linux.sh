#!/bin/bash

set -e

PWM_DIR="/dev/beagle/pwm/P9_25"

# Export PWM pin P9_25 and initialize it to prevent the R5 firmware from
# hitting the EPWM startup fault when Linux has not touched the block first.
if ! command -v beagle-pwm-export >/dev/null 2>&1; then
	echo "[error] beagle-pwm-export is not installed or not in PATH" >&2
	exit 1
fi

beagle-pwm-export --pin p9_25

if [ ! -d "$PWM_DIR" ]; then
	echo "[error] PWM path not found after export: $PWM_DIR" >&2
	exit 1
fi

# Disable first if already active, then program a known-valid non-zero period
# and duty cycle before enabling.
echo 0 | tee "$PWM_DIR/enable" > /dev/null 2>&1 || true
echo 200000 | tee "$PWM_DIR/period" > /dev/null
echo 2000 | tee "$PWM_DIR/duty_cycle" > /dev/null
echo 1 | tee "$PWM_DIR/enable" > /dev/null
