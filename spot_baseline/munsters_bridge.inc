/* Project-local adapter. Included after stock_rule and fn/data helpers exist. */
static unsigned spot_obj(struct pm_stock_rule *r) {
    struct stock_rule *x = stock161_of(r);
    return x && x->id == 17 && pm_munsters_spot_ready() ? x->obj : 0;
}
int pm_munsters_spot_ready(void) {
    static const char *const required[] = {"stock_rule_get", "spot_level_get", "spot_rule_get",
        "spot_award_grow", "spot_award_collect", "spot_super_available", "spot_super_light",
        "spot_process_cancel", "spot_process_start", "spot_pre_super_seq", "spot_post_super_seq",
        "award_screen", "callout"};
    unsigned i;
    if (!str_eq(pm_game(), "munsters_pro") || !str_eq(pm_version(), "1.28") ||
        pm_port_value("stock_shot_bits",0) != 32 || !data("spot_lamps_dirty") || !data("spot_screen_info")) return 0;
    for (i=0; i<sizeof required/sizeof required[0]; ++i) if (!fn(required[i])) return 0;
    return 1;
}
unsigned pm_munsters_spot_level(void) {
    unsigned p=pm_player(), f=fn("spot_level_get"), o;
    if (!pm_munsters_spot_ready() || p<1 || p>4 || !f) return 0;
    o=((unsigned (*)(void))(unsigned long)f)();
    return o ? rw(o + (p+13u)*4u) : 0;
}
int pm_munsters_spot_higher_level(void) { return pm_munsters_spot_level()>1u; }
uint64_t pm_munsters_spot_grow(struct pm_stock_rule *r, uint64_t value, unsigned bucket) {
    unsigned o=spot_obj(r), f=fn("spot_award_grow");
    if (!o || !f) return 0;
    return ((uint64_t (*)(unsigned,unsigned,unsigned,unsigned,unsigned))(unsigned long)f)
        (rw(o+0x34),0,(unsigned)value,(unsigned)(value>>32),bucket);
}
uint64_t pm_munsters_spot_collect(struct pm_stock_rule *r, unsigned bucket) {
    unsigned o=spot_obj(r), f=fn("spot_award_collect");
    if (!o || !f) return 0;
    return ((uint64_t (*)(unsigned,unsigned,unsigned,unsigned))(unsigned long)f)(rw(o+0x34),0,bucket,1);
}
static unsigned spot_timer(struct pm_stock_rule *r) { unsigned o=spot_obj(r); return o ? rw(o+0x7c) : 0; }
int pm_munsters_spot_timer_live(struct pm_stock_rule *r) {
    unsigned t=spot_timer(r); return t ? ((unsigned (*)(unsigned))(unsigned long)rw(rw(t)+9u*4u))(t)!=0 : 0;
}
void pm_munsters_spot_timer_extend(struct pm_stock_rule *r) {
    unsigned t=spot_timer(r); if(t) ((void (*)(unsigned,unsigned))(unsigned long)rw(rw(t)+32u*4u))(t,1);
}
void pm_munsters_spot_timer_refresh(struct pm_stock_rule *r) {
    unsigned o=spot_obj(r); if(o) ((unsigned (*)(unsigned,unsigned,unsigned))(unsigned long)rw(rw(o)+8u*4u))(o,4,0);
}
void pm_munsters_spot_cancel_sequence(void) {
    unsigned f=fn("spot_process_cancel"); if(pm_munsters_spot_ready() && f) ((void (*)(unsigned,unsigned))(unsigned long)f)(269,65535);
}
void pm_munsters_spot_sequence(int after_super) {
    unsigned f=fn("spot_process_start"), body=fn(after_super ? "spot_post_super_seq" : "spot_pre_super_seq");
    if(pm_munsters_spot_ready() && f && body) ((unsigned (*)(unsigned,unsigned,unsigned))(unsigned long)f)(269,body,0);
}
static unsigned spot_supers(void) {
    unsigned f=fn("spot_rule_get"); return pm_munsters_spot_ready() && f ? ((unsigned (*)(unsigned))(unsigned long)f)(36) : 0;
}
int pm_munsters_spot_super_available(void) {
    unsigned o=spot_supers(), f=fn("spot_super_available"); return o && f ? ((unsigned (*)(unsigned,unsigned))(unsigned long)f)(o,3)!=0 : 0;
}
void pm_munsters_spot_super_light(void) {
    unsigned o=spot_supers(), f=fn("spot_super_light"); if(o && f) ((void (*)(unsigned,unsigned))(unsigned long)f)(o,3);
}
void pm_munsters_spot_dirty(void) {
    unsigned o=data("spot_lamps_dirty"); if(pm_munsters_spot_ready() && o) *(volatile unsigned char *)(unsigned long)o=1;
}
void pm_munsters_spot_feedback(uint64_t points, unsigned remaining, unsigned just_lit) {
    unsigned f=fn("award_screen"), o;
    if(!pm_munsters_spot_ready()) return;
    pm_callout(209);
    o=f ? ((unsigned (*)(unsigned,unsigned,unsigned,unsigned))(unsigned long)f)(91,0,0,data("spot_screen_info")) : 0;
    if(o) {
        *(volatile uint64_t *)(unsigned long)(o+160)=points;
        *(volatile unsigned *)(unsigned long)(o+168)=remaining;
        *(volatile unsigned char *)(unsigned long)(o+176)=(unsigned char)just_lit;
    }
}
int pm_stock_b_set(struct pm_stock_rule *r, long at, int value) {
    volatile unsigned *w=stock161_word(r,at,0,0); if(!w || value<0 || value>255) return 0;
    *(volatile unsigned char *)w=(unsigned char)value; return 1;
}
int pm_munsters_spot_test_shot(struct pm_stock_rule *r, unsigned shot) {
    unsigned o=spot_obj(r); if(!o || !pm_in_game() || pm_stock_active(r)!=1) return 0;
    ((void (*)(unsigned,unsigned))(unsigned long)rw(rw(o)+25u*4u))(o,shot); return 1;
}

/* Multiball variant only. The baseline snapshot never arms these hooks.
 * 0x32d138 is the framework's relative add-ball call, distinct from its
 * total-in-play initial serve at 0x32cff8. Reject before serving or consuming
 * the native add-ball award. Both hooks pass through outside custom Spot. */
static int spot_mb_guard, spot_mb_armed;
static int spot_mb_veto(unsigned *regs, unsigned which) {
    struct pm_stock_rule *r;
    (void)regs; (void)which;
    if(!spot_mb_guard || !pm_in_game()) return 0;
    r=pm_stock_rule(17);
    return r && pm_stock_active(r)==1;
}
/* Grandpa kickout's dedicated save helper. Its native multiball check
 * cannot see the custom Spot lifecycle on cspot_mode's timed-mode object.
 * Refuse only this save; do not skip the scoop handler or change adjustments. */
static int spot_grandpa_save_veto(unsigned *regs, unsigned which) {
    int blocked=spot_mb_veto(regs,which);
    if(blocked) say("Spot multiball: Grandpa eject ball save suppressed");
    return blocked;
}
int pm_munsters_spot_multiball_arm(void) {
    unsigned add=fn("spot_add_ball"), eligible=fn("spot_add_eligible");
    unsigned bind=fn("spot_clock_bind_call"), pause=fn("spot_timer_pause");
    unsigned grandpa=fn("spot_grandpa_eject_save"), mystery=fn("spot_mystery_eligible");
    if(spot_mb_armed) return 1;
    if(!pm_munsters_spot_ready() || !pm_can(PM_CAN_MULTIBALL) || !add || !eligible || !bind || !pause || !grandpa || !mystery) return 0;
    if(!hook_veto_n(add,spot_mb_veto,0,0) || !hook_veto_n(eligible,spot_mb_veto,1,0) ||
       !hook_veto_n(grandpa,spot_grandpa_save_veto,2,0) ||
       /* Shared by Mystery's scoop handler and available/lamp query. Refuse
        * before the lottery, award and per-player lit-byte consumption. */
       !hook_veto_n(mystery,spot_mb_veto,3,0)) return 0;
    /* This checked call is ONLY Spot START's clock registration. The return
     * value is unused. Skip it rather than displaying a frozen countdown.
     * Other modes' calls to the shared HUD binder remain untouched. */
    if(mprotect((void *)(unsigned long)(bind & ~0xfffu),0x1000,7)) return 0;
    *(volatile unsigned *)(unsigned long)bind=0xe1a00000u;
    __builtin___clear_cache((char *)(unsigned long)bind,(char *)(unsigned long)(bind+4));
    spot_mb_armed=1;
    return 1;
}
void pm_munsters_spot_multiball_guard(int enabled) { spot_mb_guard=spot_mb_armed && enabled; }
int pm_munsters_spot_clock_off(struct pm_stock_rule *r) {
    unsigned t=spot_timer(r), f=fn("spot_timer_pause");
    if(!spot_mb_armed || !t || !f || rw(rw(t)+3u*4u)!=f) return 0;
    /* Timer v[3](1) makes its v[43] pause predicate true; the countdown
     * process stays alive, preserving native active/qualification queries. */
    ((void (*)(unsigned,unsigned))(unsigned long)f)(t,1);
    return 1;
}

void pm_munsters_spot_test_control(const char *command) {
    unsigned f,o,p=pm_player(),t,hud;
    struct pm_stock_rule *r=pm_stock_rule(17);
    if(!r || !pm_munsters_spot_ready()) return;
    if(pm_port_value("spot_test_controls",0)) {
        if(str_eq(command,"level2") || str_eq(command,"level3")) {
            f=fn("spot_level_get"); o=((unsigned (*)(void))(unsigned long)f)();
            if(o && p>=1 && p<=4) *(volatile unsigned *)(unsigned long)(o+(p+13)*4)=str_eq(command,"level2") ? 2u : 3u;
        } else if(str_eq(command,"add") && spot_mb_guard) {
            f=fn("spot_add_ball");
            if(f) pm_log("TEST add-ball return=%u",((unsigned (*)(unsigned,unsigned,unsigned,unsigned,unsigned,unsigned))(unsigned long)f)(1,0,0,0xbb,0,0));
        }
    }
    if(pm_port_value("spot_test_controls",0)) {
        unsigned i,n=rw(0x49efa0),dev;
        pm_log("INV installed=%u native_total=%u serve_target=%u serve_pending=%u devices=%u",*(volatile unsigned char *)0x553d78,
          ((unsigned (*)(void))0x32b77c)(),*(volatile unsigned char *)0x603772,*(volatile unsigned char *)0x603773,n);
        for(i=1;i<n && i<16;i++) {
            dev=rw(0x56d7b0+40*i);
            if(dev) pm_log("INV device=%u obj=0x%x count17=%u count1a=%u",i,dev,*(volatile unsigned char *)(unsigned long)(dev+0x17),*(volatile unsigned char *)(unsigned long)(dev+0x1a));
        }
    }
    t=spot_timer(r); hud=rw(0x5a389c);
    pm_log("MB DIAG level=%u balls=%d paused=%u remaining=%u clock_enabled=%u guard=%d armed=%d",pm_munsters_spot_level(),pm_balls_in_play(),
        t ? *(volatile unsigned char *)(unsigned long)(t+0x1f) : 0,t ? rw(t+0x28) : 0,
        hud ? *(volatile unsigned char *)(unsigned long)(hud+0x1d0) : 0,spot_mb_guard,spot_mb_armed);
}
