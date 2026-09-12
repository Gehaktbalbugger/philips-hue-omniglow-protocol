import serial,time,json,datetime,pathlib,argparse
p=argparse.ArgumentParser();p.add_argument("--port",required=True);p.add_argument("--log",default="all-shows-result.json");p.add_argument("--stop-file",default="stop-hue-demo");a=p.parse_args()
out=pathlib.Path(a.log);stop=pathlib.Path(a.stop_file);stop.unlink(missing_ok=True)
r={'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'order':['D','W','P','F'],'shows':[],'status':'starting'}
def save():out.write_text(json.dumps(r,indent=2))
try:
 with serial.Serial(a.port,115200,timeout=.25,write_timeout=2,exclusive=True) as port:
  time.sleep(2);banner=port.readline().decode().strip();assert banner=='OMNIGLOW_PLAYGROUND INPUT; D=colors W=white P=play F=fireworks X=stop',repr(banner)
  r['banner']=banner;save()
  for command in r['order']:
   if stop.exists():stop.unlink();r['status']='playlist stopped';save();break
   show={'command':command,'events':[]};r['shows'].append(show);r['status']='playing '+command;port.write(command.encode());port.flush();start=time.monotonic();save();print('SHOW',command,flush=True)
   cancelled=False
   while time.monotonic()-start<90:
    if stop.exists():port.write(b'X');stop.unlink();cancelled=True;print('STOP requested',flush=True)
    line=port.readline().decode().strip()
    if line:show['events'].append({'elapsed_seconds':round(time.monotonic()-start,3),'reply':line});save();print(line,flush=True)
    if line in ('DEMO done; D2 INPUT','DEMO stopped; D2 INPUT'):
     show['status']=line;show['elapsed_seconds']=round(time.monotonic()-start,3);save();break
   else:
    port.write(b'X');r['status']='completion timeout; stop requested, remaining shows cancelled';save();raise RuntimeError(r['status'])
   if cancelled or show['status']=='DEMO stopped; D2 INPUT':r['status']='playlist stopped';save();break
  else:r['status']='all four shows completed; D2 INPUT';save();print('PLAYLIST COMPLETE',flush=True)
except KeyboardInterrupt:
 # Closing/reopening resets a standard Uno to its input-only boot state.
 with serial.Serial(a.port,115200,timeout=.25,write_timeout=2,exclusive=True) as halt:
  time.sleep(2);halt.write(b"X");halt.flush()
 r["status"]="playlist interrupted; reset and stop requested";save()
