from pathlib import Path
import os,subprocess,sys,re
r=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
o=Path(sys.argv[2]) if len(sys.argv)>2 else r/'build-win/vrtest/game-zoom-integration';o.mkdir(parents=True,exist_ok=True)
h=(r/'core/rend/vr/xr_host.cpp').read_text(encoding='utf-8-sig');v=(r/'core/rend/vr_reproject.cpp').read_text(encoding='utf-8-sig');vh=(r/'core/rend/vr_reproject.h').read_text(encoding='utf-8-sig');m=(r/'core/rend/vr/xr_menu.cpp').read_text(encoding='utf-8-sig')
def block(t,s):
 a=t.index(s);b=t.index('{',a);d=1;e=b+1
 while d:d+=(t[e]=='{')-(t[e]=='}');e+=1
 return t[a:e]
pre='''#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <string>
#include <cstring>
#include <cmath>
#include <map>
#include <iostream>
#include <limits>
#include "game_zoom.h"
static int checks;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"FAIL " #x " line "<<__LINE__<<'\\n';return 1;}}while(0)
#define NOTICE_LOG(...) ((void)0)
namespace zoom=vr::zoom;
'''
camera='''using u32=unsigned;
constexpr unsigned ScaleX=0,ScaleY=20;
namespace config {float VrFocal=540.8f;unsigned VrProjAddr=100,VrViewportAddr=200;}
std::map<unsigned,float> ram;
float readFloat(unsigned a){return ram[a];}
unsigned ReadMem32_nommu(unsigned a){
static const std::map<unsigned,unsigned> reset{{0x27314,0xd25ac759},{0x27318,0xd357d15a},{0x2731c,0xf308f3fa},{0x27324,0xf13af23a},{0x2747c,0x42200000},{0x27480,0x0c154ed4},{0x27484,0x0c154ec8},{0x27494,0x43a00000}};
auto found=reset.find(a&0x1ffffff);return found==reset.end()?0:found->second;
}
struct Patch {unsigned offset=10;unsigned short patched=20;} ;
struct GameProfile {const char* gameId=" CONFIDENTIAL MISSION ---------";unsigned stockFocalAddr=300;Patch fovPatch[1];} profile;
const GameProfile* activeProfile=&profile;bool activeLargeView=true,codeThere=true;
bool profileCodeThere(const GameProfile&){return codeThere;}
bool widensByCode(){return true;}
unsigned short ReadMem16_nommu(unsigned){return 20;}
'''
camera+=block(v,'static bool plausible(')+'\n'
if 'static bool readCameraFocal(' in v:camera+=block(v,'static bool readCameraFocal(')+'\n'
camera+=block(v,'static glm::vec2 readGameFocal()')+'\n'
camera+=block(v,'float nativeCameraZoom()')
camera+='''
int main(){ram[300]=1.f/879.2f;ram[100]=436.4f/320;ram[120]=436.4f/240;ram[200]=320;ram[220]=-240;
CHECK(std::abs(nativeCameraZoom()-879.2f/879.19277f)<1e-5f);
for(float bad:{0.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){ram[100]=bad;CHECK(nativeCameraZoom()==1.f);}
ram[100]=436.4f/320;ram[120]=0;CHECK(nativeCameraZoom()==1);ram[120]=436.4f/240;
codeThere=false;CHECK(nativeCameraZoom()==1);codeThere=true;activeProfile=nullptr;CHECK(nativeCameraZoom()==1);
std::cout<<checks<<" actual native camera boundary checks PASS\\n";}
'''
start=h.index('void updateLightgun(')
p=h.index('\tconst int zoomLayout=',start);e=h.index('\n\trefreshGameZoom();',p);policy=h[p:e]
# Use the real current physical ownership adapter and primary-gun selector. The
# world zoom layout must consume an interrupted grip when ownership changes.
a=h.index('    // Native player slots remain canonical;',start)
b=h.index("\n\t// The game's first Start",a)
movement=h[a:b]
transitions='''#include "player_ownership.h"
using XrTime=long long;using u32=unsigned;
constexpr int Left=0,Right=1,OffScreen=-10000,skipAction=1;
struct Gun{int player=0,hand=1;uint64_t nativeReleaseTicket=0;bool triggerWas=false,triggerWait=false,pullOutside=false,pointedDown=false,offArmed=false,freshPull=false,gunKnown=false;XrTime pullAt=0,heldAt=0,reloadAt=0;int shotsDue=0,autosDue=0,clicksDue=0;}guns[2]{{0,1},{1,0}};
vr::xr::PlayerOwnership playerOwnership;uint64_t lightgunRequireRelease(int){return 1;}
'''+block(h,'Gun& primaryGun()')+'''
zoom::Session zoomSession;std::array<zoom::Hand,2> zoomHands;bool zoomInputAvailable=true,dualWield=false,menuOpen=false,menuEdge=false,p2JoinPending=false,p1Start=false,p2Start=false,skipSwallow=false,skip=false;
struct{bool held=false;}slide;u32 dpad=0;
bool boolAction(int,int){return false;}void lightgunSet(int,int,int,u32){}void noteHolds(const char*){}
bool serviceMenuActive(){return false;}int twoPlayerP1Hand(){return 0;}double zoomSeconds(){return 2.;}bool gameZoomOn(){return zoomSession.enabled();}
void frame(){'''+movement+'\n'+policy+'''}
int main(){vr::xr::PlayerOwnership::Input in;in.allowed=in.known=true;in.tracked[0]=in.tracked[1]=true;playerOwnership.update(in);for(auto& hand:zoomHands)hand.tracked=true;frame();zoomHands[0].grip=true;frame();zoomHands[0].grip=false;in.pull[0]=1;playerOwnership.update(in);frame();CHECK(primaryGun().hand==0);CHECK(!zoomSession.enabled());std::cout<<checks<<" coincident grip-release / actual gun-handoff checks PASS\\n";}
'''
sampleStart=h.index('for(int hand:{Left,Right})',h.index('std::array<zoom::Hand,2> zoomHands{};',start))
sample=h[sampleStart:h.index('\n\tconst int zoomLayout=',sampleStart)]
pad='''constexpr int Left=0,Right=1,gripAction=1,recenterAction=2,laserAction=3,skipAction=4;
float grip[2]{};bool button[5][2]{};bool tracked[2]{true,true};glm::vec2 stick(0);
float floatAction(int,int hand){return grip[hand];}bool boolAction(int action,int hand=-1){return hand<0?(button[action][0]||button[action][1]):button[action][hand];}
zoom::Session session;
void frame(){std::array<zoom::Hand,2> zoomHands{};'''+sample+'''session.update(zoomHands,true,0);}
int main(){frame();grip[0]=1;frame();button[skipAction][0]=true;frame();button[skipAction][0]=false;frame();grip[0]=0;frame();CHECK(!session.enabled());std::cout<<checks<<" actual PCVR pad-click consumption checks PASS\\n";}
'''
# Execute the actual event/frame hooks, or no hook when absent, to reproduce an
# interrupted held grip without an intervening updateLightgun call.
focusHook='if(!nowFocused)setZoomInputAvailable(false);' if 'if(!nowFocused)setZoomInputAvailable(false);' in h else ''
frameHook='setZoomInputAvailable(focused&&located&&frameState.shouldRender&&!splashVisible);' if 'setZoomInputAvailable(focused&&located&&frameState.shouldRender&&!splashVisible);' in h else ''
availability=block(h,'void setZoomInputAvailable(') if 'void setZoomInputAvailable(' in h else 'void setZoomInputAvailable(bool available){if(!available)zoomSession.suspend();}'
life='''zoom::Session zoomSession;bool zoomInputAvailable=true;
'''+availability+'''
void event(bool nowFocused){'''+focusHook+'''}
void availableFrame(bool located,bool shouldRender){bool focused=true,splashVisible=false;struct{bool shouldRender;}frameState{shouldRender};'''+frameHook+'''}
int main(){std::array<zoom::Hand,2> hands{};for(auto& hand:hands)hand.tracked=true;
auto tick=[&](){zoomSession.update(hands,true,0);};
tick();hands[0].grip=true;tick();event(false);event(true);hands[0].grip=false;tick();CHECK(!zoomSession.enabled());
hands[0].grip=true;tick();availableFrame(false,true);availableFrame(true,true);hands[0].grip=false;tick();CHECK(!zoomSession.enabled());
hands[0].grip=true;tick();availableFrame(true,false);availableFrame(true,true);hands[0].grip=false;tick();CHECK(!zoomSession.enabled());
std::cout<<checks<<" actual XR interruption hook checks PASS\\n";}
'''
# Actual CPU raycast reconstruction and native projection tail, with controlled hits.
a=h.index('\t\t\tconst glm::vec3 p(ndc.x');b=h.index('\n\t\t\tkind[i] = Scene;',a);vertex=h[a:b]
a=h.index('\tconstexpr float Far = 10000.f;',h.index('bool aimAtScene('));b=h.index('\n}\n',a);mapping=h[a:b]
aim='namespace config {bool VrAimWidened=false;}\n'+block(vh,'struct GameCamera')+';\n'+block(vh,'inline float comfortScale(')+'\n'+block(h,'bool onGameScreen(')+'''
bool aimOutside;const char* aimHow;
glm::vec3 vertex(const glm::vec2& ndc,float w,const GameCamera& cam){glm::vec3 pos[1];int i=0;glm::vec3 comfort(0);'''+vertex+'''return pos[0];}
bool map(const glm::vec3& o,const GameCamera& cam,glm::vec2& screen){glm::vec3 d(0);float dist=0,nearest=2;'''+mapping+'''}
int main(){GameCamera c;c.dcSize={640,480};c.tanHalf={1,.75};c.stockTan={.5,.375};c.overlayW=1;
for(float z:{.5f,1.f,2.f}){c.worldZoom=z;auto p=vertex({.2f,-.4f},8,c);CHECK(std::abs(p.x-1.6f*z)<1e-6f&&std::abs(p.y+2.4f*z)<1e-6f&&p.z==-8);glm::vec2 screen;CHECK(map(p,c,screen));CHECK(std::abs(screen.x-.7f)<1e-6f&&std::abs(screen.y-.1f)<1e-6f);}
c.worldZoom=2;glm::vec2 screen;CHECK(!map({9,0,-8},c,screen)&&aimOutside);CHECK(!map({17,0,-8},c,screen)&&!aimOutside);
std::cout<<checks<<" production world raycast and inverse projection checks PASS\\n";}
'''
env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','')
fail=0
for name,code in [('camera',camera),('transitions',transitions),('pad',pad),('life',life),('aim',aim)]:
 cpp=o/(name+'.cpp');cpp.write_text(pre+code,encoding='utf8');exe=o/(name+'.exe')
 subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(r/'core/deps/glm'),'-I'+str(r/'core/rend/vr'),str(cpp),'-o',str(exe)],check=True,env=env)
 fail|=subprocess.run([str(exe)],env=env).returncode
sys.exit(fail)
