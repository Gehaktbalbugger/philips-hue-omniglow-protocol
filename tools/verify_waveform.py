"""Narrow instruction-level timing checker for sendPacket, not a general AVR emulator."""
import pathlib,re,json,collections
root=pathlib.Path(__file__).resolve().parents[1]
s=(root/'build/disassembly.txt').read_text();ins={}
for l in s.splitlines():
 m=re.match(r'\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} ){1,4}\s*([a-z]+)\s*(.*)',l)
 if m:ins[int(m[1],16)]=(m[2],m[3])
pcs=sorted(ins);nxt=dict(zip(pcs,pcs[1:]));results={}
for color in ('red','blue','zeros','ones','random'):
 fn='sendPacket'
 import random
 red=json.load((root/'evidence/red-reconstruction.json').open());blue=json.load((root/'evidence/blue-reconstruction.json').open())
 if color in ('red','blue'):
  source=red if color=='red' else blue; expected=source['body_word_80']*80+source['candidate_footer_from_symbol6400']; payload=bytes(int(expected[i:i+8],2) for i in range(0,len(expected),8))
 else:
  rng=random.Random(42);payload=bytes([0]*806) if color=='zeros' else bytes([255]*806) if color=='ones' else bytes(rng.randrange(256) for _ in range(806));expected=''.join(format(v,'08b') for v in payload)
 payload+=b'\0'; reads=[]
 pc=int(re.search(r'^([0-9a-f]+) <_ZL\d+'+fn+r'v>:',s,re.M)[1],16);r=[0]*32;tick=0;zero=False;events=[]
 for steps in range(300000):
  op,arg=ins[pc];clean=arg.split(';')[0].strip();args=[x.strip() for x in clean.split(',')];target=nxt[pc];cost=1
  reg=lambda x:int(x[1:]);val=lambda x:int(x,0)
  if op=='in':r[reg(args[0])]=128 if val(args[1])==63 else 0
  elif op=='ldi':r[reg(args[0])]=val(args[1])
  elif op=='mov':r[reg(args[0])]=r[reg(args[1])]
  elif op=='andi':r[reg(args[0])]&=val(args[1]);zero=r[reg(args[0])]==0
  elif op=='ori':r[reg(args[0])]|=val(args[1]);zero=r[reg(args[0])]==0
  elif op=='ld':
   addr=r[26]+256*r[27];base=reads[0] if reads else addr;index=addr-base;assert 0<=index<807;reads.append(addr);r[reg(args[0])]=payload[index];addr+=1;r[26]=addr&255;r[27]=addr>>8;cost=2
  elif op=='sbiw':
   k=reg(args[0]);v=(r[k]+256*r[k+1]-val(args[1]))&65535;r[k]=v&255;r[k+1]=v>>8;zero=v==0;cost=2
  elif op=='sbrs':
   if r[reg(args[0])] & (1<<val(args[1])):target=nxt[target];cost=2
  elif op=='out':
   if val(args[0])==11:
    level=(r[reg(args[1])]>>2)&1
    if not events or level!=events[-1][1]:events.append((tick,level))
   elif val(args[0])==63:break
   else:raise AssertionError(('unexpected output',args))
  elif op=='dec':r[reg(args[0])]=(r[reg(args[0])]-1)&255;zero=r[reg(args[0])]==0
  elif op in ('breq','brne'):
   taken=zero if op=='breq' else not zero
   if taken:target=int(re.search(r'; 0x([0-9a-f]+)',arg)[1],16);cost=2
  elif op in ('rjmp','jmp'):target=int(re.search(r'; 0x([0-9a-f]+)',arg)[1],16);cost=2 if op=='rjmp' else 3
  elif op in ('nop','cli'):pass
  else:raise AssertionError((hex(pc),op,arg))
  tick+=cost;pc=target
 else:raise AssertionError('simulation did not terminate')
 assert len(events)==12896,len(events)
 assert all(events[i][1]==(1 if i%2==0 else 0) for i in range(len(events)))
 periods=collections.Counter(events[i+2][0]-events[i][0] for i in range(0,len(events)-2,2))
 widths=[events[i+1][0]-events[i][0] for i in range(0,len(events),2)]
 assert periods=={20:6447},periods
 assert set(widths)<={4,15},set(widths)
 decoded=''.join('1' if n==15 else '0' for n in widths)
 assert decoded==expected
 assert len(reads)==807
 results[color]={'symbols':len(widths),'period_cycles':dict(periods),'high_cycles':dict(collections.Counter(widths)),'compiled_output_matches_reconstruction':True,'last_output_level':events[-1][1]}
(root/'build/waveform-verification.json').write_text(json.dumps(results,indent=2));print(json.dumps(results,indent=2))
