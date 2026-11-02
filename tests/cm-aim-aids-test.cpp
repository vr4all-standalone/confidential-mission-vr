#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#define WARN_LOG(...) ((void)0)
#define NOTICE_LOG(...) ((void)0)
#define stricmp _stricmp
namespace hostfs {class File;}
#include "base/under_test.inc"
static bool clicked,laserWas;
static constexpr int laserAction=1,Right=1;
static int ticks;
static bool boolAction(int action){if(action!=laserAction)std::abort();return clicked;}
static void haptic(int hand,float seconds,float amplitude){if(hand!=Right||seconds!=.03f||amplitude!=.4f)std::abort();++ticks;}
#include "shortcut.inc"
static int checks;
static void check(bool yes,const char* message){++checks;if(!yes){std::cerr<<"FAIL "<<message<<'\n';std::exit(1);}}
static void load(const std::string& text){
 config::Settings::instance().reset();config::cfgdb=config::IniFile();config::cfgdb.load(text); config::cfgdb.load("[config]\nvr.CmWorldSizeMigration=1\n");
 config::cfgdb.save(config::persisted);config::writeCount=0;config::Settings::instance().load(false);
}
int main(){
 using namespace config;
 for(bool laser:{false,true})for(int reticle:{1,2}){
  load(std::string("[config]\nvr.Laser=")+(laser?"yes":"no")+"\nvr.ReticleMode="+std::to_string(reticle)+
       "\nvr.LaserWidth=4\nvr.ReticleMinSize=.37\nvr.ReticleMaxSize=1.7\nvr.ReticleShrinkDistance=7.3\nSentinel=73\nvr.GunAngle=17\n");
  clicked=false;laserWas=false;ticks=0;sampleShortcut();
  check(bool(VrLaser)==laser&&int(VrReticleMode)==reticle&&writeCount==0,"idle leaves explicit choices unchanged");
  const bool on=!(laser||reticle==1);
  clicked=true;sampleShortcut();
  check(bool(VrLaser)==on&&int(VrReticleMode)==(on?1:2),"fresh right-click sets laser and reticle to the same visibility");
  check(loadBool("config","vr.Laser")==on&&loadInt("config","vr.ReticleMode")==int(VrReticleMode),"both shortcut choices reach actual INI keys");
  check(writeCount==1&&ticks==1,"one atomic settings write and right-hand tick per fresh press");
  for(int i=0;i<20;i++)sampleShortcut();
  check(writeCount==1&&ticks==1&&bool(VrLaser)==on,"holding stick click does not repeat");
  clicked=false;sampleShortcut();
  check(writeCount==1&&bool(VrLaser)==on,"release neither toggles nor saves");
  clicked=true;sampleShortcut();
  check(bool(VrLaser)==!on&&int(VrReticleMode)==(!on?1:2),"second fresh press reverses both aids together");
  check(writeCount==2&&ticks==2,"two presses produce exactly two writes and ticks");
  const auto saved=persisted;load(saved);
  check(bool(VrLaser)==!on&&int(VrReticleMode)==(!on?1:2)&&writeCount==0,"paired choice survives production cold save/load");
  check(loadInt("config","vr.LaserWidth")==4&&std::abs(float(VrReticleMinSize)-.37f)<1e-6f&&std::abs(float(VrReticleMaxSize)-1.7f)<1e-6f&&std::abs(float(VrReticleShrinkDistance)-7.3f)<1e-6f,"laser width and all reticle dimensions retained");
  check(loadInt("config","Sentinel")==73&&loadInt("config","vr.GunAngle")==17,"unrelated saved choices retained");
 }
 std::cout<<checks<<" production paired aiming-aid shortcut/INI checks PASS\n";
}
