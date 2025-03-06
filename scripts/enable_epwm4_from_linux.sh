#!/bin/bash

sudo beagle-pwm-export --pin p9_25
echo 0 > /dev/beagle/pwm/P9_25/duty_cycle
echo 0 > /dev/beagle/pwm/P9_25/period

# The R5 firmware will crash if this is from not done from linux..some kinda of bug
echo 1 > /dev/beagle/pwm/P9_25/enable
