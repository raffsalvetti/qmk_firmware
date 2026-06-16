#include "mercurio.h"

/* =========================================================================
 * Global split shared state
 *
 * Zero-initialised at startup; fields are written by their respective
 * modules as each phase is activated (P2–P9).
 * ========================================================================= */
mercurio_split_state_t mercurio_state = {0};

/* =========================================================================
 * keyboard_post_init_kb
 *
 * Called once after QMK finishes its own initialisation.
 * Module init calls are added here in later phases:
 *   P4: ldr_init()
 *   P5: piezo_init(), piezo_effect_startup()
 *   P6: i2c_module_init() for both slots
 *   P7: oled_module_init()   (left half only)
 *   P8: mouse_module_init()  (right half only)
 * ========================================================================= */
void keyboard_post_init_kb(void) {
    keyboard_post_init_user();
}

/* =========================================================================
 * housekeeping_task_kb
 *
 * Called every scan cycle (~1 ms).  Rate-limited module tasks are
 * dispatched from here in later phases:
 *   P4: ldr_task()           (every 100 ms via timer_elapsed32)
 *   P5: piezo_task()         (every 1 ms)
 *   P7: oled_module_task()   (every 50 ms, left half only)
 *   P8: mouse_module_task()  (every 10 ms, right half only)
 * ========================================================================= */
void housekeeping_task_kb(void) {
    housekeeping_task_user();
}
