#!/bin/bash

SCRIPT_DIR="$(dirname "$0")"
ORIGINAL_USER=$SUDO_USER

echo ""
echo "Starting up LINUX_SIDE for development!"
echo ""

# Initialize variables
COMPILE=false
KERNEL="6"
J7_MAIN_R5F0_0_rproc_number=$($SCRIPT_DIR/get_remoteproc_number.sh j7-main-r5f0_0)

# Function to print the help message
print_help() {
    echo "Usage: $0 --kernel-5 | --kernel-6 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --kernel-5         Use kernel 5 options."
    echo "  --compile          Attempt to compile."
    echo "  help               Display this help message."
    echo ""
    exit 0
}

# Parse command-line options
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --compile)
            COMPILE=true
            echo "> Attempting compile"
            shift
            ;;
        --kernel-5)
            KERNEL=5
            echo "> Using kernel 5"
            shift
            ;;
        help)
            print_help
            ;;
        *)
            echo "Unknown option: $1"
            print_help
            ;;
    esac
done

# Special things for kernel 5
if [ "$KERNEL" = 5 ]; then
    # PLACEHOLDER
    echo "Kernel 5"
fi

# Special things for kernel 6
if [ "$KERNEL" = 6 ]; then
    # PLACEHOLDER
    echo "Kernel 6"
fi

# Compile
if [ "$COMPILE" = true ]; then
    sudo -u "$ORIGINAL_USER" bash "$SCRIPT_DIR/build_script.sh" --beaglebone || { echo "Could not run build_script.sh for some reason"; exit 1; }
fi

# Make it so that R5 firmware does not crash and go to DAbt handler (get around bug)
$SCRIPT_DIR/enable_epwm4_from_linux.sh

# Proceed with the main functionality
SESSION_NAME="LINUX_AND_R5"

# Check if the session already exists and kill it
if tmux ls | grep -q "^$SESSION_NAME:"; then
    echo "Killing old session: $SESSION_NAME"
    tmux kill-session -t "$SESSION_NAME"
fi

# Start tmux session and configure it to keep panes open
tmux new-session -d -s "$SESSION_NAME" -n main
tmux setw -t "$SESSION_NAME:main" remain-on-exit on

# Pane 1: Start R5 firmware with sudo in the top pane
tmux send-keys -t "$SESSION_NAME:main.0" "sudo $SCRIPT_DIR/start_firmware_over_remoteproc.sh $J7_MAIN_R5F0_0_rproc_number $SCRIPT_DIR/../build/R5_0/r5f_r5f0_0.elf || echo 'Failed to start R5 firmware'" C-m
tmux split-window -v -t "$SESSION_NAME:main"

# Add 1-second delay before starting the next command
sleep 1

# Pane 2: Start trace script with sudo in the second pane
tmux send-keys -t "$SESSION_NAME:main.1" "sudo $SCRIPT_DIR/print_trace0.py $J7_MAIN_R5F0_0_rproc_number" C-m

# Pane 3: Start Linux side in the bottom pane with sudo
tmux split-window -h -t "$SESSION_NAME:main.1"
tmux send-keys -t "$SESSION_NAME:main.2" "sudo $SCRIPT_DIR/../build/linux/LINUX_SIDE_aarch64 || echo 'Failed to start linux code'" C-m

# Focus on the top pane (optional)
tmux select-pane -t "$SESSION_NAME:main.0"

# Attach to the tmux session
tmux attach -t "$SESSION_NAME"

echo ""
echo "--- Both Linux and R5 should be started and working"