"""Run a bounded demo. Opening an Uno serial port normally resets it. Ctrl-C requests a stop."""
import argparse,json,pathlib,time
import serial
p=argparse.ArgumentParser();p.add_argument('--port',required=True);p.add_argument('--command',choices=['D','W','P','X'],default='P');p.add_argument('--timeout',type=float,default=90);p.add_argument('--log');a=p.parse_args()
events=[]
with serial.Serial(a.port,115200,timeout=.25,write_timeout=2,exclusive=True) as port:
 time.sleep(2);banner=port.readline().decode().strip()
 if banner!='OMNIGLOW_PLAYGROUND INPUT; D=colors W=white P=play X=stop':raise RuntimeError('Unexpected firmware: '+repr(banner))
 print(banner,flush=True);port.write(a.command.encode());port.flush();start=time.monotonic()
 try:
  if a.command!='X':
   while time.monotonic()-start<a.timeout:
    line=port.readline().decode().strip()
    if line:events.append({'seconds':round(time.monotonic()-start,3),'reply':line});print(line,flush=True)
    if line in ('DEMO done; D2 INPUT','DEMO stopped; D2 INPUT'):break
   else:
    port.write(b'X');raise TimeoutError('Completion not received; stop requested')
 except KeyboardInterrupt:
  port.write(b'X');port.flush()
  deadline=time.monotonic()+3
  while time.monotonic()<deadline:
   line=port.readline().decode().strip()
   if line:print(line,flush=True)
   if 'D2 INPUT' in line:break
if a.log:pathlib.Path(a.log).write_text(json.dumps({'banner':banner,'command':a.command,'events':events},indent=2))
