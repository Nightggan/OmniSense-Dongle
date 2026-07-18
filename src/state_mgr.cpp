//
// Created by awalol on 2026/5/15.
//

#include <cstddef>
#include <cstring>

#include "pico.h"
#include "utils.h"
#include <pico/time.h>
#include "config.h"
#include <cmath>


//Custom vars Omni
extern uint8_t host_real_color_r;
extern uint8_t host_real_color_g;
extern uint8_t host_real_color_b;
extern uint8_t lb_controlled_red;
extern uint8_t lb_controlled_green;
extern uint8_t lb_controlled_blue;
extern bool check_lb_again;
uint8_t temp_host_puro_r = 0;
uint8_t temp_host_puro_g = 0;
uint8_t temp_host_puro_b = 0;
bool different_color = false;
bool real_color_captured = false;
static uint32_t last_time_check_lb = 0;
bool first_color_captured = false;
volatile bool trigger_left_mode_0_engaged = false;
volatile bool trigger_right_mode_0_engaged = false;
extern bool config_mode_enabled;
extern volatile float local_current_volume;
extern volatile bool headset_plugged;
extern uint8_t local_profile_selected;
extern uint8_t right_trigger_real_position;
extern uint8_t left_trigger_real_position;

//End Custom vars Omni
namespace {
    constexpr size_t kAudioControlOffset = offsetof(SetStateData, MuteLightMode) - sizeof(uint8_t);
    constexpr size_t kMuteControlOffset = offsetof(SetStateData, RightTriggerFFB) - sizeof(uint8_t);
    constexpr size_t kMotorPowerLevelOffset = offsetof(SetStateData, HostTimestamp) + sizeof(uint32_t);
    constexpr size_t kAudioControl2Offset = kMotorPowerLevelOffset + sizeof(uint8_t);
    constexpr size_t kHapticLowPassFilterOffset = offsetof(SetStateData, LightFadeAnimation) - 2 * sizeof(uint8_t);
    constexpr size_t kPlayerIndicatorsOffset = offsetof(SetStateData, LedRed) - sizeof(uint8_t);
}

static constexpr uint8_t state_init_data[63] = {
    0xfd, 0xf7, 0x0, 0x0,
    0x7f, 0x64, // Headphones, Speaker
    0xff, 0x9, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xa,
    0x7, 0x0, 0x0, 0x2, 0x1,
    0x00,
    0xff, 0xd7, 0x00 // RGB LED: R, G, B (Nijika Color!)✨
};

uint8_t state[63]{};
static volatile uint16_t g_rumble_emulation = 0;

void state_init() {
    memcpy(state, state_init_data, sizeof(state));
    g_rumble_emulation = 0;
}

void __not_in_flash_func(state_set)(uint8_t *data, const uint8_t size) {
    if (size > 63) {
        printf("[StateMgr] Warning: State Set over 63 bytes\n");
    }
    memcpy(data, state, size);
}

void state_update(const uint8_t *data, const uint8_t size) {
    if (size < sizeof(SetStateData)) {
        printf(
            "[StateMgr] Error: SetStateData at least %u bytes\n",
            static_cast<unsigned>(sizeof(SetStateData))
        );
        return;
    }

    //Validating if a dummy_buffer is being sent. In that case the state was already updated and here we only tell main.cpp to check lightbar color after some delay
    static const uint8_t zero_buffer[sizeof(SetStateData)] = {0};
    // memcmp returns 0 if both memory blocks are the same
    bool dummy_buffer_received = (memcmp(data, zero_buffer, sizeof(SetStateData)) == 0);
    uint32_t actual_time = to_ms_since_boot(get_absolute_time());

    if (!dummy_buffer_received) {//real buffer received
        // Only obtaining the host color if a real buffer is received and only to a temporal var
        temp_host_puro_r = data[offsetof(SetStateData, LedRed)];
        temp_host_puro_g = data[offsetof(SetStateData, LedRed) + 1];
        temp_host_puro_b = data[offsetof(SetStateData, LedRed) + 2];
        
        if(!first_color_captured)//Only runs once per power cycle
        {
            host_real_color_r = temp_host_puro_r;//Applying the received color to the final var
            host_real_color_g = temp_host_puro_g;
            host_real_color_b = temp_host_puro_b;
            first_color_captured = true;
        }else
        {
            if((temp_host_puro_r!=0)||(temp_host_puro_g!=0)||(temp_host_puro_b!=0))//Checking if the host is not sending a black color (off)
            {
                //Checking if the color is different from the previous loop
                different_color  = (host_real_color_r!=temp_host_puro_r)||(host_real_color_g!=temp_host_puro_g)||(host_real_color_b!=temp_host_puro_b);
                real_color_captured = true; //Regardless we tell that a new real color is being received (only if not off)
            }
            else
            {
                real_color_captured = false; //received a black color (off)
                
            }

            if(different_color)//If the color is different from the prior loop
            {
                if(real_color_captured)
                    if (actual_time - last_time_check_lb < 3000) {
                        // If 3 seconds haven't passed we enforce the prior obtained color (the one that is on the controller)
                        // to the global state[]
                        // And tell main.cpp and this loop to check the color later. main.cpp does this also if this loops does not run again for any reason
                        // This avoids a flashing lightbar if two hosts actively push different colors.
                        uint8_t *mutable_data = const_cast<uint8_t*>(data);
                        mutable_data[offsetof(SetStateData, LedRed)]     = host_real_color_r;
                        mutable_data[offsetof(SetStateData, LedRed) + 1] = host_real_color_g;
                        mutable_data[offsetof(SetStateData, LedRed) + 2] = host_real_color_b;
                        check_lb_again = true; //
                        
                    } else {
                        // If 3 seconds passes without the first host sending the original color, we let the new host color to be applied.
                        // For lightbar_controller to be used as a base color to make the calculations on top of it
                        host_real_color_r = temp_host_puro_r;
                        host_real_color_g = temp_host_puro_g;
                        host_real_color_b = temp_host_puro_b;
                        // For lightbar_controller use to actively push the new color regardless if it is in the middle of a breathing effect:
                        lb_controlled_red = temp_host_puro_r; 
                        lb_controlled_green = temp_host_puro_g;
                        lb_controlled_blue = temp_host_puro_b;

                        check_lb_again = false;
                        real_color_captured = false;
                        different_color = false;
                        
                    }
                
            }
            else{
                check_lb_again = false;
                last_time_check_lb = actual_time;
                
            }
        }

        
    } 
    else{
        
        if(check_lb_again)
        {
            if(real_color_captured)
            {
                if (actual_time - last_time_check_lb >= 3000) {
                    // Repeting the process above but in case a dummy_buffer was sent to update the state
                    host_real_color_r = temp_host_puro_r;
                    host_real_color_g = temp_host_puro_g;
                    host_real_color_b = temp_host_puro_b;
                    lb_controlled_red = temp_host_puro_r;
                    lb_controlled_green = temp_host_puro_g;
                    lb_controlled_blue = temp_host_puro_b;
                    real_color_captured = false;            
                    check_lb_again = false;
                    last_time_check_lb = actual_time;
                    different_color = false;
                    
                }
            }
        }
        else
        {
            last_time_check_lb = actual_time;
            
        }
    }
    
    SetStateData update{};
    memcpy(&update, data, sizeof(update));
    g_rumble_emulation = static_cast<uint16_t>(update.RumbleEmulationRight) |
                         (static_cast<uint16_t>(update.RumbleEmulationLeft) << 8);

    const auto copy_if_allowed = [&](const bool allowed, const size_t offset, const size_t length) {
        if (allowed) {
            memcpy(state + offset, data + offset, length);
        }
    };
    auto set_bit = [](uint8_t &byte, const int bit, const bool value) {
        byte = (byte & ~(1 << bit)) | (value << bit);
    };

    // bit0=1: EnableRumbleEmulation (toujours actif)
    // bit1 (UseRumbleNotHaptics): seul le mode legacy/bypass (=1) fait vibrer
    //   les moteurs sur cette manette (le mode DSP =0 ne vibre jamais ici).
    //   On force donc bit1=1 dès qu'une valeur moteur non nulle arrive du jeu.
    //   Quand il n'y a pas de rumble, on laisse bit1=0 pour que le flux PCM
    //   0x36 (audio-haptics) reste traité. state_update() n'étant appelé que
    //   sur rapport HID, l'audio seul conserve l'état init (bit1=0).
    const bool motors_on = (update.RumbleEmulationRight || update.RumbleEmulationLeft);
    const auto &current_config = get_profile_config();
    
    set_bit(state[0], 0, true);
    set_bit(state[0], 1, motors_on);
    set_bit(state[38], 2, true);
    /*copy_if_allowed(
        true,
        offsetof(SetStateData, RumbleEmulationRight),
        2
    );*/ //Right and Left

    //Always allow Rumble Emulation to be set by the host
    memcpy(
        state + offsetof(SetStateData, RumbleEmulationRight), 
        data + offsetof(SetStateData, RumbleEmulationRight),
        sizeof(uint8_t)
    );

    memcpy(
        state + offsetof(SetStateData, RumbleEmulationLeft), 
        data + offsetof(SetStateData, RumbleEmulationLeft),
        sizeof(uint8_t)
    );

    //set_bit(state[kMotorPowerLevelOffset], 0, true);
    //set_bit(state[kMotorPowerLevelOffset], 1, true);
    
    /*
    size_t motor_flag_offset = offsetof(SetStateData, HostTimestamp) + sizeof(uint32_t); 
    if (motor_flag_offset < 63) {
        // Copying what the Host is asking to not break the haptics modes
        state[motor_flag_offset] = data[motor_flag_offset];
        
        // Enforcing Bit 0 (Compatibility with classic rumble)
        // Enforcing Bit 1 (Enabling Haptic Actuators)
        set_bit(state[motor_flag_offset], 0, true);
        set_bit(state[motor_flag_offset], 1, true);
    }*/

    
    //Always allow headphone volume control
    memcpy(
        state + offsetof(SetStateData, VolumeHeadphones), 
        data + offsetof(SetStateData, VolumeHeadphones),
        sizeof(uint8_t)
    );

    /*copy_if_allowed(
        update.AllowHeadphoneVolume,
        offsetof(SetStateData, VolumeHeadphones),
        sizeof(update.VolumeHeadphones)
    );*/
    /*copy_if_allowed(
        update.AllowSpeakerVolume,
        offsetof(SetStateData, VolumeSpeaker),
        sizeof(update.VolumeSpeaker)
    );*/
    /*copy_if_allowed(
        update.AllowMicVolume,
        offsetof(SetStateData, VolumeMic),
        sizeof(update.VolumeMic)
    );*/
    /*copy_if_allowed(
        update.AllowAudioControl,
        kAudioControlOffset,
        sizeof(uint8_t)
    );*/

    /*copy_if_allowed(
        update.AllowMuteLight,
        offsetof(SetStateData, MuteLightMode),
        sizeof(update.MuteLightMode)
    );*/

    //Always allow mute light control
    memcpy(
        state + offsetof(SetStateData, MuteLightMode), 
        data + offsetof(SetStateData, MuteLightMode),
        sizeof(uint8_t)
    );

    /*copy_if_allowed(
        update.AllowAudioMute,
        kMuteControlOffset,
        sizeof(uint8_t)
    );*/

    copy_if_allowed(
        update.AllowRightTriggerFFB,
        offsetof(SetStateData, RightTriggerFFB),
        sizeof(update.RightTriggerFFB)
    );
    copy_if_allowed(
        update.AllowLeftTriggerFFB,
        offsetof(SetStateData, LeftTriggerFFB),
        sizeof(update.LeftTriggerFFB)
    );

    /*copy_if_allowed(
        update.AllowMotorPowerLevel,
        kMotorPowerLevelOffset,
        sizeof(uint8_t)
    );*/
    /*copy_if_allowed(
        update.AllowAudioControl2,
        kAudioControl2Offset,
        sizeof(uint8_t)
    );*/
    /*copy_if_allowed(
        update.AllowHapticLowPassFilter,
        kHapticLowPassFilterOffset,
        sizeof(uint8_t)
    );*/

    copy_if_allowed(
        update.AllowColorLightFadeAnimation,
        offsetof(SetStateData, LightFadeAnimation),
        sizeof(update.LightFadeAnimation)
    );
    copy_if_allowed(
        update.AllowLightBrightnessChange,
        offsetof(SetStateData, LightBrightness),
        sizeof(update.LightBrightness)
    );

    //Always allow player indicator control for profiles
    /*copy_if_allowed(
        update.AllowPlayerIndicators,
        kPlayerIndicatorsOffset,
        sizeof(uint8_t)
    );*/

    memcpy(
        state + kPlayerIndicatorsOffset, 
        data + kPlayerIndicatorsOffset,
        sizeof(uint8_t)
    );
    
    /*
    copy_if_allowed(
        update.AllowLedColor,
        offsetof(SetStateData, LedRed),
        sizeof(update.LedRed) * 3
    );
    */

    //Always allow lighbar control
    memcpy(
        state + offsetof(SetStateData, LedRed), 
        data + offsetof(SetStateData, LedRed), 
        sizeof(update.LedRed) * 3
    );

    //Profile to player lights
    if(local_profile_selected == 0)
    {
        state[kPlayerIndicatorsOffset] = 0x04;
    }
    else if(local_profile_selected == 1)
    {
        state[kPlayerIndicatorsOffset] = 0x02;
    }
    else if(local_profile_selected == 2)
    {
        state[kPlayerIndicatorsOffset] = 0x15;
    }
    else if(local_profile_selected == 3)
    {
        state[kPlayerIndicatorsOffset] = 0x1B;
    }
    
    // Enforce trigger modes at the end of state_update
    size_t right_trigger_offset = offsetof(SetStateData, RightTriggerFFB);
    size_t left_trigger_offset = offsetof(SetStateData, LeftTriggerFFB);
    
    if(current_config.trigger_left_mode == 0 && trigger_left_mode_0_engaged == false) {
        trigger_left_mode_0_engaged = true;
        memset(state + left_trigger_offset, 0, 11);// Reset left trigger state
    }

    if(current_config.trigger_right_mode == 0 && trigger_right_mode_0_engaged == false) {
        trigger_right_mode_0_engaged = true;
        memset(state + right_trigger_offset, 0, 11);// Reset right trigger state
    }
    
    // Right Trigger
    if (current_config.trigger_right_mode != 0) {
        trigger_right_mode_0_engaged = false;
        // Other modes: Forcing validity bits on state[0] and Valid_Flag1
        set_bit(state[0], 2, true); 
        /*if (kMotorPowerLevelOffset < 63) {
            state[kMotorPowerLevelOffset] |= 0x03; // Calibration and active power stage
        }*/

        if (current_config.trigger_right_mode == 1) { // Resistance mode
            state[right_trigger_offset + 0] = 0x01; // Continous Resistance
            state[right_trigger_offset + 1] = current_config.feedback_start_point;    // Start of the effect
            state[right_trigger_offset + 2] = current_config.feedback_force;  // Hardeness
            memset(state + right_trigger_offset + 3, 0, 8);
        }
        else if (current_config.trigger_right_mode == 2) { // Trigger mode
            state[right_trigger_offset + 0] = 0x02; // Trigger mode
            state[right_trigger_offset + 1] = current_config.trigger_start_point;   // Start of the trigger wall
            state[right_trigger_offset + 2] = current_config.trigger_wall_point;   // End of the mechanical "clic"
            state[right_trigger_offset + 3] = current_config.trigger_break_force;  // Force to break the wall
            memset(state + right_trigger_offset + 4, 0, 7);
        }
        else if (current_config.trigger_right_mode == 3) { // Machine Gun mode
            if (right_trigger_real_position > 15) {
                state[right_trigger_offset + 0] = 0x06; 
                state[right_trigger_offset + 1] = get_profile_config().vibration_frequency; ; // Parameter 1: Frecuency
                state[right_trigger_offset + 2] = get_profile_config().vibration_force; // Parameter 2: Force
                state[right_trigger_offset + 3] = get_profile_config().vibration_start_point; // Parameter 3: Start Point
                memset(state + right_trigger_offset + 4, 0, 7);
            } else {
                state[right_trigger_offset + 0] = 0x05;  
                memset(state + right_trigger_offset + 4, 0, 7);
            }
        }
        else if (current_config.trigger_right_mode == 4) { // Hair Trigger mode
            state[right_trigger_offset + 0] = 0x02; // Trigger mode
            state[right_trigger_offset + 1] = current_config.hair_wall_start_point;   // Start of the trigger wall
            state[right_trigger_offset + 2] = 255;   // End of the mechanical "clic"
            state[right_trigger_offset + 3] = 255;  // Force to break the wall
            memset(state + right_trigger_offset + 4, 0, 7);
        }
        else if (current_config.trigger_right_mode == 5) {// Rumble to Trigger
            
            uint16_t amp = (uint16_t)update.RumbleEmulationRight * (uint16_t)current_config.rumble_trigger_strength / 100u;
            if (amp > 255) amp = 255;
            for (int i = 0; i < 11; ++i) 
                state[right_trigger_offset + i] = 0;//Cleans trigger parameters
            // On-press gate: below ~25% pull, emit Off so the trigger stays quiet
            // until the user actually presses it.
            if (current_config.rumble_trigger_on_press && right_trigger_real_position < 64) 
            { 
                state[right_trigger_offset + 0] = 0x05; 
            }
            else
            {
                if (amp == 0) 
                { 
                    state[right_trigger_offset + 0] = 0x05; 
                    
                }
                else
                {
                    state[right_trigger_offset + 0] = 0x26; // Vibration
                    // Full-travel zones now (position gating handles "on press"); this keeps
                    // the buzz feeling consistent once engaged rather than only in a sub-band.
                    const uint16_t zones = 0x03FF;
                    state[right_trigger_offset + 1] = (uint8_t)(zones & 0xFF);
                    state[right_trigger_offset + 2] = (uint8_t)((zones >> 8) & 0x03);
                    
                    uint8_t val3 = (uint8_t)(amp * 7u / 255u);
                    uint32_t bits = 0;
                    for (int z = 0; z <= 9; ++z)
                        if (zones & (1u << z)) bits |= (uint32_t)(val3 & 0x7u) << (3 * z);
                    state[right_trigger_offset + 3] = (uint8_t)(bits & 0xFF);
                    state[right_trigger_offset + 4] = (uint8_t)((bits >> 8) & 0xFF);
                    state[right_trigger_offset + 5] = (uint8_t)((bits >> 16) & 0xFF);
                    state[right_trigger_offset + 6] = (uint8_t)((bits >> 24) & 0xFF);
                    state[right_trigger_offset + 9] = current_config.rumble_trigger_frequency;
                }
            }
            
            
        }
    }

    // Left Trigger
    //set_bit(state[0], 3, true);
        //memcpy(state + left_trigger_offset, data + left_trigger_offset, 11);
    if (current_config.trigger_left_mode != 0) {
        trigger_left_mode_0_engaged = false;
        set_bit(state[0], 3, true); 
        /*if (kMotorPowerLevelOffset < 63) {
            state[kMotorPowerLevelOffset] |= 0x03; 
        }*/

        if (current_config.trigger_left_mode == 1) {
            state[left_trigger_offset + 0] = 0x01; 
            state[left_trigger_offset + 1] = current_config.feedback_start_point;    
            state[left_trigger_offset + 2] = current_config.feedback_force;  
            memset(state + left_trigger_offset + 3, 0, 8);
        }
        else if (current_config.trigger_left_mode == 2) {
            state[left_trigger_offset + 0] = 0x02; 
            state[left_trigger_offset + 1] = current_config.trigger_start_point;   
            state[left_trigger_offset + 2] = current_config.trigger_wall_point;   
            state[left_trigger_offset + 3] = current_config.trigger_break_force;  
            memset(state + left_trigger_offset + 4, 0, 7);
        }
        else if (current_config.trigger_left_mode == 3) { // Machine Gun mode
            if (left_trigger_real_position > 15) {
                state[left_trigger_offset + 0] = 0x06; 
                state[left_trigger_offset + 1] = get_profile_config().vibration_frequency; ; // Parameter 1: Frecuency
                state[left_trigger_offset + 2] = get_profile_config().vibration_force; // Parameter 2: Force
                state[left_trigger_offset + 3] = get_profile_config().vibration_start_point; // Parameter 3: Start Point
                memset(state + left_trigger_offset + 4, 0, 7);
            } else {
                state[left_trigger_offset + 0] = 0x05;  
                memset(state + left_trigger_offset + 4, 0, 7);
            }
        }
        else if (current_config.trigger_left_mode == 4) { // Hair Trigger mode
            state[left_trigger_offset + 0] = 0x02; // Trigger mode
            state[left_trigger_offset + 1] = current_config.hair_wall_start_point; // Start of the wall
            state[left_trigger_offset + 2] = 255;   // End of the mechanical "clic"
            state[left_trigger_offset + 3] = 255;  // Force to break the wall
            memset(state + left_trigger_offset + 4, 0, 7);
        }
        else if (current_config.trigger_left_mode == 5) {
            uint16_t amp = (uint16_t)update.RumbleEmulationLeft * (uint16_t)current_config.rumble_trigger_strength / 100u;
            if (amp > 255) 
                amp = 255;

            for (int i = 0; i < 11; ++i) 
                state[left_trigger_offset + i] = 0;
            // On-press gate: below ~25% pull, emit Off so the trigger stays quiet
            // until the user actually presses it.
            if (current_config.rumble_trigger_on_press && left_trigger_real_position < 64) 
            { 
                state[left_trigger_offset + 0] = 0x05;
                
            }
            else
            {
                if (amp == 0) 
                { 
                    state[left_trigger_offset + 0] = 0x05; 
                }
                else
                {
                    state[left_trigger_offset + 0] = 0x26; // Vibration
                    // Full-travel zones now (position gating handles "on press"); this keeps
                    // the buzz feeling consistent once engaged rather than only in a sub-band.
                    const uint16_t zones = 0x03FF;
                    state[left_trigger_offset + 1] = (uint8_t)(zones & 0xFF);
                    state[left_trigger_offset + 2] = (uint8_t)((zones >> 8) & 0x03);
                    uint8_t val3 = (uint8_t)(amp * 7u / 255u);
                    uint32_t bits = 0;
                    for (int z = 0; z <= 9; ++z)
                        if (zones & (1u << z)) bits |= (uint32_t)(val3 & 0x7u) << (3 * z);
                    state[left_trigger_offset + 3] = (uint8_t)(bits & 0xFF);
                    state[left_trigger_offset + 4] = (uint8_t)((bits >> 8) & 0xFF);
                    state[left_trigger_offset + 5] = (uint8_t)((bits >> 16) & 0xFF);
                    state[left_trigger_offset + 6] = (uint8_t)((bits >> 24) & 0xFF);
                    state[left_trigger_offset + 9] = current_config.rumble_trigger_frequency;
                }
                
            }
        }
    }

    //state[04] = 0x04;
    size_t led_red_offset = offsetof(SetStateData, LedRed);

    state[led_red_offset] = lb_controlled_red;   // Calculated colors
    state[led_red_offset + 1] = lb_controlled_green;
    state[led_red_offset + 2] = lb_controlled_blue;
    
    size_t mute_light_mode_offset = offsetof(SetStateData, MuteLightMode);
    //If in config state mode 
    if(config_mode_enabled)
    {
        state[mute_light_mode_offset] = MuteLight::Breathing; //Mute Light Breathing on config mode
    }
    else
    {
        state[mute_light_mode_offset] = MuteLight::Off; //Mute Light off on not config mode
    }
    if(headset_plugged)
    {
        size_t headphone_volume_offset = offsetof(SetStateData, VolumeHeadphones);
        float headphone_volume_float = local_current_volume + 100;
        headphone_volume_float *= 1.27;
        uint8_t headphone_volume_byte = static_cast<uint8_t>(std::round(headphone_volume_float));
        state[headphone_volume_offset] = headphone_volume_byte;
    }
}

void state_set_led_color(uint8_t r, uint8_t g, uint8_t b) {
    state[offsetof(SetStateData, LedRed)]   = r;
    state[offsetof(SetStateData, LedGreen)] = g;
    state[offsetof(SetStateData, LedBlue)]  = b;
}

void state_get_rumble_emulation(uint8_t *right, uint8_t *left) {
    const uint16_t rumble = g_rumble_emulation;
    if (right != nullptr) {
        *right = static_cast<uint8_t>(rumble & 0xFFu);
    }
    if (left != nullptr) {
        *left = static_cast<uint8_t>((rumble >> 8) & 0xFFu);
    }
}

bool state_motors_active() {
    return state[offsetof(SetStateData, RumbleEmulationRight)] != 0 ||
           state[offsetof(SetStateData, RumbleEmulationLeft)]  != 0;
}

void state_clear_motors() {
    state[offsetof(SetStateData, RumbleEmulationRight)] = 0;
    state[offsetof(SetStateData, RumbleEmulationLeft)]  = 0;
    g_rumble_emulation = 0;
}

static uint8_t  g_host_cache[sizeof(SetStateData)];
static uint8_t  g_host_cache_size = 0;
static uint32_t g_last_host_ms = 0;
