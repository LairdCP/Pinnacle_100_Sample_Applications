# Cloning

This is a Zephyr based repository. To clone this repository properly use the `west` tool. To install west you will first need Python3.

Install `west` using `pip3`:

```
# Linux
pip3 install --user -U west

# macOS (Terminal) and Windows (cmd.exe)
pip3 install -U west
```

Once `west` is installed, clone this repository by:

```
west init -m git@git.devops.rfpros.com:cp_cellular/newcastle_firmware.git
west update
```

# Preparing to Build

If this is your first time working with a Zephyr project on your PC you should follow the [Zephyr getting started guide](https://docs.zephyrproject.org/latest/getting_started/index.html#) to install all the tools.

It is recommended to build this firmware with the [GNU Arm Embedded Toolchain: 8-2019-q3-update](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads).

# Build

From the directory where you issued the `west init` and `west update` commands you can use the following command to build an app:

## Build `lte_console` app

```
# Windows
west build -b pinnacle_100_dvk -d pinnacle_firmware\build\apps\lte_console pinnacle_firmware\apps\lte_console -- -D BOARD_ROOT=%cd%\pinnacle_firmware

# Linux and macOS
west build -b pinnacle_100_dvk -d pinnacle_firmware/build/apps/lte_console pinnacle_firmware/apps/lte_console -- -D BOARD_ROOT=$PWD/pinnacle_firmware
```
