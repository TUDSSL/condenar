This folder contains the firmware that gets uploaded to the console.

# Build instructions
- Create a directory ('build') for example in this directory.
- run 'cmake ..'
- This might take a while, and if it fails, it will let you know if you are missing any required executables in your PATH environment variable. If it says you are missing GCC, you can download the latest version of the Arm GCC compiler here (Built with 23.0 in the prestudy): https://developer.arm.com/downloads/-/. After installing it, add the installation directory to your PATH variable.
- Run 'make' to build the executable. 
- If successful, the output file will be located in 'build/apps/console/'

You can upload this file to the console with your debugger/probe of choice.
After uploading a new build, it is usually recommended to go to the settings menu and clean the local storage of the console if you have added/removed settings or other storage-related functions.

# Project structure
- games/Gameboy/ and games/DOOM/ contain the source code for the DOOM port and both Game Boy emulators.
- thrid-party/ has a copy of the Ambiq SDK, the source code of DOOM itself and the Segger RTT libraries (Optional, only useful if using a J-Link probe)
- Pin definitions and other configs are placed in platform/
- lib/ contains several system modules:
  - DownloadMode: Handles the upload of new data to the external flash of the console. You can use the "StorageManager" program (in /Software/) to upload pictures, game assets, and any other kind of file to the console.
  - Kernel: All aspects of the kernel are split into its own separate source file within the 'Helpers' directory. To use Kernel call, you only need to include the "Kernel.h" header file. More detailed information can be found in https://github.com/TUDSSL/condenar/blob/master/Software/Console/libs/Kernel/README.md
  - MainMenu: Small apps that renders the main menu. It is needed, as the Kernel expects it to exist in the build and it is the default idle application.
  - Settings: Application that lets the user view and modify settings on the go, without recompiling the firmware. The system will work without this (This is just the UI), but the user won't be able to change any settings.
  - SMART_Crank: Independent application that runs in the MCU of the crank module. It needs the NXP SDK/IDE (V1.0) to be installed to build it.
  - TestMode: A small application that shows the real-time status of all the inputs of the system.
  - TurnOff: Small application that requests the kernel to go into low-power mode. Optional, not used in the preliminary study.


 # Additional info
 If you need to make a build from scratch (not incremental), run 'make clean' and then 'make'.
 
 If you want to use more than 1 core when compiling, append ' -jX' to the 'make' command, where X is the number of threads you want to use.
