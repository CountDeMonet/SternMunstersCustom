from pathlib import Path
import shutil, struct, hashlib, json
root=Path(r'E:\Stern_Fun\Munsters\Munsters Pro\analysis\stock_spot')
installed=Path(r'D:\Program Files\Pinball Asset Decryptor\tools\spike2_emu\modes\sdk')
sdk=root/'sdk'; sdk.mkdir(exist_ok=True)
for name in ['pad_mode_runtime.c','pad_mode.h','pad_stock.h','pad_mode_assets.h','build_mode.sh','mode_file.c']:
    if not (sdk/name).exists(): shutil.copy2(installed/name,sdk/name)
(sdk/'ports').mkdir(exist_ok=True)
backup=root/'sdk_original'; backup.mkdir(exist_ok=True)
for name in ['pad_mode_runtime.c','pad_stock.h','ports/munsters_pro-1.28.port']:
    dest=backup/name; dest.parent.mkdir(parents=True,exist_ok=True)
    if not dest.exists(): shutil.copy2(installed/name,dest)
s=(backup/'pad_mode_runtime.c').read_text(encoding='utf-8')
def replace(a,b):
    global s
    assert s.count(a)==1,(a[:90],s.count(a))
    s=s.replace(a,b)
replace('#include "pad_stock.h"', '#include "pad_munsters.h"\n#include "pad_stock.h"')
replace('a = x->obj + (unsigned)pm_port_value("stock_field", 0x18) + 8u * p;\n    return (uint64_t)', 'a = x->obj + (unsigned)pm_port_value("stock_field", 0x18) + (pm_port_value("stock_shot_bits", 64) == 32 ? 4u : 8u) * p;\n    if (pm_port_value("stock_shot_bits", 64) == 32) return *(volatile const unsigned *)(unsigned long)a;\n    return (uint64_t)')
replace('uint64_t shot = (uint64_t)r2 | (uint64_t)r3 << 32;','int narrow = pm_port_value("stock_shot_bits", 64) == 32;\n    uint64_t shot = narrow ? r1 : (uint64_t)r2 | (uint64_t)r3 << 32;')
replace('stock161_first(x, &shot, s0)', 'stock161_first(x, &shot, narrow ? 1u : s0)')
replace('return x->orig(r0, r1, (unsigned)shot, (unsigned)(shot >> 32), s0, s1, s2, s3);','if (narrow) return x->orig(r0, (unsigned)shot, r2, r3, s0, s1, s2, s3);\n    return x->orig(r0, r1, (unsigned)shot, (unsigned)(shot >> 32), s0, s1, s2, s3);')
replace('ret = x->orig(x->obj, 0, (unsigned)shot, (unsigned)(shot >> 32), factor, 0, 0, 0);','if (pm_port_value("stock_shot_bits", 64) == 32)\n        ret = x->orig(x->obj, (unsigned)shot, 0, 0, 0, 0, 0, 0);\n    else ret = x->orig(x->obj, 0, (unsigned)shot, (unsigned)(shot >> 32), factor, 0, 0, 0);')
replace('a = x->obj + (unsigned)pm_port_value("stock_field", 0x18) + 8u * player;\n    *(volatile unsigned *)(unsigned long)a = (unsigned)mask;\n    *(volatile unsigned *)(unsigned long)(a + 4) = (unsigned)(mask >> 32);','a = x->obj + (unsigned)pm_port_value("stock_field", 0x18) + (pm_port_value("stock_shot_bits", 64) == 32 ? 4u : 8u) * player;\n    if (pm_port_value("stock_shot_bits", 64) == 32 && (mask >> 32)) return 0;\n    *(volatile unsigned *)(unsigned long)a = (unsigned)mask;\n    if (pm_port_value("stock_shot_bits", 64) != 32)\n        *(volatile unsigned *)(unsigned long)(a + 4) = (unsigned)(mask >> 32);')
replace('return ((stock_vfn1)(unsigned long)stock_vtab(x)[slot])(x->obj);','if (pm_port_value("stock_shot_bits", 64) == 32)\n        return ((unsigned (*)(unsigned))(unsigned long)stock_vtab(x)[slot])(x->obj);\n    return ((stock_vfn1)(unsigned long)stock_vtab(x)[slot])(x->obj);')
(sdk/'pad_mode_runtime.c').write_text(s+'\n#include \"munsters_bridge.inc\"\n',encoding='utf-8',newline='\n')
print('Installed runtime adapter: opt-in 32-bit stock handler and mask layout; original saved.')
# Emit checked port additions directly from the stock ELF.
b=(root/'game.stock').read_bytes(); phoff=struct.unpack_from('<I',b,28)[0]; ents,n=struct.unpack_from('<HH',b,42)
segs=[]
for i in range(n):
    typ,off,va,pa,sz=struct.unpack_from('<IIIII',b,phoff+i*ents)
    if typ==1: segs.append((off,va,sz))
def off(va):
    return next(o+va-v for o,v,z in segs if v<=va<v+z)
sites={'stock_rule_get':0xbdff4,'spot_level_get':0x8b450,'spot_rule_get':0xbed08,'spot_award_grow':0x2ab9c,'spot_award_collect':0x2af18,'spot_super_available':0xdf4e4,'spot_super_light':0xdf578,'spot_process_cancel':0x39e1e0,'spot_process_start':0x39e270,'spot_pre_super_seq':0xd6620,'spot_post_super_seq':0xd65a0,'spot_add_ball':0x32d138,'spot_add_eligible':0x1bff8,'spot_clock_bind_call':0xd9840,'spot_timer_pause':0x697f8,'spot_grandpa_eject_save':0x9ae8c,'spot_adjust_get':0x64260,'spot_save_start':0xfb280,'spot_mystery_eligible':0xb42ac,'spot_mystery_collect':0xb4fd8,'spot_mystery_lottery':0xb4edc,'spot_toy_get':0x414bc}
lines=['','# Local Spot baseline extension: exact Munsters Pro 1.28 ELF only.','rule 17 0x004c87c0 Spot Mode','data stock_mode_manager 0x005a2248','value stock_shot_bits 32','value stock_slot_shot 25','value stock_slot_start 33','value stock_slot_stop 36','value stock_slot_active 37','value stock_slot_lit 11','value stock_field 0x20','value stock_award_at 0x34','value spot_phase_at 0x80','value spot_hits_at 0x8c','value spot_post_super_at 0x84','value spot_timer_extended_at 0x84','data spot_lamps_dirty 0x005abf20','data spot_screen_info 0x005527a8','data spot_toy_vtable 0x004c8dd0']
for name,va in sites.items():
    x,y=struct.unpack_from('<II',b,off(va)); lines.append(f'site {name:<23} 0x{va:08x} 0x{x:08x} 0x{y:08x}')
original=(backup/'ports/munsters_pro-1.28.port').read_text(encoding='utf-8')
keys={(ln.split()[0],ln.split()[1]) for ln in lines if ln and not ln.startswith('#')}
original='\n'.join(ln for ln in original.splitlines() if len(ln.split())<2 or tuple(ln.split()[:2]) not in keys)
p=original+'\n'+'\n'.join(lines)+'\n'
(sdk/'ports/munsters_pro-1.28.port').write_text(p,encoding='utf-8',newline='\n')
(root/'munsters_pro-1.28.port').write_text(p,encoding='utf-8',newline='\n')
(root/'stock_identity.json').write_text(json.dumps({'sha256':hashlib.sha256(b).hexdigest(),'build_id':'b8caff2cf23fc8cce1d5a8ca8aa232b29f204758','source':'munsters_pro-1_28_0.Release.8G.sdcard.raw, games partition, /munsters_pro/game','sites':sites},indent=2))
print('Generated checked port and binary provenance.')


