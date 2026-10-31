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
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"FAIL " #x " line "<<__LINE__<<'\n';return 1;}}while(0)
bool near(float a,float b){return std::abs(a-b)<1e-5f;}
void load(const std::string& text){config::Settings::instance().reset();config::cfgdb=config::IniFile();config::cfgdb.load(text);config::persisted=text;config::writeCount=0;config::Settings::instance().load(false);}
int main(){using namespace config;
load("[config]\nSentinel=73\n");CHECK(near(VrReticleMinSize,.20f)&&near(VrReticleMaxSize,.45f)&&near(VrReticleShrinkDistance,6.5f));
for(const char* bad:{"bogus","nan","inf","-1","999999"}){load(std::string("[config]\nvr.ReticleMinSize=")+bad+"\nvr.ReticleMaxSize="+bad+"\nvr.ReticleShrinkDistance="+bad+"\nSentinel=73\n");CHECK(near(VrReticleMinSize,.20f)&&near(VrReticleMaxSize,.45f)&&near(VrReticleShrinkDistance,6.5f));}
load("[config]\nSentinel=73\nvr.ReticleMinSize=.37\nvr.ReticleMaxSize=1.6\nvr.ReticleShrinkDistance=12.3\n");CHECK(near(VrReticleMinSize,.37f)&&near(VrReticleMaxSize,1.6f)&&near(VrReticleShrinkDistance,12.3f));
Settings::instance().save();const auto saved=persisted;load(saved);CHECK(near(VrReticleMinSize,.37f)&&near(VrReticleMaxSize,1.6f)&&near(VrReticleShrinkDistance,12.3f));
resetCaptured();CHECK(near(VrReticleMinSize,.20f)&&near(VrReticleMaxSize,.45f)&&near(VrReticleShrinkDistance,6.5f));CHECK(loadInt("config","Sentinel")==73);const auto reset=persisted;load(reset);CHECK(near(VrReticleMinSize,.20f)&&near(VrReticleMaxSize,.45f)&&near(VrReticleShrinkDistance,6.5f));
CHECK(vr::reticle::minimum(0)==.05f&&vr::reticle::maximum(99)==3.f&&vr::reticle::distance(99)==20.f);
std::cout<<checks<<" captured CM defaults/save/load/reset checks PASS\n";
}
