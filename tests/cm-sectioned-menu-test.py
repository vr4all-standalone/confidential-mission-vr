"""Run the production menu/CPU panel against controlled host boundaries, without XR."""
from pathlib import Path
import hashlib,json,os,re,subprocess,sys,zipfile
repo=Path(sys.argv[1]); out=Path(sys.argv[2]); out.mkdir(parents=True,exist_ok=True)
def source(p): return (repo/p).read_text(encoding='utf-8-sig')
def block(t,s):
 a=t.index(s); b=t.index('{',a); d=1; e=b+1
 while d: d+=(t[e]=='{')-(t[e]=='}'); e+=1
 return t[a:e]
menu=source('core/rend/vr/xr_menu.cpp'); panel=source('core/rend/vr/xr_panel.cpp')
with zipfile.ZipFile(repo/'fonts/Roboto-Bold.ttf.zip') as z:
 fontName=next(n for n in z.namelist() if n.endswith('.ttf'))
 (out/'Roboto-Bold.ttf').write_bytes(z.read(fontName))
prefix=r'''
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <array>
#include <cstring>
#include <cstdint>
#include <type_traits>
#include <iostream>
#include "xr_menu.h"
#include "xr_panel.h"
#define NOTICE_LOG(...) ((void)0)
#define WARN_LOG(...) ((void)0)
namespace config {
std::map<std::string,std::string> saved;
template<class T> struct Option {T v,def; Option(T value):v(value),def(value){} operator T()const{return v;} T get()const{return v;}void set(T value){v=value;}void reset(){v=def;}void save()const{}};
Option<int> VrGun(1),VrGunAngle(0),VrReload(0),VrReticleMode(1),VrLaserWidth(1),VrWidenMode(1);
Option<bool> VrLaser(true),VrWiden(true),VrGameHands(true);
Option<float> VrReticleMinSize(.20f),VrReticleMaxSize(.45f),VrReticleShrinkDistance(6.5f),VrWorldScale(.11f),VrHudDepth(40.f),VrHudSize(1.f);
Option<std::string> VrGunFile(""),VrHaptics("medium"),VrTwoPlayerP1Hand("left");
void saveStr(const char*,const char*k,const std::string&v){saved[k]=v;} void saveInt(const char*,const char*k,int v){saved[k]=std::to_string(v);}void saveBool(const char*,const char*k,bool v){saved[k]=v?"1":"0";}void saveFloat(const char*,const char*k,float v){saved[k]=std::to_string(v);}void setAutoSave(bool){}
std::string loadStr(const char*,const char*k,const std::string&v){return saved.count(k)?saved[k]:v;}
std::vector<std::string> getEntries(const char*){std::vector<std::string> keys;for(auto&kv:saved)keys.push_back(kv.first);return keys;}
}
'''
# Replace the initial boundary fixture with the same real INI/Settings/Option extraction
# used by the existing CM production settings tests. Only file IO is captured in memory.
prefix=prefix[:prefix.index('namespace config {')]
prefix+='#define stricmp _stricmp\nnamespace hostfs {class File;}\n'
option=source('core/cfg/option.h'); definitions=source('core/cfg/option.cpp')
prefix+=re.search(r'^const std::string defaultWs[^\n]+',source('core/stdclass.cpp'),re.M)[0]+'\n'+block(source('core/stdclass.h'),'static inline std::string trim_ws(')+'\n'
prefix+=source('core/cfg/ini.h').replace('#include "oslib/storage.h"','').replace('#pragma once','')+'\n'
ini=re.sub(r'^#include[^\n]*\n','',source('core/cfg/ini.cpp'),flags=re.M)
for sig in ['void IniFile::load(hostfs::File *file','void IniFile::save(hostfs::File *file']:ini=ini.replace(block(ini,sig),'')
prefix+=ini+'\n#include "cfg/cfg.h"\n'
prefix+='namespace config {static IniFile cfgdb;static bool autoSaving=true;std::string persisted;int writeCount=0;void saveFile(){cfgdb.save(persisted);++writeCount;}void autoSave(){if(autoSaving)saveFile();}\n'
for sig in ['void saveStr(','std::string loadStr(','void saveInt(','int loadInt(','void saveBool(','bool loadBool(','void setTransient(','bool isTransient(','bool hasSection(','void deleteSection(','void deleteEntry(','void setAutoSave(','void saveFloat(','float loadFloat(','std::vector<std::string> getEntries(']:prefix+=block(source('core/cfg/cfg.cpp'),sig)+'\n'
prefix+='}\n'
for helper in ['core/rend/vr/gun_angle_settings.h','core/rend/vr/reticle_settings.h','core/rend/vr/hud_size_settings.h','core/rend/vr/view_settings.h']:
 t=source(helper); prefix+=re.sub(r'^#(?:include|pragma)[^\n]*\n','',t,flags=re.M)+'\n'
prefix+='namespace config {\n'+';\n'.join(block(option,sig) for sig in ['class BaseOption','class Settings','template<typename T, bool PerGameOption = true>'])+';\n'
for name in ['GunAngleOption','ReticleModeOption','ReticleMinSizeOption','ReticleMaxSizeOption','ReticleDistanceOption','HudSizeOption','LaserWidthOption','WidenModeOption']:prefix+=re.search(r'^using '+name+r'[^\n]+',option,re.M)[0]+'\n'
for name in ['VrGun','VrGunAngle','VrReload','VrReticleMode','VrLaserWidth','VrWiden','VrWidenMode','VrLaser','VrGameHands','VrReticleMinSize','VrReticleMaxSize','VrReticleShrinkDistance','VrWorldScale','VrHudDepth','VrHudSize','VrGunFile','VrHaptics','VrTwoPlayerP1Hand','VrHandScale','VrGunScale']:
 prefix+=re.search(r'^\w+(?:<[^>]+>)? '+name+r'\b[^\n]+',definitions,re.M)[0]+'\n'
prefix+='}\n'
prefix+=r'''
#include "cm_game_options.h"
namespace vr::xr::cm::options {
Snapshot testNative;int applyCalls=0,discardCalls=0;
Snapshot snapshot(){return testNative;}
const char* label(Field f){const char* names[]={"Game difficulty","Boss difficulty","Shifting difficulty","Collision size","Life"};return names[(unsigned)f];}
const char* valueLabel(Field,int){return nullptr;}
bool stage(Field f,int v){unsigned n=(unsigned)f;if(testNative.busy||!testNative.available||!(testNative.supportedMask&(1u<<n)))return false;testNative.value[n]=std::clamp(v,(int)testNative.minimum[n],(int)testNative.maximum[n]);testNative.pendingMask|=1u<<n;testNative.status=Status::Pending;return true;}
bool apply(){if(!testNative.pendingMask||testNative.busy)return false;++applyCalls;testNative.busy=true;testNative.status=Status::EnteringService;testNative.message="Applying native settings";return true;}
void discard(){++discardCalls;testNative.pendingMask=0;}
bool busy(){return testNative.busy;}
}
'''
prefix+=r'''
enum class Event{Start}; struct EventManager{template<class F>static void listen(Event,F){}};
namespace vr::xr {
bool paused=false,help=false,zoom=false;int quits=0,knock=-1;
bool menuPaused(){return paused;}void requestPause(bool p){paused=p;}void recenter(){} void quitToStart(){++quits;}
bool howToPlayOn(){return help;}void setHowToPlay(bool p,const char*){help=p;}bool gameZoomOn(){return zoom;}void setGameZoom(bool p){zoom=p;}
bool serviceMenuAvailable(){return true;}bool serviceMenuExitAvailable(){return false;}bool openServiceMenu(){return true;}
int hapticsLevel(){return config::VrHaptics.get()=="off"?0:config::VrHaptics.get()=="light"?1:config::VrHaptics.get()=="strong"?3:2;}void sampleKnock(int h){knock=h;}
int twoPlayerP1Hand(){return config::VrTwoPlayerP1Hand.get()=="right"?1:0;}
constexpr int OwnGuns=4;struct GunChoice{int setting;const char*file;const char*label;bool made,complete;};
bool nativeGun=true;std::vector<GunChoice> gunChoices(){return {{1,nullptr,"Arcade",true,true},{4,"test","Custom",true,true}};}int chosenGun(){return nativeGun?1:0;}
int handGun(int){return chosenGun();}bool gameGun(int v){return v==1;}
namespace own{void scan(){}bool sameStem(const std::string&a,const char*b){return a==b;}}
void drawRowsPanel(const glm::mat4&,const glm::mat4&,const RowsPanel&){}
void drawTexture(const glm::mat4&,const glm::mat4&,unsigned,float,float,const glm::vec4&){}
void drawPanelContact(const glm::mat4&,const glm::mat4&){}
}
'''
# The sectioned implementation adds geometry/CPU raster helpers; compile those real functions.
if 'SettingsPanel panel()' in menu:
 prefix+='#define STBTT_STATIC\n#define STB_TRUETYPE_IMPLEMENTATION\n#include "imgui/imstb_truetype.h"\n'
 prefix+='namespace vr::xr { namespace {\nusing u8=unsigned char;\nstbtt_fontinfo font;int canvasWidth,canvasHeight;std::vector<u8> pixels;\n'
 prefix+=panel[panel.index('constexpr int SettingsWidth'):panel.index('std::unique_ptr<u8[]> fontData;')]
 for sig in ['void blend(','void fill(','void background(','float textWidth(','void text(','SettingsRect normalized(','bool contains(','std::vector<SettingsRect> choiceRects(','SettingsRect stepRect(','void rounded(','void card(','int baseline(','void fitText(','void wrappedText(','void paintSettings(']: prefix+=block(panel,sig)+'\n'
 prefix+='}\n'
 for sig in ['RowsLayout settingsLayout()','SettingsRect settingsRowRect(','SettingsRect settingsTabRect(','SettingsRect settingsCloseRect()','SettingsRect settingsBackRect()','SettingsHit settingsAt(','bool settingsPanelHit(','SettingsHit settingsHit(']: prefix+=block(panel,sig)+'\n'
 prefix+='void drawSettingsPanel(const glm::mat4&,const glm::mat4&,const SettingsPanel&){}\n}\n'
else:
 prefix+='namespace vr::xr { constexpr int RowsWidth=1024,TitleBand=100,RowHeight=72,FooterBand=76;constexpr float RowsWidthMetres=.9f;\n'
 for sig in ['int rowsHeight(','RowsLayout rowsLayout(','bool rowsPanelHit(','int rowAt(']:prefix+=block(panel,sig)+'\n'
 prefix+='}\n'
prefix+=re.sub(r'^#include[^\n]*\n','',menu,flags=re.M)
test=r'''
int main(){ using namespace vr::xr; menu::open(glm::mat4(1.f));
 if(menu::panel().rows.size()!=6){std::cerr<<"FAIL Gun opens on its six main controls, without Resume/Recenter\n";return 1;}
 std::cout<<"1 production menu check PASS\n"; }
'''
extra=Path(sys.argv[3]) if len(sys.argv)>3 else Path(__file__).with_suffix('.cpp')
if extra.exists():test=extra.read_text()
(out/'menu-runtime.cpp').write_text(prefix+test,encoding='utf8')
env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','')
cmd=['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O0','-I'+str(repo/'core'),'-I'+str(repo/'core/deps/glm'),'-I'+str(repo/'core/rend/vr'),'-I'+str(repo/'core/deps'),str(out/'menu-runtime.cpp'),'-o',str(out/'menu-runtime.exe')]
compiled=subprocess.run(cmd,capture_output=True,text=True,env=env)
(out/'compile.log').write_text(compiled.stdout+compiled.stderr)
receipt={'compile':{'command':cmd,'exit':compiled.returncode},'sourceSha256':{str(repo/p):hashlib.sha256((repo/p).read_bytes()).hexdigest() for p in ['core/rend/vr/xr_menu.cpp','core/rend/vr/xr_menu.h','core/rend/vr/xr_panel.cpp','core/rend/vr/xr_panel.h','core/rend/vr/cm_game_options.h']}}
if compiled.returncode:
 print(compiled.stdout,compiled.stderr);(out/'receipt.json').write_text(json.dumps(receipt,indent=2));sys.exit(compiled.returncode)
run=subprocess.run([str(out/'menu-runtime.exe')],cwd=out,capture_output=True,text=True,env=env)
print(run.stdout,end='');print(run.stderr,end='',file=sys.stderr)
(out/'run.log').write_text(run.stdout+run.stderr)
receipt['run']={'command':[str(out/'menu-runtime.exe')],'exit':run.returncode,'output':run.stdout+run.stderr}
try:
 from PIL import Image
 for capture in out.glob('*.ppm'):Image.open(capture).save(capture.with_suffix('.png'))
except ImportError:pass
(out/'receipt.json').write_text(json.dumps(receipt,indent=2))
sys.exit(run.returncode)
