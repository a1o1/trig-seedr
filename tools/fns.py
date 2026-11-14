import sys, struct
img=open('sec3.bin','rb').read(); BASE=0x40000400
a=int(sys.argv[1],16); b=int(sys.argv[2],16)
# function starts: after rts (4e75) at even addr
i=a-BASE; out=[]
while i<b-BASE:
    if img[i:i+2]==b'\x4e\x75': out.append(BASE+i+2)
    i+=2
print(' '.join('%x'%x for x in out))
