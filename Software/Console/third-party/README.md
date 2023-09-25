# Third party software/libraries
## Apollo4 SDK
Can be downloaded from the Ambiq webpage: https://ambiq.com/apollo4/
Version 4.3.0 was used to compile the firmware in the prestudy and main study.

## DOOM
DOOM source code, based on the nrf52840 port (https://github.com/next-hack/nRF52840Doom). This code is itself based on the shareware version of the original DOOM source code.
The code in the 'doom/' folder has been further modified to remove unused features (sound, networking, ...), make compatible with our architecture and optimize it even more.
The original source has a GPL2.0 license.

## Segger RTT
Source code downloaded directly https://www.segger.com/downloads/jlink/.
Its license can be found in the '/third-party/Segger_RTT/SEGGER_RTT.h' file.
