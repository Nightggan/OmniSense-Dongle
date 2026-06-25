#include "config.h"
#include "lightbar_controller.h"
#include "state_mgr.h"
#include <cstdio>
#include <cstring>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "cmd.h"
#include "pico/time.h"
#include "bt.h"
#include "audio.h"
#include "utils.h"

extern uint8_t interrupt_in_data[63]; // defined in main.cpp
extern bool spk_active; // main.cpp: true while host USB speaker stream is open
extern uint8_t host_real_color_r;
extern uint8_t host_real_color_g;
extern uint8_t host_real_color_b;
extern uint8_t lb_controlled_red;
extern uint8_t lb_controlled_green;
extern uint8_t lb_controlled_blue;
extern bool config_mode_enabled;
namespace{
    // Prototipo de la función (el compilador ahora sabrá que existe)
    
    
    // --- Charge ETA tracker --------------------------------------------------
    // The DS5 only reports battery in 10% steps (interrupt_in_data[52] low
    // nibble, 0..10; high nibble is power-state, 1 == charging). We can't read a
    // finer percentage over BT, so a smooth countdown is impossible. Instead we
    // time how long each 10% step takes while charging and extrapolate the
    // remaining steps. Sampled once per frame from oled_loop (continuously, so
    // the estimate stays current even while the panel is dimmed/off and even when
    // the user is on another screen); render_screen reads g_charge_eta.
    //
    // Taper correction: Li-ion CC/CV charging slows sharply near the top, so a
    // flat "time per step × steps left" runs optimistic in the last ~20%. Each
    // measured step is normalised to a bulk-equivalent duration (divide out the
    // step's taper weight); the remaining steps are then re-weighted. This makes
    // the estimate consistent whether the user plugged in near-empty or near-full.
    struct ChargeEta {
        bool charging;    // pstate == 1 (so the token shows only while charging)
        bool valid;       // minutes is meaningful (provisional or measured)
        bool provisional; // true until a full step is timed — using the default rate
        int  minutes;     // estimated minutes to 100%
    };
    ChargeEta g_charge_eta{};
    uint32_t last_render_us = 0;
    constexpr uint32_t kFrameUs = 100000;
    // Relative time the step *ending* at `to_level` (10% units, 1..10) takes vs a
    // bulk step. Tuned to the Li-ion CV taper: ~80% onward stretches out.
    static float charge_step_weight(int to_level) {
        if (to_level >= 10) return 2.2f;  // 90→100% (constant-voltage tail)
        if (to_level == 9)  return 1.5f;  // 80→90%  (taper begins)
        return 1.0f;                      // bulk constant-current region
    }

    // Default bulk-step duration used for a provisional estimate before any real
    // step has been timed, so the token shows "~Nm?" immediately on plug-in instead
    // of sitting on "~--m" for ~15-20 min. Tuned to an observed ~15 min per 10% on
    // this dongle's charge current; it self-corrects to the measured rate (and drops
    // the "?") as soon as the first clean step completes.
    constexpr float kDefaultStepUs = 15.0f * 60.0f * 1000000.0f;

    // Ceiling on a single timed step's bulk-equivalent duration. A genuine idle 10%
    // step on this dongle is ~15 min; anything past ~30 min is almost always an
    // anomalous/under-load sample (e.g. the controller in use while charging, or a
    // battery-nibble bounce) that would otherwise balloon the projection — observed
    // reading ~222m at 70% off one ~47-min step. We clamp such samples instead of
    // trusting them, and pair that with a median over kRing steps so one bad reading
    // can't dominate the estimate.
    constexpr float kMaxStepUs = 30.0f * 60.0f * 1000000.0f;

    // Tiny 32-step sine LUT (no <cmath>). angle 0..255 → amplitude -127..127.
    static const int8_t kSine32[32] = {
        0,   24,   49,   70,   90,  106,  117,  125,  127,  125,  117,  106,   90,   70,   49,   24,
        0,  -24,  -49,  -70,  -90, -106, -117, -125, -127, -125, -117, -106,  -90,  -70,  -49,  -24,
    };
    int sin_lut(uint8_t a) { return kSine32[(a >> 3) & 0x1F]; }
    
    constexpr uint64_t REPORT_STALE_US = 2'000'000;

    struct Color { uint8_t r, g, b; };
    constexpr Color COLOR_BLUE   = {  0,   0, 255};
    constexpr Color COLOR_GREEN  = {  0, 200,   0};
    constexpr Color COLOR_YELLOW = {255, 180,   0};
    constexpr Color COLOR_RED    = {255,   0,   0};

    // Color tiers based on PowerPercent (0–10 scale, each unit ≈ 10%)
    // pct >=4      → Green  (40–100 %)
    // 1-3       → Yellow (10–39 %)
    // pct == 0  → Red    (< 10 %)
    Color color_for_pct(uint8_t pct) {
        
        if (pct >= 4) return COLOR_GREEN;
        if (pct >= 1) return COLOR_YELLOW;
        return COLOR_RED;
    }
    uint64_t last_report_us = 0;
    uint8_t  last_pct       = 0xFF;  // sentinel: "not yet set"
    
    static uint8_t base_r = 0, base_g = 0, base_b = 0;
    
    uint8_t lb_r = 0, lb_g = 0, lb_b = 0;
    // For the modes that use fixed colors, configurable from the app and shortcut.
    
    int lb_mode = 0;
    bool g_lightbar_override = false; // true when firmware is controlling the LED, false when host controls it
    bool g_lightbar_override_breathing = false; // true when firmware is controlling the LED with a breathing effect, false otherwise

    void send_lightbar_color(uint8_t r, uint8_t g, uint8_t b) {
        uint8_t pkt[78] = {};
        pkt[0] = 0x31;
        pkt[2] = 0x10;
        // valid_flag1: LIGHTBAR_CONTROL_ENABLE (bit 2)
        // Flags de validez: Bit 0 = Motores/Gatillos, Bit 2 = Lightbar
        pkt[4] = 0x04;// valid_flag1: LIGHTBAR_CONTROL_ENABLE (bit 2)
        
        pkt[47] = r;   // lightbar_red
        pkt[48] = g;   // lightbar_green
        pkt[49] = b;   // lightbar_blue
        
        //If in config state mode 
        if(config_mode_enabled)
        {
            pkt[11] = MuteLight::Breathing; //Mute Light Breathing on config mode 3+8
        }
        else
        {
            pkt[11] = MuteLight::Off; //Mute Light Breathing on config mode 3+8
        }
        

        bt_write(pkt, sizeof(pkt));
    }

    void battery_color_init(void) {
        last_report_us = 0;
        last_pct       = 0xFF;
    }

    void sample_charge_eta() {
        constexpr int kRing = 5;                 // median over the last few steps
        static float    ring[kRing] = {0};       // bulk-equivalent step durations (us)
        static int      ring_count = 0;
        static int      ring_head = 0;
        static int      cur_step = -1;           // last observed 10% step
        static uint64_t step_start_us = 0;
        static bool     was_charging = false;
        static bool     first_step_pending = false;  // discard the partial step at plug-in

        const uint8_t pwr   = interrupt_in_data[52];
        int           step  = pwr & 0x0F;
        if (step > 10) step = 10;
        const uint8_t pstate = pwr >> 4;
        const bool charging = bt_is_connected() && (pstate == 1);

        
        if (!charging) {
            g_charge_eta = ChargeEta{};          // clears charging/valid/minutes
            ring_count = ring_head = 0;
            cur_step = -1;
            was_charging = false;
            return;
        }

        const uint64_t now = time_us_64();
        if (!was_charging) {
            // Just plugged in: start timing from here. The step in progress is
            // partial, so its duration gets discarded when it completes.
            cur_step = step;
            step_start_us = now;
            ring_count = ring_head = 0;
            first_step_pending = true;
            was_charging = true;
        } else if (step == cur_step + 1) {
            // One clean step completed. Skip the first (partial) one; otherwise
            // record its bulk-equivalent duration.
            const float dur = (float)(now - step_start_us);
            if (first_step_pending) {
                first_step_pending = false;
            } else {
                float be = dur / charge_step_weight(step);
                if (be > kMaxStepUs) be = kMaxStepUs;   // clamp under-load/anomalous outliers
                ring[ring_head] = be;
                ring_head = (ring_head + 1) % kRing;
                if (ring_count < kRing) ring_count++;
            }
            cur_step = step;
            step_start_us = now;
        } else if (step != cur_step) {
            // Multi-step jump (e.g. woke from sleep across several steps) or a
            // small dip under heavy use — can't attribute timing cleanly, so just
            // resync without polluting the ring.
            cur_step = step;
            step_start_us = now;
            first_step_pending = false;
        }

        g_charge_eta.charging = true;
        if (cur_step < 10) {
            // Use the measured rate once we have a timed step; until then fall back
            // to the default rate and flag the estimate provisional (renders "?").
            const bool measured = (ring_count > 0);
            float bulk;
            if (measured) {
                // Median of the timed steps — robust to a single slow/fast outlier
                // in a way the old mean wasn't (one 47-min under-load step used to
                // drag the whole projection up). kRing is tiny, so insertion-sort.
                float tmp[kRing];
                for (int i = 0; i < ring_count; i++) tmp[i] = ring[i];
                for (int i = 1; i < ring_count; i++) {
                    const float v = tmp[i];
                    int j = i - 1;
                    while (j >= 0 && tmp[j] > v) { tmp[j + 1] = tmp[j]; j--; }
                    tmp[j + 1] = v;
                }
                bulk = tmp[ring_count / 2];
            } else {
                bulk = kDefaultStepUs;
            }
            float rem_us = 0.0f;
            for (int L = cur_step + 1; L <= 10; L++) rem_us += bulk * charge_step_weight(L);
            int mins = (int)(rem_us / 60000000.0f + 0.5f);
            if (mins < 0)   mins = 0;
            if (mins > 999) mins = 999;
            g_charge_eta.valid = true;
            g_charge_eta.provisional = !measured;
            g_charge_eta.minutes = mins;
        } else {
            // cur_step == 10 → essentially full; nothing meaningful to count down.
            g_charge_eta.valid = true;
            g_charge_eta.provisional = false;
            g_charge_eta.minutes = 0;
        }
    }

    void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
        if (h >= 360) h %= 360;
        const uint8_t region = (uint8_t)(h / 60);
        const uint16_t remainder = (uint16_t)((h - region * 60u) * 256u / 60u);
        const uint8_t p = (uint8_t)(((uint16_t)v * (255u - s)) >> 8);
        const uint8_t q = (uint8_t)(((uint16_t)v * (255u - (((uint16_t)s * remainder) >> 8))) >> 8);
        const uint8_t t = (uint8_t)(((uint16_t)v * (255u - (((uint16_t)s * (255u - remainder)) >> 8))) >> 8);
        switch (region) {
            case 0:  *r = v; *g = t; *b = p; break;
            case 1:  *r = q; *g = v; *b = p; break;
            case 2:  *r = p; *g = v; *b = t; break;
            case 3:  *r = p; *g = q; *b = v; break;
            case 4:  *r = t; *g = p; *b = v; break;
            default: *r = v; *g = p; *b = q; break;
        }
    }
    
    void lightbar_compute_mode(int mode, uint32_t now_ms) {
        Config_body local_config = get_config();
                
        if(g_lightbar_override)
        {
            if (mode == 1) {
                // DISABLED: solid off
                lb_r = lb_g = lb_b = 0;
                lb_controlled_red = lb_r;
                lb_controlled_green = lb_g;
                lb_controlled_blue = lb_b;
                
            } else if (mode <= 5) {
                // FAV slot: fixed color
                const int slot = mode - 2;
                lb_r = local_config.lb_fav_r[slot];
                lb_g = local_config.lb_fav_g[slot];
                lb_b = local_config.lb_fav_b[slot];
                lb_controlled_red = lb_r;
                lb_controlled_green = lb_g;
                lb_controlled_blue = lb_b;
            }else if (mode == 6) {
                // BATTERY: reflect battery level in brightness, solid color from config

                const uint8_t b   = interrupt_in_data[52];
                const uint8_t pct = b & 0x0F;

                const Color c = color_for_pct(pct);
                lb_r = c.r;
                lb_g = c.g;
                lb_b = c.b;
                lb_controlled_red = lb_r;
                lb_controlled_green = lb_g;
                lb_controlled_blue = lb_b;
            } else if (mode == 7) {
                // RAINBOW: hue sweep over ~6 s
                const uint16_t hue = (uint16_t)((now_ms / 17) % 360);
                hsv_to_rgb(hue, 255, 255, &lb_r, &lb_g, &lb_b);
            } else if(mode == 8) {
                // FADE between FAV slots, 2 s per slot
                const uint32_t kSlotMs = 2000;
                const uint32_t total = now_ms % (4 * kSlotMs);
                const int slot = (int)(total / kSlotMs);
                const int next = (slot + 1) & 3;
                const uint16_t blend = (uint16_t)(((total - slot * kSlotMs) * 256u) / kSlotMs);
                lb_r = (uint8_t)((local_config.lb_fav_r[slot] * (255 - blend) + local_config.lb_fav_r[next] * blend) / 255);
                lb_g = (uint8_t)((local_config.lb_fav_g[slot] * (255 - blend) + local_config.lb_fav_g[next] * blend) / 255);
                lb_b = (uint8_t)((local_config.lb_fav_b[slot] * (255 - blend) + local_config.lb_fav_b[next] * blend) / 255);
                lb_controlled_red = lb_r;
                lb_controlled_green = lb_g;
                lb_controlled_blue = lb_b;
            }
        }
        //Breathing mode only applies for mode 0 and solid fav slots (mode 2 to 5)
        if(g_lightbar_override_breathing) {
            if((mode<=5)&&(mode!=1)) {
                if(mode==0)
                {
                    const uint8_t phase = (uint8_t)(now_ms / 12);
                    const int s = sin_lut(phase); // -127..127
                    const uint16_t scale = (uint16_t)(32 + (s + 127) / 2); // 32..191
                    
                    
                    base_r = host_real_color_r;
                    base_g = host_real_color_g;
                    base_b = host_real_color_b;
                    
                    
                    uint8_t led_final_r = (uint8_t)((base_r * scale) / 255);
                    uint8_t led_final_g = (uint8_t)((base_g * scale) / 255);
                    uint8_t led_final_b = (uint8_t)((base_b * scale) / 255);
                    lb_controlled_red = led_final_r;
                    lb_controlled_green = led_final_g;
                    lb_controlled_blue = led_final_b;
                    state_set_led_color(led_final_r, led_final_g, led_final_b);
                    
                    
                    if (!spk_active) {
                        send_lightbar_color(led_final_r, led_final_g, led_final_b);
                    }
                }
                else
                {
                    
                    // Mode selected to fav slots 2-5 to 0-3
                    const int slot = mode - 2;
                    
                    base_r = local_config.lb_fav_r[slot];
                    base_g = local_config.lb_fav_g[slot];
                    base_b = local_config.lb_fav_b[slot];

                    const uint8_t phase = (uint8_t)(now_ms / 12);
                    const int s = sin_lut(phase); 
                    const uint16_t scale = (uint16_t)(32 + (s + 127) / 2); 
                    
                    // The attenuation is applied on the final var to be applied, not the base color of the fav selected
                    lb_r = (uint8_t)((base_r * scale) / 255);
                    lb_g = (uint8_t)((base_g * scale) / 255);
                    lb_b = (uint8_t)((base_b * scale) / 255);
                    lb_controlled_red = lb_r;
                    lb_controlled_green = lb_g;
                    lb_controlled_blue = lb_b;
                }
                
                
            }
            
        }
        
    }

    // The single owner of the controller LED. Runs every frame (~10 Hz)
    //   1. Charging  -> Breathing green color, overrides all other modes
    //   2. Low Battery -> Fast breathing red color, overrides all other modes if it is not charging
    //   2. Selected mode -> 0: default (host controlled), 1: disabled, 2: Fav 0, 3: Fav 1, 4: Fav 2, 5: Fav 3, 6: battery level, 7: rainbow, 8: favs fade
    // When the firmware owns the LED it (a) writes state[] so the color rides every
    // host/audio packet and (b) actively pushes it via send_lightbar_color so it
    // updates even when the host is idle and animations keep moving.
    // If a second host tries to send another color (not black) (Like Steam vs Xbox GP Game), state_mgr (state_update) waits for 3 seconds,
    // if the host is still sending another color after the 3 secs, host color changes (only for mode 0). Ignores the host if it is sending a black color (Full zeroes)
    
    void lightbar_service() {
        if (!bt_is_connected()) { return; }// if we're not connected, we shouldn't be controlling the lightbar at all, so we just return here and do nothing, which means the lightbar will be off (or in its default state) when not connected, and we won't be trying to send any BT packets or update the LED color when there's no connection, which is what we want.
        const auto &config = get_config();
        lb_mode = config.lightbar_mode;
        g_lightbar_override_breathing = config.lightbar_breathing;
        const uint32_t now_ms = (uint32_t)time_us_32() / 1000;
        g_lightbar_override = true; // assume control until we check the mode; if the mode is HOST, we'll set this to false to hand control back to the host
        

        const uint8_t b   = interrupt_in_data[52];
        const uint8_t pct = b & 0x0F;
        bool valid_battery_reading = (pct <= 10); // Valid readings are 0-10; 0x0F indicates an invalid reading
        bool low_battery = valid_battery_reading && (pct <= 1); // Consider battery low if we have a valid reading and it's 10% or less
        // Charging status trumps all: overrides all other modes
        if (g_charge_eta.charging) {
            //Fast breathing green color to indicate it is charging
            const uint8_t  phase = (uint8_t)(now_ms / 18);
            const int      s     = sin_lut(phase);                          // -127..127
            const uint16_t scale = (uint16_t)(24 + ((s + 127) * 216) / 254); // 24..240
            lb_r = 0;
            lb_g = (uint8_t)((255u * scale) / 255u);
            lb_b = 0;
        }
        else
        {
            if(valid_battery_reading&&low_battery) {// If the battery level is critically low, override all modes with a distinct fast breathing red to alert the user to charge soon. This ensures the warning is visible even if the user has set a different mode or color.
                lb_r = COLOR_RED.r;
                lb_g = COLOR_RED.g;
                lb_b = COLOR_RED.b;
                
                const uint8_t phase = (uint8_t)(now_ms / 2);
                const int s = sin_lut(phase); // -127..127
                const uint16_t scale = (uint16_t)(32 + (s + 127) / 2); // 32..191
                lb_r = (uint8_t)((lb_r * scale) / 255);
                lb_g = (uint8_t)((lb_g * scale) / 255);
                lb_b = (uint8_t)((lb_b * scale) / 255);
            }
            else if (lb_mode == 0) {
                // Reflect the host's current LED

                if(g_lightbar_override_breathing)
                {
                    lightbar_compute_mode(lb_mode, now_ms);
                    return;// in HOST mode with breathing enabled, we want to keep the breathing effect but use the host color as the base color for the breathing effect, so we call lightbar_compute_mode to compute the breathing effect on top of the host color, and then we return here to skip the rest of the function, which means we won't be calling state_set_led_color or send_lightbar_color again in this function, so the firmware won't be controlling the LED anymore after this point, and the host will have control over the LED with a breathing effect applied on top of it, which is what we want in this case.
                }
                else
                {
                    lb_r = host_real_color_r;
                    lb_g = host_real_color_g;
                    lb_b = host_real_color_b;//Asigned the host real color to the temp var for it to to be sent to the controller
                }
            
            } 
            else 
            {
                lightbar_compute_mode(lb_mode, now_ms);
            }
        }

        lb_controlled_red = lb_r; //Asigned the calculated new color with all the effects applied to the extern var for main.cpp to use on keep_alive
        lb_controlled_green = lb_g;
        lb_controlled_blue = lb_b;
        state_set_led_color(lb_r, lb_g, lb_b);  // ride every host/audio frame
        if (!spk_active) {
            // Active push so the LED updates when the host is idle and animations
            // keep moving. Skipped during audio: the 0x36 frames already carry
            // state[]'s LED at audio rate, and slipping a 0x31 between them would
            // intrude on the load-bearing audio/haptic packet cadence.
            send_lightbar_color(lb_r, lb_g, lb_b);
        }
    }
    
    
        
}

void lightbar_init() {
    
}

void lightbar_loop() {
    const uint32_t now = time_us_32();
    if ((now - last_render_us) < kFrameUs) return;
    last_render_us = now;
    sample_charge_eta();
    lightbar_service();
    
}