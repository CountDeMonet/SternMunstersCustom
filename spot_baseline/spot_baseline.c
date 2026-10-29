/* Stock Spot shot decisions, translated from Munsters Pro 1.28 cspot_mode
 * v[25] (0xd3ca8..0xd3ef4). Stock qualification, START/STOP, clock, displays,
 * toy control and the shared Super Jackpot system remain the game's own.
 * No prototype multiball rules. The original handler is never called by this
 * handler once the build-checked bridge is ready. See BASELINE.md for evidence.
 */
#include "pad_munsters.h"
#define MODE_NAME "Stock Spot baseline"
static long phase_at, hits_at, post_at, once_at;
static int ready;
static unsigned entries;
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
    uint64_t field,points;
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
            if(pm_munsters_spot_timer_live(r) && !read_byte(r,once_at)) {
                pm_munsters_spot_timer_extend(r);
                pm_stock_b_set(r,once_at,1);
            }
        } else pm_munsters_spot_grow(r,bucket ? 175000u : 75000u,bucket);
        state(r,"shot handled");
        return PM_STOCK_DONE;
    }
    if(shot!=0x800000u || read_word(r,phase_at)!=1) return PM_STOCK_DONE;
hit:
    points=pm_munsters_spot_collect(r,bucket);
    pm_munsters_spot_timer_refresh(r);
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
    pm_munsters_spot_feedback(points,remaining,just_lit);
    state(r,"Spot hit");
    return PM_STOCK_DONE;
}
static void started(struct pm_stock_rule *r) {
    entries=0;
    badge_active=ready && pm_munsters_spot_ready() && pm_stock_active(r)==1;
    badge_player=pm_player();
    badge_update();
    pm_log("VISUAL MARKER: CUSTOM SPOT %s",badge_active ? "enabled" : "off");
    state(r,"START native");
}
static void stopped(struct pm_stock_rule *r,unsigned reason) { badge_active=0; badge_update(); pm_log("STOP native reason=%u",reason); state(r,"STOP"); }
static void init(void) {
    phase_at=pm_stock_at("spot_phase_at"); hits_at=pm_stock_at("spot_hits_at");
    post_at=pm_stock_at("spot_post_super_at"); once_at=pm_stock_at("spot_timer_extended_at");
    ready=pm_munsters_spot_ready() && phase_at>=0 && hits_at>=0 && post_at>=0 && once_at>=0;
    pm_log("%s: C shot decisions; native lifecycle, mechanism, awards and supers",ready ? "READY" : "DISABLED");
}
static const struct pm_mode spot_baseline={.name=MODE_NAME,.init=init,.tick=badge_tick};
PM_REGISTER(spot_baseline);
static const struct pm_stock_handler spot_handler={.rule_id=17,.shot=spot_shot,.started=started,.stopped=stopped,.name=MODE_NAME,.mode=&spot_baseline};
#ifndef SPOT_CONTROL
PM_STOCK_RULE(spot_handler);
#endif
