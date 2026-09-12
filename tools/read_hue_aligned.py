import base64,datetime,json,pathlib,struct,time,argparse
import serial
from read_hue_capture import read_exact,analyze
p=argparse.ArgumentParser();p.add_argument('--port',required=True);p.add_argument('--output',required=True);p.add_argument('--step',type=int,default=150);p.add_argument('--end',type=int,default=8400);p.add_argument('--repeats',type=int,default=3);a=p.parse_args()
frames=[]; dest=pathlib.Path(a.output)
def save():
 dest.write_text(json.dumps({'timestamp_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'sample_period_ns':187.5,'window_us':192,'acquisition_mode':'Discontinuous gap-triggered fine snapshots. Offsets relative to first detected rising-edge flag after >=1ms idle; trigger latency and jitter not calibrated. Separate packets may differ. Not a verified stitched full packet.','frames':frames},indent=2))
with serial.Serial(a.port,115200,timeout=.5,write_timeout=2,exclusive=True) as port:
 time.sleep(2);port.reset_input_buffer();port.write(b'?');print(port.readline().decode().strip(),flush=True)
 for offset in range(0,a.end+1,a.step):
  for repeat in range(a.repeats):
   port.write(b'A'+struct.pack('<H',offset*4))
   header=port.readline().strip()
   if header==b'TIMEOUT':
    frames.append({'offset_us':offset,'repeat':repeat,'timeout':True});save();continue
   if header!=b'CAPTURE_V1 1024 3 16000000 2':raise RuntimeError(repr(header))
   raw=read_exact(port,1024)
   if read_exact(port,7)!=b'\r\nEND\r\n':raise RuntimeError('trailer')
   frames.append({'offset_us':offset,'repeat':repeat,'data_base64':base64.b64encode(raw).decode(),**analyze(raw)})
   save()
  print('offset',offset,'us:',[f.get('rising_edges','timeout') for f in frames[-a.repeats:]],flush=True)
print('saved',len(frames),'snapshots',dest,flush=True)
