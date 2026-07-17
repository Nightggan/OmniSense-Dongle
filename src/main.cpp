//
// Created by awalol on 2026/3/4.
//

#include <cstdio>
#include "bsp/board_api.h"
#include "bt.h"
#include "utils.h"
#include "resample.h"
#include "audio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"
#include "pico/cyw43_arch.h"
#include "pico/time.h"
#include "state_mgr.h"
#if ENABLE_SERIAL
#include "pico/stdio_usb.h"
#endif
#include "config.h"
#include "cmd.h"
#include "wake.h"
#if ENABLE_BATT_LED
#include "battery_led.h"
#endif
#include "lightbar_controller.h"
#include <algorithm> // Para std::clamp
#include <cmath>     // Para std::round

// Pico SDK speciifically for waiting on conditions
#include "pico/critical_section.h"

int reportSeqCounter = 0;
uint8_t packetCounter = 0;
bool spk_active = false;
bool touchpad_runtime_enabled = true;
static uint64_t last_rumble_report_us = 0;

//Custom vars Omni

uint8_t right_trigger_real_position = 0;
uint8_t left_trigger_real_position = 0;

uint8_t host_real_color_r = 0;
uint8_t host_real_color_g = 0;
uint8_t host_real_color_b = 0;

uint8_t lb_controlled_red = 0;
uint8_t lb_controlled_green = 0;
uint8_t lb_controlled_blue = 0;

bool check_lb_again = false;
static uint32_t last_time_check_lb = 0;

volatile bool flash_busy = false;
volatile bool usb_reconnect_requested = false;
static bool usb_is_enabled = false;
volatile bool save_requested = false;
uint32_t pending_save_time = 0;
bool config_mode_enabled = false;
bool config_button_pressed = false;
bool save_config_now = false;
uint32_t left_analog_up_holding_time = 0;
uint32_t left_analog_down_holding_time = 0;
bool left_analog_up_triggered = false;
bool left_analog_down_triggered = false;
uint32_t right_analog_up_holding_time = 0;
uint32_t right_analog_down_holding_time = 0;
bool right_analog_up_triggered = false;
bool right_analog_down_triggered = false;
bool request_temp_save = false;
volatile float local_current_volume = -100.0f;
volatile bool headset_plugged = false;
volatile bool audio_mute = false;
volatile float current_auto_haptics_gain = 1.5f;
uint8_t local_profile_selected;
uint32_t time_config_pending_time;
uint32_t mute_button_press_time;
bool exiting_config = false;
bool ghost_press_mute = false;
bool mute_button_soft_pressed = false;
//Gyro vars
int16_t raw_ang_vel_x = 0;
int16_t raw_ang_vel_z = 0;
float final_pct_x = 0;
float final_pct_z = 0;
//End Custom vars Omni

uint8_t interrupt_in_data[63] = {
    0x7f, 0x7d, 0x7f, 0x7e, 0x00, 0x00, 0xa7,
    0x08, 0x00, 0x00, 0x00, 0x52, 0x43, 0x30, 0x41,
    0x01, 0x00, 0x0e, 0x00, 0xef, 0xff, 0x03, 0x03,
    0x7b, 0x1b, 0x18, 0xf0, 0xcc, 0x9c, 0x60, 0x00,
    0xfc, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x00, 0x00, 0x09, 0x09, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xa7, 0xad, 0x60, 0x00, 0x29, 0x18, 0x00,
    0x53, 0x9f, 0x28, 0x35, 0xa5, 0xa8, 0x0c, 0x8b
};

critical_section_t report_cs;
volatile bool report_dirty = false;

void __not_in_flash_func(interrupt_loop)() {
    if (!tud_hid_ready()) return;

    // TODO: Refactor for better code reuse
    if (get_global_config().polling_rate_mode != 2) {
        if (!tud_hid_report(0x01, interrupt_in_data, 63)) {
            printf("[USBHID] tud_hid_report error\n");
        }
        return;
    }

    bool should_send = false;
    // Local buffer to hold the report data while we prepare it to send. 
    uint8_t safe_report[63];


    critical_section_enter_blocking(&report_cs);
    if (report_dirty) {
        memcpy(safe_report, interrupt_in_data, 63);
        report_dirty = false;
        should_send = true;
    }
    critical_section_exit(&report_cs);

    // Only send to TinyUSB if we actually grabbed fresh data
    if (should_send) {
        if (!tud_hid_report(0x01, safe_report, 63)) {
            printf("[USBHID] tud_hid_report error\n");

            // If the report failed to queue, restore the dirty flag 
            // so we try again on the next loop iteration.
            critical_section_enter_blocking(&report_cs);
            report_dirty = true;
            critical_section_exit(&report_cs);
        }
    }
}

// Byte offset (+mask) for each ShortcutButton in the raw BT report (data[] offset from 0).
// BT report: data[3+7]=face buttons, data[3+8]=shoulder/etc, data[3+9]=PS/Pad/Mute
struct BtnEntry { uint8_t off; uint8_t mask; };
static constexpr BtnEntry SHORTCUT_BTN_MAP[] = {
    {10, 0x10}, // 0: Square
    {10, 0x20}, // 1: Cross
    {10, 0x40}, // 2: Circle
    {10, 0x80}, // 3: Triangle
    {11, 0x01}, // 4: L1
    {11, 0x02}, // 5: R1
    {11, 0x10}, // 6: Create
    {11, 0x20}, // 7: Options
    {12, 0x04}, // 8: Mute
};
//0: Disabled, 1: Always On, 
//2: L2, 3: R2, 4: L1, 5: R1, 
//6: L3, 7: R3, 8: Square, 9: Cross, 
//10: Circle, 11: Triangle
static constexpr BtnEntry GYRO_SHORTCUT_BTN_MAP[] = {
    //Offset of 3
    {11, 0x04}, //0:L2
    {11, 0x08}, //1:R2
    {11, 0x01}, //2:L1
    {11, 0x02}, //3:R1
    {11, 0x40}, //4:L3
    {11, 0x80}, //5:R3
    {10, 0x10}, //6: Square
    {10, 0x20}, //7: Cross
    {10, 0x40}, //8: Circle
    {10, 0x80}, //9: Triangle
};

static bool shortcut_btn_pressed(const uint8_t *data, uint8_t btn) {
    if (btn > BTN_SHORTCUT_MAX) return false;
    return (data[SHORTCUT_BTN_MAP[btn].off] & SHORTCUT_BTN_MAP[btn].mask) != 0;
}

static bool gyro_shortcut_btn_pressed(const uint8_t *data, uint8_t btn) {
    if (btn > BTN_SHORTCUT_MAX) return false;
    return (data[GYRO_SHORTCUT_BTN_MAP[btn].off] & GYRO_SHORTCUT_BTN_MAP[btn].mask) != 0;
}

//Not in active use on this fork, except for the power off button coded by loteran.
static void shortcut_btn_suppress(uint8_t *data, uint8_t btn) {
    if (btn > BTN_SHORTCUT_MAX) return;
    data[SHORTCUT_BTN_MAP[btn].off] &= ~SHORTCUT_BTN_MAP[btn].mask;
}

//Custom Safe Device_Config Save. Made to not save if the flash is in use (web app or shortcut)
void safe_config_save() {
    // If the flash is busu, we return to avoid locking the stack USB
    if (flash_busy) return; 
    flash_busy = true;
    
    //Vanilla save function
    bool success = device_config_save();
    
    if (success) {
        flash_busy = false;
    } else {
        printf("[Flash] Error crítico de CRC.\n");
    }
}

//Used only if the web app sends a save to flash request 0xF6 0x02
void save_now_update()
{
    // 1. Tries to save to the flash
    safe_config_save();

    // 2. Enforce the update of the internal state[] reading the new config from the prior request 0xF6 0x01
    uint8_t dummy_buffer[sizeof(SetStateData) + 1] = {0};
    state_update(dummy_buffer + 1, sizeof(SetStateData));

    // 3. Sending the new package with the new state[] to the controller
    uint8_t forced_pkt[78] = {0};
    forced_pkt[0] = 0x31;
    forced_pkt[1] = (uint8_t)(reportSeqCounter << 4);
    if (++reportSeqCounter == 256) reportSeqCounter = 0;
    forced_pkt[2] = 0x10;
    
    // Filling the forced package with the actual new state[]
    state_set(forced_pkt + 3, sizeof(SetStateData));
    
    // Sending by BT the forced package to update the controller
    bt_write(forced_pkt, sizeof(forced_pkt));
}

//Used to supress all inputs (except touchpad WIP) on config mode. Using an offset of 3
void suppress_all_inputs(uint8_t *data) {
    // 1. Sets analogs on its center
    data[3] = 128; data[4] = 128; // Left Analog
    data[5] = 128; data[6] = 128; // Right Analog

    // 2. Triggers to 0
    data[7] = 0; 
    data[8] = 0;
    
    data[10] = 0x08; // Cleans D-Pad, SQUARE, TRIANGLE, CIRCLE and CROSS
    data[11] = 0; // Cleans extra buttons (Create, Options, L1, R1, L3, R3)
    data[12] = 0; // Cleans PS, Pad Click, Mute, DSE Paddles
}

auto set_bit = [](uint8_t &byte, const int bit, const bool value) {
        byte = (byte & ~(1 << bit)) | (value << bit);
    };

// HID Async system
static constexpr uint8_t HID_REPORT_ID_KEYBOARD = 1;
static constexpr uint8_t HID_REPORT_ID_VOLUME = 3;
static constexpr uint8_t HID_REPORT_ID_SYSTEM_CONTROL = 2;
//static constexpr uint16_t HID_USAGE_CONSUMER_SLEEP = 0x0032;
static uint16_t current_media_key = 0;
static bool media_key_needs_release = false;
static uint32_t media_key_timer = 0;
static uint8_t current_system_control = 0;
static bool system_control_needs_release = false;
static uint32_t system_control_timer = 0;

void process_media_keys() {
    if (!tud_hid_n_ready(1)) return; 

    if (system_control_needs_release) {
        if (to_ms_since_boot(get_absolute_time()) - system_control_timer > 20) {
            uint8_t empty = 0;
            tud_hid_n_report(1, HID_REPORT_ID_SYSTEM_CONTROL, &empty, sizeof(empty));
            system_control_needs_release = false;
        }
        return;
    }

    if (current_system_control != 0) {
        tud_hid_n_report(1, HID_REPORT_ID_SYSTEM_CONTROL, &current_system_control, sizeof(current_system_control));
        system_control_needs_release = true;
        system_control_timer = to_ms_since_boot(get_absolute_time());
        current_system_control = 0;
        return;
    }

    if (media_key_needs_release) {
        // Han pasado 20 milisegundos? Soltamos la tecla.
        if (to_ms_since_boot(get_absolute_time()) - media_key_timer > 20) {
            uint16_t empty = 0;
            tud_hid_n_report(1, HID_REPORT_ID_VOLUME, &empty, sizeof(empty));
            media_key_needs_release = false;
        }
    } 
    else if (current_media_key != 0) {
        // Enviar la nueva tecla
        tud_hid_n_report(1, HID_REPORT_ID_VOLUME, &current_media_key, sizeof(current_media_key));
        media_key_needs_release = true;
        media_key_timer = to_ms_since_boot(get_absolute_time());
        current_media_key = 0; // Limpiamos la solicitud
    }
}

void send_volume_up_command() {
    if (!media_key_needs_release) current_media_key = HID_USAGE_CONSUMER_VOLUME_INCREMENT;  
    
}

void send_volume_down_command() {  
    if (!media_key_needs_release) current_media_key = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
}

void send_mute_command() {
    if (!media_key_needs_release) current_media_key = HID_USAGE_CONSUMER_MUTE;
}

void send_sleep_command() {
    if (!media_key_needs_release && !system_control_needs_release) current_system_control = 0x02;
}

//Custon on_bt_data adding trigger/lightbar modes and shortcuts
void __not_in_flash_func(on_bt_data)(CHANNEL_TYPE channel, uint8_t *data, uint16_t len) {
    static bool request_flash_save = false;
    if (channel == INTERRUPT && data[1] == 0x31) {
        if ((data[56] & 1) != (interrupt_in_data[53] & 1)) {
            set_headset(data[56] & 1);
            
            config_button_pressed = true;
            config_mode_enabled = false; //Exit config mode when plug/unplug to avoid mismatch in volume setting
            
            headset_plugged = data[56] & 1; 
            if(headset_plugged)//Updating the volatile to update the volume live on headset plug/unplugg
            {
                local_current_volume = get_global_config().headset_volume - 100.0f;
            }
            else
            {
                local_current_volume = get_global_config().speaker_volume - 100.0f;
            }

            uint8_t dummy_buffer[sizeof(SetStateData) + 1] = {0};
            state_update(dummy_buffer + 1, sizeof(SetStateData));
            
            // Sending the new package with the new state[] to the controller
            uint8_t forced_pkt[78] = {0};
            forced_pkt[0] = 0x31;
            forced_pkt[1] = (uint8_t)(reportSeqCounter << 4);
            if (++reportSeqCounter == 256) reportSeqCounter = 0;
            forced_pkt[2] = 0x10;
            
            // Filling the forced package with the actual new state[]
            state_set(forced_pkt + 3, sizeof(SetStateData));
            
            // Sending by BT the forced package to update the controller
            bt_write(forced_pkt, sizeof(forced_pkt));
        }
        // Obtaining real triggers position
        left_trigger_real_position = data[7];  // Byte 7: L2 (0-255)
        right_trigger_real_position = data[8];  // Byte 8: R2 (0-255)
        
        // Obtaining real Left Analog position
        uint8_t left_stick_x = data[3]; //0: Full Left - 128: Center - 255: Full Right (Not in use atm)
        uint8_t left_stick_y = data[4]; //0: Full Up - 128: Center - 255: Full Down
        
        // Obtaining real Right Analog position
        uint8_t right_stick_x = data[5]; //0: Full Left - 128: Center - 255: Full Right (Not in use atm)
        uint8_t right_stick_y = data[6]; //0: Full Up - 128: Center - 255: Full Down

        //Only process gyro to analog if the gyro_button_activator is set to a value > 0 (Not Disabled)
        if(get_profile_config_index(local_profile_selected).gyro_button_activator>0)
        {
            raw_ang_vel_x = (int16_t)((data[19] << 8) | data[18]);
            raw_ang_vel_z = (int16_t)((data[21] << 8) | data[20]);

            float speed_ang_vel_x = raw_ang_vel_x / 16.384f;
            float speed_ang_vel_z = raw_ang_vel_z / 16.384f;
            
            if((speed_ang_vel_x < 2.0f)&&(speed_ang_vel_x > -2.0f))
            {
                speed_ang_vel_x = 0.0f;
            }
            if((speed_ang_vel_z < 2.0f)&&(speed_ang_vel_z > -2.0f))
            {
                speed_ang_vel_z = 0.0f;
            }

            // Normalize the angular velocity values
            uint16_t local_max_stick_dps = get_profile_config_index(local_profile_selected).max_stick_dps;
            float local_gyro_multiplier = get_profile_config_index(local_profile_selected).gyro_multiplier;
            speed_ang_vel_x = (speed_ang_vel_x / local_max_stick_dps) * 100.0f;
            speed_ang_vel_z = (speed_ang_vel_z / local_max_stick_dps) * 100.0f;

            speed_ang_vel_x *= local_gyro_multiplier;
            speed_ang_vel_z *= local_gyro_multiplier;

            final_pct_x = std::clamp(speed_ang_vel_x, -100.0f, 100.0f);
            final_pct_z = std::clamp(speed_ang_vel_z, -100.0f, 100.0f);
        }
        
        // Device_Config Shortcut: Mute press
        if (shortcut_btn_pressed(data, 8)) {
            if(!config_mode_enabled)
            {
                if(get_global_config().time_config_mode>0)
                {
                    data[12] &= ~0x04;//Supress mute until release if realeased prior to get_global_config().time_config_mode
                    mute_button_press_time = 0;
                    
                    if(!config_button_pressed)
                    {
                        mute_button_soft_pressed = true;    
                        config_button_pressed = true;
                        time_config_pending_time = to_ms_since_boot(get_absolute_time());
                        
                    }
                    
                    
                    if (time_config_pending_time != 0 && (to_ms_since_boot(get_absolute_time()) - time_config_pending_time >= get_global_config().time_config_mode)) {
                        time_config_pending_time = 0;          
                        config_mode_enabled = true;
                        mute_button_soft_pressed = false;
                        request_temp_save = true; //To apply breathing effect on mute led            
                    }
                }
                else
                {
                    if(!config_button_pressed) 
                    {
                        config_button_pressed = true;
                        config_mode_enabled = true;
                        request_temp_save = true;
                        mute_button_soft_pressed = false;
                    }
                }
            }
            else
            {
                if(!config_button_pressed)
                {
                    config_button_pressed = true;
                    config_mode_enabled = false;
                    request_temp_save = true; //To apply breathing effect on mute led
                    exiting_config = true;
                }
            }
            if(exiting_config)
            {
                suppress_all_inputs(data);
            }
        }
        else {
                        
            // Freeing the lock after button release
            config_button_pressed = false;
            time_config_pending_time = 0;
            exiting_config = false;
            if(mute_button_soft_pressed)
            {
                ghost_press_mute = true;
                mute_button_soft_pressed = false;
            }
                
        }
        
        if(config_mode_enabled)
        {   
            //send_release_command();
            //command_release_once = true;
            // Locks for shortcuts to run only one time per button press
            static bool lightbar_mode_switch_shortcut_lock = false;
            static bool lightbar_breathing_shortcut_lock = false;
            static bool right_trigger_mode_override_shortcut_lock = false;
            static bool left_trigger_mode_override_shortcut_lock = false;
            static bool speaker_volume_up_shortcut_lock = false;
            static bool speaker_volume_down_shortcut_lock = false;
            static bool mute_speaker_shortcut_lock = false;
            static bool profile_switch_shortcut_lock_up = false;
            static bool profile_switch_shortcut_lock_down = false;
            static bool profile_switch_shortcut_lock_left = false;
            static bool profile_switch_shortcut_lock_right = false;
            static bool haptic_gain_up_shortcut_lock = false;
            static bool haptic_gain_down_shortcut_lock = false;
            static bool sleep_host_shortcut_lock = false;
            static bool rumble_mode_switch_shortcut_lock = false;
            // Obtaining D-Pad value
            uint8_t dpad_value = data[10] & 0x0F;
            Global_Config_body actual_global_config = get_global_config();
            // Profile switch: DPAD
            if (dpad_value == 0)//Profile 0
            {    
                if (!profile_switch_shortcut_lock_up) {
                    set_profile_index(0);
                    local_profile_selected = 0;
                    request_temp_save = true; //Temp save request to update the state
                    
                    profile_switch_shortcut_lock_up = true; // Locking the lock
                }
            } else {
                // Unlocking the lock on button release
                profile_switch_shortcut_lock_up = false;
            }
            
            if (dpad_value == 2)//Profile 1
            {    
                if (!profile_switch_shortcut_lock_right) {
                    set_profile_index(1);
                    local_profile_selected = 1;
                    request_temp_save = true; //Temp save request to update the state
                    
                    profile_switch_shortcut_lock_right = true; // Locking the lock
                }
            } else {
                // Unlocking the lock on button release
                profile_switch_shortcut_lock_right = false;
            }

            if (dpad_value == 4)//Profile 2
            {    
                if (!profile_switch_shortcut_lock_down) {
                    set_profile_index(2);
                    local_profile_selected = 2;
                    request_temp_save = true; //Temp save request to update the state
                    
                    profile_switch_shortcut_lock_down = true; // Locking the lock
                }
            } else {
                // Unlocking the lock on button release
                profile_switch_shortcut_lock_down = false;
            }

            if (dpad_value == 6)//Profile 3
            {    
                if (!profile_switch_shortcut_lock_left) {
                    set_profile_index(3);
                    local_profile_selected = 3;
                    request_temp_save = true; //Temp save request to update the state
                    
                    profile_switch_shortcut_lock_left = true; // Locking the lock
                }
            } else {
                // Unlocking the lock on button release
                profile_switch_shortcut_lock_left = false;
            }
            #if !USE_LINUX_USB_DESCRIPTORS
            // Sleep Host: Circle
            if (get_global_config().sleep_host_enable) {
                    
                    
                if (shortcut_btn_pressed(data, 2))
                {
                    send_sleep_command();
                    config_button_pressed = false;
                    config_mode_enabled = false;
                    sleep_host_shortcut_lock = true;
                    bt_disconnect();
                } else {
                    sleep_host_shortcut_lock = false;
                }
            }
            #endif
            // Power Off: TRIANGLE
            if (get_global_config().enable_poweroff_shortcut)
            {
                if (shortcut_btn_pressed(data, 3))
                {
                    config_button_pressed = false;
                    config_mode_enabled = false;
                    bt_disconnect();
                }
            }
            // Mute Speaker: SQUARE
            if (shortcut_btn_pressed(data, 0))
            {    
                if(get_global_config().control_host_volume==1)
                {
                    if (!mute_speaker_shortcut_lock) {
                        //Send a volume up command to the host
                        send_mute_command();
                        mute_speaker_shortcut_lock = true;
                    }
                }
                else
                {
                    if (!mute_speaker_shortcut_lock) {
                        audio_mute = !audio_mute; //Cycle speaker mute
                        //No need to save config cause it is not saved between cycles
                        mute_speaker_shortcut_lock = true;
                    }
                }
            } else {
                mute_speaker_shortcut_lock = false;
            }

            // Lightbar mode change: Create
            if (shortcut_btn_pressed(data, 6))
            {    
                if (!lightbar_mode_switch_shortcut_lock) {
                    Profile_Config_body new_config = get_profile_config();
                    new_config.lightbar_mode = (new_config.lightbar_mode + 1) % 9; // Ciclar modos de luz
                    
                    set_profile_config(new_config);
                    request_temp_save = true; //Temp save request to update the state
                    
                    lightbar_mode_switch_shortcut_lock = true; // Locking the lock
                }
            } else {
                // Unlocking the lock on button release
                lightbar_mode_switch_shortcut_lock = false;
            }
            
            // Rumble To Haptics mode: Cross
            if (shortcut_btn_pressed(data, 1))
            {    
                if (!rumble_mode_switch_shortcut_lock) {
                    
                    actual_global_config.classic_rumble_mix_profile = (actual_global_config.classic_rumble_mix_profile + 1) % 3; // Ciclar modos de luz
                    set_global_config(actual_global_config);
                    request_temp_save = true; //Temp save request to update the state
                    rumble_mode_switch_shortcut_lock = true; // Locking the lock
                }
            } else {
                // Unlocking the lock on button release
                rumble_mode_switch_shortcut_lock = false;
            }

            //Breathing on/off: Options
            if (shortcut_btn_pressed(data, 7))
            {
                if (!lightbar_breathing_shortcut_lock) {
                    Profile_Config_body new_config = get_profile_config();
                    new_config.lightbar_breathing = !new_config.lightbar_breathing;
                    set_profile_config(new_config);
                    request_temp_save = true;
                    lightbar_breathing_shortcut_lock = true;
                }
            } else {
                lightbar_breathing_shortcut_lock = false;
            }
            
            // Right Trigger mode: R1
            if (shortcut_btn_pressed(data, 5) /* R1 */) {
                if (!right_trigger_mode_override_shortcut_lock) {
                    Profile_Config_body new_config = get_profile_config();
                    new_config.trigger_right_mode = (new_config.trigger_right_mode + 1) % 6; 
                    set_profile_config(new_config);
                    request_temp_save = true;
                    right_trigger_mode_override_shortcut_lock = true; 
                }
            } else {
                right_trigger_mode_override_shortcut_lock = false;
            }
                        
            // Left Trigger mode: L1
            if (shortcut_btn_pressed(data, 4)) {
                if (!left_trigger_mode_override_shortcut_lock) {
                    Profile_Config_body new_config = get_profile_config();
                    new_config.trigger_left_mode = (new_config.trigger_left_mode + 1) % 6; 
                    set_profile_config(new_config);
                    request_temp_save = true;
                    left_trigger_mode_override_shortcut_lock = true; 
                }
            } else {
                left_trigger_mode_override_shortcut_lock = false;
            }

            //Volume Control - Left Analog Up/Down Holding Control. Only updates the volume every 50ms on full analog hold.
            if(left_stick_y<10) // Full up
            {
                if(!left_analog_up_triggered)
                {
                    left_analog_up_holding_time = to_ms_since_boot(get_absolute_time());
                    left_analog_up_triggered = true;
                }
            }
            else
            {
                left_analog_up_holding_time = 0;
                left_analog_up_triggered = false;
                speaker_volume_up_shortcut_lock = false;
                
            }
            if(left_analog_up_holding_time != 0 && (to_ms_since_boot(get_absolute_time()) - left_analog_up_holding_time > 100))
            {
                speaker_volume_up_shortcut_lock = true;
            }
            
            
            if(left_stick_y>245) // Full down
            {
                if(!left_analog_down_triggered)
                {
                    left_analog_down_holding_time = to_ms_since_boot(get_absolute_time());
                    left_analog_down_triggered = true;
                }
                
            }
            else
            {
                left_analog_down_holding_time = 0;
                left_analog_down_triggered = false;
                speaker_volume_down_shortcut_lock = false;
            }
            if(left_analog_down_holding_time != 0 && (to_ms_since_boot(get_absolute_time()) - left_analog_down_holding_time > 100))
            {
                speaker_volume_down_shortcut_lock = true;
            }
            
            
            
            //Speaker Volume Up Shortcut control: Left Analog Up
            if (speaker_volume_up_shortcut_lock) {
                
                if(get_global_config().control_host_volume==1)
                {
                    //Send a volume up command to the host
                    send_volume_up_command();
                }
                else
                {   
                    if(headset_plugged)
                    {
                        actual_global_config.headset_volume = actual_global_config.headset_volume + 2.0f;
                        if (actual_global_config.headset_volume > 100.0f) {
                            actual_global_config.headset_volume = 100.0f;
                        }
                        
                        local_current_volume = actual_global_config.headset_volume - 100.0f;
                        set_global_config(actual_global_config);
                        request_temp_save = true;    
                    }
                    else
                    {
                        actual_global_config.speaker_volume = actual_global_config.speaker_volume + 2.0f;
                        if (actual_global_config.speaker_volume > 100.0f) {
                            actual_global_config.speaker_volume = 100.0f;
                        }
                        
                        local_current_volume = actual_global_config.speaker_volume - 100.0f;
                        set_global_config(actual_global_config);
                        request_temp_save = true;    
                    }
                    audio_mute = false; // Disable mute
                }
                left_analog_up_holding_time = 0;
                left_analog_up_triggered = false;
                speaker_volume_up_shortcut_lock = false;
            }

            //Speaker Volume Down Shortcut control: Left Analog Down
            if (speaker_volume_down_shortcut_lock) {
                
                if(get_global_config().control_host_volume==1)
                {
                    //Send a volume down command to the host
                    send_volume_down_command();
                }
                else
                {
                    if(headset_plugged)
                    {
                        actual_global_config.headset_volume = actual_global_config.headset_volume - 2.0f;
                        if (actual_global_config.headset_volume < 0.0f) {
                            actual_global_config.headset_volume = 0.0f;
                        }
                        
                        local_current_volume = actual_global_config.headset_volume - 100.0f;
                        set_global_config(actual_global_config);
                        request_temp_save = true;    
                    }
                    else
                    {
                        actual_global_config.speaker_volume = actual_global_config.speaker_volume - 2.0f;
                        if (actual_global_config.speaker_volume < 0.0f) {
                            actual_global_config.speaker_volume = 0.0f;
                        }
                        
                        local_current_volume = actual_global_config.speaker_volume - 100.0f;
                        set_global_config(actual_global_config);
                        request_temp_save = true;    
                    } 
                    audio_mute = false; // Disable mute
                }
                left_analog_down_triggered = false;
                left_analog_down_holding_time = 0;
                speaker_volume_down_shortcut_lock = false;
            }
            
            //Haptic Gain Control - Right Analog Up/Down Holding Control. Only updates the haptic gain every 100ms on full analog hold.
            if(right_stick_y<10) // Full up
            {
                if(!right_analog_up_triggered)
                {
                right_analog_up_holding_time = to_ms_since_boot(get_absolute_time());
                    right_analog_up_triggered = true;
                }
            }
            else
            {
                right_analog_up_holding_time = 0;
                right_analog_up_triggered = false;
                haptic_gain_up_shortcut_lock = false;
            }
            if(right_analog_up_holding_time != 0 && (to_ms_since_boot(get_absolute_time()) - right_analog_up_holding_time > 100))
            {
                haptic_gain_up_shortcut_lock = true;
            }
            
            if(right_stick_y>245) // Full down
            {
                if(!right_analog_down_triggered)
                {
                    right_analog_down_holding_time = to_ms_since_boot(get_absolute_time());
                    right_analog_down_triggered = true;
                }
                
            }
            else
            {
                right_analog_down_holding_time = 0;
                right_analog_down_triggered = false;
                haptic_gain_down_shortcut_lock = false;
            }
            if(right_analog_down_holding_time != 0 && (to_ms_since_boot(get_absolute_time()) - right_analog_down_holding_time > 100))
            {
                haptic_gain_down_shortcut_lock = true;
            }
            
            
            //Haptic Gain Up Shortcut control: Right Analog Up
            if (haptic_gain_up_shortcut_lock) {
                
                actual_global_config.auto_haptics_gain = actual_global_config.auto_haptics_gain + 4;
                if (actual_global_config.auto_haptics_gain > 200) {
                    actual_global_config.auto_haptics_gain = 200;
                }
                else
                {
                    current_auto_haptics_gain = actual_global_config.auto_haptics_gain;
                    set_global_config(actual_global_config);
                    request_temp_save = true;
                }
                right_analog_up_holding_time = 0;
                right_analog_up_triggered = false;
                haptic_gain_up_shortcut_lock = false;
            }

            //Haptic Gain Down Shortcut control: Right Analog Down
            if (haptic_gain_down_shortcut_lock) {
                
                actual_global_config.auto_haptics_gain = actual_global_config.auto_haptics_gain - 4;
                if (actual_global_config.auto_haptics_gain < 0) {
                    actual_global_config.auto_haptics_gain = 0;
                }
                else
                {
                    current_auto_haptics_gain = actual_global_config.auto_haptics_gain;
                    set_global_config(actual_global_config);
                    request_temp_save = true;
                }
                right_analog_down_holding_time = 0;
                right_analog_down_triggered = false;
                haptic_gain_down_shortcut_lock = false;
            }

            if(request_temp_save && !left_trigger_mode_override_shortcut_lock
            && !right_trigger_mode_override_shortcut_lock && !lightbar_breathing_shortcut_lock
            && !lightbar_mode_switch_shortcut_lock && !speaker_volume_down_shortcut_lock && !speaker_volume_up_shortcut_lock && !profile_switch_shortcut_lock_up
            && !profile_switch_shortcut_lock_down && !profile_switch_shortcut_lock_left && !profile_switch_shortcut_lock_right
            && !haptic_gain_up_shortcut_lock && !haptic_gain_down_shortcut_lock && !sleep_host_shortcut_lock && !rumble_mode_switch_shortcut_lock)
            {
                if(headset_plugged)
                {
                    local_current_volume = get_global_config().headset_volume - 100.0f; //Updating the volatile var for audio loop to use without a full flash save 
                }
                else
                {
                    local_current_volume = get_global_config().speaker_volume - 100.0f; //Updating the volatile var for audio loop to use without a full flash save 
                }
                
                current_auto_haptics_gain = get_global_config().auto_haptics_gain;
                request_temp_save = false;
                // 2. Enforce the update of the internal state[] reading the new config modified by the shortcuts
                uint8_t dummy_buffer[sizeof(SetStateData) + 1] = {0};
                state_update(dummy_buffer + 1, sizeof(SetStateData));
                
                // 3. Sending the new package with the new state[] to the controller
                uint8_t forced_pkt[78] = {0};
                forced_pkt[0] = 0x31;
                forced_pkt[1] = (uint8_t)(reportSeqCounter << 4);
                if (++reportSeqCounter == 256) reportSeqCounter = 0;
                forced_pkt[2] = 0x10;
                
                // Filling the forced package with the actual new state[]
                state_set(forced_pkt + 3, sizeof(SetStateData));
                
                // Sending by BT the forced package to update the controller
                bt_write(forced_pkt, sizeof(forced_pkt));
                request_flash_save = true;
                
            }
            suppress_all_inputs(data);
            
        }
        else//Exiting config state mode
        {
            //Only safe save to flash when out of config mode after request_flash_save is true
            if (request_flash_save) {
                
                save_requested = true;//Telling main loop to call flash save after some ms.
                request_flash_save = false;
                
                current_auto_haptics_gain = get_global_config().auto_haptics_gain;
                uint8_t dummy_buffer[sizeof(SetStateData) + 1] = {0};
                state_update(dummy_buffer + 1, sizeof(SetStateData));

                uint8_t forced_pkt[78] = {0};
                forced_pkt[0] = 0x31;
                forced_pkt[1] = (uint8_t)(reportSeqCounter << 4);
                if (++reportSeqCounter == 256) reportSeqCounter = 0;
                forced_pkt[2] = 0x10;
                
                state_set(forced_pkt + 3, sizeof(SetStateData));
                bt_write(forced_pkt, sizeof(forced_pkt));

            }
            if(headset_plugged)    
            {
                local_current_volume = get_global_config().headset_volume - 100.0f; //Updating the volatile var for audio loop to use without a full flash save
            }
            else
            {
                local_current_volume = get_global_config().speaker_volume - 100.0f; //Updating the volatile var for audio loop to use without a full flash save
            }
            set_volume(local_current_volume);
            Profile_Config_body local_profile_config = get_profile_config_index(local_profile_selected);
            //Hair trigger values override
            if(left_trigger_real_position>0)
            {
                if(local_profile_config.trigger_left_mode == 4)
                {
                    data[7] = 255;
                }
            }
            if(right_trigger_real_position>0)
            {
                //Hair trigger values override
                if(local_profile_config.trigger_right_mode == 4)
                {
                    data[8] = 255;
                }
            }
            
            
            //Only calculate gyro to analog values if the user has set a gyro button activator on the profile
            if(local_profile_config.gyro_button_activator > 0)
            {
                //Check if user is pressing the gyro button activator
                if((gyro_shortcut_btn_pressed(data, get_profile_config_index(local_profile_selected).gyro_button_activator-2))||(get_profile_config_index(local_profile_selected).gyro_button_activator==1))//If the user is pressing the gyro button activator or if it is set to always on, we calculate the gyro to analog values
                {
                    float local_analog_gyro_deadzone = get_profile_config_index(local_profile_selected).analog_gyro_deadzone;
                    
                    float final_right_stick_y = 0;
                    if((final_pct_x * ((127 - local_analog_gyro_deadzone) / 100.0f)) < 0.0f)//To 255
                    {
                        final_right_stick_y = (128 + local_analog_gyro_deadzone) - (final_pct_x * ((127 - local_analog_gyro_deadzone) / 100.0f));
                    }
                    else if((final_pct_x * ((127 - local_analog_gyro_deadzone) / 100.0f)) > 0.0f)
                    {
                        final_right_stick_y = (128 - local_analog_gyro_deadzone) - (final_pct_x * ((127 - local_analog_gyro_deadzone) / 100.0f));
                    }
                    else
                    {
                        final_right_stick_y = 128.0f;
                    }

                    float final_right_stick_x = 0;
                    if((final_pct_z * ((127 - local_analog_gyro_deadzone) / 100.0f)) < 0.0f)//To 255
                    {
                        final_right_stick_x = (128 + local_analog_gyro_deadzone) - (final_pct_z * ((127 - local_analog_gyro_deadzone) / 100.0f));
                    }
                    else if((final_pct_z * ((127 - local_analog_gyro_deadzone) / 100.0f)) > 0.0f)
                    {
                        final_right_stick_x = (128 - local_analog_gyro_deadzone) - (final_pct_z * ((127 - local_analog_gyro_deadzone) / 100.0f));
                    }
                    else
                    {
                        final_right_stick_x = 128.0f;
                    }

                    
                    uint8_t inverted_x = (local_profile_config.inverted_x_gyro) ? -1 : 1;
                    uint8_t inverted_y = (local_profile_config.inverted_y_gyro) ? -1 : 1;

                    uint8_t output_x = (uint8_t)std::clamp(std::round(final_right_stick_x), 0.0f, 255.0f) * inverted_x;
                    uint8_t output_y = (uint8_t)std::clamp(std::round(final_right_stick_y), 0.0f, 255.0f) * inverted_y;
                    

                    //Assigning the new values to the data[] to be sent to the host only if gyro to analog values are higher than the current user input ones.

                    if((output_x>128+local_analog_gyro_deadzone)&&(data[5]>128+local_analog_gyro_deadzone)&&(output_x>data[5]))//If the output_x is positive and higher than the current user input (higher input), we assign it to the data[] to be sent to the host
                    {
                        data[5] = output_x; // 0: Full Left - 128: Center - 255: Full Right
                    }
                    if((output_x<128-local_analog_gyro_deadzone)&&(data[5]<128-local_analog_gyro_deadzone)&&(output_x<data[5]))//If the output_x is negative and lower than the current user input (higher input), we assign it to the data[] to be sent to the host
                    {
                        data[5] = output_x; // 0: Full Left - 128: Center - 255: Full Right
                    }

                    if((output_y>128+local_analog_gyro_deadzone)&&(data[6]>128+local_analog_gyro_deadzone)&&(output_y>data[6]))//If the output_y is positive and higher than the current user input (higher input), we assign it to the data[] to be sent to the host
                    {
                        data[6] = output_y; // 0: Full Up   - 128: Center - 255: Full Down
                    }
                    if((output_y<128-local_analog_gyro_deadzone)&&(data[6]<128-local_analog_gyro_deadzone)&&(output_y<data[6]))//If the output_y is negative and lower than the current user input (higher input), we assign it to the data[] to be sent to the host
                    {
                        data[6] = output_y; // 0: Full Up   - 128: Center - 255: Full Down
                    }
                    if(data[5]<128+local_analog_gyro_deadzone && data[5]>128-local_analog_gyro_deadzone)//If the user input is inside the deadzone, we assign the gyro to analog value to the data[] to be sent to the host
                    {
                        data[5] = output_x; // 0: Full Left - 128: Center - 255: Full Right
                    }
                    if(data[6]<128+local_analog_gyro_deadzone && data[6]>128-local_analog_gyro_deadzone)//If the user input is inside the deadzone, we assign the gyro to analog value to the data[] to be sent to the host
                    {
                        data[6] = output_y; // 0: Full Up   - 128: Center - 255: Full Down
                    }

                    
                }
            }
            if((ghost_press_mute)&&(!exiting_config))
            {
                if(mute_button_press_time == 0)
                {
                    mute_button_press_time = to_ms_since_boot(get_absolute_time());
                }
                
                if (mute_button_press_time != 0)
                {
                    if(to_ms_since_boot(get_absolute_time()) - mute_button_press_time <= 250)
                    {
                        data[12] |= 0x04;//Press Mute for 100ms if released prior to enter config mode if get_global_config().time_config_mode > 0
                    }
                    else
                    {
                        mute_button_press_time = 0;
                        ghost_press_mute = false;
                    }
                }
            }
            if(exiting_config)
            {
                suppress_all_inputs(data);
            }
        }

        

        if (get_global_config().polling_rate_mode != 2) {
        memcpy(interrupt_in_data, data + 3, 63);
        
        #if ENABLE_BATT_LED
                    battery_led_note_report();
        #endif
            return;
        }

        

        critical_section_enter_blocking(&report_cs);
        memcpy(interrupt_in_data, data + 3, 63);
        
        report_dirty = true;
        critical_section_exit(&report_cs);
        #if ENABLE_BATT_LED
                battery_led_note_report();
        #endif

    }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
    (void) report_type;
    
    // if (itf == 1) itf=0; //Ignores Control requests on Consumer interface (itf=1) to avoid STALLs on the host side and reroutes them to DS itf
    if (is_pico_cmd(report_id)) {
        return pico_cmd_get(report_id, buffer, reqlen);
    }

    #if !USE_LINUX_USB_DESCRIPTORS
        if (report_id == HID_REPORT_ID_KEYBOARD) {
            return 0;
        }
    #endif

    std::vector<uint8_t> feature_data = get_feature_data(report_id, reqlen);
    if (!feature_data.empty()) {
        uint16_t len = (uint16_t)std::min((size_t)reqlen, feature_data.size() - 1);
        memcpy(buffer, feature_data.data() + 1, len);
        return len;
    }
    

    
    // BT feature data not yet cached — return zeros instead of STALLing.
    // hid_playstation calls GET_FEATURE(0x20) during probe; a STALL causes
    // it to fail and leaves the device without a driver.
    memset(buffer, 0, reqlen);
    return reqlen;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void) rhport;
    uint8_t const itf = tu_u16_low(p_request->wIndex); // wInterface
    uint8_t const alt = tu_u16_low(p_request->wValue); // bAlternateSetting

    if (itf == 1) {
        printf("[AUDIO] Set interface Speaker to alternate setting %d\n", alt);
        if (alt == 0 && spk_active) {
            state_init();
        }
        spk_active = alt;
    }
    

    return true;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;

    #if USE_LINUX_USB_DESCRIPTORS
        
        if (is_pico_cmd(report_id)) {
            printf("[HID] Receive 0xf6 setting config, funcid:0x%02X\n", buffer[0]);
            pico_cmd_set(report_id, buffer, bufsize);
            return;
        }

        // INTERRUPT OUT
        if (report_id == 0) {
            switch (buffer[0]) {
                case 0x02: {
                    state_update(buffer + 1, bufsize - 1);
                    last_rumble_report_us = to_us_since_boot(get_absolute_time());
                    uint8_t outputData[78]{};
                    outputData[0] = 0x31;
                    outputData[1] = reportSeqCounter << 4;
                    if (++reportSeqCounter == 256) {
                        reportSeqCounter = 0;
                    }
                    outputData[2] = 0x10;
                    // memcpy(outputData + 3, buffer + 1, bufsize - 1);
                    state_set(outputData + 3,sizeof(SetStateData));
                    bt_write(outputData, sizeof(outputData));
                    break;
                }
            }
        }
        if (report_id == 0x80 ||
            // DSE: Write Profile Block
            report_id == 0x60 ||
            report_id == 0x62 ||
            report_id == 0x61) {
            set_feature_data(report_id, const_cast<uint8_t *>(buffer), bufsize);
            return;
        }
    #else
        if (itf == 1) itf=0; //Ignores Control requests on Consumer interface (itf=1) to avoid STALLs on the host side and reroutes them to DS itf
        
        if (is_pico_cmd(report_id)) {
            printf("[HID] Receive 0xf6 setting config, funcid:0x%02X\n", buffer[0]);
            pico_cmd_set(report_id, buffer, bufsize);
            return;
        }

        auto forward_ds_output_report = [&](const uint8_t *payload, uint16_t payload_size) {
            state_update(payload, payload_size);
            last_rumble_report_us = to_us_since_boot(get_absolute_time());
            uint8_t outputData[78]{};
            outputData[0] = 0x31;
            outputData[1] = reportSeqCounter << 4;
            if (++reportSeqCounter == 256) {
                reportSeqCounter = 0;
            }
            outputData[2] = 0x10;
            state_set(outputData + 3, sizeof(SetStateData));
            bt_write(outputData, sizeof(outputData));
        };

        // INTERRUPT OUT:
        // - Some hosts send report_id=0 and put 0x02 in buffer[0].
        // - Others send report_id=0x02 directly with payload-only data.
        if (report_id == 0 && bufsize > 0 && buffer[0] == 0x02) {
            forward_ds_output_report(buffer + 1, bufsize - 1);
        } else if (report_id == 0x02) {
            const bool prefixed_payload = (bufsize > sizeof(SetStateData) && buffer[0] == 0x02);
            const uint8_t *payload = prefixed_payload ? (buffer + 1) : buffer;
            const uint16_t payload_size = prefixed_payload ? (bufsize - 1) : bufsize;
            forward_ds_output_report(payload, payload_size);
        }
        if (report_id == 0x80 ||
            // DSE: Write Profile Block
            report_id == 0x60 ||
            report_id == 0x62 ||
            report_id == 0x61) {
            set_feature_data(report_id, const_cast<uint8_t *>(buffer), bufsize);
            return;
        }
    #endif
}

// Periodic 0x31 keepalive: the DualSense haptic subsystem times out if no
// output report arrives for a few hundred ms. When a game crashes, HID reports
// stop but audio keeps flowing — this sends state every 20 ms so the actuators
// stay alive and 0x36 audio-haptics continue working.
static void state_keepalive() {
    static uint64_t last_us = 0;
    const uint64_t now = to_us_since_boot(get_absolute_time());
    if (now - last_us < 20000ULL) return;
    last_us = now;

    uint8_t pkt[78]{};
    pkt[0] = 0x31;
    pkt[1] = (uint8_t)(reportSeqCounter << 4);
    if (++reportSeqCounter == 256) reportSeqCounter = 0;
    pkt[2] = 0x10;
    
    state_set(pkt + 3, sizeof(SetStateData));
    // Read actual config
    const auto &cfg = get_profile_config();

    // Offset of Valid_Flag1 to push max amps to the trigger motors
    //size_t out_motor_flag_offset = 3 + offsetof(SetStateData, HostTimestamp) + sizeof(uint32_t);

    // Pushing trigger mode 3 (Machine Gun) every loop, otherwise it falls back to relax mode
    if (cfg.trigger_right_mode == 3) {
        size_t r_off = 3 + offsetof(SetStateData, RightTriggerFFB);
        

        // Only applies if the user is actually pushing the trigger beyond a threshold of 15 (0 to 255)
        if (right_trigger_real_position > 15) {
            set_bit(pkt[3], 2, true);
            //pkt[3 + 0] |= 0x04; // Valid_Flag0: Telling that the R2 is being updated
            /*if (out_motor_flag_offset < 78) {
                pkt[out_motor_flag_offset] |= 0x03; // Enforcing power stage
            }*/

            
            pkt[r_off + 0] = 0x06; // L2/R2 trigger effect mode (Machine Gun)
            pkt[r_off + 1] = get_profile_config().vibration_frequency; ; // Parameter 1: Frecuency
            pkt[r_off + 2] = get_profile_config().vibration_force; // Parameter 2: Force
            pkt[r_off + 3] = get_profile_config().vibration_start_point; // Parameter 3: Start Point
            memset(&pkt[r_off + 4], 0, 7);
            
        } else {
            // Full relax state after releasing the finger
            pkt[r_off + 0] = 0x05; 
            memset(&pkt[r_off + 1], 0, 10);
        }
    }

    
    if (cfg.trigger_left_mode == 3) {
        
        size_t l_off = 3 + offsetof(SetStateData, LeftTriggerFFB);
        if (left_trigger_real_position > 15) {
            set_bit(pkt[3], 3, true);
            //pkt[3 + 0] |= 0x08; // Valid_Flag0: Telling that the L2 is being updated
            /*if (out_motor_flag_offset < 78) {
                pkt[out_motor_flag_offset] |= 0x03; 
            }*/

            pkt[l_off + 0] = 0x06; 
            pkt[l_off + 1] = get_profile_config().vibration_frequency; ; // Parameter 1: Frecuency
            pkt[l_off + 2] = get_profile_config().vibration_force; // Parameter 2: Force
            pkt[l_off + 3] = get_profile_config().vibration_start_point; // Parameter 3: Start Point
            memset(&pkt[l_off + 4], 0, 7);
        } else {
            pkt[l_off + 0] = 0x05; 
            memset(&pkt[l_off + 1], 0, 10);
        }
    }

    
    pkt[4] |= 0x04;// valid_flag1: LIGHTBAR_CONTROL_ENABLE (bit 2)
    pkt[47] = lb_controlled_red;   // lightbar_red
    pkt[48] = lb_controlled_green;   // lightbar_green
    pkt[49] = lb_controlled_blue;   // lightbar_blue
    pkt[4] |= 0x01;//Enable Mute Light Control
    //if in config state mode 
    if(config_mode_enabled)
    {
        size_t mute_light_mode_offset = 3 + offsetof(SetStateData, MuteLightMode);
        pkt[mute_light_mode_offset] = MuteLight::Breathing; //Mute Light Breathing on config mode 3+8
    }
    else
    {
        size_t mute_light_mode_offset = 3 + offsetof(SetStateData, MuteLightMode);
        pkt[mute_light_mode_offset] = MuteLight::Off; //Mute Light Off on config mode 3+8
    }
    //Profile to player lights
    if(local_profile_selected == 0)
    {
        pkt[46] = 0x04;
    }
    else if(local_profile_selected == 1)
    {
        pkt[46] = 0x02;
    }
    else if(local_profile_selected == 2)
    {
        pkt[46] = 0x15;
    }
    else if(local_profile_selected == 3)
    {
        pkt[46] = 0x1B;
    }
    
    bt_write(pkt, sizeof(pkt));
}

int main() {
#if SYS_CLOCK_KHZ != 150000
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(1000);
    set_sys_clock_khz(SYS_CLOCK_KHZ, true);
#endif

    board_init();
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);
#if !ENABLE_SERIAL
    tud_disconnect();
#endif
    board_init_after_tusb();
#if ENABLE_SERIAL
    stdio_usb_init();
#endif

    if (cyw43_arch_init()) {
        printf("Failed to initialize CYW43\n");
        return 1;
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);

#if ENABLE_BATT_LED
    battery_led_init();
#endif


#if !ENABLE_SERIAL
    if (watchdog_caused_reboot()) {
        printf("Rebooted by Watchdog!\n");
        // 当崩溃重启以后，闪三下灯
        for (int i = 0; i < 6; i++) {
            if (i % 2 == 0) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
            } else {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
            }
            sleep_ms(500);
        }
    } else {
        printf("Clean boot\n");
    }
#endif

    // Initialize the critical section for the report buffer
    critical_section_init(&report_cs);

    device_config_load();
    // Touchpad always starts active; enable_touchpad controls whether the PS+button
    // shortcut is available (1 = shortcut enabled, 0 = touchpad always on, no toggle).
    touchpad_runtime_enabled = true;

    wake_init();

    bt_init();
    bt_register_data_callback(on_bt_data);

    audio_init();
    state_init();

#if !ENABLE_SERIAL
    // Prime USB before arming the watchdog so enumeration completes first.
    tud_task();
    watchdog_enable(4000, true);
#endif

    while (1) {
#if !ENABLE_SERIAL
        watchdog_update();
#endif
        /*{
            static uint32_t last_synth_tick_ms = 0;
            const uint32_t now = to_ms_since_boot(get_absolute_time());
            if (now - last_synth_tick_ms >= 50) {
                last_synth_tick_ms = now;
                if (state_synth_tick()) {
                    uint8_t outputData[78]{};
                    outputData[0] = 0x31;
                    outputData[1] = reportSeqCounter << 4;
                    if (++reportSeqCounter == 256) reportSeqCounter = 0;
                    outputData[2] = 0x10;
                    state_set(outputData + 3, sizeof(SetStateData));
                    bt_write(outputData, sizeof(outputData));
                }
            }
        }*/
        cyw43_arch_poll();
        tud_task();
        process_media_keys();
        //original audio loop location
        audio_loop();
        interrupt_loop();
        // Run keepalive when audio haptics are active OR when game motors are on.
        // This sustains vibration for game rumble (DualSense needs periodic 0x31).
        static uint32_t reconnect_start_time = 0;
        if (spk_active || state_motors_active()) {
            // Safety: if no USB output report for 2s and audio is off, zero motors
            // (handles game crash without proper cleanup).
            if (!spk_active && state_motors_active()) {
                const uint64_t now = to_us_since_boot(get_absolute_time());
                if (now - last_rumble_report_us > 2000000ULL) {
                    state_clear_motors();
                }
            }
            
        }
        
        if (usb_reconnect_requested) {
            if (usb_is_enabled) {
                tud_disconnect();
                usb_is_enabled = false; // Avoiding spam
            }
            reconnect_start_time = to_ms_since_boot(get_absolute_time());
            usb_reconnect_requested = false;
        }

        if(save_config_now)//Only requested by web app on 0xF6 0x02 flash write
        {
            save_config_now = false;
            save_now_update();
        }
        // Petition to save later
        if (save_requested) {
            pending_save_time = to_ms_since_boot(get_absolute_time());
            
            save_requested = false; // Avoiding spam
        }

        // Only safe save after a 150ms cooldown to avoid TinyUSB to not being able to answer to the host
        if (pending_save_time != 0 && (to_ms_since_boot(get_absolute_time()) - pending_save_time > 500)) {
            safe_config_save();
            pending_save_time = 0;
        }

        
        //test audio loop location
        if(check_lb_again)//Enforce to check lightbar host color after a cooldown to avoid flashing between 2 host color request
        {
            uint32_t tiempo_actual_local = to_ms_since_boot(get_absolute_time());
            if (tiempo_actual_local - last_time_check_lb > 3050) {
                uint8_t dummy_buffer[sizeof(SetStateData) + 1] = {0};
                state_update(dummy_buffer + 1, sizeof(SetStateData));
                last_time_check_lb = tiempo_actual_local;
            }

        }
        else
        {
            last_time_check_lb = to_ms_since_boot(get_absolute_time());
        }
        
        state_keepalive(); //Running every cycle to keep alive the DualSense haptic subsystem and lightbar effects
        
        lightbar_loop();

#if ENABLE_BATT_LED
        battery_led_tick();
#endif

    }
}
