"""Execute production per-polygon scaling and exact-frame cinematic policies."""
from pathlib import Path
import os,re,subprocess,sys
r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
v=(r/'core/rend/vr_reproject.cpp').read_text(encoding='utf-8-sig')
g=(r/'core/rend/gles/gldraw.cpp').read_text(encoding='utf-8-sig')
h=(r/'core/rend/vr/xr_host.cpp').read_text(encoding='utf-8-sig')
def block(t,s):
 a=t.index(s);b=t.index('{',a);d=1;e=b+1
 while d:d+=(t[e]=='{')-(t[e]=='}');e+=1
 return t[a:e]
call=re.search(r'glUniform1f\(CurrentShader->vrHudSize,(vr::hudScaleForPolygon\(.*?\))\);',g)[1]
code='''#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <iterator>
#include <limits>
#include <vector>
#include <glm/glm.hpp>
#include "game_zoom.h"
using u32=unsigned;using u64=unsigned long long;
int checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"FAIL " #x " line "<<__LINE__<<'\\n';return 1;}}while(0)
struct PolyParam{struct{bool Texture=true;}pcw;struct{unsigned TexAddr=0;}tcw;unsigned first=0,count=4;bool vrSight=false;};
namespace vr {
struct GameProfile{const char*gameId=" CONFIDENTIAL MISSION ---------";unsigned sightTextures[8]{0x400000};unsigned letterboxTextures[4]{0x400680};float letterboxW=.2f;};
GameProfile profile;const GameProfile*activeProfile=&profile;u32 markerTextures[4]{0x5e8000};
'''+block(v,'float hudScaleForPolygon(')+'''
}
namespace TileClipping{constexpr int Off=0,Inside=1;}
struct{float vrHudSize=1.4f,vrWorldZoom=.615f;}ShaderUniforms;
float drawScale(const PolyParam*gp,int clipmode=0){return '''+call+''';}
int main(){PolyParam p;p.tcw.TexAddr=0x400000>>3;
CHECK(drawScale(&p)==1.f); // shared texture alone does not prove a relocated enemy marker
p.vrSight=true;
CHECK(std::abs(drawScale(&p)-.615f)<1e-6f);
ShaderUniforms.vrWorldZoom=1.f;CHECK(drawScale(&p)==1.f);
p.vrSight=false;
p.tcw.TexAddr=0x5e8000>>3;CHECK(drawScale(&p)==1.f);
p.tcw.TexAddr=0x44c800>>3;CHECK(drawScale(&p)==1.4f);CHECK(drawScale(&p,TileClipping::Inside)==1.f);
p.pcw.Texture=false;CHECK(drawScale(&p)==1.4f);
std::cout<<checks<<" actual scene-marker/HUD draw-policy checks PASS\\n";}
'''
e=dict(os.environ);e['PATH']='D:/msys64/mingw64/bin;'+e.get('PATH','')
cpp=o/'scene.cpp';cpp.write_text(code,encoding='utf8');exe=o/'scene.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(r/'core/deps/glm'),'-I'+str(r/'core/rend/vr'),str(cpp),'-o',str(exe)],env=e,check=True)
failed=subprocess.run([str(exe)],env=e).returncode
refresh=block(h,'void refreshGameZoom(') if 'void refreshGameZoom(' in h else 'void refreshGameZoom(){'+h[h.index('\tframeWorldZoom=',h.index('void updateLightgun(')):].split('\n',1)[0]+'}'
policy='''#include <iostream>
#include "game_zoom.h"
namespace zoom=vr::zoom;
int checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"FAIL " #x " line "<<__LINE__<<'\\n';return 1;}}while(0)
zoom::Session zoomSession;float frameWorldZoom=1.f;unsigned long long sequence=1;
struct rend_context{bool bars=true;}ctx;
const rend_context*gles_vr_frame(){return &ctx;}unsigned long long gles_vr_frame_number(){return sequence;}
bool zoomCutsceneFrame(const rend_context& c){return c.bars;}
bool service=false;
bool gameZoomOn(){return zoomSession.enabled();}bool serviceMenuActive(){return service;}float nativeCameraZoom(){return .615f;}
'''+refresh+'''
int main(){
// Cold creation and a grip held through startup cannot opt in.
CHECK(!gameZoomOn());std::array<zoom::Hand,2> hands{};for(auto& hand:hands){hand.tracked=true;hand.grip=true;}
for(int i=0;i<8;i++)zoomSession.update(hands,true,0);
for(auto& hand:hands)hand.grip=false;zoomSession.update(hands,true,0);CHECK(!gameZoomOn());
zoomSession.set(true,2);refreshGameZoom();CHECK(frameWorldZoom==1.f&&gameZoomOn()&&zoomSession.paused());
ctx.bars=false;for(int i=0;i<500;i++)refreshGameZoom();CHECK(frameWorldZoom==1.f);
// Two native bar-free frames are a documented cinematic blink, then bars return.
for(int i=0;i<2;i++){++sequence;refreshGameZoom();CHECK(frameWorldZoom==1.f);}
ctx.bars=true;++sequence;refreshGameZoom();CHECK(frameWorldZoom==1.f);
ctx.bars=false;for(int i=0;i<5;i++){++sequence;refreshGameZoom();CHECK(frameWorldZoom==1.f);for(int eye=0;eye<5;eye++)refreshGameZoom();CHECK(frameWorldZoom==1.f);}
++sequence;refreshGameZoom();CHECK(frameWorldZoom==.615f&&gameZoomOn()&&!zoomSession.paused());
// Explicit Off during a cinematic cancels the requested On, including later restoration.
ctx.bars=true;++sequence;refreshGameZoom();zoomSession.set(false,3);refreshGameZoom();CHECK(frameWorldZoom==1.f&&!gameZoomOn());
ctx.bars=false;for(int i=0;i<7;i++){++sequence;refreshGameZoom();CHECK(frameWorldZoom==1.f&&!gameZoomOn());}
// Explicit On during a cinematic waits until the authored cinematic has ended.
ctx.bars=true;++sequence;refreshGameZoom();zoomSession.set(true,4);refreshGameZoom();CHECK(frameWorldZoom==1.f&&gameZoomOn());
ctx.bars=false;for(int i=0;i<6;i++){++sequence;refreshGameZoom();}CHECK(frameWorldZoom==.615f&&gameZoomOn());
service=true;refreshGameZoom();CHECK(frameWorldZoom==1.f&&gameZoomOn());service=false;refreshGameZoom();CHECK(frameWorldZoom==.615f);
zoomSession.reset();refreshGameZoom();CHECK(frameWorldZoom==1.f&&!gameZoomOn()&&!zoomSession.paused());
std::cout<<checks<<" production cinematic/startup/explicit-choice lifecycle checks PASS\\n";}
'''
cpp=o/'cinematic.cpp';cpp.write_text(policy,encoding='utf8');exe=o/'cinematic.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(r/'core/rend/vr'),str(cpp),'-o',str(exe)],env=e,check=True)
failed|=subprocess.run([str(exe)],env=e).returncode
scan=block(v,'static bool nativeZoomCutsceneBars(') if 'static bool nativeZoomCutsceneBars(' in v else ''
capture=v[v.index('    ctx.vrZoomCutscene = ctx.vrZoomCutscene'):v.index(';',v.index('    ctx.vrZoomCutscene = ctx.vrZoomCutscene'))+1] if scan else ''
barcode=code[:code.index('namespace TileClipping')].replace('namespace vr {','''struct Vertex{float x,y,z;unsigned char col[4]{255,255,255,255};};
struct rend_context{bool isRTT=false,vrZoomCutscene=false;std::vector<Vertex> verts;std::vector<PolyParam> global_param_op,global_param_pt,global_param_tr;};
namespace config{float VrHudW=1;}
namespace vr {
''').replace(block(v,'float hudScaleForPolygon('),block(v,'static bool onOverlay(')+'\n'+scan+'\n'+block(v,'bool zoomCutsceneFrame('))
barcode+='''
namespace vr {void capture(rend_context& ctx){struct Pass{unsigned op_count=0,pt_count=0,tr_count=0;}previousPass,pass;pass.op_count=ctx.global_param_op.size();pass.pt_count=ctx.global_param_pt.size();pass.tr_count=ctx.global_param_tr.size();int fbWidth=640,fbHeight=480;
'''+capture+'''}}
bool observed(rend_context& c){vr::capture(c);return vr::zoomCutsceneFrame(c);}
int main(){rend_context c;PolyParam p;p.tcw.TexAddr=0x400680>>3;c.global_param_tr={p};
c.verts={{0,0,5},{640,0,5},{0,480,5},{640,480,5}};
CHECK(!observed(c)); // gameplay's fullscreen fade uses the same texture/depth
c.verts={{0,0,5},{640,0,5},{0,64,5},{640,64,5}};CHECK(observed(c));
for(auto& v:c.verts)v.x=v.y=NAN;CHECK(vr::zoomCutsceneFrame(c));
c.vrZoomCutscene=false;CHECK(!vr::zoomCutsceneFrame(c));
c.verts={{0,416,5},{640,416,5},{0,480,5},{640,480,5}};CHECK(observed(c));
c.vrZoomCutscene=false;for(auto& v:c.verts)v.col[3]=0;CHECK(!observed(c));
for(auto& v:c.verts)v.col[3]=25;CHECK(observed(c)); // opening fade is immediate
c.vrZoomCutscene=false;for(auto& v:c.verts)v.z=1;CHECK(!observed(c));
for(auto& v:c.verts)v.z=5;c.isRTT=true;CHECK(!observed(c));c.isRTT=false;
vr::profile.gameId="other";CHECK(!observed(c));vr::profile.gameId=" CONFIDENTIAL MISSION ---------";
c.global_param_tr[0].tcw.TexAddr=0x404800>>3;CHECK(!observed(c));
c.global_param_tr[0].tcw.TexAddr=0x400680>>3;c.global_param_tr[0].count=3;CHECK(!observed(c));
std::cout<<checks<<" native letterbox-versus-fullscreen-fade checks PASS\\n";}
'''
cpp=o/'bars.cpp';cpp.write_text(barcode,encoding='utf8');exe=o/'bars.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(r/'core/deps/glm'),'-I'+str(r/'core/rend/vr'),str(cpp),'-o',str(exe)],env=e,check=True)
failed|=subprocess.run([str(exe)],env=e).returncode
# Execute the real native-present switch; eyes, menus, framebuffer updates and
# no-op Present calls must never advance the scene number used by the policy.
gles=(r/'core/rend/gles/gles.cpp').read_text(encoding='utf-8-sig')
present='''#include <iostream>
#include <utility>
using u64=unsigned long long;
namespace vr::xr {bool active=true;bool enabled(){return active;}}
enum class VrPending {None,Frame,Framebuffer};
VrPending vrPendingKind=VrPending::None;int vrFrame=0,vrPending=0,vrFramesNotShown=7;
bool vrFrameValid=false,vrShowFramebuffer=false,vrFramebufferBlank=false,vrPendingBlank=false;u64 vrFrameNumber=0;
'''+block(gles,'void gles_vr_present(')+'\n'+block(gles,'u64 gles_vr_frame_number(')+'''
int main(){gles_vr_present();if(gles_vr_frame_number()!=0)return 1;
vrPendingKind=VrPending::Frame;vrPending=42;gles_vr_present();if(gles_vr_frame_number()!=1||vrFrame!=42||!vrFrameValid)return 2;
for(int i=0;i<500;i++)gles_vr_present();if(gles_vr_frame_number()!=1)return 3;
vrPendingKind=VrPending::Framebuffer;gles_vr_present();if(gles_vr_frame_number()!=1)return 4;
vr::xr::active=false;vrPendingKind=VrPending::Frame;gles_vr_present();if(gles_vr_frame_number()!=1)return 5;
vr::xr::active=true;gles_vr_present();if(gles_vr_frame_number()!=2)return 6;
std::cout<<"6 native frame-presentation generation checks PASS\\n";}
'''
cpp=o/'present.cpp';cpp.write_text(present,encoding='utf8');exe=o/'present.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2',str(cpp),'-o',str(exe)],env=e,check=True)
failed|=subprocess.run([str(exe)],env=e).returncode
sys.exit(failed)
