# Files Relating to the Software of the TURNER Console

This folder contains the project’s software components, including firmware, logging modules, desktop tools, storage utilities, and log processing scripts.

- [Console](https://github.com/TUDSSL/condenar/tree/master/Software/Console): CMake project for the console firmware. Only needs the arm compiler toolchain to run.

- [LogProcessing](https://github.com/TUDSSL/condenar/tree/master/Software/LogProcessing): Cleans and trims raw log files in the ‘RAW’ folder, links each to its experiment variation, prepares the processed data as a pandas DataFrame, and calculates and plots the user interaction percentage.`
  
- [Logger](https://github.com/TUDSSL/condenar/tree/master/Software/Logger): Logging module firmware for the ESP32 MCU.
  
- [PCTerminal](https://github.com/TUDSSL/condenar/tree/master/Software/PCTerminal): C# project that compiles a desktop application (Windows only) useful for debugging purposes.
  
- [StorageManager](https://github.com/TUDSSL/condenar/tree/master/Software/StorageManager): C# project that compiles a small console application that allows the upload of files to the console.
