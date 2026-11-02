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
namespace hostfs {class File;}
#include "view-settings.inc"
static int checks;
static void check(bool ok,const char* label){++checks;if(!ok){std::cerr<<"FAIL: "<<label<<'\n';std::exit(1);}}
static void restart(const std::string& text){config::Settings::instance().reset();config::cfgdb=config::IniFile();config::cfgdb.load(text);config::persisted=text;config::writeCount=0;config::Settings::instance().load(false);}
int main(){
 check(HAS_VIEW_SETTINGS,"production options expose laser widths and Wider view Fade");
#if HAS_VIEW_SETTINGS
 using namespace config;
 restart("[config]\nvr.Laser=no\nvr.Widen=no\nvr.WorldScale=.123456\nvr.HudDepth=43.75\nSentinel=73\n");
 check(!VrLaser&&int(VrLaserWidth)==1,"legacy LaserOff preserved with missing width Normal");
 check(!VrWiden&&int(VrWidenMode)==0,"legacy WiderOff migrates into Off");
 check(float(VrWorldScale)==.123456f&&float(VrHudDepth)==43.75f,"migration retains exact numeric world preferences");
 check(writeCount==0,"load/migration has no unrelated save side effects");
 for(int mode=0;mode<=2;++mode){restart("[config]\nvr.Widen=yes\nvr.WidenMode="+std::to_string(mode)+"\n");check(int(VrWidenMode)==mode&&bool(VrWiden)==(mode!=0),"explicit mode overrides legacy bool");Settings::instance().save();check(loadBool("config","vr.Widen")==bool(mode!=0),"saving mode synchronizes compatibility bool");const auto saved=persisted;restart(saved);check(int(VrWidenMode)==mode&&bool(VrWiden)==bool(mode!=0),"Wider modes survive fresh INI restart");}
 for(int width=1;width<=4;++width){restart("[config]\nvr.Laser=no\nvr.LaserWidth="+std::to_string(width)+"\n");check(int(VrLaserWidth)==width&&!VrLaser,"explicit width persists while LaserOff");Settings::instance().save();const auto saved=persisted;restart(saved);check(int(VrLaserWidth)==width&&!VrLaser,"all four widths survive save/load");}
 for(const char* bad:{"","0","5","-1","junk","2junk","2.5","nan","9999999999999999999"}){restart(std::string("[config]\nvr.LaserWidth=")+bad+"\n");check(int(VrLaserWidth)==1,"invalid width falls back Normal");}
 for(const char* bad:{"","-1","3","junk","2junk","2.5","nan","9999999999999999999"}){restart(std::string("[config]\nvr.Widen=no\nvr.WidenMode=")+bad+"\n");check(int(VrWidenMode)==1&&bool(VrWiden),"invalid explicit mode uses compiled On default");}
 restart("[config]\nvr.Widen=yes\n");check(int(VrWidenMode)==1,"missing mode derives legacy On");
 VrWidenMode.set(2);check(bool(VrWiden),"runtime Fade enables existing wide path");VrWidenMode.set(0);check(!VrWiden,"runtime Off disables future wide startup");VrWidenMode.reset();VrLaserWidth.reset();VrLaser.reset();Settings::instance().save();restart(persisted);check(int(VrWidenMode)==1&&bool(VrWiden)&&int(VrLaserWidth)==1&&bool(VrLaser),"reset and restart restore WiderOn and Normal laser");
 restart("[config]\nvr.WidenMode=2\nvr.LaserWidth=4\n[game-a]\nconfig.vr.Widen=no\nconfig.vr.WidenMode=0\nconfig.vr.LaserWidth=3\n");Settings::instance().setGameId("game-a");Settings::instance().load(true);check(int(VrWidenMode)==2&&bool(VrWiden)&&int(VrLaserWidth)==4,"global menu choices remain authoritative after per-game load");
 const float multipliers[]={1,2,5,10};for(int i=0;i<4;++i){check(vr::laser::multiplier(i+1)==multipliers[i],"exact laser thickness multipliers");check(vr::laser::halfWidth(i+1)==.0008f*multipliers[i],"Normal beam half-width stays calibrated .0008m");}
 check(vr::view::nativeFocal(540.8f,270.f,true)==540.8f,"gameplay original raster follows live hit-test focal");check(vr::view::nativeFocal(400.f,400.f,true)==800.f,"cinematic equal widened focal undoes measured lens factor2");check(vr::view::nativeFocal(400.f,400.f,false)==400.f,"unpatched camera does not undo lens");check(vr::view::nativeFocal(0,400,true)==0,"invalid camera disables Fade framing");
 for(bool service:{false,true})for(bool native3D:{false,true})for(bool wide:{false,true})check(vr::view::fadeActive(2,wide,service,native3D)==(wide&&!service&&native3D),"Fade excludes native Service and flat/fallback frames");
 std::cout<<checks<<" production view Option/INI/policy checks passed\n";
#endif
}
