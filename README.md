[Español](https://github.com/Nightggan/OmniSense-Dongle/blob/master/LEEME.md)
# OmniSense Dongle — DS5Dongle Advanced Edition

A fork of [DS5Dongle](https://github.com/loteran/DS5Dongle) for the Raspberry Pi Pico 2W that unlocks the full potential of your DualSense controller on any system. 

This Advanced Edition inherits all the great features of the original firmware—like Audio Auto-Haptics—and adds powerful new ways to customize your triggers, lighting, and performance at the hardware level, **no native game support required**.

---

## 🌟 Key Features

- **Hardware-Level Adaptive Triggers:** Take full control of your triggers in *any* game. Choose from modes like Resistance, Weapon Click, Machine Gun, and the new Hair Trigger.
- **Hardware-Level Universal Gyro Aiming:** Take control of the right joystick using gyro movement, activated by the button of your choice. Works seamlessly in any game regardless of native support featuring fully tunable variables to customize the sensitivity and response to your exact playstyle.
- **Custom Profiles:** Save up to 4 different profiles to easily swap between trigger and lighting setups for different games.
- **On-the-Fly Configuration:** Use controller button shortcuts to switch profiles, adjust trigger/lightbar modes, tweak volume, and change haptic gain without touching your PC.
- **Advanced Lightbar Control:** Customize your controller's glow with multiple dynamic modes and 4 custom favorite color slots.
- **Web-Based UI:** Easily manage all your deep settings visually through a custom WebHID application.
- **Zero Background Apps:** Everything runs directly on the dongle itself. You don't need any software running in the background on your PC. *(Note: A lightweight app is currently in development specifically for routing PC audio to the controller if you aren't using headphones, but it is entirely optional).*

---

## 🚀 Getting Started

Configuring your dongle is simple and happens right in your browser:

1. Go to the [OmniSense Web Configurator](https://nightggan.github.io/OmniSense-Config-Web/).
2. Connect your Raspberry Pi Pico 2W dongle via USB and pair your DualSense controller.
3. Click **"Connect"** in the web app and start customizing!

---

## 🎮 Configuration Mode (On-the-Fly Shortcuts)

You don't always need the web app to make changes. You can enter **Configuration Mode** directly from your controller by pressing the **Mute** button.

- **Config Mode ON:** The Mute button will slowly pulse (breathing effect). Shortcuts are active and will not be sent to your PC/game.
- **Config Mode OFF:** The Mute button is unlit. The controller behaves normally.

*Any changes made here are instantly saved to the dongle's flash memory when you exit Config Mode.*

### Controller Shortcuts

| Button | Action | Scope |
| :--- | :--- | :--- |
| **Create (Share)** | Cycle Lightbar Mode | Saved to Profile |
| **Options** | Toggle Lightbar Breathing Effect | Saved to Profile |
| **L1** | Cycle Left Trigger Mode | Saved to Profile |
| **R1** | Cycle Right Trigger Mode | Saved to Profile |
| **Left Stick Up/Down** | Increase/Decrease Volume (Speaker & Headphones / Windows Host) | Global |
| **Right Stick Up/Down** | Increase/Decrease Haptic Gain | Global |
| **Square** | Mute Audio (Speaker & Headphones / Windows Host) | Global |
| **Circle** | Sleep Host (Windows) | Global |
| **Cross** | Change Rumble to Haptics Mode | Global |
| **Triangle** | Power off controller | Global |
| **D-PAD (Any dir.)** | Cycle through Profiles 0 to 3 | Global |

> **Pro-Tip:** The quick Mute state (Square) resets when you turn off the controller. If you want to permanently mute the audio, use the Left Stick Down to lower the volume to 0%, as volume levels are saved permanently.

---

## ⚙️ Settings Overview

### Global Settings
These settings apply to the dongle universally, regardless of which profile is active:
- Audio-based Haptic Configuration & Gain
- Mixing Classic Rumble signal with Audio-based Haptics
- Speaker/Headphone Master Volume
- USB Emulation Profile (DualSense, DualSense Edge, or Auto)
- USB Polling Rate
- Idle Disconnect Timeout & Host Wake-up
- Activation of Power-off Shortcut
- Option to control Host volume
- Option to sleep host on button Shortcut.

### Profile Settings
You have **4 distinct profiles**, each storing its own unique setup for:
- Left & Right Trigger operating modes
- Fine-tuned parameters for each trigger mode
- Activation mode for mapping controller gyro to right analog stick
- Default parameters tuned for fine aiming movements
- Lightbar operating mode
- 4 Custom Favorite Color slots
- Breathing animation toggle

---

## 🔫 Adaptive Trigger Modes

| Mode | Name | Description |
| :--- | :--- | :--- |
| **0** | **Host-Controlled** | Native passthrough. Best for games that officially support the DualSense. |
| **1** | **Resistance** | Applies constant stiffness throughout the trigger pull. |
| **2** | **Weapon Click** | Simulates the tactile "snap" or click of firing a gun. |
| **3** | **Machine Gun** | Provides continuous recoil and vibration when pulled. |
| **4** | **Hair Trigger** | The trigger hits a hard wall halfway down, immediately sending a 100% full-press signal with minimal effort. Great for shooters. |
| **5** | **Rumble Trigger** | Sends the classic rumble signal to the triggers. |

---

## 💡 Lightbar Modes

| Mode | Name | Description |
| :--- | :--- | :--- |
| **0** | **Host-Controlled** | Native game lighting. *(If multiple sources send color data, the dongle locks onto the first valid signal).* |
| **1** | **Off** | Disables the lightbar completely. |
| **2-5**| **Favorites (0-3)** | Displays one of your 4 custom colors defined in the web app. |
| **6** | **Battery Level** | Visual battery indicator (See table below). |
| **7** | **Rainbow** | Cycles through all colors. |
| **8** | **Fade** | Smoothly transitions between your 4 favorite colors. |

### Battery Level Indicator (Mode 6)

| Charge Level | Color & Behavior |
| :--- | :--- |
| **Charging** | Smooth Green fade (Overrides all other modes) |
| **> 40%** | Solid Green |
| **10% - 39%** | Solid Yellow |
| **< 10%** | Fast Red flashing (Overrides all other modes to warn you) |

---

## ⚙️ Steam Deck

If you want to send audio to the dongle at the same time as HDMI, Speakers or 3.5mm jack, you can use the provided script "restaurar_respaldo.sh" on the scripts folder. This will give a stable name to the dongle and then every time you plug it the deck will send audio to the HDMI, Speakers or headphones at the same time as the dongle so you can enjoy haptics using your desired audio out. Also it will set the audio profile for the dongle to pro-audio which is needed for haptics to work properly on Linux.

First, make sure you have a password set for the `deck` user. If not enter the terminal an run `passwd`. The system will ask for a new password and a confirmation.

Then, plug the dongle, pair the controller and follow these steps:

1. Make a new directory on `/home/deck/` called `audio_mix` 
2. Paste the `scripts` folder contents on the new folder `/home/deck/audio_mix`
3. Enter the terminal an run `sh /home/deck/audio_mix/restaurar_respaldo.sh`
4. The script will ask you the root pasword.
5. After it ends, unplug an plug the dongle again for the changes to apply

You will need to do this once, but, after an update, Steam could probably overwrite the changes, so you will have to run the script again (Step 3 only).

---

## 📝 Changes from the Original Fork

- Removed the shortcut to disable the touchpad.
- Overhauled default parameter values out-of-the-box to provide a better initial experience.

---

## 🙌 Credits & Acknowledgments

This project is a fork of **[DS5Dongle](https://github.com/loteran/DS5Dongle)**. Massive thanks to the original creators for the foundation and memory architecture that made this possible. 

Please check out their amazing work:
- The original architecture by awalol: **[DS5Dongle](https://github.com/awalol/DS5Dongle)**
- The Audio Auto-Haptics implementation by loteran: **[DS5Dongle - Auto Haptics Edition](https://github.com/loteran/DS5Dongle)**
- The OLED integration by MarcelineVPQ: **[DS5Dongle-OLED-Edition](https://github.com/MarcelineVPQ/DS5Dongle-OLED-Edition)**

*Licensed under the **MIT License**.*