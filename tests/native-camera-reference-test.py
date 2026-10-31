from pathlib import Path
import subprocess,os,sys
r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
source=(r/'core/rend/vr_reproject.cpp').read_text(encoding='utf8')
def block(t,s):
 a=t.index(s);b=t.index('{',a);e=b+1;d=1
 while d:d+=(t[e]=='{')-(t[e]=='}');e+=1
 return t[a:e]
code='''#include <glm/glm.hpp>
#include <iostream>
#include <cmath>
#include <map>
#include <cstring>
#include <fstream>
#include <vector>
#include <limits>
#include "game_zoom.h"
namespace zoom=vr::zoom;
using u32=unsigned;constexpr unsigned ScaleX=0,ScaleY=20;
namespace config{float VrFocal=540.8f;unsigned VrProjAddr=100,VrViewportAddr=200;}
std::map<unsigned,float> ram;std::map<unsigned,unsigned> codeRam;
float readFloat(unsigned a){return ram[a];}unsigned ReadMem32_nommu(unsigned a){return codeRam[a&0x1ffffff];}
struct Patch{unsigned offset=10;unsigned short patched=20;};struct GameProfile{const char*gameId=" CONFIDENTIAL MISSION ---------";unsigned stockFocalAddr=300;Patch fovPatch[1];}profile;
const GameProfile*activeProfile=&profile;bool activeLargeView=true,codeThere=true;
bool profileCodeThere(const GameProfile&){return codeThere;}bool widensByCode(){return true;}unsigned short ReadMem16_nommu(unsigned){return 20;}
'''
for sig in ['static bool plausible(','static bool readCameraFocal(','static glm::vec2 readGameFocal()','float nativeCameraZoom()']:code+=block(source,sig)+'\n'
code+='''static int checks;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"FAIL " #x " line "<<__LINE__<<"\\n";return 1;}}while(0)
int main(int argc,char**argv){
if(argc>1){std::ifstream file(argv[1],std::ios::binary);std::vector<unsigned char>data((std::istreambuf_iterator<char>(file)),{});CHECK(data.size()>=0xe0000);for(unsigned a=0x20000;a<0xe0000;a+=4){unsigned value;memcpy(&value,&data[a],4);codeRam[a]=value;}}
else codeRam={{0x27314,0xd25ac759},{0x27318,0xd357d15a},{0x2731c,0xf308f3fa},{0x27324,0xf13af23a},{0x2747c,0x42200000},{0x27480,0x0c154ed4},{0x27484,0x0c154ec8},{0x27494,0x43a00000}};
ram[100]=436.4f/320;ram[120]=436.4f/240;ram[200]=320;ram[220]=-240;ram[300]=1.f/879.19277f;
CHECK(std::abs(nativeCameraZoom()-1.f)<1e-5f); // native40deg must be neutral even with presentation540.8
config::VrFocal=320;CHECK(std::abs(nativeCameraZoom()-1.f)<1e-5f);config::VrFocal=1200;CHECK(std::abs(nativeCameraZoom()-1.f)<1e-5f);
ram[300]=1.f/1327.1f;const auto zoomed=nativeCameraZoom();CHECK(zoomed>1.50f&&zoomed<1.52f);
ram[300]=1.f/879.19277f;CHECK(std::abs(nativeCameraZoom()-1.f)<1e-5f);
ram[300]=1.f/540.8f;CHECK(nativeCameraZoom()>.61f&&nativeCameraZoom()<.62f);
ram[300]=1.f/879.19277f;const auto literal=codeRam[0x2747c];codeRam[0x2747c]^=1;CHECK(nativeCameraZoom()==1);codeRam[0x2747c]=literal;
for(auto a:{0x27314u,0x27318u,0x2731cu,0x27324u,0x27480u,0x27484u,0x27494u}){auto saved=codeRam[a];codeRam[a]^=1;ram[300]=1.f/1327.1f;CHECK(nativeCameraZoom()==1);codeRam[a]=saved;}
std::cout<<checks<<" native reset-FOV / zoom-and-return / independent presentation checks PASS\\n";}
'''
(o/'camera.cpp').write_text(code,encoding='utf8');env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','');exe=o/'camera.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(r/'core/deps/glm'),'-I'+str(r/'core/rend/vr'),str(o/'camera.cpp'),'-o',str(exe)],check=True,env=env)
sys.exit(subprocess.run([str(exe),*sys.argv[3:]],env=env).returncode)
