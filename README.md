HEADS UP, THIS REPO IS JUST A FORK OF AN R5 example for now. I have not dug in yet.


For this example to work, use beagle firmware with kernel 5.10. Also, make sure to use this sketchy device tree setup...If you do not, even your GPIO may not work...

https://github.com/kevinacahalan/bbai64_sketch_dt_setup_5_10



This example was built off of Fred Eckert's example: https://github.com/FredEckert/bbai64_cortex-r5_example/tree/r5_toggle

1. Grab the TI library from https://www.ti.com/tool/download/PROCESSOR-SDK-LINUX-SK-TDA4VM/09.01.00.06 
2. Use 'tar -xzf download_name' to decompress the folder and place it in ~/ti/
3. Run the installer and set the install location in ~/ti/. The TI libraries can only be compiled on x86, so they cannot be compiled on the BeagleBone.
4. Compile the program on the Beagle Board using the make command. It will not compile on Arch-based Linux systems. Before compiling on the Beagle Board, move the TI CSL library files to the Beagle Board.
5. To change which pin is toggled, get pin information in this spreadsheet https://drive.google.com/file/d/15NLaUeMBy-iT8s6rFrP4Esf0Qh57T4xu/view?usp=sharing looking at column K. Example: pin9-14 is at GPIO_93.

### Random Notes:
* Use Debian as compile environment
* https://forum.beagleboard.org/t/show-pins-with-ai-64-support-added/32489 This script may be helpful.

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

pin 9_14 will be toggling at about 22.7Hz.
 




### Random helpful resources:
- This guy is doing PRU and DSP stuff
https://github.com/loic-fejoz/beaglebone-ai64-tutorial

- Using SDK10 with kernel 6.6 for R5 (These guys are using the TI build system)
https://forum.beagleboard.org/t/bbai64-now-can-use-ti-sdk10-0-and-debug-r5/39459

- Zephyr (currently work in progress)
https://docs.zephyrproject.org/latest/boards/beagle/beaglebone_ai64/doc/index.html

- TDA4VM TRM
https://www.ti.com/lit/zip/spruil1

- Cortex R5 TRM
https://developer.arm.com/documentation/ddi0460/d/?lang=en

- BeagleBone Images
https://rcn-ee.com/rootfs/

- Debugging
    - https://nmenon.github.io/k3ocd/
    - https://forum.beagleboard.org/t/debugging-options-for-bbai64/33583/5
    - https://www.youtube.com/watch?v=n3u3QgnAvV8
    - OpenOCD config https://git.beagleboard.org/beagleboard/beaglebone-ai-64/-/issues/31
    - All sorts of Beagle images
    https://rcn-ee.com/rootfs/