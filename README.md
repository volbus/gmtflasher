# gmtflasher
StLinkV2 flashing tool for STM8

This is a new programming tool for STM8 MCUs, to be used with ST's STLinkV2 programmer.
Its main purpose is to offer the flexibility stm8flash does not have for programming STM8 MCUs via SWIM
in Linux. It was developed for my personal needs, so now, if anyone has the same needs, is free to use it.
It can only be used with StLinkV2 tool, and the communication with it was taken from stm8flash, with thanks
to it's developers - an official release of the communication protocol from ST is not available, so this
might not be the best implementation.

It has a simple xml device definition file, that can be easily edited to fit other MCUs, if not already defined.

When compiling code the full device definition (option bytes, eeprom and flash) can be assembled into one
hex file, and selectively programmed into the MCU. This has the advantage of a single hex file versus separate
files for option bytes, eeprom and flash memory.
It also skips by default pages that already have the same content, this greatly reducing the number of write
cycles during development. Some STM8 devices have an endurance of only 100 erase/write cycles according to
datasheet, which can easily be reached during development.

Support for other proprietary platforms (like Windows or MAC) will never be provided.

The compilation being very simple, a make file was not needed, so, to install just run install.sh as sudo.
Make sure libxml2-dev and libusb-1.0-dev are installed as dependencies.

Usage: `gmtflasher [option] -u <mcu> <command> [<args>] [<ihex_file>]`

For usage details call the program with the -h optiion to print help:
  `gmtflasher -h`
