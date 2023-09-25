# Console benchmarks

## Procedure and data collection
Power measurements were taken with a SDM3055 multimeter in series with the respective power rails.
Everything else was measured using the logging module of the console (through the logs it generates).

## Files in directory
- CheckpointingPerformance.xlsx: This file contains the theoretical calculations used to estimate the overhead that checkpointing creates. It is compared and graphed against FPS and the FPS limit set in the firmware.
- CrankTests.xlsx: This file contains multiple cranking output power tests carried out through the different iterations of the cranking module. Each experiment is on its own sheet, with the experiment setup described on top.
- SSD1963_PowerConsumption.xlsx: Contains power usage measurements taken directly from the power rails feeding the SSD1963 IC. Many different parameter combinations (PLL enabled, clock input frequency, output refresh frequency) are tested.
