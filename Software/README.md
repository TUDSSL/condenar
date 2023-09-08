# Console Settings Explanation

This README file provides an explanation of the console settings, including their purpose and default values. These settings allow users to customize various aspects of the console to suit their preferences and requirements.

## General Settings

### Actions

- **Reset settings:** This action allows you to reset all console settings to their default values.
- **Reset storage (inc. settings):** Use this action to erase the console's non-volatile storage (save files, checkpoints, ...), with the side effect of resetting the settings too.
- **Invalidate checkpoints:** Invalidates any existing checkpoints, preventing their use in recovery. Useful in case a checkpoint is corrupt or to force a full re-initialization of the console.

### Checkpoint Settings

- **Checkpoint creation enabled:** Determines whether checkpoint creation is enabled. Useful for app porting and debugging purposes. (default: true).
- **Checkpoint recovery enabled:** Controls whether checkpoint recovery is enabled. Useful for app porting and debugging purposes. (default: true).
- **Checkpoint interval (ms):** Specifies the minimum interval (in milliseconds) between checkpoint creation (default: 1000ms).

- **Only checkpoint while active:** When enabled, checkpoints are created only while the console is active. This setting prevents the degradation of the non-volatile storage due to its limited number of write cycles.  (default: true).
- **Active timeout (ms):** Sets the timeout (in milliseconds) for considering the console as active (default: 5000ms).

## Energy Settings

- **Screen brightness:** Adjusts the screen brightness level (default: 3).
- **Automatic brightness:** Enables automatic screen brightness adjustment (default: false).
- **Infinite battery:** Toggles infinite battery mode. When a logging module is present, it will use energy from the module instead of the supercapacitors. (default: false).
- **Fast CPU:** Activates high-performance mode, which doubles the CPU speed when necessary to reach the "Target refresh rate". (default: true).
- **Fast LCD refresh:** Increases the LCD refresh rate (from 30Hz to 60Hz) for improved visual clarity in exchange for more power consumption. (default: false).
- **Target refresh rate (Hz):** Sets the target screen refresh rate (default: 20Hz).
- **No input when crank enabled:** Prevents traditional button inputs when the crank is active (default: false).
- **Pause enabled:** Enables pausing functionality within games. (default: true).

## Debugging Settings

- **Show FPS:** Displays the frames per second (FPS) on the screen (default: false).
- **Show heap modifications:** Shows heap modification information. Any parts of the app heap that have changed since the last checkpoint will be colored black. Useful for optimizing application checkpointing.  (default: false).
- **Send screen over RTT:** Sends the screen output over a Real-Time Transfer (RTT) connection. When this is enabled, no physical screen is needed. The debugging connection will be used to send the screen frames to the computer. (default: false).
- **Show battery percentage:** Displays the battery percentage on the screen. (default: true).
- **Show crank status:** Shows whether the crank is being rotated in the screen. (default: true).

## DOOM Settings

- **Enable DEMO mode:** Activates DOOM's DEMO mode. In this mode, the game will play by itself, requiring no user input to finish the level. (default: true).
- **DEMO mode selection:** Specifies the DEMO mode selection (default: 3).
- **Force enter level:** Forces entry into DOOM levels, skipping the main menu. (default: true).
- **Enter level episode:** Sets the episode for entering DOOM levels (default: 2).
- **Brightness:** Adjusts the gamma level (color brightness) in DOOM (default: 2).
- **Use crank:** Enables the cranking incentives in DOOM (shooting and healing while cranking) (default: true).

## Game Boy Color (GBC) Settings

- **Use crank as A-B input:** Allows using the crank as an A/B button input (One cranking direction pulses the A button and the other one, the B button) for GBC games (default: false).
- **Slow down time when cranking:** Slows down the game time while cranking (default: true).

## Experiments Settings

- **Experiment ID:** Specifies the current experiment ID (default: 0).

### Presets (For Experiment Use)
These presets are used for quickly changing certain settings within the console and are not meant to be typically adjusted by users.
Each preset adjusts the settings as follows:

- **Battery-powered | Crank input**:
		Infinite battery -> true		
		Use crank -> true
		Use crank as A-B input -> true
		Slow down time when cranking -> true
		Show battery %% -> true
		Pause enabled -> true

- **Battery-free | No crank input**
    Infinite battery -> false
		Use crank -> false
		Use crank as A-B input -> false
		Slow down time when cranking -> false
		Show battery %%",false
		Pause enabled -> true

- **Battery-free | Crank input**
		Infinite battery -> false		
		Use crank -> false
		Use crank as A-B input -> false
		Slow down time when cranking -> false
		Show battery %% -> true
		Pause enabled -> true

Any setting not outlined in a preset is not modified by selecting the preset. Therefore, the default value will be used unless the researcher previously changed it. This allows easier creation of more experiment variations without having to change the console firmware.

---



If you ever need to return to the default settings, you can use the provided actions.

If you want to add/remove settings, they are all defined in the [k_apps.c](https://github.com/TUDSSL/condenar/blob/master/Software/Console/libs/Kernel/k_apps.c) file. Once you declare them there, you can access them from anywhere in the kernel or in any app.
