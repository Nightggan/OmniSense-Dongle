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

// Pico SDK speciifically for waiting on conditions
#include "pico/critical_section.h"

int reportSeqCounter = 0;
uint8_t packetCounter = 0;
bool spk_active = false;
bool touchpad_runtime_enabled = true;
static uint64_t last_rumble_report_us = 0;

//Custom vars Omega

uint8_t right_trigger_real_position = 0;
uint8_t left_trigger_real_position = 0;

uint8_t host_puro_r = 0;
uint8_t host_puro_g = 0;
uint8_t host_puro_b = 0;

uint8_t lb_controlled_red = 0;
uint8_t lb_controlled_green = 0;
uint8_t lb_controlled_blue = 0;

bool check_lb_again = false;
static uint32_t ultimo_tiempo_check_lb = 0;

volatile bool flash_busy = false;
volatile bool usb_reconnect_requested = false;
static bool usb_is_enabled = false;
volatile bool save_requested = false;
uint32_t pending_save_time = 0;
bool config_mode_enabled = false;
bool save_config_now = false;
uint32_t left_analog_up_holding_time = 0;
uint32_t left_analog_down_holding_time = 0;
bool left_analog_up_triggered = false;
bool left_analog_down_triggered = false;
bool request_temp_save = false;
volatile float current_speaker_volume = -100.0f;

//End Custom vars Omega

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
    if (get_config().polling_rate_mode != 2) {
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

static bool shortcut_btn_pressed(const uint8_t *data, uint8_t btn) {
    if (btn > BTN_SHORTCUT_MAX) return false;
    return (data[SHORTCUT_BTN_MAP[btn].off] & SHORTCUT_BTN_MAP[btn].mask) != 0;
}

static void shortcut_btn_suppress(uint8_t *data, uint8_t btn) {
    if (btn > BTN_SHORTCUT_MAX) return;
    data[SHORTCUT_BTN_MAP[btn].off] &= ~SHORTCUT_BTN_MAP[btn].mask;
}

//Custom Safe Config Save
void safe_config_save() {
    // Si ya estamos operando en la flash, salimos para no bloquear el stack USB
    if (flash_busy) return; 
    flash_busy = true;
    // Ejecutamos el guardado
    bool success = config_save();
    
    if (success) {
        flash_busy = false;
    } else {
        printf("[Flash] Error crítico de CRC.\n");
    }
}

void save_now_update()
{
    safe_config_save();

    // 2. Forzamos la actualización interna de state[] leyendo la nueva config
    uint8_t dummy_buffer[sizeof(SetStateData) + 1] = {0};
    state_update(dummy_buffer + 1, sizeof(SetStateData));

    // 3. ⚡ SOLUCIÓN DEFINITIVA: Despachamos el paquete al DualSense inmediatamente
    // Esto vacía el buffer state[] recién modificado y lo eyecta al aire
    uint8_t forced_pkt[78] = {0};
    forced_pkt[0] = 0x31;
    forced_pkt[1] = (uint8_t)(reportSeqCounter << 4);
    if (++reportSeqCounter == 256) reportSeqCounter = 0;
    forced_pkt[2] = 0x10;
    
    // Inyectamos el estado actual que ya contiene tus gatillos nuevos
    state_set(forced_pkt + 3, sizeof(SetStateData));
    
    // Mandamos por Bluetooth en caliente
    bt_write(forced_pkt, sizeof(forced_pkt));
}

void suppress_all_inputs(uint8_t *data) {
    // 1. Resetear Joysticks al centro (128)
    data[3] = 128; data[4] = 128; // Analógico Izquierdo
    data[5] = 128; data[6] = 128; // Analógico Derecho

    // 2. Resetear Gatillos a 0
    data[7] = 0; 
    data[8] = 0;

    // 3. Limpiar botones (usando máscaras binarias)
    // Esto es más seguro que poner data[x] = 0, porque no borras 
    // bytes que puedan contener información de otros sensores.
    //data[9] &= 0xF0;  // Limpia D-Pad y botones básicos del byte 9
    data[10] = 0;     // Limpia D-Pad y Botones Frontales
    data[11] = 0;     // Limpia botones extras
    data[12] = 0; // Limpia PS, Pad, Mute
}

//Custon on_bt_data adding trigger/lightbar modes and shortcuts
void __not_in_flash_func(on_bt_data)(CHANNEL_TYPE channel, uint8_t *data, uint16_t len) {
    static bool request_flash_save = false;
    if (channel == INTERRUPT && data[1] == 0x31) {
        if ((data[56] & 1) != (interrupt_in_data[53] & 1)) {
            set_headset(data[56] & 1);
        }
        // 🛠️ CAPTURA DE POTENCIÓMETROS REALES EN TIEMPO REAL:
        left_trigger_real_position = data[7];  // Byte 7: L2 Analógico (0-255)
        right_trigger_real_position = data[8];  // Byte 8: R2 Analógico (0-255)
        // 🕹️ CAPTURA DE LOS EJES DEL ANALÓGICO IZQUIERDO:
        uint8_t left_stick_x = data[3]; //0: Full Left - 128: Center - 255: Full Right
        uint8_t left_stick_y = data[4]; //0: Full Up - 128: Center - 255: Full Down
        
        static bool config_mode_switch_shortcut_lock = false;
        // Config Shortcut: Mute
        if (shortcut_btn_pressed(data, 8)) {
            if(!config_mode_switch_shortcut_lock)
            {
                printf("[Atajo] Mute presionado\n");
                config_mode_enabled = !config_mode_enabled;
                config_mode_switch_shortcut_lock = true;
                if(config_mode_enabled)
                {
                    request_temp_save = true;
                }
            }

            
        }
        else {
            // Cuando dejes de presionar al menos uno de los dos, se libera el candado
            config_mode_switch_shortcut_lock = false;
        }

        if(config_mode_enabled)
        {
            

            // Candado estático para que el atajo se ejecute UNA sola vez por pulsación
            static bool lightbar_mode_switch_shortcut_lock = false;
            static bool lightbar_breathing_shortcut_lock = false;
            static bool right_trigger_mode_override_shortcut_lock = false;
            static bool left_trigger_mode_override_shortcut_lock = false;
            static bool speaker_volume_up_shortcut_lock = false;
            static bool speaker_volume_down_shortcut_lock = false;
            
            // Evaluamos usando puramente las funciones del dongle
            // Atajo para cambiar modo de lightbar: Create
            if (shortcut_btn_pressed(data, 6)) /* Create */ //&& shortcut_btn_pressed(data, 5) /* R1 */) {
            {    
                /*

                // 🛠️ SOLUCIÓN: Limpiamos los bytes reales que van hacia la PC (con el offset de 3 integrado)
                // Esto "apaga" los botones para el juego en tiempo real
                data[12] &= ~0x20; // Borra el bit de R1 (Byte 3 + 9 = 12)
                data[16] &= ~0x10; // Borra el bit de Create (Byte 3 + 13 = 16)
                */
                if (!lightbar_mode_switch_shortcut_lock) {
                    Config_body new_config = get_config();
                    new_config.lightbar_mode = (new_config.lightbar_mode + 1) % 9; // Ciclar modos de luz
                    
                    set_config(new_config);
                    request_temp_save = true;
                    
                    
                    lightbar_mode_switch_shortcut_lock = true; // Bloqueamos hasta que sueltes los botones
                }
            } else {
                // Cuando dejes de presionar al menos uno de los dos, se libera el candado
                lightbar_mode_switch_shortcut_lock = false;
            }
            
            //Atajo para breathing: Options
            if (shortcut_btn_pressed(data, 7)){
                
                // Suprimimos los botones nativos usando la función del creador (con el offset correcto)
                //shortcut_btn_suppress(data, 6); Create
                //shortcut_btn_suppress(data, 4); L1

                // Limpiamos los bytes reales que van hacia la PC
                //data[12] &= ~0x10; // Borra el bit de L1 (Byte 3 + 9 = 12) y Create (Byte 3 + 13 = 16)
                //data[16] &= ~0x10; // Borra el bit de Create (Byte 3 + 13 = 16)

                if (!lightbar_breathing_shortcut_lock) {
                    Config_body new_config = get_config();
                    new_config.lightbar_breathing = !new_config.lightbar_breathing; // Alternar breathing
                    
                    set_config(new_config);
                    request_temp_save = true;
                    
                    
                    lightbar_breathing_shortcut_lock = true; // Bloqueamos hasta que sueltes los botones
                }
            } else {
                lightbar_breathing_shortcut_lock = false;
            }

            
            // Atajo para cambiar modo de trigger derecho: R1
            if (shortcut_btn_pressed(data, 5) /* R1 */) {
                if (!right_trigger_mode_override_shortcut_lock) {
                    Config_body new_config = get_config();
                    new_config.trigger_right_mode = (new_config.trigger_right_mode + 1) % 4; 
                    
                    set_config(new_config); // ⚡ Cambia la RAM al instante (Sin congelar)
                    request_temp_save = true; // 📌 Agendamos el guardado para después
                    
                    right_trigger_mode_override_shortcut_lock = true; 
                }
            } else {
                right_trigger_mode_override_shortcut_lock = false;
            }
            
            
            // Atajo para cambiar modo de trigger izquierdo: L1
            if (shortcut_btn_pressed(data, 4) /* L1 */) {
                if (!left_trigger_mode_override_shortcut_lock) {
                    Config_body new_config = get_config();
                    new_config.trigger_left_mode = (new_config.trigger_left_mode + 1) % 4; 
                    
                    set_config(new_config); // ⚡ Cambia la RAM al instante
                    request_temp_save = true; // 📌 Agendamos el guardado
                    
                    left_trigger_mode_override_shortcut_lock = true; 
                }
            } else {
                left_trigger_mode_override_shortcut_lock = false;
            }

            //Left Analog Up/Down Holding Control
            if(left_stick_y<10)
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
            
            if(left_stick_y>245)
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
            
            
            //Speaker Volume Up Shortcut control
            if (speaker_volume_up_shortcut_lock) {
                Config_body new_config = get_config();
                new_config.speaker_volume = new_config.speaker_volume + 2.0f;
                
                if (new_config.speaker_volume > 0.0f) {
                    new_config.speaker_volume = 0.0f;
                }
                current_speaker_volume = new_config.speaker_volume;
                set_config(new_config); // ⚡ Cambia la RAM al instante (Sin congelar)
                request_temp_save = true; // 📌 Agendamos el guardado para después
                left_analog_up_holding_time = 0;
                left_analog_up_triggered = false;
                printf("Volume Up!\n");
                speaker_volume_up_shortcut_lock = false;
            }

            //Speaker Volume Down Shortcut control
            if (speaker_volume_down_shortcut_lock) {
                Config_body new_config = get_config();
                new_config.speaker_volume = new_config.speaker_volume - 2.0f;
                
                if (new_config.speaker_volume < -100.0f) {
                    new_config.speaker_volume = -100.0f;
                }
                current_speaker_volume = new_config.speaker_volume;
                set_config(new_config); // ⚡ Cambia la RAM al instante (Sin congelar)
                request_temp_save = true; // 📌 Agendamos el guardado para después
                left_analog_down_holding_time = 0;
                left_analog_down_triggered = false;
                printf("Volume Down!\n");
                speaker_volume_down_shortcut_lock = false;
            }
            

            if(request_temp_save && !left_trigger_mode_override_shortcut_lock
            && !right_trigger_mode_override_shortcut_lock && !lightbar_breathing_shortcut_lock
            && !lightbar_mode_switch_shortcut_lock && !speaker_volume_down_shortcut_lock && !speaker_volume_up_shortcut_lock)
            {
                current_speaker_volume = get_config().speaker_volume;
                printf("Speaker Volume Config: %d \n", (int)current_speaker_volume);
                request_temp_save = false;
                // 2. Forzamos la actualización interna de state[] leyendo la nueva config
                uint8_t dummy_buffer[sizeof(SetStateData) + 1] = {0};
                state_update(dummy_buffer + 1, sizeof(SetStateData));
                
                // 3. ⚡ SOLUCIÓN DEFINITIVA: Despachamos el paquete al DualSense inmediatamente
                // Esto vacía el buffer state[] recién modificado y lo eyecta al aire
                uint8_t forced_pkt[78] = {0};
                forced_pkt[0] = 0x31;
                forced_pkt[1] = (uint8_t)(reportSeqCounter << 4);
                if (++reportSeqCounter == 256) reportSeqCounter = 0;
                forced_pkt[2] = 0x10;
                
                // Inyectamos el estado actual que ya contiene tus gatillos nuevos
                state_set(forced_pkt + 3, sizeof(SetStateData));
                
                // Mandamos por Bluetooth en caliente
                bt_write(forced_pkt, sizeof(forced_pkt));
                request_flash_save = true;
                
            }
            suppress_all_inputs(data);
        }
        else//Exiting config state mode
        {
            //Only safe save to flash when out of config mode after request_flash_save is true
            if (request_flash_save) {
                
            
                save_requested = true; 
                request_flash_save = false;    
                current_speaker_volume = get_config().speaker_volume;
                // 2. Forzamos la actualización interna de state[] leyendo la nueva config
                uint8_t dummy_buffer[sizeof(SetStateData) + 1] = {0};
                state_update(dummy_buffer + 1, sizeof(SetStateData));

                // 3. ⚡ SOLUCIÓN DEFINITIVA: Despachamos el paquete al DualSense inmediatamente
                // Esto vacía el buffer state[] recién modificado y lo eyecta al aire
                uint8_t forced_pkt[78] = {0};
                forced_pkt[0] = 0x31;
                forced_pkt[1] = (uint8_t)(reportSeqCounter << 4);
                if (++reportSeqCounter == 256) reportSeqCounter = 0;
                forced_pkt[2] = 0x10;
                
                // Inyectamos el estado actual que ya contiene tus gatillos nuevos
                state_set(forced_pkt + 3, sizeof(SetStateData));
                
                // Mandamos por Bluetooth en caliente
                bt_write(forced_pkt, sizeof(forced_pkt));

            
            }

            // data[3+7] = byte 7 of USBGetStateData, data[3+9] = byte 9: bit0=PS(Home)
            const bool ps    = (data[12] & 0x01) != 0;
            bool suppress_ps = false;

            // PS + poweroff_button → power off controller
            static bool poweroff_held = false;
            const uint8_t po_btn = get_config().poweroff_button;
            if (get_config().enable_poweroff_shortcut && ps && shortcut_btn_pressed(data, po_btn)) {
                shortcut_btn_suppress(data, po_btn);
                suppress_ps = true;
                if (!poweroff_held) {
                    poweroff_held = true;
                    bt_disconnect();
                    return;
                }
            } else {
                poweroff_held = false;
            }

            

            if (suppress_ps) {
                data[12] &= ~0x01; // suppress PS
            }
        }

        

        if (get_config().polling_rate_mode != 2) {
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
    (void) itf;
    (void) report_type;

    if (is_pico_cmd(report_id)) {
        return pico_cmd_get(report_id, buffer, reqlen);
    }

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
    const auto &cfg = get_config();

    // Offset de Valid_Flag1 para inyectar corriente máxima a los servomotores
    size_t out_motor_flag_offset = 3 + offsetof(SetStateData, HostTimestamp) + sizeof(uint32_t);

    // --- 🔫 MODO METRALLA NATIVO MAQUINA (0x06): GATILLO DERECHO ---
    if (cfg.trigger_right_mode == 3) {
        size_t r_off = 3 + offsetof(SetStateData, RightTriggerFFB);
        

        // Solo actúa si detecta físicamente tu dedo hundiendo el gatillo R2
        if (right_trigger_real_position > 15) {
            pkt[3 + 0] |= 0x04; // Valid_Flag0: Avisa que actualizamos el gatillo derecho
            if (out_motor_flag_offset < 78) {
                pkt[out_motor_flag_offset] |= 0x03; // Forza calibración y etapa de potencia
            }

            // Mapeo idéntico a tu captura del DualSense Explorer:
            pkt[r_off + 0] = 0x06; // L2/R2 trigger effect mode (Machine Gun)
            pkt[r_off + 1] = 0x20; // Parameter 1: Inicio del efecto (32 analógico)
            pkt[r_off + 2] = 0xFF; // Parameter 2: Fin del efecto (255 analógico)
            pkt[r_off + 3] = 0x06; // Parameter 3: Frecuencia / Velocidad de ráfaga
            pkt[r_off + 4] = 0x06; // Parameter 4: Fuerza / Amplitud del martillazo
            pkt[r_off + 5] = 0x00; // Parameter 5
            pkt[r_off + 6] = 0x00; // Parameter 6
        } else {
            // Estado de reposo absoluto al soltar el dedo
            pkt[r_off + 0] = 0x05; 
            memset(&pkt[r_off + 1], 0, 6);
        }
    }

    // --- 🔫 MODO METRALLA NATIVO MAQUINA (0x06): GATILLO IZQUIERDO ---
    if (cfg.trigger_left_mode == 3) {
        
        size_t l_off = 3 + offsetof(SetStateData, LeftTriggerFFB);
        if (left_trigger_real_position > 15) {
            
            pkt[3 + 0] |= 0x08; // Valid_Flag0: Avisa que actualizamos el gatillo izquierdo
            if (out_motor_flag_offset < 78) {
                pkt[out_motor_flag_offset] |= 0x03; 
            }

            // Mapeo idéntico a tu captura del DualSense Explorer:
            pkt[l_off + 0] = 0x06; 
            pkt[l_off + 1] = 0x20; 
            pkt[l_off + 2] = 0xFF; 
            pkt[l_off + 3] = 0x06; 
            pkt[l_off + 4] = 0x06; 
            pkt[l_off + 5] = 0x00; 
            pkt[l_off + 6] = 0x00; 
        } else {
            pkt[l_off + 0] = 0x05; 
            memset(&pkt[l_off + 1], 0, 6);
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

    config_load();
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
        cyw43_arch_poll();
        tud_task();
        //original audio loop location
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
            //state_keepalive();
        }
        if (usb_reconnect_requested) {
            if (usb_is_enabled) {
                tud_disconnect();
                usb_is_enabled = false; // El cerrojo evita el spam
            }
            reconnect_start_time = to_ms_since_boot(get_absolute_time());
            usb_reconnect_requested = false;
        }

        if(save_config_now)
        {
            save_config_now = false;
            save_now_update();
        }
        // ⏱️ PHASE 1: Petition to save config
        if (save_requested) {
            pending_save_time = to_ms_since_boot(get_absolute_time());
            
            save_requested = false; // Bajamos la bandera inmediata
        }

        // ⏱️ FASE 2: Ejecución tras 150ms de gracia
        // Esto le da a TinyUSB tiempo para enviar el ACK a Chrome antes de congelar la Flash
        if (pending_save_time != 0 && (to_ms_since_boot(get_absolute_time()) - pending_save_time > 500)) {
            safe_config_save();
            pending_save_time = 0; // Reseteamos el temporizador
        }

        audio_loop();
        if(check_lb_again)
        {
            uint32_t tiempo_actual_local = to_ms_since_boot(get_absolute_time());
            if (tiempo_actual_local - ultimo_tiempo_check_lb > 3050) {
                uint8_t dummy_buffer[sizeof(SetStateData) + 1] = {0};
                state_update(dummy_buffer + 1, sizeof(SetStateData));
                ultimo_tiempo_check_lb = tiempo_actual_local;
            }

        }
        else
        {
            ultimo_tiempo_check_lb = to_ms_since_boot(get_absolute_time());
        }
        
        state_keepalive();
        
        lightbar_loop();

#if ENABLE_BATT_LED
        battery_led_tick();
#endif

    }
}
