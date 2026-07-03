//
// Created by awalol on 2026/5/4.
//

#include "cmd.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "bt.h"
#include "config.h"
#include "device/usbd.h"
#include "pico/time.h"
#include "state_mgr.h"

//Custom vars Omni
extern volatile bool usb_reconnect_requested;
extern bool save_requested;
extern bool save_config_now;
volatile uint8_t profile_index_request;
//End Custom vars Omni
bool is_pico_cmd(uint8_t report_id) {
    if (report_id == 0xf6 ||
        report_id == 0xf7 ||
        report_id == 0xf8 ||
        report_id == 0xf9 ||
        report_id == 0xf5 ||
        report_id == 0xf4
    ) {
        return true;
    }
    return false;
}

uint16_t pico_cmd_get(uint8_t report_id, uint8_t *buffer, uint16_t reqlen) {
    if (report_id == 0xf7) {
        printf("[HID] Receive 0xf7 getting config\n");
        // Skip config_version (internal field), expose from haptics_gain so
        // clients see a layout matching their struct definition.
        const Global_Config_body& global_body = get_global_config();
        constexpr size_t offset = offsetof(Global_Config_body, haptics_gain);
        constexpr size_t body_len = sizeof(Global_Config_body) - offset;
        if (body_len > reqlen) {
            printf("[Device_Config] Warning: Config_body overflow\n");
        }
        const auto len = std::min(body_len, static_cast<size_t>(reqlen));
        memcpy(buffer, reinterpret_cast<const uint8_t*>(&global_body) + offset, len);
        return static_cast<uint16_t>(len);
    }
    if (report_id == 0xf8) {
        printf("[HID] Receive 0xf8 getting firmware version\n");
        const auto len = std::min(strlen(PICO_PROGRAM_VERSION_STRING), static_cast<size_t>(reqlen));
        memcpy(buffer, PICO_PROGRAM_VERSION_STRING, len);
        return len;
    }
    if (report_id == 0xf9) {
        // [-128,0]
        int8_t rssi = 0;
        bt_get_signal_strength(&rssi);
        if (reqlen == 0) {
            return 0;
        }
        buffer[0] = rssi;
        #if ENABLE_VERBOSE
                printf("[HID] 0xf9 RSSI=%d raw=0x%02X\n", rssi, buffer[0]);
        #endif
        return 1;
    }
    if(report_id == 0xf5){//profile data get
        printf("Step 2: Profile Data Requested with Index: %d \n", (int)profile_index_request);
        const Profile_Config_body& profile_body = get_profile_config_index(profile_index_request);
        constexpr size_t body_len = sizeof(Profile_Config_body);
        if (body_len > reqlen) {
            printf("[Device_Config] Warning: Profile_body overflow\n");
        }
        const auto len = std::min(body_len, static_cast<size_t>(reqlen));
        memcpy(buffer, reinterpret_cast<const uint8_t*>(&profile_body), len);
        return static_cast<uint16_t>(len);
        
    }
    return 0;
}

void pico_cmd_set(uint8_t report_id, uint8_t const *buffer, uint16_t bufsize) {
    printf("[CMD] Buffer command: 0x%02X\n",buffer[0] );
    (void)report_id;
    if (bufsize == 0) {
        return;
    }
    // 0x01 update config in variable
    // 0x02 write config to flash
    // 0x03 reconnect tinyusb device;
    if (buffer[0] == 0x01) {
        printf("[CMD] Enter config set func\n");
        set_global_config(buffer + 1, bufsize - 1);
    }
    if (buffer[0] == 0x02) {
        printf("[CMD] Enter config save func\n");
        save_config_now = true;
    }
    if (buffer[0] == 0x03) {
        printf("[CMD] Enter tud reconnect func\n");
        usb_reconnect_requested = true;
    }
    if (buffer[0] == 0x05) {//profile data set
        printf("[CMD] Enter Profile Data Set Index: %d\n", buffer[1]);
        set_profile_config(buffer + 2, bufsize - 1, buffer[1]);
    }
    
    if (buffer[0] == 0x07) {//profile data set
        printf("Step 1: Enter Profile Index Request: Profile ");
        profile_index_request = buffer[1];
        
        printf("%d\n", (int)profile_index_request);
        printf("Buffer[1]: %d\n", (int)buffer[1]);
    }

}
