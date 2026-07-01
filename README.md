[Español](https://github.com/Nightggan/OmniSense-Dongle/blob/master/LEEME.md)
# OmniSense Dongle — DS5Dongle Advanced Edition

A fork of [DS5Dongle](https://github.com/loteran/DS5Dongle) for Raspberry Pi Pico 2W that takes the DualSense experience to the next level. It inherits all the features of the original firmware and adds advanced functions to customize your controller without needing native game support.

---

## Key Features

In addition to the base features of the loteran fork, such as Audio Auto-Haptics, this firmware adds:

- **Profiles:** Choose between 4 profiles to configure triggers and lighting in detail.
- **Lightbar Modes:** Control the lightbar color with multiple modes and 4 custom favorite color slots.
- **Trigger Modes:** Take full control of the adaptive triggers even in games without support. Add **Resistance**, **Weapon Click**, and **Machine Gun** modes.
- **Controller Shortcuts:** Quickly switch between the 4 profiles. Change trigger and lighting settings. Adjust speaker/headphone volume and haptic gain directly from the controller.
- **Web Configuration:** Manage all features through a custom web application.
- **No companion apps needed:** All features are handled by the dongle, with no need for background software on the host, with one exception: audio must be redirected from the host to the controller if you are not using headphones (application in development).

---

## Configuration and Web-App

1. Access the web application [here](https://nightggan.github.io/OmniSense-Config-Web/)
2. Connect your dongle via USB and pair the DualSense controller.
3. Click **"Connect"** and adjust your preferences.

---

## Configuration Mode

To enter configuration mode from your controller, press the **Mute** button.
- **Configuration Mode:** The Mute button will have a **breathing** effect.
- **Normal Mode:** The Mute button will be turned off.

Changes are saved to flash upon exiting the mode. Shortcuts are blocked and are not passed to the host.

### Controls in Configuration Mode:

| Button | Function | Level |
| :--- | :--- | :--- |
| **Create** | Switch Lightbar mode | Per profile |
| **Options** | Toggle breathing effect | Per profile |
| **L1** | Switch Left Trigger mode | Per profile |
| **R1** | Switch Right Trigger mode | Per profile |
| **Left Stick Up** | Increase volume (Speaker/Headphones) | Global |
| **Left Stick Down** | Decrease volume (Speaker/Headphones) | Global |
| **Right Stick Up** | Increase haptic gain | Global |
| **Right Stick Down** | Decrease haptic gain | Global |
| **Square** | Mute (Speaker/Headphones) | Global |
| **DPAD** | Switch between profiles | |

Manual mute is not maintained across power cycles, but you can lower the volume to 0 with the **Left Stick**, which is maintained between power cycles.

---

### Global Configuration

The following options can be changed globally, regardless of the selected profile:

- Audio-based haptic configuration
- Speaker/Headphone volume
- Emulation profile (DS - DSE - Auto)
- Polling Rate
- Idle timeout
- Power-off shortcut
- Host wake-up
- Dongle LED

### Profiles

Each profile has the following options that are independent and customizable per profile:

- Left and Right trigger operating modes
- Configuration of each trigger mode to fine-tune to user preference
- Lightbar operating mode
    - 4 Favorite color slots per profile
- Breathing effect

---

### Trigger modes per profile:

| Mode | Function |
| :--- | :--- |
| 0 | Host-controlled. Compatible with games that send trigger data |
| 1 | Resistance mode |
| 2 | Weapon click mode |
| 3 | Machine Gun / Continuous vibration |

### Lightbar modes per profile:

| Mode | Function |
| :--- | :--- |
| 0 | Host-controlled |
| 1 | Off |
| 2 | Favorite 0 |
| 3 | Favorite 1 |
| 4 | Favorite 2 |
| 5 | Favorite 3 |
| 6 | Battery level indicator |
| 7 | Rainbow |
| 8 | Fade between favorites |

### Mode 0 (Host):

If 2 hosts send different data (e.g., Steam and GamePass), the dongle prioritizes the first valid color received unless the first host stops sending data.

### Battery Indicator:

| Percentage | Color |
| :--- | :--- |
| > 40% | Green |
| 10% - 39% | Yellow |
| < 10% | Fast red fade |

- If the battery is critical (<10%), the selected mode is overridden to provide a continuous alert.
- If the controller is charging, it is indicated by a green fade, overriding any other mode even if the battery is at critical levels.

---

## Work in Progress (Upcoming features)

- Full support for the built-in microphone with a dedicated controller shortcut.
- Controller shortcut to change auto-haptic modes.

---

## Changes from the original fork

- Removed shortcut to disable the touchpad.
- Changed default values to those I found most optimal.

---

## Credits

This project is a fork of **[DS5Dongle](https://github.com/loteran/DS5Dongle)**. All credit for the base development and memory architecture goes to its original creators.

---

## Credited Projects

Thanks to the following projects; without them, this would not be possible.

- Original project by awalol **[DS5Dongle](https://github.com/awalol/DS5Dongle)**
- Audio Auto-Haptics by loteran **[DS5Dongle - Auto Haptics Edition](https://github.com/loteran/DS5Dongle)**
- OLED Edition by MarcelineVPQ **[DS5Dongle-OLED-Edition](https://github.com/MarcelineVPQ/DS5Dongle-OLED-Edition)**

*Licensed under the **MIT License**.*