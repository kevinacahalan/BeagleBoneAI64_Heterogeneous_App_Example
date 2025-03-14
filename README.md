![demo_image](R5_LINUX.png)

For this example to work, use beagle firmware with kernel 6.12. Make to to have the latest device tree overlay setup for 6.12.

This example was built off of Fred Eckert's example: https://github.com/FredEckert/bbai64_cortex-r5_example/tree/r5_toggle

Example shows:
1. How to initialize a remoteproc resource table with a working trace log.
2. How to setup boot code to enable the FPU.
3. How to initialize the MPU and cache to run code from DDR memory.
3. How to setup handlers to deal with interrupts.
4. EPWM flashing LED on pin P9_25 for  seconds.
5. Rpmsg talking...Linux calling simple math functions in R5.
4. SPI7 transfers, pins P9_28(cs), P9_31(clk), and P9_30(MOSI).




#### Setup board
1. Grab debian 6.12 firmware. Flash the emmc, and also an sd card.
2. Flash emmc with https://www.beagleboard.org/distros/bbai64-debian-12-9-2025-03-05-minimal-flasher-v6-12-x-ti
3. Flash SD card with https://www.beagleboard.org/distros/bbai64-debian-12-9-2025-03-05-minimal-v6-12-x-ti
4. Power cycle board several times
5. Run `df -h` to ensure you are now booting from your SD card
6. Run this to get latest DT source among other important things `sudo apt update ; sudo apt-get dist-upgrade -y`
7. Add the overlay k3-j721e-beagleboneai64-pwm-epwm4-p9_25.dtbo to extlinux.conf
8. Power cycle the boars several times
9. Verify overlay is loaded `sudo beagle-version | grep UBOOT`
10. Check and make sure P9_25 is muxed to ehrpwm4_b `sudo ./scripts/show-pins.pl`



#### Setup and compile ti RTOS SDK
Make sure to use **Debian12** for building. It will make your life easier. On windows use WSL Debian 12.

1.  Run `wget https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-bA0wfI4X2g/10.00.00.05/ti-processor-sdk-rtos-j721e-evm-10_00_00_05.tar.gz`
2.  Use `tar -xzf ti-processor-sdk-rtos-j721e-evm-10_00_00_05.tar.gz` to decompress
3.  Place decompressed sdk folder in ~/ti/
4.  Run `sudo apt install libtinfo5`  needed for ti sdk build

##### Build sdk/pdk:
1. Change to the directory `~/ti/ti-processor-sdk-rtos-j721e-evm-10_00_00_05/pdk_jacinto_10_00_00_27/packages` using `cd`.
2. 
    From the `packages` directory within the Jacinto PDK, run `make -s all_libs BUILD_PROFILE=release` to build the release libraries, and run `make -s all_libs BUILD_PROFILE=debug` to build the debug libraries.

    OPTIONAL:
    If you want to build the TI examples you may have to direct TI to also use the system compiler for some reason... You will also need to install the `mono-complete` package.

    For gcc arm to work install `arm-none-eabi-gcc`. Next setup the environment variables that TI uses to find the compiler. 
    - `export TOOLCHAIN_PATH_GCC=/usr`
    - `export TOOLCHAIN_PATH_GCC_ARCH64=/usr`
    - `export GCC_ARCH64_BIN_PREFIX=arm-none-eabi`

    Run `make all_examples CORE=mcu2_0` to compile examples. NOTE that `make all_examples` does not actually compile all examples, it only compiles the examples for mcu1_0. Some examples do not work with mcu1_0.

3.  The library release .a files are located at `~/ti/ti-processor-sdk-rtos-j721e-evm-10_00_00_05/pdk_jacinto_10_00_00_27/packages/ti/[LIBRARY]/lib/j721e/r5f/release`, and the debug files are at `~/ti/ti-processor-sdk-rtos-j721e-evm-10_00_00_05/pdk_jacinto_10_00_00_27/packages/ti/[LIBRARY]/lib/j721e/r5f/release/debug`.
    
    These ".a" files have an unusual extension, ".aer5f".


#### To build R5 code:
- Build TI sdk and place it at correct location as directed above.
- install `gcc-arm-none-eabi`
- Run `make` from folder with R5 makefile

#### To build Linux code:
```bash
sudo apt-get update
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
sudo dpkg --add-architecture arm64
sudo apt-get update
sudo apt-get install -y libgpiod-dev:arm64

make CROSSCOMPILE=true # default is to not cross compile
```

#### To build everything at once:
`[SCRIPT_DIR]/build_script.sh --beaglebone`

#### To build and copy to board:
`[SCRIPT_DIR]/compile_and_push.sh --ip [BEAGLE_IP]`

#### To run
`sudo [SCRIPT_DIR]/debug_run.sh`

### Device tree info
- Copy the overlays from our `custom_overlays/` folder to `/opt/source/dtb-6.12-Beagle/src/arm64/overlays` on the board.
- To compile overlays source on board run `sudo make clean` and `sudo make` from `/opt/source/dtb-6.12-Beagle`
- To install the overlays, from `/opt/source/dtb-6.12-Beagle` run `sudo make arm64_install`
- On the board, in the config file`/boot/firmware/extlinux/extlinux.conf`, replace the line `#fdtoverlays /overlays/<file>.dtbo` with `fdtoverlays /overlays/k3-j721e-beagleboneai64-pwm-epwm4-p9_25.dtbo /overlays/k3-j721e-beagleboneai64-spi-mcspi7-cs0.dtbo`
- Connect to debug uart and have fun. If things go bad you'll be down for some trial and error. You may end up re-flashing your board several times to recover it...

If your boot works correctly, you will see the following in your debug uart output on boot:
```
Retrieving file: /ti/k3-j721e-beagleboneai64.dtb
Retrieving file: /overlays/k3-j721e-beagleboneai64-pwm-epwm4-p9_25.dtbo
Retrieving file: /overlays/k3-j721e-beagleboneai64-spi-mcspi7-cs0.dtbo
## Flattened Device Tree blob at 88000000
   Booting using the fdt blob at 0x88000000
Working FDT set to 88000000
   Loading Device Tree to 000000008ffde000, end 000000008fffffff ... OK
Working FDT set to 8ffde000

```
#### Resources:

- You could make use of https://www.ti.com/tool/download/SYSCONFIG to figure out pin muxing when making your own overlays.
- v6.12.x-Beagle/src/arm64/ti/k3-j721e-main.dtsi is a very important file. If you want to use
some IO device that is not defined in here, you will have to dig into the TDA4VM TRM and write your equivalent fanciness in your own overlay. Defining power-domains, clocks, so on. For example for eqep...

- To figure out which SoC pad numbers go with which BB header pins, look at columns A and B
in the following spreadsheet. To figure out mux modes, look at row 10.
https://drive.google.com/file/d/15NLaUeMBy-iT8s6rFrP4Esf0Qh57T4xu/view?pli=1

- To figure out the addresses of SoC pads, look at table "Table 5-125. Pin Multiplexing" in the TDA4VM Processors datasheet
 https://www.ti.com/lit/ds/symlink/tda4vm.pdf?ts=1741890214437&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FTDA4VM
 

#### Walkthrough of process to figure out muxing

From the spreadsheet we see that pad AC22 is the first pad on BB header pin P9_22. AC22 is thus known as P9_22a. The alternative
pad, U29, is known as P9_22b. From Looking at the TDA4VM datasheet, we see that AC22(aka P9_22a) has the address 0x00011C09C. For 
the J721E_IOPAD() pin-mux macro, we need the bottom 24bits of the address, so 0x09c, or 0x9c. 

To mux AC22(aka P9_22a) to be SPI6_CLK, as figured out from the spreadsheet, we need to set AC22 to mux-mode 4. And since
a SPI clock is an output signal, the pin should be put set to PIN_OUTPUT mode.
So `J721E_IOPAD(0x9c, PIN_OUTPUT, 4)`

We will also need to disable the second SoC pad that shares the same BB header pin. The SoC pad known as pin P9_22b, or 
pad U29. Using the same process as walked through with pad AC22, you get the following:
`J721E_IOPAD(0x170, PIN_DISABLE, 7)`

So to mux SPI6_CLK on BB pin P9_22:
J721E_IOPAD(0x9c, PIN_OUTPUT, 4)
J721E_IOPAD(0x170, PIN_DISABLE, 7)

### Useful Commands

| Command                                           | Description                                  |
|---------------------------------------------------|----------------------------------------------|
| `sudo k3conf show clocks`                         | Displays all clock information.              |
| `sudo k3conf dump clocks <device ID>`             | Check status of clocks for device.           |
| `dmesg \| grep -i "reserved mem"`                 | Shows memory mapping information from logs.  |
| `sudo cat /proc/iomem`                            | More memory mapping info.                    |
| `sudo beagle-version \| grep UBOOT`               | Displays loaded device tree overlays.        |
| `ls /sys/devices/platform/bus@100000/`            | Devices that can now be used from linux???   |
| `dtc -I fs /sys/firmware/devicetree/base > dt.txt`| For dt debugging                 |

### Useful Links

#### Documentation
- **[Pin Mappings](https://drive.google.com/file/d/15NLaUeMBy-iT8s6rFrP4Esf0Qh57T4xu/view)**: Pin mapping spreadsheet.
- **[Device and Clock IDs](https://software-dl.ti.com/tisci/esd/latest/5_soc_doc/j721e/clocks.html)**: TI documentation detailing device and clock IDs.
- **[TDA4VM Processor Page](https://www.ti.com/product/TDA4VM)**: Official TI page for the TDA4VM processor.
- **[TDA4VM TRM](https://www.ti.com/lit/zip/spruil1)**: Technical Reference Manual for the TDA4VM.
- **[Cortex R5 TRM](https://developer.arm.com/documentation/ddi0460/d/?lang=en)**: Technical Reference Manual for the Cortex R5.
- **[TI RTOS SDK Documentation](https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-jacinto7/10_00_00_05/exports/docs/psdk_rtos/docs/user_guide/overview.html#)**: Overview of the TI RTOS SDK.
- **[TI PDK Documentation](https://software-dl.ti.com/jacinto7/esd/processor-sdk-rtos-jacinto7/10_00_00_05/exports/docs/pdk_jacinto_10_01_00_25/docs/pdk_introduction.html#Documentation)**: Links to API guide and user guide

#### Tutorials
- **[Flashing eMMC](https://forum.beagleboard.org/t/ai-64-how-to-flash-emmc/32384)**: Forum guide on how to flash the eMMC on the BeagleBone AI-64.
- **[More on Flashing](https://forum.beagleboard.org/t/tda4vm-debian-11-3-flasher-does-not-produce-a-functional-emmc/33288)**: Additional forum discussion on flashing-related issues.

#### Debugging

- **[K3 OCD Guide](https://nmenon.github.io/k3ocd/)**: A guide on using OpenOCD for debugging on the BeagleBone AI-64.
- **[Debugging Options Forum Thread](https://forum.beagleboard.org/t/debugging-options-for-bbai64/33583/5)**: A discussion on various debugging options available for the BeagleBone AI-64.
- **[YouTube Debugging Tutorial](https://www.youtube.com/watch?v=n3u3QgnAvV8)**: A video tutorial covering debugging techniques.
- **[OpenOCD Config Issue](https://git.beagleboard.org/beagleboard/beaglebone-ai-64/-/issues/31)**: Issue tracker for OpenOCD configuration specific to the BeagleBone AI-64.

#### Work by others:
- This guy is doing PRU and DSP stuff
https://github.com/loic-fejoz/beaglebone-ai64-tutorial

- Using SDK10 with kernel 6.6 for R5 (These guys are using the TI build system)
https://forum.beagleboard.org/t/bbai64-now-can-use-ti-sdk10-0-and-debug-r5/39459

- Zephyr (currently work in progress)
https://docs.zephyrproject.org/latest/boards/beagle/beaglebone_ai64/doc/index.html

#### Other
- **[The PRU Development Kit](https://git.ti.com/cgit/pru-software-support-package/pru-software-support-package/)**: TI’s PRU software support package for development.
- **[Beagle Images](https://www.beagleboard.org/distros)**: Release images.
- **[Random Beagle Images](https://rcn-ee.com/rootfs/)**: Random images.


