"""Compile exact production CM host paths with deterministic platform/native seams.

No OpenXR or graphics runtime is simulated: poses are fixed on-screen; all input,
roster, suppression, native dispatch and gun-knock code under test is extracted
without rewriting it. The extraction manifest records the exact source and blocks.
"""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys
R=Path(__file__).resolve().parents[2]
O=Path(sys.argv[1]) if len(sys.argv)>1 else R/'build-win/vrtest/player-assignment'
O.mkdir(parents=True,exist_ok=True)
h=(R/'core/rend/vr/xr_host.cpp').read_text(encoding='utf-8-sig')
v=(R/'core/rend/vr_reproject.cpp').read_text(encoding='utf-8-sig')
m=(R/'core/rend/vr/xr_menu.cpp').read_text(encoding='utf-8-sig')
manifest={}
def block(t,s):
 a=t.index(s);b=t.index('{',a);d=1;e=b+1
 while d and e<len(t):d+=(t[e]=='{')-(t[e]=='}');e+=1
 if d:raise ValueError('unbalanced '+s)
 result=t[a:e];manifest[s]=hashlib.sha256(result.encode()).hexdigest();return result

def body(t,s):
 code=block(t,s);return code[code.index('{')+1:-1]

# Optional fault injection changes only copied/extracted test inputs. Production
# files are never edited. Each mutation must produce failing runtime assertions.
mutation=sys.argv[3] if len(sys.argv)>3 and sys.argv[2]=='--mutation' else ''
if mutation:
 if mutation in ('stale-focus','stale-sync'):
  sig='if (!focused)' if mutation=='stale-focus' else 'if (!XR_SUCCEEDED(XR(xrSyncActions)'
  old=block(h[h.index('void updateLightgun('):],sig)
  target='if(config::VrXrGun)for(Gun& g:guns)releaseGunInput(g);'
  if target not in old:raise ValueError('missing failure suppression mutation anchor')
  h=h.replace(old,old.replace(target,'(void)0;'),1)
 elif mutation=='stale-menu':
  target='g.heldAt=g.knockEnds=0;g.gunKnown=g.freshPull=false;'
  if target not in h:raise ValueError('missing menu suppression anchor')
  h=h.replace(target,'(void)0;',1)
 elif mutation=='paused-roster':
  target='player2PromptKnown()&&ready&&allowed&&emu.running()'
  if target not in h:raise ValueError('missing trusted roster anchor')
  h=h.replace(target,'player2PromptKnown()',1)
 elif mutation=='one-gun-restart':
  target='guns[1].triggerWas = guns[1].triggerWait = true;'
  if target not in h:raise ValueError('missing restart trigger guard anchor')
  h=h.replace(target,'(void)0;',1)
 elif mutation in ('busy-idle-stall','busy-visible-stall'):
  target='if (menuOpen && !cm::options::busy())'
  current=block(h,'bool frame(')
  at=current.index(target,current.index('auto idle = []') if mutation=='busy-idle-stall' else current.index('const bool preparing = hands::preparing();'))
  changed=current[:at]+current[at:].replace(target,'if (menuOpen)',1)
  h=h.replace(current,changed,1)
 elif mutation=='half-rearm':
  policy=(R/'core/rend/vr/player_ownership.h').read_text(encoding='utf-8-sig')
  target='if(in.pull[h]<=.35f)triggerReady_[h]=true;'
  if target not in policy:raise ValueError('missing strict rearm anchor')
  (O/'player_ownership.h').write_text(policy.replace(target,'if(!held)triggerReady_[h]=true;',1),encoding='utf8')
 elif mutation!='xr-only-release':raise ValueError('unknown mutation '+mutation)
(O/'gun_under_test.inc').write_text(block(h,'struct Gun')+';',encoding='utf8')
parts=[]
udp=(R/'core/input/udp_lightgun.cpp').read_text(encoding='utf-8-sig')
if mutation=='xr-only-release':udp=udp.replace('return nativeReleaseGate.observed(player,ticket);','return true;',1)
for sig in ('u64 lightgunRequireRelease(','bool lightgunReleaseObserved(','void lightgunAcknowledgeRelease('):parts.append(block(udp,sig))
for sig in ('bool gameStartLoaded()','bool gameStartReady()','bool playerInGame(int player)'):parts.append(block(v,sig))
for sig in ('int twoPlayerP1Hand()','int p1Hand()','Gun& primaryGun()','void releaseGunInput(Gun& g)','bool triggerHeld(','void gunKnocks(','int startButtonOwner(','void resetStartGestures()','bool serviceMenuAvailable()','bool serviceMenuActive()','bool serviceMenuExitAvailable()','bool openServiceMenu()','void notifyServiceGameBoot()','void sampleStartButtons('):parts.append(block(h,sig))
update=block(h,'void updateLightgun(')
reset=body(update,'if (config::VrXrGun && newGame.exchange(false))')
parts.append('void resetForNewGame(){if(config::VrXrGun&&newGame.exchange(false)){'+reset+'}}')
# Execute the exact failure bodies: their returns remain real returns.
parts.append('void focusFailure(){'+body(update,'if (!focused)')+'}')
parts.append('void syncFailure(){'+body(update,'if (!XR_SUCCEEDED(XR(xrSyncActions)')+'}')
# All input/output logic after pose calculation is production code, including
# updateGun(!live), triggerWait/reload guard and the final lightgunSet.
gun=block(h,'void updateGun(')
a=gun.index('\n\tif (!live)')
tail=gun[a:-1];manifest['updateGun:input tail']=hashlib.sha256(tail.encode()).hexdigest()
parts.append('void updateGun(Gun& g,XrTime time,const XrSpaceLocation* locations,const bool* tracked,const float* pulls,u32 extra,bool slide,bool live=true){const float pull=pulls[g.hand];const bool trigger=triggerHeld(g,pull);const bool onScreen=tracked[g.hand];glm::vec2 screen(.5f);'+tail+'}')
a=update.index('\n\tfloat pulls[2];');b=update.index("\n\t// The game's first Start",a)
routing=update[a:b]
arming=update[b:update.index('\n\tif(slide.held)',b)]
# Final dispatch stays verbatim; camera/menu zoom between the two regions is
# covered independently by game-zoom-integration-test.py.
a=update.index('    for(Gun& g:guns){',update.index('frameWorldZoom=',b))
dispatch=update[a:-1]
manifest['canonical routing']=hashlib.sha256(routing.encode()).hexdigest();manifest['final dispatch']=hashlib.sha256(dispatch.encode()).hexdigest()
a=update.index('sampleStartButtons(time,');b=update.index(';',a)+1
sample_call=update[a:b];manifest['gameplay sample call']=hashlib.sha256(sample_call.encode()).hexdigest()
parts.append('void frameCode(XrTime time,u32 dpad=0){if(!starts[0]&&!starts[1])startSwallow=false;bool p1Start,p2Start;const bool preparing=hands::preparing(),menuEdge=false,leftStart=starts[Left],rightStart=starts[Right]&&!startSwallow;'+sample_call+'XrSpaceLocation locations[2];bool tracked[2];locateHands(time,locations,tracked);const bool skip=skips[0]||skips[1];'+routing+arming+dispatch+'}')
# Native apply lifecycle uses the exact host pause function, idle lambda and
# visible-frame drain branch. Native backend Snapshot is the only controlled seam.
parts.append(block(h,'void applyPause()'))
frame=block(h,'bool frame(')
a=frame.index('auto idle = []');b=frame.index('};',a)+2
idle=frame[a:b];manifest['frame idle lambda']=hashlib.sha256(idle.encode()).hexdigest()
parts.append('void idleFrame(){'+idle+'idle();}')
a=frame.index('const bool preparing = hands::preparing();');b=frame.index('\n\t// What the panel says:',a)
drain=frame[a:b];manifest['frame drain branch']=hashlib.sha256(drain.encode()).hexdigest()
parts.append('void drainFrame(){'+drain+'}')
# Retain real menu layout value/change and explicit Service row dispatch.
parts.append(re.search(r'const char \*const HandsNames\[2\] \{[^;]+;',m).group())
cases=re.findall(r'case TwoPlayers:(.*?)case Laser:',m,re.S)
if len(cases)!=2:raise ValueError('TwoPlayers menu value/change missing')
parts.append('std::string menuValue(){switch(0){case 0:'+cases[0]+'}return {};}')
parts.append('void toggleMenu(){switch(0){case 0:'+cases[1]+'}}')
choose=block(m,'void choose(');service=re.search(r'case ServiceMenu:(.*?)(?=case ApplyGame:)',choose,re.S).group(1)
parts.append('void chooseServiceRow(){switch(0){case 0:'+service+'}}')
j=(R/'core/hw/maple/maple_jvs.cpp').read_text(encoding='utf-8-sig')
keys=(R/'core/hw/maple/maple_devs.h').read_text(encoding='utf-8-sig')
inputs=(R/'core/hw/naomi/naomi_roms_input.h').read_text(encoding='utf-8-sig')
cart=(R/'core/hw/naomi/naomi_cart.h').read_text(encoding='utf-8-sig')
parts += [re.search(r'enum NAOMI_KEYS\s*\{.*?\};',keys,re.S).group(),re.search(r'struct ButtonDescriptor\s*\{.*?\};',cart,re.S).group(),'struct InputDescriptors {ButtonDescriptor buttons[18];};',re.search(r'#define NAO_BASE_BTN_DESC.*?(?=#define)',inputs,re.S).group(),re.search(r'static InputDescriptors ninjaslt_inputs = \{.*?\};',inputs,re.S).group(),'struct MapleInputState {struct {int x=0,y=0;}absPos;};MapleInputState mapleInputState[4];','#define verify(x) assert(x)']
ack_at=j.index('lightgunAcknowledgeRelease(player,')
a=j.rindex('u32 inputs[4];',0,ack_at);b=j.index('JVS_OUT(',ack_at)
ack=j[a:b];manifest['native digital command consumed release']=hashlib.sha256(ack.encode()).hexdigest()
parts.append('struct NativeJvsMapper {std::array<u32,32>cur_mapping{},p1_mapping{},p2_mapping{};unsigned first_player=0;bool lightgun_as_analog=false;InputDescriptors* NaomiGameInputs=&ninjaslt_inputs;'+block(j,'void init_mappings()')+block(j,'virtual void read_digital_in(const u32 *buttons, u32 *v)')+'void nativeReleasePoll(const u32* buttons){'+ack+'}};')
parts.append('u32 serviceEnterMask(ServiceStyle serviceStyle){'+re.search(r'const u32 enter=.*?;',j,re.S).group()+'return enter;}')
(O/'under_test.inc').write_text('\n'.join(parts),encoding='utf8')
files=['core/rend/vr/xr_host.cpp','core/rend/vr_reproject.cpp','core/rend/vr/player_ownership.h','core/rend/vr/service_controls.h','core/rend/vr/service_boot.h','core/rend/vr/cm_game_options.h','core/rend/vr/native_release.h','core/input/udp_lightgun.cpp','core/hw/maple/maple_jvs.cpp','core/rend/vr/xr_menu.cpp']
(O/'extraction-manifest.json').write_text(json.dumps({'mutation':mutation,'files':{p:hashlib.sha256((R/p).read_bytes()).hexdigest() for p in files},'productionBlocks':manifest},indent=2),encoding='utf8')
env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','')
cmd=['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-Wall','-I'+str(O),'-I'+str(R/'core/rend/vr'),'-I'+str(R/'core/deps/glm'),str(R/'cm-vr/re/player_assignment_test.cpp'),'-o',str(O/'player-assignment-test.exe')]
(O/'compile-command.json').write_text(json.dumps(cmd,indent=2),encoding='utf8')
result=subprocess.run(cmd,capture_output=True,text=True,env=env)
(O/'build.log').write_text(result.stdout+result.stderr,encoding='utf8')
if result.returncode:print(result.stdout+result.stderr);sys.exit(result.returncode)
sys.exit(subprocess.run([str(O/'player-assignment-test.exe')],env=env).returncode)
