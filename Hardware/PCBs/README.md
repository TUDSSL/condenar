# PCBs

Each folder here is its own different PCB:

- [CPUModule](https://github.com/TUDSSL/condenar/tree/master/Hardware/PCBs/CPUModule): Module that hold the main processor, flash and display controller.
  
- [Logger](https://github.com/TUDSSL/condenar/tree/master/Hardware/PCBs/Logger): Optional module that can be used to record experiment data without affecting the system.
- [Mainboard](https://github.com/TUDSSL/condenar/tree/master/Hardware/PCBs/Mainboard): Main PCB, where all the different modules are connected.
- [PowerModules](https://github.com/TUDSSL/condenar/tree/master/Hardware/PCBs/PowerModules): The different power harvesting modules that generate power for the console. Up to two of them can be connected at the same time to the console.
- [Misc/Apo4devkit2COM50](https://github.com/TUDSSL/condenar/tree/master/Hardware/PCBs/Misc/Apo4devkit2COM50): Small shield PCB designed to test and measure the energy consumption of the SSD1963 IC
- [Misc/LightSensingModule](https://github.com/TUDSSL/condenar/tree/master/Hardware/PCBs/Misc/LightSensingModule): Small PCB that hosts a light sensor IC (OPT3001 or compatible from TI) to measure the ambient light.

Most PCB projects can be opened with Altium Designer (https://www.altium.com/altium-designer) directly. The CPUModule one uses KiCad (https://www.kicad.org/).
