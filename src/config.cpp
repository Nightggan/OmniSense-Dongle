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

constexpr uint32_t CONFIG_MAGIC = 0x66ccff01;
constexpr uint16_t CONFIG_VERSION = 1;
constexpr uint32_t CONFIG_FLASH_OFFSET = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
static Config config{};
extern volatile float current_speaker_volume;
bool is_dse = false;

// 编译期保护
// 判断Config结构体是否能放进flash 256bytes
static_assert(sizeof(Config) <= FLASH_PAGE_SIZE);
// 配置区起始地址必须按 flash sector 对齐。
static_assert(CONFIG_FLASH_OFFSET % FLASH_SECTOR_SIZE == 0);

uint32_t calc_config_crc(const Config &con) {
    return crc32(reinterpret_cast<const uint8_t *>(&con.body), sizeof(Config_body));
}

const Config *flash_config() {
    return reinterpret_cast<const Config *>(XIP_BASE + CONFIG_FLASH_OFFSET);
}

void config_valid() {
    // valid config and set default value
    if (config.magic != CONFIG_MAGIC) {
        config.magic = CONFIG_MAGIC;
        //printf("[Config] Config Magic Header is invalid\n");
    }
    if (config.version != CONFIG_VERSION) {
        config.version = CONFIG_VERSION;
        //printf("[Config] Config Version is invalid\n");
    }
    if (config.size != sizeof(Config_body)) {
        config.size = sizeof(Config_body);
        //printf("[Config] Config Body size is invalid\n");
    }
    auto body = &config.body;
    if (std::isnan(body->haptics_gain) || body->haptics_gain < 1.0f || body->haptics_gain > 2.0f) {
        body->haptics_gain = 1.5f;
        //printf("[Config] Haptics Gain value is invalid\n");
    }
    if (body->inactive_time < 5 || body->inactive_time > 60) {
        body->inactive_time = 30;
        //printf("[Config] Inactive time is invalid\n");
    }
    if (body->disable_inactive_disconnect > 1) {
        body->disable_inactive_disconnect = 0;
        //printf("[Config] disable_auto_disconnect is invalid\n");
    }
    if (body->disable_pico_led > 1) {
        body->disable_pico_led = 0;
        //printf("[Config] disable_pico_led is invalid\n");
    }
    if (body->polling_rate_mode > 2) {
        body->polling_rate_mode = 0;
        //printf("[Config] polling_rate_mode is invalid\n");
    }
    if (body->audio_buffer_length < 16 || body->audio_buffer_length > 128) {
        body->audio_buffer_length = 64;
        //printf("[Config] haptics_buffer_length is invalid\n");
    }
    if (body->controller_mode > 2) {
        body->controller_mode = 2;
        //printf("[Config] controller_mode is invalid\n");
    }
    if (body->config_version != CONFIG_VERSION) {
        body->config_version = CONFIG_VERSION;
        //printf("[Config] Warning: Config may breaking change\n");
    }
    if (body->auto_haptics_enable > 2) {
        body->auto_haptics_enable = 1;
        //printf("[Config] auto_haptics_enable is invalid, defaulting to 2\n");
    }
    if (body->auto_haptics_gain > 200) {
        body->auto_haptics_gain = 100;
        //printf("[Config] auto_haptics_gain is invalid, defaulting to 100\n");
    }
    if (body->auto_haptics_lowpass_hz < 20 || body->auto_haptics_lowpass_hz > 400) {
        body->auto_haptics_lowpass_hz = 80;
        //printf("[Config] auto_haptics_lowpass_hz is invalid, defaulting to 80 Hz\n");
    }
    if (body->enable_poweroff_shortcut > 1) {
        body->enable_poweroff_shortcut = 1;
    }
    if (body->poweroff_button > BTN_SHORTCUT_MAX) {
        body->poweroff_button = BTN_TRIANGLE;
    }
    // Defaults to 0 (off) for configs saved before this field existed: the
    // erased flash tail reads 0xFF, which the >1 clamp turns into 0.
    if (body->wake_enable > 1) {
        body->wake_enable = 0;
    }
    if (body->lightbar_mode > 8) {
        body->lightbar_mode = 0;//default to "HOST" mode if invalid
    }
    if (body->lb_fav_r[0] == 0xFF && body->lb_fav_g[0] == 0xFF && body->lb_fav_b[0] == 0xFF) {
        // Default FAV0 to bright cyan to make it obvious if the config isn't loaded
        body->lb_fav_r[0] = 0;
        body->lb_fav_g[0] = 221;
        body->lb_fav_b[0] = 255;
    }
    if (body->lb_fav_r[1] == 0xFF && body->lb_fav_g[1] == 0xFF && body->lb_fav_b[1] == 0xFF) {
        // Default FAV1 to bright red to make it obvious if the config isn't loaded
        body->lb_fav_r[1] = 255;
        body->lb_fav_g[1] = 0;
        body->lb_fav_b[1] = 17;
    }
    if (body->lb_fav_r[2] == 0xFF && body->lb_fav_g[2] == 0xFF && body->lb_fav_b[2] == 0xFF) {
        // Default FAV2 to bright lime green to make it obvious if the config isn't loaded
        body->lb_fav_r[2] = 191;
        body->lb_fav_g[2] = 255;
        body->lb_fav_b[2] = 0;
    }
    if (body->lb_fav_r[3] == 0xFF && body->lb_fav_g[3] == 0xFF && body->lb_fav_b[3] == 0xFF) {
        // Default FAV3 to bright white to make it obvious if the config isn't loaded
        body->lb_fav_r[3] = 255;
        body->lb_fav_g[3] = 255;
        body->lb_fav_b[3] = 255;
    }
    if (body->lightbar_breathing > 1) {
        body->lightbar_breathing = 1; //breathing enabled by default if invalid
    }

    if (body->trigger_left_mode > 3) {
        body->trigger_left_mode = 0; //Relexed/Host controlled by default if invalid
    }
    if (body->trigger_right_mode > 3) {
        body->trigger_right_mode = 0; //Relexed/Host controlled by default if invalid
    }

    //Default to 0 to allow sound pass to speaker/headphones even on haptics modes
    if (body->auto_mute_mode > 1) body->auto_mute_mode = 0;

    //Default speaker volume to -100 (min) to not scare your sleeping wife
    if (std::isnan(body->speaker_volume) || body->speaker_volume < -100 || body->speaker_volume > 0) {
        body->speaker_volume = -100;
        printf("[Config] Speaker Volume is invalid\n");
    }
}

void config_load() {
    memcpy(&config, flash_config(), sizeof(Config));

    config_valid();
    current_speaker_volume = get_config().speaker_volume;
}

// Runs with core1 parked (flash_safe_execute) and core0 interrupts disabled, so
// neither core touches XIP flash while the sector is erased/programmed. Without
// the core1 park this races the audio core and corrupts audio (buzzing).
static void config_save_flash_op(void *param) {
    const uint8_t *page = static_cast<const uint8_t *>(param);
    const uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(CONFIG_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(CONFIG_FLASH_OFFSET, page, FLASH_PAGE_SIZE);
    restore_interrupts(interrupts);
}


bool config_save() {
    config.crc32 = calc_config_crc(config);
    alignas(4) uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xff, sizeof(page));
    memcpy(page, &config, sizeof(Config));

    const int rc = flash_safe_execute(config_save_flash_op, page, 1000);
    if (rc != PICO_OK) {
        printf("[Config] config_save flash_safe_execute failed: %d\n", rc);
        return false;
    }

    Config verify{};
    memcpy(&verify, flash_config(), sizeof(verify));
    const auto verify_crc32 = calc_config_crc(verify);
    if (verify_crc32 == config.crc32) {
        printf("[Config] Config write flash verify success\n");
        return true;
    }
    printf("[Config] Config write flash verify failed\n");
    return false;
}

const Config_body& get_config() {
    return config.body;
}

void set_config(const uint8_t *new_config, const uint16_t len) {
    
    // HID payload starts at haptics_gain (config_version is internal, not sent over HID)
    constexpr size_t offset = offsetof(Config_body, haptics_gain);
    const size_t body_len = sizeof(Config_body) - offset;
    const auto copy_len = len < body_len ? len : body_len;
    memcpy(reinterpret_cast<uint8_t*>(&config.body) + offset, new_config, copy_len);
    config_valid();
    if (config.body.disable_pico_led) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
    }else {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
    }
}

void set_config(const Config_body &new_config) {
    config.body = new_config;
    config_valid();
}
