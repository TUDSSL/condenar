# Console Settings Explanation

This README file provides an explanation of the console settings, including their purpose and default values. These settings allow users to customize various aspects of the console to suit their preferences and requirements.

## General Settings

### Actions

- **Reset settings:** This action allows you to reset all console settings to their default values.
- **Reset storage (inc. settings):** Use this action to reset the console's storage, including saved settings.
- **Invalidate checkpoints:** Invalidates any existing checkpoints, preventing their use in recovery.

### Checkpoint Settings

- **Checkpoint creation enabled:** Determines whether checkpoint creation is enabled (default: true).
- **Checkpoint recovery enabled:** Controls whether checkpoint recovery is enabled (default: true).
- **Checkpoint interval (ms):** Specifies the interval (in milliseconds) between checkpoint creation (default: 1000ms).

- **Only checkpoint while active:** When enabled, checkpoints are created only while the console is active (default: true).
- **Active timeout (ms):** Sets the timeout (in milliseconds) for considering the console as active (default: 5000ms).

## Energy Settings

- **Screen brightness:** Adjusts the screen brightness level (default: 3).
- **Automatic brightness:** Enables automatic screen brightness adjustment (default: false).
- **Infinite battery:** Toggles infinite battery mode (default: false).
- **Fast CPU:** Activates high-performance mode (default: true).
- **Fast LCD refresh:** Increases the LCD refresh rate for improved performance (default: false).
- **Target refresh rate (Hz):** Sets the target screen refresh rate (default: 20Hz).
- **No input when crank enabled:** Prevents default input when the crank is active (default: false).
- **Pause enabled:** Enables pausing (default: true).

## Debugging Settings

- **Show FPS:** Displays the frames per second (FPS) on the screen (default: false).
- **Show heap modifications:** Shows heap modification information (default: false).
- **Send screen over RTT:** Sends the screen output over a Real-Time Transfer (RTT) connection (default: false).
- **Show battery percentage:** Displays the battery percentage on the screen (default: true).
- **Show crank status:** Shows the status of the crank mechanism (default: true).

## DOOM Settings

- **Enable DEMO mode:** Activates DOOM's DEMO mode (default: true).
- **DEMO mode selection:** Specifies the DEMO mode selection (default: 3).
- **Force enter level:** Forces entry into DOOM levels (default: true).
- **Enter level episode:** Sets the episode for entering DOOM levels (default: 2).
- **Brightness:** Adjusts the brightness level for DOOM (default: 2).
- **Use crank:** Enables the use of the crank in DOOM (default: true).

## Game Boy Color (GBC) Settings

- **Use crank as A-B input:** Allows using the crank as an A-B input for GBC games (default: false).
- **Slow down time when cranking:** Slows down the game time while cranking (default: true).

## Experiments Settings

- **Experiment ID:** Specifies the current experiment ID (default: 0).

### Presets (For Experiment Use)
These presets are used for quickly changing certain settings within the console and are not meant to be typically adjusted by users.

- **Battery-powered | Crank input**
- **Battery-free | No crank input**
- **Battery-free | Crank input**

---

If you ever need to return to the default settings, you can use the provided actions.

If you want to add/remove settings, they are all defined in the k_apps.c file. Once you declare them there, you can access them from anywhere in the kernel or in any app.
