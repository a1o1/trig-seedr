import sys, struct
img=open('sec3.bin','rb').read(); BASE=0x40000400
def xrefs(t):
    out=[]
    for i in range(0,len(img)-6,2):
        w=struct.unpack_from('>H',img,i)[0]
        a=BASE+i
        if w in (0x4eb9,0x4ef9) and struct.unpack_from('>I',img,i+2)[0]==t: out.append((a,'abs'))
        elif w in (0x4eba,0x4efa) and a+2+struct.unpack_from('>h',img,i+2)[0]==t: out.append((a,'pc'))
        elif w==0x61ff and a+2+struct.unpack_from('>i',img,i+2)[0]==t: out.append((a,'bsr'))
        elif (w&0xf1ff)==0x41fa and a+2+struct.unpack_from('>h',img,i+2)[0]==t: out.append((a,'lea'))
        elif (w&0xf1ff)==0x203c and struct.unpack_from('>I',img,i+2)[0]==t: out.append((a,'imm'))
        elif (w&0xf1ff)==0x41f9 and struct.unpack_from('>I',img,i+2)[0]==t: out.append((a,'leaabs'))
    return out
if __name__=='__main__':
    for t in sys.argv[1:]:
        print(t, ' '.join('%08x/%s'%x for x in xrefs(int(t,16))))
