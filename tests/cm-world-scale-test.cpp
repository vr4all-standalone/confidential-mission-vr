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
#define stricmp _stricmp
namespace hostfs{class File;}
#include "under_test.inc"
#include "reset-captured.inc"
static int checks;
static void check(bool yes,const char* why){++checks;if(!yes){std::cerr<<"FAIL "<<why<<'\n';std::exit(1);}}
static bool near(float a,float b){return std::abs(a-b)<1e-5f;}
static void load(const std::string& text){config::Settings::instance().reset();config::cfgdb=config::IniFile();config::cfgdb.load(text);config::persisted=text;config::writeCount=0;config::Settings::instance().load(false);}
int main(){using namespace config;
 for(const char* old:{".42",".11",".01","1.7","nan","bogus",""}){
  load(std::string("[config]\nvr.WorldScale=")+old+"\nvr.WorldScaleDefaultVersion=99\nSentinel=73\nvr.ReticleMode=2\nvr.ReticleMinSize=.37\nvr.GunAngle=17\n");
  check(near(VrWorldScale,.10f),"every pre-update World size resets to0.10");
  check(near(loadFloat("config","vr.WorldScale"),.10f)&&loadInt("config","vr.CmWorldSizeMigration")==1,"migration and new value reach persisted global keys");
  check(writeCount==1,"migration writes the paired value/marker once");
  check(loadInt("config","Sentinel")==73&&loadInt("config","vr.GunAngle")==17&&int(VrReticleMode)==2&&near(VrReticleMinSize,.37f),"migration preserves unrelated settings");
  const auto migrated=persisted;load(migrated);
  check(near(VrWorldScale,.10f)&&writeCount==0,"cold restart does not repeat migration");
 }
 load("[config]\nvr.WorldScale=.42\nSentinel=73\n[confmiss]\nconfig.vr.WorldScale=.07\nconfig.Sentinel=82\n");
 Settings::instance().setGameId("confmiss");Settings::instance().load(true);
 check(near(VrWorldScale,.10f),"old game-specific value cannot resurrect after global migration");
 check(loadInt("confmiss","config.Sentinel")==82&&near(loadFloat("confmiss","config.vr.WorldScale"),.07f),"unrelated game settings retained; stale world key is ignored without deleting the section");
 VrWorldScale.set(.14f);Settings::instance().save();
 check(near(loadFloat("config","vr.WorldScale"),.14f),"later menu-style edit saves globally even with native game selected");
 const auto custom=persisted;load(custom);Settings::instance().setGameId("confmiss");Settings::instance().load(true);
 check(near(VrWorldScale,.14f)&&writeCount==0,"post-update custom size survives cold global/game loads");
 for(int n=0;n<20;n++)Settings::instance().load(false);
 check(near(VrWorldScale,.14f)&&writeCount==0,"repeated settings loads preserve later custom size");
 resetCaptured();check(near(VrWorldScale,.10f)&&loadInt("config","vr.CmWorldSizeMigration")==1,"Reset uses0.10 without rearming migration");
 VrWorldScale.set(.16f);saveFloat("config","vr.WorldScale",VrWorldScale);const auto afterReset=persisted;load(afterReset);
 check(near(VrWorldScale,.16f)&&writeCount==0,"custom edit after Reset remains saved");
 load("[config]\nvr.CmWorldSizeMigration=2\nvr.WorldScale=.15\n");
 check(near(VrWorldScale,.15f)&&writeCount==0,"newer migration marker is not downgraded or reset");
 std::cout<<checks<<" production CM world migration/persistence/reset checks PASS\n";
}
