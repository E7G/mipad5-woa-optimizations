"""Compile and run production-code mocks; default inputs are SYNTHETIC, not device firmware."""
from pathlib import Path
import argparse,os,struct,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('--cc',default=os.environ.get('CC','gcc'));p.add_argument('--firmware',nargs=2,type=Path);a=p.parse_args()
root=Path(__file__).resolve().parents[1];driver=root/'drivers/nt36xxx';tests=driver/'tests'
with tempfile.TemporaryDirectory(prefix='nabu-mocks-') as d:
    d=Path(d)
    images=a.firmware
    if not images:
        # Minimal valid 2-part layout; no executable controller firmware or vendor data.
        import re
        h=(tests/'mock.h').read_text(encoding='utf-8')
        size=int(re.search(r'#define FWBUFFER_SIZE (0x[0-9a-fA-F]+|\d+)',h).group(1),0)
        images=[]
        for i in range(2):
            b=bytearray(size);struct.pack_into('<6I',b,0,0x30,0x1000,31,0x100,0x2000,31)
            b[0x30:0x50]=bytes([0x31+i])*32;b[0x100:0x120]=bytes([0x42+i])*32
            f=d/('synthetic-'+str(i)+'.data');f.write_bytes(b);images.append(f)
        print('SYNTHETIC mock inputs: NOT device firmware',flush=True)
    for name in ('power','firmware'):
        exe=d/(name+('.exe' if os.name=='nt' else ''))
        subprocess.run([a.cc,'-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-O1','-I'+str(tests),'-I'+str(driver/'include'),str(tests/(name+'-test.c')),'-o',str(exe)],check=True,timeout=45)
        subprocess.run([str(exe)]+([] if name=='power' else [str(f.resolve()) for f in images]),check=True,timeout=45)
