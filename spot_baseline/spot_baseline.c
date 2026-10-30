/* Spot multiball: stock shot/scoring rules for SPOT with only the
 * timer, starting ball count, add-ball and one-ball ending changes.
 * 
 * Core changes from the stock SPOT mode
 * 1. Timer Removed, the mode stops when you drain down to 1 ball
 * 2. Starts as a 2 ball multiball on Spot 1 and increases the ball count by 1 each time you pass munster madness
 * 3. It will not light add a ball at dragula during spot mode
 * 4. Only Lily can be stacked just like the stock mode. 
 *
 */
#include "pad_munsters.h"
#define MODE_NAME "Spot multiball"
/* Experimental Quick Shot: set ENABLED to 0 to compare with the stable rules. */
#define QUICK_SHOT_ENABLED 1
#define QUICK_SHOT_WINDOW_MS 5000ul
#define QUICK_SHOT_BASE_BONUS 1000000ull
#define QUICK_SHOT_EXTRA_BALL_BONUS 250000ull
#define QUICK_SHOT_MAX_BALLS 6
#define QUICK_SHOT_LAMP "LEFT RAMP JACKPOT"
#define QUICK_SHOT_FLASH_MS 200u
static long phase_at, hits_at, post_at, once_at;
static int ready;
static unsigned entries;
static unsigned mb_pending, mb_active, mb_seen, mb_target, mb_player;
static int qs_enabled,qs_active,qs_was_ready;
static unsigned qs_player,qs_collections;
static unsigned long qs_started;
static uint64_t qs_total;
/* Sample at collection: a drain during the window immediately lowers its value.
 * Unknown/transition counts retain only the base bonus; never use the serve target. */
static uint64_t quick_value(int balls) {
    if(balls<2) balls=2;
    if(balls>QUICK_SHOT_MAX_BALLS) balls=QUICK_SHOT_MAX_BALLS;
    return QUICK_SHOT_BASE_BONUS+(uint64_t)(balls-2)*QUICK_SHOT_EXTRA_BALL_BONUS;
}
static void quick_reset(void) {
    if(qs_active) pm_lamp_release(QUICK_SHOT_LAMP);
    qs_active=qs_was_ready=0;qs_player=0;
}
static void quick_sync(struct pm_stock_rule *r) {
    int up;
    unsigned long now=pm_ms();
    if(!qs_enabled || !r || !pm_in_game() || pm_player()!=mb_player || pm_stock_active(r)!=1) {
        quick_reset();return;
    }
    up=pm_munsters_spot_quick_ready(r);
    if(!up) {
        if(qs_active) pm_lamp_release(QUICK_SHOT_LAMP);
        qs_active=qs_was_ready=0;return;
    }
    if(!qs_was_ready) {
        qs_was_ready=1;
        if(pm_lamp_set(QUICK_SHOT_LAMP,PM_RGB(255,255,0),PM_LAMP_BLINK,QUICK_SHOT_FLASH_MS)>0) {
            qs_active=1;qs_player=pm_player();qs_started=now;
            pm_log("QUICK SHOT armed p%u window=%lums bonus_at_hit=live_ball_count lamp=%s",qs_player,QUICK_SHOT_WINDOW_MS,QUICK_SHOT_LAMP);
        } else pm_log("QUICK SHOT not armed: cue lamp unavailable");
    }
    if(qs_active && (unsigned long)(now-qs_started)>=QUICK_SHOT_WINDOW_MS) {
        qs_active=0;pm_lamp_release(QUICK_SHOT_LAMP);
        pm_log("QUICK SHOT expired p%u",qs_player);
    }
}
static int quick_take(struct pm_stock_rule *r) {
    int earned;
    quick_sync(r);
    earned=qs_active && qs_player==pm_player() &&
        (unsigned long)(pm_ms()-qs_started)<QUICK_SHOT_WINDOW_MS;
    if(qs_active) pm_lamp_release(QUICK_SHOT_LAMP);
    qs_active=0; /* Keep the edge latch: one opportunity per native raise. */
    return earned;
}
static void *badge, *badge_words;
static unsigned badge_active, badge_player, badge_ticks;
static void badge_update(void) {
    struct pm_stock_rule *r;
    unsigned show=badge_active;
    if(show) {
        r=pm_stock_rule(17);
        if(!ready || !pm_in_game() || pm_player()!=badge_player || !r || pm_stock_active(r)!=1)
            show=badge_active=0;
    }
    if(!pm_can(PM_CAN_SCREENS)) return;
    badge=pm_node("hud","PadMode_spot_baseline_Screen");
    badge_words=badge ? pm_text("hud","PadMode_spot_baseline_Screen.PadMode_spot_baseline_Screen_Words") : 0;
    if(badge_words) pm_set_text(badge_words,"");
    if(badge) pm_show(badge,(int)show);
}
static void badge_tick(void) {
    struct pm_stock_rule *r=pm_stock_rule(17);
    int balls;
    if((mb_pending || mb_active) && r) {
        if(!pm_in_game() || pm_player()!=mb_player || pm_stock_active(r)!=1) {
            mb_pending=mb_active=mb_seen=0;
            pm_munsters_spot_multiball_guard(0);
        } else if(mb_pending) {
            /* Deferred from native START: never serve inside its dispatch. */
            mb_pending=0;
            if(pm_multiball_start(mb_target,0)) {
                mb_active=1;
                pm_log("SPOT MB serving target=%u level=%u player=%u; no timer, no add-a-ball",mb_target,pm_munsters_spot_level(),mb_player);
            } else {
                pm_log("SPOT MB initial serve refused; ending through native STOP");
                pm_stock_stop(r,0);
            }
        } else {
            balls=pm_balls_in_play();
            if(balls>=2) mb_seen=1;
            /* The initial one-ball state is not a drain. Wait until the
             * framework has actually reported multiball before arming end. */
            if(mb_seen && balls>=0 && balls<=1) {
                pm_log("SPOT MB ended: %d ball(s) remain",balls);
                pm_stock_stop(r,0);
            }
        }
    }
    quick_sync(r);
    if(++badge_ticks%30==0 || !badge) badge_update();
}
static const struct pm_mode spot_baseline;
static int read_word(struct pm_stock_rule *r,long at) { int v=0; pm_stock_w_get(r,at,&v); return v; }
static int read_byte(struct pm_stock_rule *r,long at) { int v=0; pm_stock_b_get(r,at,&v); return v; }
static void state(struct pm_stock_rule *r,const char *why) {
    unsigned p=pm_player();
    pm_log("BASELINE %s p%u active=%d field=0x%llx phase=%d hits=%d post_super=%d extended=%d super_available=%d higher_level=%d score=%llu entries=%u",
        why,p,pm_stock_active(r),(unsigned long long)pm_stock_field(r,p),read_word(r,phase_at),read_word(r,hits_at),
        read_byte(r,post_at+p),read_byte(r,once_at),pm_munsters_spot_super_available(),pm_munsters_spot_higher_level(),
        (unsigned long long)pm_score(p),entries);
}
static int spot_shot(struct pm_stock_rule *r,uint64_t dispatched,unsigned factor) {
    unsigned p=pm_stock_player(),shot=(unsigned)dispatched,bucket,hits,remaining=0,just_lit=0;
    uint64_t field,points,bonus=0;
    int quick,quick_balls=0;
    (void)factor; /* Munsters' handler has (this, uint32_t shot), no factor. */
    if(!ready || !pm_munsters_spot_ready()) return PM_STOCK_PASS;
    ++entries;
    bucket=(unsigned)pm_munsters_spot_higher_level();
    field=pm_stock_field(r,p);
    if(shot & (unsigned)field) {
        /* Preserve stock's EXACT opto comparison, including composite masks. */
        if(shot==0x800000u) goto hit;
        if(shot & 9u) {
            pm_munsters_spot_grow(r,bucket ? 125000u : 100000u,bucket);
            pm_stock_field_set(r,p,0x800000u);
            pm_stock_w_set(r,phase_at,1);
            pm_munsters_spot_cancel_sequence();
            pm_munsters_spot_dirty();

        } else pm_munsters_spot_grow(r,bucket ? 175000u : 75000u,bucket);
        state(r,"shot handled");
        return PM_STOCK_DONE;
    }
    if(shot!=0x800000u || read_word(r,phase_at)!=1) return PM_STOCK_DONE;
hit:
    quick=quick_take(r);
    if(quick) {quick_balls=pm_balls_in_play();bonus=quick_value(quick_balls);}
    points=pm_munsters_spot_collect(r,bucket);
    if(quick) {
        bonus=pm_munsters_spot_flat_bonus(r,bonus);
        if(bonus) {++qs_collections;qs_total+=bonus;}
        pm_log("QUICK SHOT collected p%u balls=%d bonus=%llu count=%u total=%llu",p,quick_balls,(unsigned long long)bonus,qs_collections,(unsigned long long)qs_total);
    }
    hits=(unsigned)read_word(r,hits_at)+1u;
    pm_stock_w_set(r,hits_at,(int)hits);
    if(hits>3u) {
        pm_stock_w_set(r,hits_at,0);
        pm_munsters_spot_super_light();
        pm_stock_b_set(r,post_at+p,1);
        just_lit=1;
    } else if(pm_munsters_spot_super_available()) remaining=4u-hits;
    if(read_byte(r,post_at+p)) {
        pm_stock_field_set(r,p,0x1du);
        pm_munsters_spot_sequence(1);
    } else pm_munsters_spot_sequence(0);
    pm_munsters_spot_feedback(points+bonus,remaining,just_lit);
    state(r,"Spot hit");
    return PM_STOCK_DONE;
}
static void started(struct pm_stock_rule *r) {
    quick_reset();qs_collections=0;qs_total=0;
    entries=0;
    badge_active=ready && pm_munsters_spot_ready() && pm_stock_active(r)==1;
    badge_player=pm_player();
    mb_pending=mb_active=mb_seen=0;
    if(badge_active) {
        mb_player=badge_player;
        mb_target=pm_munsters_spot_level();
        if(mb_target<1) mb_target=1;
        if(mb_target>5) mb_target=5; /* SDK limit; engine also clips to installed balls. */
        ++mb_target;
        if(pm_munsters_spot_clock_off(r)) {
            pm_munsters_spot_multiball_guard(1);
            mb_pending=1;
        } else {
            pm_log("SPOT MB clock control failed; ending through native STOP");
            pm_stock_stop(r,0);
        }
    }
    badge_update();
    pm_log("VISUAL MARKER: CUSTOM SPOT %s",badge_active ? "enabled" : "off");
    state(r,"START native");
}
static void stopped(struct pm_stock_rule *r,unsigned reason) { quick_reset(); mb_pending=mb_active=mb_seen=0; pm_munsters_spot_multiball_guard(0); badge_active=0; badge_update(); pm_log("STOP native reason=%u",reason); state(r,"STOP"); }
static void init(void) {
    phase_at=pm_stock_at("spot_phase_at"); hits_at=pm_stock_at("spot_hits_at");
    post_at=pm_stock_at("spot_post_super_at"); once_at=pm_stock_at("spot_timer_extended_at");
    ready=pm_munsters_spot_ready() && phase_at>=0 && hits_at>=0 && post_at>=0 && once_at>=0;
    ready=ready && pm_munsters_spot_multiball_arm();
    qs_enabled=QUICK_SHOT_ENABLED && ready && pm_munsters_spot_quick_supported() && pm_can(PM_CAN_LAMPS) && pm_lamp_find(QUICK_SHOT_LAMP)>=0;
    pm_log("Quick Shot %s: %lums, base %llu + %llu per ball above two (cap %d balls), no multipliers; %s flashes yellow",qs_enabled ? "enabled" : "disabled",QUICK_SHOT_WINDOW_MS,(unsigned long long)QUICK_SHOT_BASE_BONUS,(unsigned long long)QUICK_SHOT_EXTRA_BALL_BONUS,QUICK_SHOT_MAX_BALLS,QUICK_SHOT_LAMP);
    pm_log("%s: C shot decisions; Spot multiball lifecycle; native mechanism, awards and supers",ready ? "READY" : "DISABLED");
}
static const struct pm_mode spot_baseline={.name=MODE_NAME,.init=init,.tick=badge_tick};
PM_REGISTER(spot_baseline);
static const struct pm_stock_handler spot_handler={.rule_id=17,.shot=spot_shot,.started=started,.stopped=stopped,.name=MODE_NAME,.mode=&spot_baseline};
#ifndef SPOT_CONTROL
PM_STOCK_RULE(spot_handler);
#endif
