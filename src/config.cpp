//
// Created by awalol on 2026/5/4.
//

#include "config.h"

#include <cmath>
#include <cstring>

#include "utils.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"

constexpr uint32_t CONFIG_MAGIC = 0xCAFEBABE;
constexpr uint16_t CONFIG_VERSION = 2;
constexpr uint32_t CONFIG_FLASH_OFFSET = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
static Device_Config config{};
extern volatile float current_speaker_volume;
extern volatile float current_auto_haptics_gain;
bool is_dse = false;

// 编译期保护
// 判断Config结构体是否能放进flash 256bytes
static_assert(sizeof(Device_Config) <= FLASH_PAGE_SIZE);
// 配置区起始地址必须按 flash sector 对齐。
static_assert(CONFIG_FLASH_OFFSET % FLASH_SECTOR_SIZE == 0);

uint32_t calc_global_config_crc(const Device_Config &con) {
    return crc32(reinterpret_cast<const uint8_t *>(&con.global_body), sizeof(Global_Config_body));
}

uint32_t calc_profile_config_crc(const Device_Config &con) {
    return crc32(reinterpret_cast<const uint8_t *>(&con.profile_body), sizeof(Profile_Config_body));
}

const Device_Config *flash_config() {
    return reinterpret_cast<const Device_Config *>(XIP_BASE + CONFIG_FLASH_OFFSET);
}

void global_config_valid() {
    // valid config and set default value
    if (config.magic != CONFIG_MAGIC) {
        config.magic = CONFIG_MAGIC;
        //printf("[Config] Config Magic Header is invalid\n");
    }
    if (config.version != CONFIG_VERSION) {
        config.version = CONFIG_VERSION;
        //printf("[Config] Config Version is invalid\n");
    }
    if (config.global_config_size != sizeof(Device_Config)) {
        config.global_config_size = sizeof(Device_Config);
        //printf("[Config] Config Body size is invalid\n");
    }
    //Profile index validation
    if (config.profile_selected > 3) {//0 - 3 profile index
        config.profile_selected = 0;
    }
    auto global_body = &config.global_body;
    if (std::isnan(global_body->haptics_gain) || global_body->haptics_gain < 1.0f || global_body->haptics_gain > 2.0f) {
        global_body->haptics_gain = 1.5f;
        //printf("[Config] Haptics Gain value is invalid\n");
    }
    if (global_body->inactive_time < 5 || global_body->inactive_time > 60) {
        global_body->inactive_time = 30;
        //printf("[Config] Inactive time is invalid\n");
    }
    if (global_body->disable_inactive_disconnect > 1) {
        global_body->disable_inactive_disconnect = 0;
        //printf("[Config] disable_auto_disconnect is invalid\n");
    }
    if (global_body->disable_pico_led > 1) {
        global_body->disable_pico_led = 0;
        //printf("[Config] disable_pico_led is invalid\n");
    }
    if (global_body->polling_rate_mode > 2) {
        global_body->polling_rate_mode = 0;
        //printf("[Config] polling_rate_mode is invalid\n");
    }
    if (global_body->audio_buffer_length < 16 || global_body->audio_buffer_length > 128) {
        global_body->audio_buffer_length = 64;
        //printf("[Config] haptics_buffer_length is invalid\n");
    }
    if (global_body->controller_mode > 2) {
        global_body->controller_mode = 2;
        //printf("[Config] controller_mode is invalid\n");
    }
    if (global_body->config_version != CONFIG_VERSION) {
        global_body->config_version = CONFIG_VERSION;
        //printf("[Config] Warning: Config may breaking change\n");
    }
    if (global_body->auto_haptics_enable > 2) {
        global_body->auto_haptics_enable = 1;
        //printf("[Config] auto_haptics_enable is invalid, defaulting to 2\n");
    }
    if (global_body->auto_haptics_gain > 200) {
        global_body->auto_haptics_gain = 100;
        //printf("[Config] auto_haptics_gain is invalid, defaulting to 100\n");
    }
    if (global_body->auto_haptics_lowpass_hz < 20 || global_body->auto_haptics_lowpass_hz > 400) {
        global_body->auto_haptics_lowpass_hz = 80;
        //printf("[Config] auto_haptics_lowpass_hz is invalid, defaulting to 80 Hz\n");
    }
    if (global_body->enable_poweroff_shortcut > 1) {
        global_body->enable_poweroff_shortcut = 1;
    }
    if (global_body->poweroff_button > BTN_SHORTCUT_MAX) {
        global_body->poweroff_button = BTN_TRIANGLE;
    }
    // Defaults to 0 (off) for configs saved before this field existed: the
    // erased flash tail reads 0xFF, which the >1 clamp turns into 0.
    if (global_body->wake_enable > 1) {
        global_body->wake_enable = 0;
    }
    
    //Default speaker volume to -100 (min) to not scare your sleeping wife
    if (std::isnan(global_body->speaker_volume) || global_body->speaker_volume < -100 || global_body->speaker_volume > 0) {
        global_body->speaker_volume = -100;
        printf("[Config] Speaker Volume is invalid\n");
    }
    
    //Default to 0 to allow sound pass to speaker/headphones even on haptics modes
    if (global_body->auto_mute_mode > 1) global_body->auto_mute_mode = 0;

    if(global_body->time_config_mode < 0  || global_body->time_config_mode > 3000){
        global_body->time_config_mode = 0;//Default instant press for config mode
    }

}

void profile_config_valid()
{
    for(int z = 0; z < 4; z++)
    {
        auto &profile = config.profile_body[z];

        if (profile.lightbar_mode > 8) {
            profile.lightbar_mode = 0; // default to "HOST" mode if invalid
        }
        if (profile.lb_fav_r[0] == 0xFF && profile.lb_fav_g[0] == 0xFF && profile.lb_fav_b[0] == 0xFF) {
            // Default FAV0 to bright cyan to make it obvious if the config isn't loaded
            profile.lb_fav_r[0] = 0;
            profile.lb_fav_g[0] = 221;
            profile.lb_fav_b[0] = 255;
        }
        if (profile.lb_fav_r[1] == 0xFF && profile.lb_fav_g[1] == 0xFF && profile.lb_fav_b[1] == 0xFF) {
            // Default FAV1 to bright red to make it obvious if the config isn't loaded
            profile.lb_fav_r[1] = 255;
            profile.lb_fav_g[1] = 0;
            profile.lb_fav_b[1] = 17;
        }
        if (profile.lb_fav_r[2] == 0xFF && profile.lb_fav_g[2] == 0xFF && profile.lb_fav_b[2] == 0xFF) {
            // Default FAV2 to bright lime green to make it obvious if the config isn't loaded
            profile.lb_fav_r[2] = 191;
            profile.lb_fav_g[2] = 255;
            profile.lb_fav_b[2] = 0;
        }
        if (profile.lb_fav_r[3] == 0xFF && profile.lb_fav_g[3] == 0xFF && profile.lb_fav_b[3] == 0xFF) {
            // Default FAV3 to bright white to make it obvious if the config isn't loaded
            profile.lb_fav_r[3] = 255;
            profile.lb_fav_g[3] = 255;
            profile.lb_fav_b[3] = 255;
        }
        if (profile.lightbar_breathing > 1) {
            profile.lightbar_breathing = 1; //breathing enabled by default if invalid
        }

        if (profile.trigger_left_mode > 4) {
            profile.trigger_left_mode = 0; //Relaxed/Host controlled by default if invalid
        }
        if (profile.trigger_right_mode > 4) {
            profile.trigger_right_mode = 0; //Relaxed/Host controlled by default if invalid
        }
        if(config.initial_setup_completed!=7)
        {
            profile.feedback_start_point = 0;
            profile.feedback_force = 200;
            profile.trigger_start_point = 20;
            profile.trigger_wall_point = 45;
            profile.trigger_break_force = 255;
            profile.vibration_start_point = 32;
            profile.vibration_frequency = 36;
            profile.vibration_force = 255;
            profile.hair_wall_start_point = 97;
        }    
    }
    config.initial_setup_completed = 7;
}

void device_config_load() {
    memcpy(&config, flash_config(), sizeof(Device_Config));
    global_config_valid();
    profile_config_valid();
    
    current_speaker_volume = get_global_config().speaker_volume;
    current_auto_haptics_gain = get_global_config().auto_haptics_gain;
}

void set_profile_index(uint8_t new_profile)
{
    config.profile_selected = new_profile;
}


// Runs with core1 parked (flash_safe_execute) and core0 interrupts disabled, so
// neither core touches XIP flash while the sector is erased/programmed. Without
// the core1 park this races the audio core and corrupts audio (buzzing).
static void config_save_flash_op(void *param) {
    size_t config_size = sizeof(Device_Config);
    //size_t flash_write_size = ((config_size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;
    size_t flash_write_size = FLASH_PAGE_SIZE * 2;
    
    const uint8_t *page = static_cast<const uint8_t *>(param);
    const uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(CONFIG_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(CONFIG_FLASH_OFFSET, page, flash_write_size);
    restore_interrupts(interrupts);
}


bool device_config_save() {
    config.global_crc32 = calc_global_config_crc(config);
    config.profile_crc32 = calc_profile_config_crc(config);
    size_t config_size = sizeof(Device_Config);
    size_t flash_write_size = FLASH_PAGE_SIZE * 2; //((config_size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;

    alignas(4) uint8_t page[flash_write_size];

    memset(page, 0xff,sizeof(page));
    memcpy(page, &config, config_size);
    bool flashSuccess = false;
    const int rc = flash_safe_execute(config_save_flash_op, page, UINT32_MAX);
    if (rc != PICO_OK) {
        printf("[Config] device_config_save flash_safe_execute failed: %d\n", rc);
        return false;
    }

    Device_Config verify{};
    memcpy(&verify, flash_config(), sizeof(verify));
    const auto verify_global_crc32 = calc_global_config_crc(verify);
    if (verify_global_crc32 == config.global_crc32) {
        printf("[Config] Config write flash verify success\n");
        flashSuccess = true;
    }
    else
    {
        printf("[Config] Config write flash verify failed\n");
    }

    const auto verify_profile_crc32 = calc_profile_config_crc(verify);
    if (verify_profile_crc32 == config.profile_crc32) {
        printf("[Config] Profile configs write flash verify success\n");
        flashSuccess = true;
    }
    else
    {
        printf("[Config] Profile configs write flash verify failed\n");    
    }

    
    return flashSuccess;
}

const Global_Config_body& get_global_config() {
    return config.global_body;
}

const Profile_Config_body& get_profile_config(){
    return config.profile_body[config.profile_selected];
}
const Profile_Config_body& get_profile_config_index(uint8_t index){
    return config.profile_body[index];
}
void set_global_config(const uint8_t *new_global_config, const uint16_t len) {
    
    // HID payload starts at haptics_gain (config_version is internal, not sent over HID)
    constexpr size_t offset = offsetof(Global_Config_body, haptics_gain);
    const size_t body_len = sizeof(Global_Config_body) - offset;
    const auto copy_len = len < body_len ? len : body_len;
    memcpy(reinterpret_cast<uint8_t*>(&config.global_body) + offset, new_global_config, copy_len);
    global_config_valid();
    if (config.global_body.disable_pico_led) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
    }else {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
    }
}

void set_global_config(const Global_Config_body &new_global_config) {
    config.global_body = new_global_config;
    global_config_valid();
}

//Profile saves

void set_profile_config(const uint8_t *new_profile_config, const uint16_t len, uint8_t profile_index_set) {
    
    const size_t body_len = sizeof(Profile_Config_body);
    const auto copy_len = len < body_len ? len : body_len;
    memcpy(reinterpret_cast<uint8_t*>(&config.profile_body[profile_index_set]), new_profile_config, copy_len);
    profile_config_valid();
}

void set_profile_config(const Profile_Config_body &new_profile_config){
    config.profile_body[config.profile_selected] = new_profile_config;
    profile_config_valid();
}
