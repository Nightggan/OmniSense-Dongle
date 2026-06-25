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
extern bool spk_active;

//Custom vars Omega
extern uint8_t host_puro_r;
extern uint8_t host_puro_g;
extern uint8_t host_puro_b;
extern uint8_t lb_controlled_red;
extern uint8_t lb_controlled_green;
extern uint8_t lb_controlled_blue;
extern bool check_lb_again;
uint8_t temp_host_puro_r = 0;
uint8_t temp_host_puro_g = 0;
uint8_t temp_host_puro_b = 0;
bool color_diferente = false;
bool real_color_captured = false;
static uint32_t ultimo_tiempo_check_lb = 0;
bool first_color_captured = false;
extern bool config_mode_enabled;
extern volatile float current_speaker_volume;
//End Custom vars Omega
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

void state_init() {
    memcpy(state, state_init_data, sizeof(state));
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

    //Custom buffer overwrite

    // 🛠️ DETECCIÓN QUIRÚRGICA DEL DUMMY BUFFER:
    // Creamos una referencia estática llena de ceros del mismo tamaño
    static const uint8_t zero_buffer[sizeof(SetStateData)] = {0};

    // memcmp devuelve 0 si ambos bloques de memoria son exactamente idénticos
    bool es_dummy_buffer = (memcmp(data, zero_buffer, sizeof(SetStateData)) == 0);
    uint32_t tiempo_actual = to_ms_since_boot(get_absolute_time());

    if (!es_dummy_buffer) {
        // CORREGIDO: Solo actualizamos host_puro si el paquete viene de la PC con datos reales
        temp_host_puro_r = data[offsetof(SetStateData, LedRed)];
        temp_host_puro_g = data[offsetof(SetStateData, LedRed) + 1];
        temp_host_puro_b = data[offsetof(SetStateData, LedRed) + 2];
        

        

        if(!first_color_captured)
        {
            host_puro_r = temp_host_puro_r;
            host_puro_g = temp_host_puro_g;
            host_puro_b = temp_host_puro_b;
            first_color_captured = true;
        }else
        {
            if((temp_host_puro_r!=0)||(temp_host_puro_g!=0)||(temp_host_puro_b!=0))
            {
                color_diferente  = (host_puro_r!=temp_host_puro_r)||(host_puro_g!=temp_host_puro_g)||(host_puro_b!=temp_host_puro_b);
                real_color_captured = true;
            }
            else
            {
                real_color_captured = false;
                
            }

            if(color_diferente)
            {
                if(real_color_captured)
                    if (tiempo_actual - ultimo_tiempo_check_lb < 3000) {
                        // 🔒 INTERCEPTACIÓN MAESTRA: Forzamos que el buffer 'data' que va a copiarse
                        // en el state[] global mantenga el color del host primario (Steam).
                        // Esto evita que los ceros se propaguen a tud_hid_set_report_cb.
                        uint8_t *mutable_data = const_cast<uint8_t*>(data);
                        mutable_data[offsetof(SetStateData, LedRed)]     = host_puro_r;
                        mutable_data[offsetof(SetStateData, LedRed) + 1] = host_puro_g;
                        mutable_data[offsetof(SetStateData, LedRed) + 2] = host_puro_b;
                        check_lb_again = true;
                        
                    } else {
                        // Si pasaron más de 3 segundos sin que Steam mande nada, permitimos el cambio de color
                        host_puro_r = temp_host_puro_r;
                        host_puro_g = temp_host_puro_g;
                        host_puro_b = temp_host_puro_b;
                        lb_controlled_red = temp_host_puro_r;
                        lb_controlled_green = temp_host_puro_g;
                        lb_controlled_blue = temp_host_puro_b;
                        check_lb_again = false;
                        real_color_captured = false;
                        color_diferente = false;
                        
                    }
                
            }
            else{
                check_lb_again = false;
                ultimo_tiempo_check_lb = tiempo_actual;
                
            }
        }

        
    } 
    else{
        
        if(check_lb_again)
        {
            if(real_color_captured)
            {
                if (tiempo_actual - ultimo_tiempo_check_lb >= 3000) {
                    // Si pasaron más de 3 segundos sin que Steam mande nada, permitimos el cambio de color
                    host_puro_r = temp_host_puro_r;
                    host_puro_g = temp_host_puro_g;
                    host_puro_b = temp_host_puro_b;
                    lb_controlled_red = temp_host_puro_r;
                    lb_controlled_green = temp_host_puro_g;
                    lb_controlled_blue = temp_host_puro_b;
                    real_color_captured = false;            
                    check_lb_again = false;
                    ultimo_tiempo_check_lb = tiempo_actual;
                    color_diferente = false;
                    
                }
            }
        }
        else
        {
            ultimo_tiempo_check_lb = tiempo_actual;
            
        }
        // Si es nuestro dummy_buffer, ignoramos la captura para no pisar la RAM con negros (0,0,0)
        //printf("[StateMgr] Atajo detectado por huella digital de memoria. Preservando LEDs.\n");
    }

    SetStateData update{};
    memcpy(&update, data, sizeof(update));

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
    set_bit(state[0], 0, true);
    set_bit(state[0], 1, motors_on);
    set_bit(state[38], 2, true);
    copy_if_allowed(
        true,
        offsetof(SetStateData, RumbleEmulationRight),
        2
    );

    size_t motor_flag_offset = offsetof(SetStateData, HostTimestamp) + sizeof(uint32_t); 
    if (motor_flag_offset < 63) {
        // Copiamos lo que pida el Host (Steam) para no romper los modos hápicos
        state[motor_flag_offset] = data[motor_flag_offset];
        
        // Forzamos el Bit 0 (Compatibilidad de motores de vibración clásicos)
        // y el Bit 1 (Habilitación de los actuadores hápicos)
        set_bit(state[motor_flag_offset], 0, true);
        set_bit(state[motor_flag_offset], 1, true);
    }

    
    //Always allow headphone volume
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
    copy_if_allowed(
        update.AllowPlayerIndicators,
        kPlayerIndicatorsOffset,
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

    // =========================================================================
    // 🛠️ Enforce trigger modes at the end of state_update
    // =========================================================================
    const auto &current_config = get_config();
    size_t right_trigger_offset = offsetof(SetStateData, RightTriggerFFB);
    size_t left_trigger_offset = offsetof(SetStateData, LeftTriggerFFB);

    // -------------------------------------------------------------------------
    // === GATILLO DERECHO ===
    // -------------------------------------------------------------------------
    if (current_config.trigger_right_mode == 0) {
        // Si el juego envía datos reales, los respetamos
        if (data[right_trigger_offset + 0] != 0) {
            set_bit(state[0], 2, true);
            memcpy(state + right_trigger_offset, data + right_trigger_offset, 7);
        } else {
            // MODO 0 (Limpio): Forzamos el comando 0x05 de Sony para desactivar físicamente el motor
            set_bit(state[0], 2, true); // Forzar validez para que el control procese el apagado
            state[right_trigger_offset + 0] = 0x05; // Apagado / Suelto total
            state[right_trigger_offset + 1] = 0;
            state[right_trigger_offset + 2] = 0;
            state[right_trigger_offset + 3] = 0;
            state[right_trigger_offset + 4] = 0;
            state[right_trigger_offset + 5] = 0;
            state[right_trigger_offset + 6] = 0;
        }
    }
    else {
        // Modos modificados: Forzamos bits de validez en state[0] y Valid_Flag1
        set_bit(state[0], 2, true); 
        if (motor_flag_offset < 63) {
            state[motor_flag_offset] |= 0x03; // Calibración y etapa de potencia activa
        }

        if (current_config.trigger_right_mode == 1) { // Modo Rígido
            state[right_trigger_offset + 0] = 0x01; // Resistencia Continua
            state[right_trigger_offset + 1] = 0;    // Inicio
            state[right_trigger_offset + 2] = 200;  // Dureza pesada
            memset(state + right_trigger_offset + 3, 0, 4);
        }
        else if (current_config.trigger_right_mode == 2) { // Modo Disparo
            state[right_trigger_offset + 0] = 0x02; // Gatillo de Arma
            state[right_trigger_offset + 1] = 20;   // Inicio de la pared
            state[right_trigger_offset + 2] = 45;   // Fin del "clic" mecánico
            state[right_trigger_offset + 3] = 255;  // Fuerza para romper el muro
            memset(state + right_trigger_offset + 4, 0, 3);
        }
        
    }

    // -------------------------------------------------------------------------
    // === GATILLO IZQUIERDO ===
    // -------------------------------------------------------------------------
    if (current_config.trigger_left_mode == 0) {
        if (data[left_trigger_offset + 0] != 0) {
            set_bit(state[0], 3, true);
            memcpy(state + left_trigger_offset, data + left_trigger_offset, 7);
        } else {
            set_bit(state[0], 3, true);
            state[left_trigger_offset + 0] = 0x05; // Apagado / Suelto total
            state[left_trigger_offset + 1] = 0;
            state[left_trigger_offset + 2] = 0;
            state[left_trigger_offset + 3] = 0;
            state[left_trigger_offset + 4] = 0;
            state[left_trigger_offset + 5] = 0;
            state[left_trigger_offset + 6] = 0;
        }
    }
    else {
        set_bit(state[0], 3, true); 
        if (motor_flag_offset < 63) {
            state[motor_flag_offset] |= 0x03; 
        }

        if (current_config.trigger_left_mode == 1) { // Modo Rígido
            state[left_trigger_offset + 0] = 0x01; 
            state[left_trigger_offset + 1] = 0;    
            state[left_trigger_offset + 2] = 200;  
            memset(state + left_trigger_offset + 3, 0, 4);
        }
        else if (current_config.trigger_left_mode == 2) { // Modo Disparo
            state[left_trigger_offset + 0] = 0x02; 
            state[left_trigger_offset + 1] = 20;   
            state[left_trigger_offset + 2] = 45;   
            state[left_trigger_offset + 3] = 255;  
            memset(state + left_trigger_offset + 4, 0, 3);
        }
        
    }

    state[04] = 0x04;
    size_t led_red_offset = offsetof(SetStateData, LedRed);

    state[led_red_offset] = lb_controlled_red;   // lightbar_red
    state[led_red_offset + 1] = lb_controlled_green; // lightbar_green
    state[led_red_offset + 2] = lb_controlled_blue; // lightbar_blue
    
    size_t mute_light_mode_offset = offsetof(SetStateData, MuteLightMode);
    //If in config state mode 
    if(config_mode_enabled)
    {
        state[mute_light_mode_offset] = MuteLight::Breathing; //Mute Light Breathing on config mode 3+8
    }
    else
    {
        state[mute_light_mode_offset] = MuteLight::Off; //Mute Light Breathing on config mode 3+8
    }
    size_t headphone_volume_offset = offsetof(SetStateData, VolumeHeadphones);
    float headphone_volume_float = current_speaker_volume + 100;
    headphone_volume_float *= 1.27;
    uint8_t headphone_volume_byte = static_cast<uint8_t>(std::round(headphone_volume_float));

    state[headphone_volume_offset] = headphone_volume_byte;
}

void state_set_led_color(uint8_t r, uint8_t g, uint8_t b) {
    state[offsetof(SetStateData, LedRed)]   = r;
    state[offsetof(SetStateData, LedGreen)] = g;
    state[offsetof(SetStateData, LedBlue)]  = b;
}

bool state_motors_active() {
    return state[offsetof(SetStateData, RumbleEmulationRight)] != 0 ||
           state[offsetof(SetStateData, RumbleEmulationLeft)]  != 0;
}

void state_clear_motors() {
    state[offsetof(SetStateData, RumbleEmulationRight)] = 0;
    state[offsetof(SetStateData, RumbleEmulationLeft)]  = 0;
}
