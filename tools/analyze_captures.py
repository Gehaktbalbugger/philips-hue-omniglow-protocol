import base64,collections,json,pathlib,sys
p=pathlib.Path(sys.argv[1]);d=json.loads(p.read_text())
def decode(f):
 b=[(v>>2)&1 for v in base64.b64decode(f['data_base64'])];e=[i for i in range(1,len(b)) if b[i]!=b[i-1]]
 return [(a,'0' if z-a<=2 else '1' if 4<=z-a<=6 else '?') for a,z in zip(e,e[1:]) if b[a]]
first=[f for f in d['frames'] if f['offset_us']==0 and not f.get('timeout')]
seq=''.join(v for _,v in decode(first[0]));word=seq[79]+seq[:79]
votes=collections.defaultdict(collections.Counter);positions=[]
for f in d['frames']:
 if f.get('timeout'):continue
 sy=decode(f)
 if len(sy)<20:continue
 s=''.join(v for _,v in sy)
 # Search cyclic motif against first32symbols, allowing footer after the prefix.
 phases=[k for k in range(80) if all(c==word[(k+j)%80] for j,c in enumerate(s[:32]))]
 if len(phases)!=1:positions.append({'offset_us':f['offset_us'],'repeat':f['repeat'],'ambiguous_phases':phases});continue
 rough=(f['offset_us']+sy[0][0]*.1875+.75)/1.25
 start=phases[0]+80*round((rough-phases[0])/80)
 for j,(_,c) in enumerate(sy):votes[start+j][c]+=1
 positions.append({'offset_us':f['offset_us'],'repeat':f['repeat'],'first_assigned_symbol':start,'decoded_symbols':len(sy)})
conflicts={i:dict(v) for i,v in votes.items() if len(v)>1}
last=max(votes) if votes else None
mismatches=[i for i,v in sorted(votes.items()) if v.most_common(1)[0][0]!=word[i%80]]
footer=''.join(votes[i].most_common(1)[0][0] if i in votes else '?' for i in range(6400,last+1)) if last is not None and last>=6400 else ''
result={'source':p.name,'classification':'short high=0,long high=1 is a convention, not verified device encoding','body_word_80':word,'body_as_five_16_bit_groups':[hex(int(word[i:i+16],2)) for i in range(0,80,16)],'alignment_note':'Assumes first complete rising pulse in zero-offset snapshot is symbol1. Symbol0 inferred from body periodicity; not completely recorded. Absolute indices then chosen by nominal1.25us timing plus unique80symbol cyclic phase. Separate transmissions assumed constant, checked via overlapping symbols.','assigned_symbol_range':[min(votes),last],'overlap_conflicts':conflicts,'first_mismatches_to_body':mismatches[:20],'candidate_footer_from_symbol6400':footer,'candidate_footer_hex':hex(int(footer,2)) if footer and '?' not in footer else None,'footer_boundary_note':'6400is a candidate boundary suggested by80 repeated80symbol units and waveform change around8000us. Not an identified protocol field boundary.','positions':positions}
out=p.with_name(p.stem+'-analyse.json');out.write_text(json.dumps(result,indent=2));print(json.dumps({k:v for k,v in result.items() if k!='positions'},indent=2))
