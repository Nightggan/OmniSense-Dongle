[Español](https://github.com/Nightggan/OmniSense-Dongle/blob/master/LEEME.md)
# OmniSense Dongle — DS5Dongle Advanced Edition

A fork of [DS5Dongle](https://github.com/loteran/DS5Dongle) for Raspberry Pi Pico 2W that takes the DualSense experience to the next level. It inherits all features from the original firmware and adds advanced functions to customize your controller without needing native game support.

---

## Key Features

In addition to the base features of loteran fork, such as Audio Auto-Haptics, this firmware adds:

- **Lightbar Modes:** Control the DualSense lightbar with multiple modes and 4 custom favorite color slots.
- **Trigger Modes:** Take full control of adaptive triggers, even in games without native support. Includes **Rigid**, **Weapon Click**, and **Machine Gun** modes.
- **On-Controller Config Mode:** Change lightbar modes, trigger modes, and speaker volume directly from the DualSense.
- **Web Configuration:** Manage all features through a custom web application.
- **No need for companion apps:** All the functions are drived from the dongle, without the need of a companion app running on the host.

---

## Configuration and Web-App

1. Access the web app [here](https://nightggan.github.io/OmniSense-Config-Web/)
2. Connect your dongle via USB and pair your DualSense controller.
3. Click **"Connect"** and adjust your preferences.

---

## Configuration Mode

To enter configuration mode from your controller, press the **Mute** button.
- **Config Mode:** The Mute button will have a **breathing** effect.
- **Normal Mode:** The Mute button will be off.

Changes are saved to flash upon exiting the mode. Inputs are blocked and do not reach the host.

### Controls in Configuration Mode:

| Button | Function |
| :--- | :--- |
| **Create** | Cycle Lightbar mode |
| **Options** | Toggle breathing effect |
| **L1** | Cycle Left Trigger mode |
| **R1** | Cycle Right Trigger mode |
| **Left Stick Up** | Increase Volume (Speaker/Headset) |
| **Left Stick Down** | Decrease Volume (Speaker/Headset) |
| **Square** | Mute (Speaker/Headset) |

Manual Mute does not persists after a power off but you can low the volume manually with the **Left Stick** wich it is maintained between power cycles.

---

### Lightbar modes:

| Mode | Function |
| :--- | :--- |
| 0 | Host controlled |
| 1 | Disabled |
| 2 | Favorite 0 |
| 3 | Favorite 1 |
| 4 | Favorite 2 |
| 5 | Favorite 3 |
| 6 | Battery Indicator |
| 7 | Rainbow |
| 8 | Fade between favorites |

### Mode 0 Host:

If 2 hosts send different data (like Steam and GamePass) the dongle prioritize the first valid color received unless the first host stops sending data.

### Battery Indicator:

| percentage | Color |
| :--- | :--- |
| > 40% | Green |
| 10% - 39% | Yellow |
| < 10% | Fast red fade |

- If the battery is critical (<10%) the dongle overrides any selected mode to alert the user.
- If the controller is charging, the dongle indicates it with a green fade overriding any other mode selected even if the battery is critically low.

---

## Work in Progress

- Full support for the integrated microphone with a dedicated controller shortcut.
- On-controller shortcut to change auto-haptics modes.

---

## Changes from the original fork

- Deactivate touchpad shortcut removed
- Default values changed to what worked best for me

---

## Credits

This project is a direct fork of **[DS5Dongle](https://github.com/loteran/DS5Dongle)**. All credit for the base development and memory architecture goes to the original creators.

---

## Credited projects

Thanks to the following projects, whitout them this would be impossible.

- Original project by awalol **[DS5Dongle](https://github.com/awalol/DS5Dongle)**
- Audio Auto-Haptics by loteran **[DS5Dongle - Auto Haptics Edition](https://github.com/loteran/DS5Dongle)**
- OLED Edition by MarcelineVPQ **[DS5Dongle-OLED-Edition](https://github.com/MarcelineVPQ/DS5Dongle-OLED-Edition)**



*Licensed under the **MIT License**.*