"""Function-level check of the Rytm MKII OS 1.74 step setters (Unicorn, ColdFire V4e).
Runs stock routines against a fake track object. Usage: emu_steps.py sec3.bin"""
import sys, struct
from unicorn import *
from unicorn.m68k_const import *
img = open(sys.argv[1], 'rb').read(); BASE = 0x40000400
REC, OBJ, VT, STUB, STACK, PAT, PVT, LOCKS = 0x50000000, 0x50010000, 0x50010100, 0x50020000, 0x50100000, 0x50011000, 0x50011100, 0x50040000
SET = dict(flags=0x400c2482, note_trig=0x400c2620, lock_trig=0x400c2698, note=0x400c0776, vel=0x400c07f0,
           length=0x400c086a, utime=0x400c08e4, cond=0x400c03d6, rlen=0x400c0a02, rrate=0x400c0978,
           rvel=0x400c0a8c, shift=0x400c1d7a, has_trig=0x400bff7e)
def mk(with_pattern):
    uc = Uc(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN)
    uc.ctl_set_cpu_model(UC_CPU_M68K_CFV4E)
    uc.mem_map(0x40000000, 0x400000); uc.mem_write(BASE, img)
    uc.mem_map(0x50000000, 0x200000)
    rts = b'\x4e\x75'
    uc.mem_write(STUB, b'\x20\x3c' + struct.pack('>I', REC) + rts)            # move.l #REC,d0; rts
    uc.mem_write(STUB + 0x10, rts)
    uc.mem_write(STUB + 0x20, b'\x20\x3c' + struct.pack('>I', LOCKS) + rts)
    uc.mem_write(STUB + 0x30, b'\x4e\x71' * 2)                                # return landing
    for off in range(0, 0x80, 4):
        uc.mem_write(VT + off, struct.pack('>I', STUB + 0x10)); uc.mem_write(PVT + off, struct.pack('>I', STUB + 0x10))
    uc.mem_write(VT + 0x28, struct.pack('>I', STUB)); uc.mem_write(PVT + 0x28, struct.pack('>I', STUB + 0x20))
    uc.mem_write(OBJ, struct.pack('>I', VT)); uc.mem_write(PAT, struct.pack('>I', PVT))
    uc.mem_write(OBJ + 0x2c, struct.pack('>I', PAT if with_pattern else 0))
    uc.mem_write(OBJ + 0x40, struct.pack('>i', 0)); uc.mem_write(OBJ + 0x44, struct.pack('>I', 0))
    def bad(uc, acc, addr, size, val, ud):
        print('  unmapped access %08x size %d pc=%08x' % (addr, size, uc.reg_read(UC_M68K_REG_PC))); return False
    uc.hook_add(UC_HOOK_MEM_UNMAPPED, bad)
    uc.mem_write(REC + 0x2c5, bytes([16]))                                     # track length 16
    uc.mem_write(REC + 0x2c3, bytes([0x03]))
    return uc
def call(uc, fn, *args):
    sp = STACK
    for a in reversed(args):
        sp -= 4; uc.mem_write(sp, struct.pack('>i', a))
    sp -= 4; uc.mem_write(sp, struct.pack('>I', STUB + 0x30))
    uc.reg_write(UC_M68K_REG_SR, 0x2700); uc.reg_write(UC_M68K_REG_A7, sp)
    uc.emu_start(fn, STUB + 0x30, count=2000000)
    return uc.reg_read(UC_M68K_REG_D0)
def step(uc, s):
    r = lambda o, n=1: uc.mem_read(REC + o, n)
    f = struct.unpack('>H', r(2 * s, 2))[0]
    b = lambda o: r(o + s)[0]
    sb = lambda o: struct.unpack('b', r(o + s))[0]
    return dict(flags='%04x' % f, note=b(0x80), vel=b(0xc0), len=b(0x100), utime=sb(0x140), rlen=b(0x180), rrate=b(0x1c0), rvel=sb(0x200), cond=sb(0x240), snd=b(0x280))
uc = mk(False)
print('empty step 4      ', step(uc, 4))
call(uc, SET['note_trig'], OBJ, 4, 1);  print('note trig          ', step(uc, 4))
call(uc, SET['note'], OBJ, 4, 60); call(uc, SET['vel'], OBJ, 4, 100); call(uc, SET['length'], OBJ, 4, 14)
call(uc, SET['utime'], OBJ, 4, 64 + 5); call(uc, SET['cond'], OBJ, 4, 10)
call(uc, SET['rlen'], OBJ, 4, 30); call(uc, SET['rrate'], OBJ, 4, 12); call(uc, SET['rvel'], OBJ, 4, -20)
print('fields set         ', step(uc, 4))
call(uc, SET['flags'], OBJ, 4, 0x08, 1); call(uc, SET['flags'], OBJ, 4, 0x02, 1); print('accent + retrig    ', step(uc, 4))
call(uc, SET['utime'], OBJ, 4, 64 + 40); print('utime +40 clamps   ', step(uc, 4)['utime'])
call(uc, SET['utime'], OBJ, 4, 64 - 40); print('utime -40 clamps   ', step(uc, 4)['utime'])
call(uc, SET['vel'], OBJ, 20, 77); print('step 20 past length', step(uc, 20)['vel'], '(0 = refused)')
print('has_trig 4, 5      ', call(uc, SET['has_trig'], OBJ, 4) & 0xff, call(uc, SET['has_trig'], OBJ, 5) & 0xff)
call(uc, SET['note_trig'], OBJ, 4, 0);  print('trig removed       ', step(uc, 4))
try:
    uc = mk(True)
    call(uc, SET['note_trig'], OBJ, 15, 1); call(uc, SET['vel'], OBJ, 15, 99); call(uc, SET['flags'], OBJ, 15, 0x18, 1)
    call(uc, SET['note_trig'], OBJ, 0, 1); call(uc, SET['vel'], OBJ, 0, 11)
    r = call(uc, SET['shift'], OBJ, 1, -1) & 0xff
    print('shift right ->', r, [ (s, step(uc, s)['flags'], step(uc, s)['vel']) for s in range(16) if step(uc, s)['flags'] != '0000'])
    r = call(uc, SET['shift'], OBJ, -1, -1) & 0xff
    print('shift left  ->', r, [ (s, step(uc, s)['flags'], step(uc, s)['vel']) for s in range(16) if step(uc, s)['flags'] != '0000'])
except UcError as e:
    print('shift run failed:', e, 'pc=%08x' % uc.reg_read(UC_M68K_REG_PC))
