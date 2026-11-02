"""Compile actual reproject update/camera/profile guards on controlled TA/RAM data."""
from pathlib import Path
import os,subprocess,sys
r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
t=(r/'core/rend/vr_reproject.cpp').read_text(encoding='utf-8-sig');h=(r/'core/rend/vr_reproject.h').read_text(encoding='utf-8-sig')
def block(text,s):
 a=text.index(s);b=text.index('{',a);e=b+1;d=1
 while d:d+=(text[e]=='{')-(text[e]=='}');e+=1
 return text[a:e]
code='''#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstring>
#include <vector>
#include <map>
#include <iostream>
#include <cstdlib>
#include "rend/vr/view_settings.h"
#include "rend/vr/hud_size_settings.h"
using u32=unsigned;using u16=unsigned short;using u64=unsigned long long;
namespace config {bool VrReproject=true,VrAnimate=false,VrWidenFov=true;float VrFocal=540.8f,VrHudW=1,VrHudDepth=40,VrHudSize=1.2f,VrYaw=0,VrPitch=0,VrOffsetX=0,VrOffsetY=0,VrOffsetZ=0,VrWindowTanX=0,VrWindowTanY=0;int VrProjAddr=100,VrViewportAddr=200,VrWidenMode=2;}
struct Vertex{float x,y,z;};struct Poly{unsigned first,count;};struct rend_context{bool isRTT=false;float fZ_max=1;std::vector<Vertex> verts;std::vector<Poly> global_param_op,global_param_pt,global_param_tr;};
void getPvrFramebufferSize(const rend_context&,int& w,int& h){w=640;h=480;}
namespace xr {struct Eye{glm::mat4 viewProj{1};} eye;bool hasEye=true,service=false;const Eye* currentEye(){return hasEye?&eye:nullptr;}bool enabled(){return true;}float gameZoomFactor(){return 1.7f;}bool serviceMenuActive(){return service;}}
namespace vr {
'''+block(h,'struct ReprojectParams')+''';
ReprojectParams params;u64 frameCount=0;constexpr float NearFraction=.01f;
bool activeLargeView=true;struct CodePatch{unsigned offset;unsigned short stock,patched;};struct GameProfile{const char* gameId=" CONFIDENTIAL MISSION ---------";unsigned stockFocalAddr=0x154ed0;CodePatch fovPatch[2]{{10,20,30},{0,0,0}};} profile;const GameProfile* activeProfile=&profile;
std::map<unsigned,float> floats;std::map<unsigned,unsigned short> words;
float readFloat(unsigned address){return floats[address];}unsigned short ReadMem16_nommu(unsigned address){return words[address&0x1ffffff];}
constexpr u32 ScaleX=0,ScaleY=20;
'''+ '\n'.join(block(t,s) for s in ('static bool plausible(', 'static bool readCameraFocal(', 'static glm::vec2 readGameFocal('))+'''
bool playerInGame(int player){return player==0;}
glm::vec3 comfortParams(){return glm::vec3(0);}
void logSceneStats(const rend_context&,glm::vec2){}
bool widensByCode(){return activeProfile&&config::VrWidenFov;}
'''+block(t,'static bool profileCodeThere(')+ '\n'+block(t,'static glm::mat4 projection(')+'\n'+block(t,'const ReprojectParams& update(')+'''
}
int checks;void check(bool yes,const char* m){++checks;if(!yes){std::cerr<<"FAIL: "<<m<<"\\n";std::exit(1);}}
void camera(float focal){vr::floats[100]=focal/320.f;vr::floats[120]=focal/240.f;vr::floats[200]=320.f;vr::floats[220]=-240.f;}
int main(){using namespace vr;rend_context ctx;ctx.verts={{0,0,.5f},{640,0,.25f},{640,480,.5f}};ctx.global_param_op={{0,0},{0,3}};camera(270.f);floats[0x154ed0]=1.f/540.8f;words[10]=30;
 auto wide=update(ctx);check(wide.nativeRaster.z==1&&std::abs(wide.nativeRaster.x-320.f/540.8f)<1e-6f&&std::abs(wide.nativeRaster.y-240.f/540.8f)<1e-6f,"actual CM update uses authored live gameplay raster");check(wide.worldZoom==1.7f&&wide.overlay.z==40&&wide.tanHalf.x==320.f/270.f,"Fade preserves accepted camera/world zoom and panel depth");
 camera(400.f);floats[0x154ed0]=1.f/400.f;auto cinema=update(ctx);check(std::abs(cinema.nativeRaster.x-.4f)<1e-6f,"actual cinematic camera uses unpatched factor2 original raster");
 // Keep a valid native hit focal equal to the general rendering fallback. The
 // real reader must reject bad matrix data without inventing a cinematic lens.
 floats[0x154ed0]=1.f/config::VrFocal;
 for(unsigned address:{100u,120u,200u,220u})for(float bad:{0.f,NAN,INFINITY,-INFINITY,1e30f,1e-30f}){
  const float saved=floats[address];floats[address]=bad;glm::vec2 raw;
  check(!readCameraFocal(raw),"actual raw camera rejects invalid projection/viewport X or Y");
  check(readGameFocal()==glm::vec2(config::VrFocal),"general rendering retains calibrated focal fallback");
  const auto fallback=update(ctx);check(fallback.nativeRaster.z==0&&fallback.tanHalf==glm::vec2(320.f,240.f)/config::VrFocal,"invalid live camera keeps full fallback raster and disables Fade");floats[address]=saved;
 }
 for(int* address:{&config::VrProjAddr,&config::VrViewportAddr})for(int bad:{0,-1}){
  const int saved=*address;*address=bad;glm::vec2 raw;
  check(!readCameraFocal(raw),"missing camera matrix address is invalid");check(readGameFocal()==glm::vec2(config::VrFocal),"missing camera address keeps rendering fallback");check(update(ctx).nativeRaster.z==0,"missing camera address cannot enable Fade");*address=saved;
 }
 floats[0x154ed0]=1.f/400.f;
 config::VrWidenMode=1;check(update(ctx).nativeRaster.z==0,"On mode retains all wider geometry with no Fade");config::VrWidenMode=0;check(update(ctx).nativeRaster.z==0,"Off never enables Fade");config::VrWidenMode=2;
 activeLargeView=false;check(update(ctx).nativeRaster.z==0,"saved Fade waits for startup-latched wide render path");activeLargeView=true;
 xr::service=true;check(update(ctx).nativeRaster.z==0,"native Service stays unfaded");xr::service=false;
 words[10]=99;check(update(ctx).nativeRaster.z==0,"unsupported native code cannot enable Fade");words[10]=20;check(update(ctx).nativeRaster.z==0,"unpatched wide camera cannot enable Fade");words[10]=30;
 for(float bad:{0.f,INFINITY,NAN}){floats[0x154ed0]=bad;check(update(ctx).nativeRaster.z==0,"invalid native inverse focal disables Fade");}floats[0x154ed0]=1.f/400.f;
 profile.gameId="unsupported";check(update(ctx).nativeRaster.z==0,"other game profiles remain unaffected");profile.gameId=" CONFIDENTIAL MISSION ---------";
 ctx.isRTT=true;check(update(ctx).nativeRaster.z==0&&update(ctx).worldZoom==1,"RTT resets Fade and zoom");ctx.isRTT=false;
 config::VrReproject=false;check(update(ctx).nativeRaster.z==0,"flat original reprojection resets Fade");config::VrReproject=true;
 xr::hasEye=false;check(update(ctx).nativeRaster.z==0,"non-XR debug passes never inherit headset Fade");xr::hasEye=true;
 for(auto& v:ctx.verts)v.z=1;check(update(ctx).nativeRaster.z==0,"actual all-HUD native frame has no scene Fade");
 std::cout<<checks<<" production reproject Fade guard/camera checks passed\\n";
}
'''
(o/'reproject-fade-test.cpp').write_text(code)
e=dict(os.environ);e['PATH']='D:/msys64/mingw64/bin;'+e.get('PATH','');b=o/'reproject-fade-test.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(r/'core'),'-I'+str(r/'core/deps/glm'),str(o/'reproject-fade-test.cpp'),'-o',str(b)],env=e,check=True)
sys.exit(subprocess.run([str(b)],env=e).returncode)
