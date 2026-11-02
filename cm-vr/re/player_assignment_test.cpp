#include <algorithm>
#include <atomic>
#include <array>
#include <cstring>
#include <cassert>
#include <iterator>
#include <cctype>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <chrono>
#include "cm_game_options.h"
#include <glm/glm.hpp>
#include "player_ownership.h"
#include "native_release.h"
#include "service_controls.h"
#include "service_boot.h"
#include "game_zoom.h"
using XrTime=long long;using u32=unsigned;using u64=uint64_t;using u16=uint16_t;using s16=int16_t;
constexpr int Left=0,Right=1,OffScreen=-10000,startAction=1,skipAction=2,triggerAction=3;
using vr::xr::PlayerOwnership;using vr::xr::ServiceStyle;using vr::xr::serviceStyle;
namespace zoom=vr::zoom;namespace cm=vr::xr::cm;
namespace vr::xr::cm::options {
Snapshot fixture;bool lastAllowed=false,lastFocused=false,lastManual=false;int contexts=0;
Snapshot snapshot(){return fixture;}bool busy(){return fixture.busy;}
void context(bool allowed,bool focused,bool manual){lastAllowed=allowed;lastFocused=focused;lastManual=manual;contexts++;}
}
bool focused=true,nativeOptionsWasBusy=false,quitPending=false,redrawLogged=false;int pauseRequest=-1,exits=0;void dc_exit(){exits++;}
namespace hands {bool prep=false;bool preparing(){return prep;}}
namespace ggpo {bool active(){return false;}}
std::vector<int> drains;void rend_vr_drain(int timeout){drains.push_back(timeout);}
namespace config {
struct Bool {bool value=true;operator bool()const{return value;}void set(bool v){value=v;}};
struct Str {std::string value="left";std::string get()const{return value;}void set(const char* v){value=v;}};
Bool VrXrGun, VrDualP1Left,VrSwapP1P2,VrDualWield,GGPOEnable{false};Str VrTwoPlayerP1Hand;
int VrReload=0;std::string savedKey,savedValue;
void saveBool(const char*,const char* key,bool value){savedKey=key;savedValue=value?"true":"false";}
void saveStr(const char*,const char* key,const char* value){savedKey=key;savedValue=value;}
}
namespace vr {
struct GunState {bool valid;uint8_t ammo,magazine;u16 shots,fired;bool rifle;};
GunState snapshots[2]{};
bool gunState(int player,GunState& now){now=snapshots[player];return true;}
namespace survivor {void neutral(){} bool game(){return false;} bool ready(){return false;}}
}
namespace survivor=vr::survivor;
struct GunView {int kind=0;};
#include "gun_under_test.inc"
Gun guns[2]{{0,Right},{1,Left}};
PlayerOwnership playerOwnership;vr::xr::NativeReleaseGate nativeReleaseGate;
bool menuOpen=false,menuFresh=false;namespace menu {struct Input{bool perHand=false;};int updates=0,resets=0;void reset(){resets++;}void update(const Input&){updates++;}}
bool p2JoinPending=false;std::atomic<unsigned> confirmedPlayers{1};auto& players=confirmedPlayers;bool autoAccept=false;
bool dualWield=false,p2StartWas=false,p2JoinHeld=false,startSwallow=false,skipSwallow=false;
bool triggerReleaseWait[2]{},startHeld[2]{};int startPlayer[2]{1,0},startOwner[2]{-1,-1};
XrTime dualSince=0,leftHoldSince=0,leftTapUntil=0;bool leftHoldMenu=false;struct {bool held=false;}slide;
bool starts[2]{},skips[2]{},trackedHands[2]{true,true},known=true,prompt=false;
float rawPulls[2]{};u32 lastP1=0,lastP2=0;int feedback=-1,knocks=0;
bool boolAction(int action,int hand){return action==startAction?starts[hand]:skips[hand];}
float floatAction(int,int hand){return rawPulls[hand];}
struct XrSpaceLocation{};void locateHands(XrTime,XrSpaceLocation*,bool* tracked){std::copy(std::begin(trackedHands),std::end(trackedHands),tracked);}
bool player2PromptKnown(){return known;}bool player2Prompting(){return prompt;}
struct Write{int player,x,y;u32 buttons;};std::vector<Write> writes;
void lightgunSet(int player,int x,int y,u32 buttons){(player==0?lastP1:lastP2)=buttons;writes.push_back({player,x,y,buttons});}
void noteHolds(const char*){}float haptic(int hand,float duration,float){feedback=hand;knocks++;return duration;}
float recoil(int hand){return haptic(hand,.03f,1.f);}float autoKnock(int hand){return haptic(hand,.02f,.6f);}float emptyClick(int hand){return haptic(hand,.012f,.35f);}
#define NOTICE_LOG(...)
bool inCutscene=false,cutsceneVeto=false,originSet=true,aimOutside=false;const char* aimHow="3D";
glm::vec2 lastShotScreen;unsigned shotSerial=0;enum class Holds{Gun};Holds holds(int){return Holds::Gun;}
std::atomic<bool> newGame{false};bool p1Armed=false,p1StartWas=false;void setHowToPlay(bool,const char*){}
struct {struct {std::string gameId;}content;}settings;
vr::xr::StartReleaseGate startRelease;vr::xr::ServiceControls serviceControls;
zoom::Session zoomSession;float frameWorldZoom=1.f;
std::atomic<uint64_t> serviceGameBootGeneration{0};
std::atomic<bool> startInputReady{true};auto& gameReady=startInputReady;bool emuRunning=true;struct {int starts=0,stops=0;bool running(){return emuRunning;}void start(){emuRunning=true;starts++;}void stop(){emuRunning=false;stops++;}} emu;
bool menuCloseRequested=false;void close(){menuCloseRequested=true;}bool serviceRequest=false,serviceNextRequest=false,serviceEntryRequest=false,serviceSelectRequest=false;
void lightgunService(bool value,bool next=false,bool select=false){serviceRequest=value||select;serviceEntryRequest=value;serviceNextRequest=next;serviceSelectRequest=select;}
#include "under_test.inc"
int checks=0,failures=0;
void check(bool ok,const char* what){++checks;if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",what);}}
void layout(bool left){config::VrDualP1Left.value=config::VrSwapP1P2.value=left;config::VrTwoPlayerP1Hand.value=left?"left":"right";}
void reset(bool left=true){settings.content.gameId="";serviceControls.reset();startRelease.reset();playerOwnership.reset();serviceRequest=serviceNextRequest=serviceEntryRequest=serviceSelectRequest=false;startSwallow=menuCloseRequested=false;p2JoinPending=false;players=1;autoAccept=false;menuOpen=false;guns[0]=Gun{0,Right};guns[1]=Gun{1,Left};dualWield=false;dualSince=0;p2StartWas=p2JoinHeld=startSwallow=skipSwallow=p1StartWas=p1Armed=false;known=true;prompt=false;gameReady=emuRunning=true;config::VrXrGun.value=config::VrDualWield.value=true;config::GGPOEnable.value=false;feedback=-1;lastP1=lastP2=0;for(int h:{Left,Right}){starts[h]=skips[h]=triggerReleaseWait[h]=startHeld[h]=false;startOwner[h]=-1;startPlayer[h]=1-h;trackedHands[h]=true;rawPulls[h]=0;vr::snapshots[h]={true,6,6,100,100,false};}layout(left);writes.clear();knocks=0;}
void frame(XrTime t=1'000'000'000LL,float left=0,float right=0,bool tl=true,bool tr=true){rawPulls[0]=left;rawPulls[1]=right;trackedHands[0]=tl;trackedHands[1]=tr;frameCode(t);}
void nativePoll(){u32 buttons[4]{};for(int p:{0,1}){u32 xr=p?lastP2:lastP1;buttons[p]=((xr&1)?NAOMI_BTN0_KEY:0)|((xr&2)?NAOMI_RELOAD_KEY:0);}NativeJvsMapper mapper;mapper.init_mappings();mapper.nativeReleasePoll(buttons);}
void hostRegressions();void nativeApplyHostRegressions();

int main(){
 gameReady=false;check(!serviceMenuAvailable(),"Service row waits for initial native game loading");gameReady=true;
 emuRunning=false;check(!gameStartReady()&&serviceMenuAvailable(),"paused VR menu can open Service from loaded snapshot");emuRunning=true;
 using vr::xr::nativeMainBoot;
 check(nativeMainBoot(0x0c020800,0x0c01f820,0x0c020800,0x0c020000),"standard BIOS main handoff restores controls");
 check(nativeMainBoot(0x8c021000,0x0c01f820,0x8c021000,0x8c021000),"equal main/test PC still recognizes main selector");
 check(!nativeMainBoot(0x8c021000,0x0c01f824,0x8c021000,0x8c021000),"equal main/test PC rejects Game Test selector");
 check(!nativeMainBoot(0x0c020000,0x0c01f824,0x0c020800,0x0c020000),"Game Test boot preserves navigation");
 check(nativeMainBoot(0xac020800,0x8c01f820,0x0c020800,0x0c020000),"cached and uncached RAM aliases agree");
 check(!nativeMainBoot(0x0c020802,0x0c01f820,0x0c020800,0x0c020000),"only exact native entry is accepted");
 check(!nativeMainBoot(0x0c020800,0x0c01f824,0x0c020800,0x0c020800),"test boot cannot mimic equal main entry");
 check(!nativeMainBoot(0x0c020800,0,0x0c020800,0x0c020000),"ordinary recompiled code is not a BIOS handoff");
 check(!nativeMainBoot(0x0c020800,0x0c01f820,0x0c020800,0),"uninitialized test header is rejected");
 check(!nativeMainBoot(0x0c01f820,0x0c01f820,0x0c01f820,0x0c020000),"BIOS metadata is not executable game entry");
 check(!nativeMainBoot(0x0c020801,0x0c01f820,0x0c020801,0x0c020000),"unaligned header is rejected");
 check(!nativeMainBoot(0x0d020800,0x0c01f820,0x0d020800,0x0c020000),"entry outside NAOMI RAM is rejected");

 hostRegressions();nativeApplyHostRegressions();

 {
  vr::xr::StartReleaseGate gate;
  check(!gate.sample(Right,true,false),"boot A does not reach native Start");
  check(!gate.sample(Right,true,true),"A held across native readiness requires release");
  gate.sample(Right,false,true);check(gate.sample(Right,true,true),"fresh A after loading remains normal Start");
  check(!gate.sample(Left,true,false)&&!gate.sample(Left,true,true),"boot X also requires release");
  gate.sample(Left,false,true);check(gate.sample(Left,true,true),"fresh X passes after readiness");
  gate.reset();check(!gate.sample(Right,true,true),"reset cannot inherit held Start");
  check(vr::xr::serviceButtons(0x71,true,false,0x40)==0x71,"existing keyboard/gamepad cabinet controls remain available");
  check(vr::xr::serviceButtons(0x01,true,true,0x40,false,0x20)==0x41,"explicit Select adds only Test");
  check(vr::xr::serviceButtons(0x01,true,false,0x40,true,0x20)==0x21,"explicit Next adds only Service");
  check(vr::xr::serviceButtons(0x01,true,true,0x40,true,0x20)==0x61,"Next and Select remain independent native inputs");
  check(vr::xr::serviceButtons(0x71,false,true,0x40,true,0x20)==0x71,"disabled XR injection does not alter cabinet input");
 }
 {
  bool p1=false,p2=false;
  reset();sampleStartButtons(0,false,true,false,true,p1,p2);
  sampleStartButtons(1'000'000'000LL,false,true,true,true,p1,p2);
  check(!p1&&!p2&&!serviceRequest&&!serviceControls.active,"A held from loading neither starts nor opens Service");
  sampleStartButtons(1'100'000'000LL,false,false,true,true,p1,p2);
  sampleStartButtons(2'000'000'000LL,false,true,true,true,p1,p2);
  check(p1&&!p2&&!serviceRequest,"fresh A starts P1 immediately without a Service gesture");
  sampleStartButtons(10'000'000'000LL,false,true,true,true,p1,p2);
  check(p1&&!serviceRequest&&!serviceNextRequest&&!serviceControls.active,"holding A for any duration cannot open Service");
  sampleStartButtons(11'000'000'000LL,false,false,true,true,p1,p2);
  check(!p1&&!p2&&!serviceRequest,"released A releases Start without a late Test pulse");
  reset();config::GGPOEnable.value=true;sampleStartButtons(0,false,false,true,true,p1,p2);
  sampleStartButtons(1,false,true,true,true,p1,p2);
  check(p1&&!serviceRequest&&!serviceControls.active,"online A remains ordinary synchronized Start");
  check(!openServiceMenu()&&!serviceControls.active,"online play cannot request local Service");
  config::GGPOEnable.value=false;config::VrXrGun.value=false;
  check(!openServiceMenu()&&!serviceControls.active,"disabled VR guns cannot request Service");
  config::VrXrGun.value=true;
 }
 {
  bool p1=false,p2=false;
  reset();serviceGameBootGeneration=4;chooseServiceRow();
  check(menuCloseRequested&&serviceControls.active&&serviceControls.pending&&!serviceRequest,"the real Service row queues entry and resumes the paused game without immediate Test");
  sampleStartButtons(0,false,true,false,false,p1,p2);
  check(serviceControls.pending&&!serviceRequest&&!serviceNextRequest&&!p1&&!p2,"paused/menu-held A cannot dispatch Service before unpause");
  sampleStartButtons(1'000'000'000LL,true,true,false,true,p1,p2);
  check(serviceRequest&&!serviceNextRequest&&!serviceControls.pending&&!p1&&!p2,"explicit menu entry sends exactly one Test pulse without either Start");
  sampleStartButtons(1'400'000'000LL,true,true,false,true,p1,p2);
  check(!serviceRequest&&!serviceNextRequest&&!p1&&!p2,"buttons held through menu entry cannot select again or start P2");
  sampleStartButtons(1'500'000'000LL,false,false,false,true,p1,p2);
  sampleStartButtons(2'000'000'000LL,true,false,false,true,p1,p2);
  check(serviceNextRequest&&!serviceRequest&&!p1&&!p2,"Service X navigates native options while game readiness is false");
  sampleStartButtons(2'090'000'000LL,false,false,false,true,p1,p2);
  check(!serviceNextRequest,"rapid Next taps have a native release edge");
  sampleStartButtons(2'120'000'000LL,true,false,false,true,p1,p2);
  check(serviceNextRequest,"a second rapid Next tap stays separate");
  sampleStartButtons(2'300'000'000LL,false,false,false,true,p1,p2);
  sampleStartButtons(3'000'000'000LL,false,true,false,true,p1,p2);
  check(serviceRequest&&!serviceNextRequest&&!p1&&!p2,"short A selects immediately inside Service without game Start");
  sampleStartButtons(3'090'000'000LL,false,false,false,true,p1,p2);
  check(!serviceRequest,"rapid Select taps have a native release edge");
  sampleStartButtons(3'120'000'000LL,false,true,false,true,p1,p2);
  check(serviceRequest,"second rapid Select tap stays separate");
  resetStartGestures();sampleStartButtons(4'000'000'000LL,true,true,false,true,p1,p2);
  check(serviceControls.active&&!serviceRequest&&!serviceNextRequest&&!p1&&!p2,"focus or sync recovery preserves mode but consumes held A/X");
  sampleStartButtons(4'100'000'000LL,false,false,false,true,p1,p2);
  sampleStartButtons(5'000'000'000LL,true,false,false,true,p1,p2);
  check(serviceNextRequest,"fresh native Next works after focus recovery");
  serviceControls.cancel();openServiceMenu();sampleStartButtons(6'000'000'000LL,false,false,false,true,p1,p2);
  check(serviceControls.active&&!serviceRequest&&!serviceNextRequest,"reopening an already active Service row only resumes it without another Test");
  sampleStartButtons(7'000'000'000LL,false,true,true,true,p1,p2);
  check(!p1&&!p2,"returning native readiness alone cannot close Service or start a player");
  notifyServiceGameBoot();sampleStartButtons(8'000'000'000LL,true,true,true,true,p1,p2);
  check(!serviceControls.active&&!serviceRequest&&!serviceNextRequest&&!p1&&!p2,"native main boot automatically clears Service without B and consumes the exit press");
  sampleStartButtons(8'100'000'000LL,false,false,true,true,p1,p2);
  sampleStartButtons(9'000'000'000LL,false,true,true,true,p1,p2);
  check(p1&&!p2&&!serviceRequest,"a fresh A starts P1 normally after native EXIT without a B step");
  serviceControls.request(serviceGameBootGeneration.load());config::GGPOEnable.value=true;
  sampleStartButtons(10'000'000'000LL,false,false,true,true,p1,p2);
  check(!serviceControls.active&&!serviceRequest&&!serviceNextRequest,"online transition clears Service mode and native pulses");
  config::GGPOEnable.value=false;serviceControls.request(serviceGameBootGeneration.load());newGame=true;resetForNewGame();
  check(!serviceControls.active&&!serviceRequest&&!serviceNextRequest,"new game clears Service mode and pending input");
  reset();autoAccept=false;serviceControls.request(serviceGameBootGeneration.load());p2JoinPending=true;starts[Left]=true;frame();
  check(!p2JoinPending&&!dualWield&&lastP2==0,"Service mode cancels speculative P2 requests");
  serviceControls.reset();
 }
 for(const char* id:{"ninjaslt","gunsur2","NINJA ASSAULT"," BIOHAZARD  GUN SURVIVOR2"}) {
  bool p1=false,p2=false;reset();settings.content.gameId=id;
  check(!serviceMenuExitAvailable(),"ordinary Namco gameplay shows only explicit Service entry");
  chooseServiceRow();chooseServiceRow();check(serviceControls.pending&&!serviceControls.exiting,"two Service selects in one frame queue entry once");sampleStartButtons(1'000'000'000LL,false,false,true,true,p1,p2);
  check(serviceEntryRequest&&!serviceSelectRequest&&serviceMenuExitAvailable(),"Namco menu entry uses native Test only");
  sampleStartButtons(1'300'000'000LL,false,false,true,true,p1,p2);
  sampleStartButtons(2'000'000'000LL,true,false,true,true,p1,p2);
  auto bits=vr::xr::serviceButtons(0,true,serviceEntryRequest,1,serviceNextRequest,4,serviceSelectRequest,16);
  check(bits==4&&!p1&&!p2,"Namco X injects only native menu Down");
  sampleStartButtons(2'200'000'000LL,false,false,true,true,p1,p2);
  sampleStartButtons(3'000'000'000LL,false,true,true,true,p1,p2);
  const unsigned enter=serviceStyle(id)==ServiceStyle::Ninja?16:32;
  bits=vr::xr::serviceButtons(0,true,serviceEntryRequest,1,serviceNextRequest,4,serviceSelectRequest,enter);
  check(bits==enter&&!serviceEntryRequest&&!p1&&!p2,"Namco A injects the title's Enter without toggling Test or starting players");
  chooseServiceRow();chooseServiceRow();sampleStartButtons(4'000'000'000LL,true,true,true,false,p1,p2);
  check(serviceControls.pending&&serviceControls.exiting&&!serviceRequest,"Namco Exit Service Menu waits for resume");
  sampleStartButtons(5'000'000'000LL,true,true,true,true,p1,p2);
  check(serviceEntryRequest&&!serviceSelectRequest&&!serviceNextRequest&&!p1&&!p2,"only the explicit Namco Exit row sends the return Test toggle");
  notifyServiceGameBoot();sampleStartButtons(5'100'000'000LL,true,true,true,true,p1,p2);
  check(!serviceControls.active&&!serviceMenuExitAvailable()&&!serviceRequest&&!p1&&!p2,"native Namco return automatically clears mapping and consumes held inputs");
 }
 {
  NativeJvsMapper mapper;mapper.init_mappings();u32 buttons[4]{},native[4]{};
  buttons[0]=vr::xr::serviceButtons(0,true,false,NAOMI_TEST_KEY,false,NAOMI_DOWN_KEY,true,serviceEnterMask(ServiceStyle::Ninja));
  mapper.read_digital_in(buttons,native);
  check(native[0]==NAOMI_BTN0_KEY&&native[1]==0,"actual Ninja JVS descriptor maps Service A to only cabinet Enter");
  buttons[0]=NAOMI_DOWN_KEY;mapper.read_digital_in(buttons,native);
  check(native[0]==NAOMI_DOWN_KEY&&native[1]==0,"actual Ninja JVS descriptor preserves Service X Down");
 }
 std::printf("%s %d production assignment checks (%d failures)\n",failures?"FAIL":"PASS",checks,failures);return failures?1:0;
}
#include "host_input_regressions.inc"

#include "host_native_apply_regressions.inc"
