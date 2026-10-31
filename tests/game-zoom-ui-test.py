from pathlib import Path
import re,os,subprocess,sys
r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
h=(r/'core/rend/vr/xr_host.cpp').read_text(encoding='utf-8-sig');m=(r/'core/rend/vr/xr_menu.cpp').read_text(encoding='utf-8-sig');labels=(r/'core/rend/vr/xr_labels.cpp').read_text(encoding='utf-8-sig')
def block(t,s):
 a=t.index(s);b=t.index('{',a);d=1;e=b+1
 while d:d+=(t[e]=='{')-(t[e]=='}');e+=1
 return t[a:e]
enum=re.search(r'enum Row[^\n]+',(r/'core/rend/vr/xr_menu.h').read_text())[0]
menuValue=re.search(r'case GameZoom:(.*?)case HowToPlay:',m[m.index('std::string value('):],re.S)[1]
choose=re.search(r'case GameZoom:(.*?)case HowToPlay:',m[m.index('void change('):],re.S)[1]
notice=block(h,'if(focused&&!darkRoom&&!serviceMenuActive()&&zoomSession.notice(')
code='''#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include "game_zoom.h"
#include "xr_theme.h"
namespace theme=vr::xr::theme;
vr::zoom::Session zoomSession;float frameWorldZoom=1;double now=2;
bool service=false,focused=true,darkRoom=false;int draws=0;std::string text;glm::vec3 colour;
double zoomSeconds(){return now;}bool serviceMenuActive(){return service;}float nativeCameraZoom(){return 2;}
'''
for sig in ('bool gameZoomOn()','void setGameZoom(','float gameZoomFactor()'):code+=block(h,sig)+'\n'
code+=enum+'\nstd::string value(){switch(GameZoom){case GameZoom:'+menuValue+'}return "";}\n'
code+='void choose(){switch(GameZoom){case GameZoom:'+choose+'}}\n'
code+='''unsigned textTexture(const char* t,float,const glm::vec3& c,int& w,int& h){text=t;colour=c;w=300;h=50;return 1;}
glm::mat4 headPose(){return glm::mat4(1);}glm::mat4 eyeProjection(int){return glm::mat4(1);}
void drawTexture(const glm::mat4&,const glm::mat4&,unsigned,float,float){++draws;}
struct {int fov=0;} views[2];glm::mat4 view(1);int i=0;
void draw(){'''+notice+'''}
static int checks;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"FAIL " #x "\\n";return 1;}}while(0)
int main(){CHECK(GameZoom==WiderView+1&&HowToPlay==GameZoom+1);CHECK(value()=="Off");choose();CHECK(value()=="On"&&gameZoomFactor()==2);draw();CHECK(draws==1&&text=="Zoom enabled"&&colour==theme::Accent);
now=2.999;draw();CHECK(draws==2);now=3;draw();CHECK(draws==2);
choose();draw();CHECK(draws==3&&text=="Zoom disabled"&&gameZoomFactor()==1);
focused=false;draw();CHECK(draws==3);focused=true;service=true;draw();CHECK(draws==3&&gameZoomFactor()==1);service=false;
now=4;choose();zoomSession.suspend(false);draw();CHECK(draws==4);zoomSession.suspend();draw();CHECK(draws==4);
std::cout<<checks<<" actual menu and feedback drawing-policy checks PASS\\n";}
'''
assert labels.count('controllers::Grip, "Grip - Toggle zoom"')==16
assert 'list[i].button != controllers::Grip' in labels and 'planeX(at[i])-side*Gap' in labels
assert 'setGameZoom(false);' in block(m,'void resetMenuSettings()')
assert 'vr.GameZoom' not in m and 'vr.GameZoom' not in (r/'core/cfg/option.cpp').read_text()
(o/'ui.cpp').write_text(code,encoding='utf8');binary=o/'ui.exe';env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','')
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(r/'core/deps/glm'),'-I'+str(r/'core/rend/vr'),str(o/'ui.cpp'),'-o',str(binary)],check=True,env=env)
sys.exit(subprocess.run([str(binary)],env=env).returncode)
