# Cloning
This is a Zephyr based repository.  To clone this repository properly use the `west` tool. To install west you will first need Python3.

Install `west` using `pip3`:
```
# Linux
pip3 install --user -U west

# macOS (Terminal) and Windows (cmd.exe)
pip3 install -U west
```

Once `west` is installed, clone this repository by:
```
west init -m git@git.devops.lairdtech.com:cp_cellular/newcastle_firmware.git
west update
```

# Preparing to Build

If this is your first time working with a Zephyr project on your PC you should follow the [Zephyr getting started guide](https://docs.zephyrproject.org/latest/getting_started/index.html#) to install all the tools.

It is recommended to build this firmware with the [GNU Arm Embedded Toolchain: 8-2019-q3-update](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads).