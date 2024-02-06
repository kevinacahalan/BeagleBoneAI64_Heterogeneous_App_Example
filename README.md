# Cortex-R5 GPIO0_93 pin 9_14 toggle for Beaglebone AI-64

Cortex-R5 example on Beaglebone AI-64.

Example shows:
1. How to initialize a remoteproc resource table with a working trace log.
2. How to setup boot code to enable the FPU
3. How to initialize the MPU and cache to run code from DDR memory.
4. How to map the entire 4GB address space to enable access to peripherials.

To compile this example, you need the arm-none-eabi gcc toolchain installed on your system.

https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/downloads

If you were compiling this directly on the Beaglebone AI-64, you can uncomment the commented lines in the Makefile to run the example. If everything worked you should see pin 9_14 toggling at 1MHz and a status message printed to the remoteproc trace log.

Otherwise, upload `test.elf` to `/lib/firmware/` on the AI-64, then type in the following commands as root:
```
echo stop > /sys/class/remoteproc/remoteproc18/state
echo test.elf > /sys/class/remoteproc/remoteproc18/firmware
echo start > /sys/class/remoteproc/remoteproc18/state
cat /sys/kernel/debug/remoteproc/remoteproc18/trace0
```

You should now see:
```
r5_toggle (Language C)

started
```

pin 9_14 will be toggling at 1MHz.
 
