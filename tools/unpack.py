import sys; sys.path.insert(0, sys.argv[1])
from elekloader import syx
s = syx.Syx.load(sys.argv[2])
print(s.family, s.version, s.device_id, [(t[0], hex(t[1]), t[2], hex(t[3])) for t in s.table])
for sid,off,clen,dest in s.table:
    try:
        d = s.section(sid); open('sec%d.bin'%sid,'wb').write(d); print(sid, hex(dest), len(d))
    except Exception as e: print(sid, 'err', e)
