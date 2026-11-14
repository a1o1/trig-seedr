import sys, re, capstone as C
img=open('sec3.bin','rb').read(); BASE=0x40000400
md=C.Cs(C.CS_ARCH_M68K, C.CS_MODE_BIG_ENDIAN|C.CS_MODE_M68K_040); md.skipdata=True
for s in sys.argv[1:]:
    a=int(s,16); calls=[]; consts=[]; n=0
    for i in md.disasm(img[a-BASE:a-BASE+0x600], a):
        n+=1
        if i.mnemonic in('jsr','lea.l') and '$40' in i.op_str:
            m=re.search(r'\$(40[0-9a-f]{6})',i.op_str); 
            if m and m.group(1) not in calls: calls.append(m.group(1))
        m=re.search(r'\$([0-9a-f]{2,3})\(a[0-5]\)',i.op_str)
        if m and m.group(1) not in consts: consts.append(m.group(1))
        if i.mnemonic=='rts': break
    print(s,'len=%x'%(i.address-a),'calls',' '.join(calls),'| offs',' '.join(consts))
