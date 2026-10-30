#ifndef PAD_MUNSTERS_H
#define PAD_MUNSTERS_H
#include "pad_stock.h"
/* Local, build-checked Munsters Pro 1.28 bridge. No coil commands. The stock
 * START/STOP, qualification, toy controller and displays remain native.
 * Multiball lifecycle hooks below are explicitly armed by the new variant. */
int pm_munsters_spot_ready(void);
int pm_munsters_spot_higher_level(void);
unsigned pm_munsters_spot_level(void);
int pm_munsters_spot_multiball_arm(void);
void pm_munsters_spot_multiball_guard(int enabled);
int pm_munsters_spot_clock_off(struct pm_stock_rule *);
uint64_t pm_munsters_spot_grow(struct pm_stock_rule *, uint64_t, unsigned bucket);
uint64_t pm_munsters_spot_collect(struct pm_stock_rule *, unsigned bucket);
int pm_munsters_spot_timer_live(struct pm_stock_rule *);
void pm_munsters_spot_timer_extend(struct pm_stock_rule *);
void pm_munsters_spot_timer_refresh(struct pm_stock_rule *);
void pm_munsters_spot_cancel_sequence(void);
void pm_munsters_spot_sequence(int after_super);
int pm_munsters_spot_super_available(void);
void pm_munsters_spot_super_light(void);
void pm_munsters_spot_feedback(uint64_t points, unsigned remaining, unsigned just_lit);
void pm_munsters_spot_dirty(void);
int pm_stock_b_set(struct pm_stock_rule *, long at, int value);
/* Emulator test input: invokes this rule's CURRENT virtual, including its C
 * replacement, while active. It does not generate a physical switch edge. */
int pm_munsters_spot_test_shot(struct pm_stock_rule *, unsigned shot);
/* Emulator diagnostics; mutation requires test-only port flag. */
void pm_munsters_spot_test_control(const char *command);
#endif
