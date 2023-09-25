# Power module PCBs

## Crank module
The crank module has gone through several iteration. The final one (the one used in the studies) is the "CrankModuleSMARTEST" one.

- CrankModuleV1: First version of the crank module. Not MCU/MPPT. Unidirectional.
- CrankModuleV2: Second version of the crank module. Efficiency improvements on low capacitor voltages. Not MCU/MPPT. Unidirectional.
- CrankModuleV2_SMART: CrankModuleV2 improved to add MPPT support. Unidirectional.
- CrankModuleV2_SMART: Builds upon CrankModuleV2_SMART to allow for custom MPPT algorithms, bidirectional cranking and feedback to the main console processor.

## Solar module

- SolarModule: Based on the CrankModuleV1 architecture, features a high-efficiency MPPT converter. Compatible with solar cells