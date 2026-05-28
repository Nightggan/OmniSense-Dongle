//
// Lightbar color indicator based on DualSense battery level.
// Reads PowerPercent from interrupt_in_data[52] (0x31 BT report).
// Updates the state_mgr LED color only when the battery tier changes.
//

#pragma once

void battery_color_init(void);

// Call once per main-loop iteration.
void battery_color_tick(void);

// Call whenever a fresh 0x31 report has been copied into interrupt_in_data.
void battery_color_note_report(void);

// Call from the BT disconnect handler. Resets state until a fresh report arrives.
void battery_color_on_disconnect(void);
