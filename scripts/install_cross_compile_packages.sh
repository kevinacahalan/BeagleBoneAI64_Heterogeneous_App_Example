#!/bin/bash

# Script to install necessary packages for cross-compiling to ARMv8

# Update package lists
echo "Updating package lists..."
sudo apt-get update

# Install the ARM64 cross-compiler toolchain
echo "Installing ARM64 cross-compiler toolchain..."
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Enable multiarch to install ARM64 libraries on x86 system
echo "Enabling multiarch support for ARM64..."
sudo dpkg --add-architecture arm64
sudo apt-get update

# Install necessary ARM64 libraries (libgpiod and dependencies)
echo "Installing ARM64 libraries (libgpiod and dependencies)..."
sudo apt-get install -y libgpiod-dev:arm64

echo "Installation complete! Your system is now set up for cross-compilation to ARMv8."

