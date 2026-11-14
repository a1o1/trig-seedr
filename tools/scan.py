import sys, re, capstone as C
img=open('sec3.bin','rb').read(); BASE=0x40000400
md=C.Cs(C.CS_ARCH_M68K, C.CS_MODE_BIG_ENDIAN|C.CS_MODE_M68K_040); md.skipdata=True
a=int(sys.argv[1],16); b=int(sys.argv[2],16)
fn=None; out={}
for i in md.disasm(img[a-BASE:b-BASE], a):
    if i.mnemonic.startswith('link'): fn=i.address
    if fn and re.match(r'(move|clr|or|and|bset|bclr)',i.mnemonic) and re.search(r'\$[0-9a-f]+\(a[0-7]\)$',i.op_str) and 'a7' not in i.op_str and 'a6' not in i.op_str:
        out.setdefault(fn,[]).append("%08x %s %s"%(i.address,i.mnemonic,i.op_str))
    if fn and i.mnemonic=='jsr' and '$401e' in i.op_str: pass
for f,v in out.items(): print(hex(f), ' | '.join(v[:4]))
