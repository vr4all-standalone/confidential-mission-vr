#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include "hands_rip.h"
using u32=uint32_t;using u8=uint8_t;
#define NOTICE_LOG(...) ((void)0)
#define INFO_LOG(...) ((void)0)
#define WARN_LOG(...) ((void)0)
namespace config{bool VrGameHands=true,VrHandsPrep=true,GGPOEnable=false;}
static struct{struct{bool fastForwardMode=false;}input;struct{std::string path;}content;}settings;
static std::set<std::string> files,valid;
static double clockNow;
static bool xrEnabled=true,simulator=false,writeOk=true;
static int writes,neutralInputs;
static bool file_exists(const std::string& p){return files.count(p);}
static std::string get_writable_data_path(const char* p){return p;}
static void lightgunSet(int,int,int,int buttons){if(!buttons)++neutralInputs;}
namespace vr{static hands::Source source;const hands::Source* gameModelSource(){return &source;}
namespace xr{bool enabled(){return xrEnabled;}bool completeModelFile(const std::string& p){return valid.count(p);}namespace sim{bool active(){return simulator;}}}
namespace hands{
#include "prep-paths.inc"
namespace{
double gameSeconds(){return clockNow;}
bool save(const std::vector<u8>&,const std::string& p){++writes;if(writeOk){files.insert(p);valid.insert(p);}return writeOk;}
void finishMerged(const GunTarget&,int,std::vector<Rip>,std::vector<struct CachedTexture>);
#include "prep-production.inc"
void finishMerged(const GunTarget&,int,std::vector<Rip>,std::vector<CachedTexture>){}
static int checks;
static void check(bool yes,const char* text){++checks;if(!yes){std::cerr<<"FAIL "<<text<<'\n';std::exit(1);}}
static void reset(){state=Off;files.clear();valid.clear();clockNow=0;xrEnabled=true;simulator=false;writeOk=true;writes=neutralInputs=0;config::VrGameHands=config::VrHandsPrep=true;config::GGPOEnable=false;settings.input.fastForwardMode=false;source=Source{};source.kind=Source::CmGuns;source.guns[1].file="pistol.bin";source.guns[1].name="pistol";source.guns[1].merge=true;}
int run(){
 reset();config::VrGameHands=false;config::VrHandsPrep=false;xrEnabled=false;simulator=true;onStart();
 check(preparing(),"CM extraction is required despite old GameHands/HandsPrep opt-outs and simulator mode");
 skipPrep();check(!skipRequested,"CM skip API cannot cancel required extraction");clockNow=1;onVBlank();check(preparing(),"skip cannot release preparation input gate");
 reset();files.insert("pistol.bin");onStart();check(preparing(),"damaged/incomplete cached pistol triggers extraction");
 reset();files.insert("pistol.bin");valid.insert("pistol.bin");onStart();check(!preparing(),"valid extracted pistol skips repeated preparation");
 reset();config::GGPOEnable=true;onStart();check(preparing()&&prepFailed()&&!prepFailure().empty(),"online state cannot bypass first extraction and reports recovery");
 reset();onStart();for(auto& x:gunOver)x=true;clockNow=500;onVBlank();
 check(preparing()&&prepFailed(),"failed extraction stays blocked instead of entering gameplay");
 check(!settings.input.fastForwardMode&&neutralInputs>0,"failure restores clock/input state");skipPrep();check(preparing(),"failure cannot be skipped with B");
 reset();onStart();const std::vector<u8> bytes{1,2,3};
 check(!saveGun(source.guns[1],1,bytes,30,false,nullptr)&&writes==0&&!gunDone[1],"incomplete required pistol is not accepted as finished");
 writeOk=false;check(!saveGun(source.guns[1],1,bytes,30,true,nullptr)&&!gunDone[1],"disk failure cannot complete required extraction");
 writeOk=true;check(saveGun(source.guns[1],1,bytes,30,true,nullptr),"complete pistol saves through original model writer");
 for(auto& x:gunOver)x=true;clockNow=500;onVBlank();check(!preparing()&&!prepFailed()&&prepProgress()==1.f,"completed validated cache releases gameplay");
 reset();source.kind=Source::AgentsPistol;source.file="hands.bin";onStart();check(preparing(),"other legacy preparation still starts");skipPrep();onVBlank();check(!preparing(),"other game workflows retain original optional behavior");
 std::cout<<checks<<" production required-CM-preparation checks PASS\n";return 0;
}
}}}
int main(){return vr::hands::run();}
