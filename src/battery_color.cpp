//
// Lightbar color indicator. See battery_color.h.
//

#include "battery_color.h"

#include <cstdint>

#include "config.h"
#include "pico/time.h"
#include "state_mgr.h"

extern uint8_t interrupt_in_data[63];

namespace {

// Color tiers based on PowerPercent (0–10 scale, each unit ≈ 10%).
// pct >= 7  → Blue   (70–100 %)
// 4-6       → Green  (40–69 %)
// 1-3       → Yellow (10–39 %)
// pct == 0  → Red    (< 10 %)
struct Color { uint8_t r, g, b; };

constexpr Color COLOR_BLUE   = {  0,   0, 255};
constexpr Color COLOR_GREEN  = {  0, 200,   0};
constexpr Color COLOR_YELLOW = {255, 180,   0};
constexpr Color COLOR_RED    = {255,   0,   0};

constexpr uint64_t REPORT_STALE_US = 2'000'000;

uint64_t last_report_us = 0;
uint8_t  last_pct       = 0xFF;  // sentinel: "not yet set"

Color color_for_pct(uint8_t pct) {
    if (pct >= 7) return COLOR_BLUE;
    if (pct >= 4) return COLOR_GREEN;
    if (pct >= 1) return COLOR_YELLOW;
    return COLOR_RED;
}

}  // namespace

void battery_color_init(void) {
    last_report_us = 0;
    last_pct       = 0xFF;
}

void battery_color_note_report(void) {
    last_report_us = time_us_64();
}

void battery_color_on_disconnect(void) {
    last_report_us = 0;
    last_pct       = 0xFF;
}

void battery_color_tick(void) {
    if (!get_config().battery_color_enable) return;

    const uint64_t now = time_us_64();
    if (last_report_us == 0 || (now - last_report_us) >= REPORT_STALE_US) return;

    const uint8_t b   = interrupt_in_data[52];
    const uint8_t pct = b & 0x0F;

    // Only update the lightbar when the battery tier changes.
    if (pct == last_pct) return;
    last_pct = pct;

    const Color c = color_for_pct(pct);
    state_set_led_color(c.r, c.g, c.b);
}
