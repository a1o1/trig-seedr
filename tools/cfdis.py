import sys, capstone as C
img = open('sec3.bin','rb').read(); BASE=0x40000400
md = C.Cs(C.CS_ARCH_M68K, C.CS_MODE_BIG_ENDIAN | C.CS_MODE_M68K_040)
md.skipdata = True
a=int(sys.argv[1],16); n=int(sys.argv[2],16)
for i in md.disasm(img[a-BASE:a-BASE+n], a):
    print("%08x  %-8s %s"%(i.address,i.mnemonic,i.op_str))
