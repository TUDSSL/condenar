# Console Kernel Functions

This is an overview of functions defined in the `Kernel.h` header file, outlining their purpose and usage.

For general app creation/porting, ONLY this file should be included under normal scenarios. (Maybe also the Helpers/Kernel_Draw.h if you need more rendering utilities)

## App Registration and Management

### `int kernel_main(void)`
Kernel entry point. This function never returns and serves as the main entry point for the console kernel.

### `void k_registerApp(k_app* app)`
Registers an app with the kernel. Should only be called from `k_registerApps` in `k_apps.c`. The `k_app` object should always be valid, even after exiting this function.

### `k_app* k_getAppByName(char* appName)`
Gets an app from the registered list by its name. Returns `NULL` if not found.

### `k_app** k_getAppList(uint32_t* appCountOut)`
Gets the list of registered apps and provides the count of registered apps.

### `k_app* k_getNextApp()`
Gets the next app scheduled to be run.

### `k_app* k_getCurrentApp()`
Gets the app that is currently running.

### `void k_setNextApp(k_app* _nextAppToRun)`
Sets the next app scheduled to be run.

### `void k_setShouldCurrentAppExit(bool newState)`
Sets whether the current app should exit.

### `bool k_getShouldCurrentAppExit()`
Gets the exit status of the current app.

### `uint32_t k_getCurrentFrame()`
Gets the current frame number.

### `bool k_isRecoveringFromACheckpoint()`
Checks if the system is recovering from a checkpoint.

### `void k_ForceRestart()`
Forces a restart of the system.

### `bool k_IsBeingCranked()`
Checks if the crank is being turned.

### `bool k_GetCrankDirection()`
Gets the direction of the crank: `true` for clockwise, `false` for counter-clockwise.

## Storage and Memory

### `void k_WriteToAppStorage(k_app* app, uint32_t storageOffset, uint8_t* data, uint32_t size)`
Writes a chunk of bytes to the app storage of a certain app.

### `uint8_t* k_GetStorageReadOnlyAddressForApp(k_app* app, uint32_t* sizeOut)`
Gets a READ-ONLY pointer to the app storage of a certain app. Returns `NULL` if not available.

### `uint8_t* k_getHeapPtr()`
Gets a pointer to the app heap.

### `uint32_t k_getHeapSize()`
Gets the size of the app heap.

### `void k_MemWatcher_ResetRegions()`
Resets memory regions for monitoring.

### `void k_MemWatcher_GetModifiedRegions(k_memRegions* k_memRegions)`
Gets modified memory regions.

## Input Handling

### `void k_GetInput(k_input_state* state, bool includeJoysticks)`
Gets the state of console inputs (buttons, triggers, joystick) into the provided `k_input_state` structure.

### `bool k_IsBeingCranked()`
Checks if the crank is being turned.

### `bool k_GetCrankDirection()`
Gets the direction of the crank: `true` for clockwise, `false` for counter-clockwise.

## Screen Rendering

### `void k_BeginScreenUpdate()`
To be called before starting to write a new frame to the screen.

### `void k_EndScreenUpdate(bool checkpointsAllowed, bool showOverlays)`
To be called after writing a new frame to the screen. Usually outputs the frame.

## Metrics

### `void IncreaseMetrics(const char *name, int value)`
Increments a metric by a certain amount; the final value will be calculated as a rate per second.

### `void SetMetricDirect(const char *name, int value)`
Sets a metric to a certain value.

## Logging

### `void UART_LOG_SendCommand(char* toSend)`
Sends a command over UART for logging purposes.

### `void UART_LOG_StartExperiment(int experimentNumber)`
Logs the start of an experiment.

### `void UART_LOG_EndExperiment()`
Logs the end of an experiment.

### `void UART_LOG_SendEvent(int eventNumber)`
Logs an event.

### `void UART_LOG_SetChannelData(int channelNumber, int channelData)`
Sets data for a specific logging channel.

## Miscellaneous

### `void k_DelayMS(int ms)`
Delays the execution for a specified number of milliseconds.

### `uint32_t k_GetTimeMS()`
Gets the current time in milliseconds.

### `float k_GetVBat()`
Gets the current battery voltage.

### `uint32_t k_getLastFPS()`
Gets the last recorded frames per second (FPS).
