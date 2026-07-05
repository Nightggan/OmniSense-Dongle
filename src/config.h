//
// Created by awalol on 2026/5/4.
//

#ifndef DS5_BRIDGE_CONFIG_H
#define DS5_BRIDGE_CONFIG_H

#include <cstdint>
struct __attribute__((packed)) Global_Config_body {
    uint8_t config_version; // Device_Config Version
    float haptics_gain; // [1.0,2.0]
    uint8_t inactive_time; // [5,60] min
    uint8_t disable_inactive_disconnect; // bool: 0 disable,1 enable
    uint8_t disable_pico_led; // bool
    uint8_t polling_rate_mode; // 0: 250Hz, 1: 500Hz, 2: real-time
    uint8_t audio_buffer_length; // [16,128]
    uint8_t controller_mode; // 0: DS5, 1: DSE, 2: Auto
    // Auto haptics: derive rumble from game audio (ch1/ch2) even when game sends no haptic commands
    uint8_t auto_haptics_enable;      // 0: off, 1: mix with native haptics, 2: replace native haptics
    uint8_t auto_haptics_gain;        // [0,200] percent applied to derived signal, default 100
    uint16_t auto_haptics_lowpass_hz; // LP cutoff in Hz [20-400], default 80
    uint8_t enable_poweroff_shortcut; // 1: PS+<button> powers off controller, 0: disabled
    uint8_t poweroff_button;          // ShortcutButton: which button + PS triggers power off (default 3=Triangle)
    uint8_t wake_enable;              // 1: power off controller on host sleep + wake host on reconnect, 0: disabled
    float speaker_volume;     //Master Speaker volume [Min -100 to Max 0]
    uint8_t auto_mute_mode; //Disable Speaker on Auto Haptics mode 1 and 2
    uint16_t time_config_mode; //Millisecs to hold mute to enter config mode
    
};
struct __attribute__((packed)) Profile_Config_body {
    uint8_t lightbar_mode;            // 0: default, 1: disabled, 2: Fav 0, 3: Fav 1, 4: Fav 2, 5: Fav 3, 6: battery level, 7: rainbow, 8: favs fade
    uint8_t lb_fav_r[4];
    uint8_t lb_fav_g[4];
    uint8_t lb_fav_b[4];
    uint8_t lightbar_breathing;        // 1 - Enabled, 0 - Disabled
    //Trigger modes
    uint8_t trigger_left_mode;       // 0: Suelto, 1: Rígido, 2: Disparo, 3: Metralla, 4: Gatillo Rápido
    uint8_t trigger_right_mode;      // 0: Suelto, 1: Rígido, 2: Disparo, 3: Metralla, 4: Gatillo Rápido
    uint8_t feedback_start_point; //0 - 255
    uint8_t feedback_force; //0 - 255
    uint8_t trigger_start_point; //0 - 255
    uint8_t trigger_wall_point; //0 - 255
    uint8_t trigger_break_force; //0 - 255
    uint8_t vibration_start_point; //32 - 255
    uint8_t vibration_frequency; //0 - 255
    uint8_t vibration_force; //0 - 255
    uint8_t hair_wall_start_point; //60 - 100

    //Gyro to Analog values
    uint8_t gyro_button_activator; //0: Disabled, 1: Always On, 2: L2, 3: R2, 4: L1, 5: R1, 6: L3, 7: R3, 8: Square, 9: Cross, 10: Circle, 11: Triangle
    float analog_gyro_deadzone; //0.0f - 100.0f Default: 42.5f
    uint16_t max_stick_dps; //50 - 500 Default: 100
    float gyro_multiplier; //0.1f - 10.0f Default: 1.0f
};


// Button IDs used in poweroff_button / touchpad_button
enum ShortcutButton : uint8_t {
    BTN_SQUARE   = 0,
    BTN_CROSS    = 1,
    BTN_CIRCLE   = 2,
    BTN_TRIANGLE = 3,
    BTN_L1       = 4,
    BTN_R1       = 5,
    BTN_CREATE   = 6,
    BTN_OPTIONS  = 7,
    BTN_MUTE     = 8,
    BTN_SHORTCUT_MAX = BTN_MUTE,
};

struct __attribute__((packed)) Device_Config {
    uint32_t magic;
    uint16_t version;
    uint32_t global_crc32; // Config_body crc32, only calc and verify when save
    uint16_t global_config_size;  // Config_body size
    uint32_t profile_crc32; // Config_body crc32, only calc and verify when save
    uint16_t profile_size;  // Config_body size
    Global_Config_body global_body;
    Profile_Config_body profile_body[4];
    uint8_t profile_selected;
    uint8_t initial_setup_completed;
};

void config_default();
void device_config_load();
bool device_config_save();
const Global_Config_body& get_global_config();
const Profile_Config_body& get_profile_config();
const Profile_Config_body& get_profile_config_index(uint8_t index);
void set_global_config(const uint8_t *new_config, const uint16_t len);
void global_config_valid();
void set_global_config(const Global_Config_body &new_config);
void set_profile_config(const Profile_Config_body &new_profile_config);
void set_profile_config(const uint8_t *new_profile_config, const uint16_t len, uint8_t profile_index_set);
void set_profile_index(uint8_t new_profile);
extern bool is_dse;

#endif //DS5_BRIDGE_CONFIG_H
