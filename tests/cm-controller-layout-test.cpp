#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "xr_controllers.h"
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "imgui/imstb_truetype.h"
stbtt_fontinfo font;
namespace vr::xr {
unsigned textTexture(const std::string& s,float px,const glm::vec3&,int& w,int& h){
 float width=0,scale=stbtt_ScaleForPixelHeight(&font,px);for(size_t i=0;i<s.size();i++){int a,b;stbtt_GetCodepointHMetrics(&font,(unsigned char)s[i],&a,&b);width+=a*scale;if(i+1<s.size())width+=stbtt_GetCodepointKernAdvance(&font,(unsigned char)s[i],(unsigned char)s[i+1])*scale;}
 w=int(std::ceil(width))+int(std::lround(px*.4f))*2;h=int(std::lround(px*1.45f));return 1;
}
namespace labels {
#include "layout.inc"
}}
int main(int argc,char**argv){
 std::ifstream f(argv[1],std::ios::binary);std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(f),{}};if(bytes.empty()||!stbtt_InitFont(&font,bytes.data(),0))return 2;
 using namespace vr::xr::labels;int checks=0,failures=0;
 auto check=[&](bool good,const char*what){checks++;if(!good){failures++;if(failures<12)std::cerr<<"FAIL "<<what<<'\n';}};
 for(const auto& words:Kinds)for(int hand=0;hand<2;hand++)for(int pose=0;pose<15;pose++){
  const float side=hand?1.f:-1.f;
  auto grip=glm::translate(glm::mat4(1),glm::vec3(side*.22f,-.25f,-.5f))*glm::rotate(glm::mat4(1),float(pose-7)*.07f,glm::vec3(0,0,1));
  Layout l;for(const auto& label:words.hand[hand]){auto at=Anchors[hand][label.button];l.at.push_back(glm::vec3(grip*glm::vec4(at,1)));check(!std::strstr(label.text,"P1")&&!std::strstr(label.text,"P2"),"generic controller roles");}
  const glm::vec3 eye(0,0,0);layOut(l,hand,grip,words,eye);glm::vec3 mid(0);for(auto a:l.at)mid+=a;mid/=float(l.at.size());glm::vec3 x,y,z;facing(mid,eye,glm::vec3(grip[0]),x,y,z);
  const Placed* trigger=nullptr;for(const auto& placed:l.placed)if(placed.text.find("Trigger")!=std::string::npos)trigger=&placed;
  check(trigger!=nullptr,"trigger exists");if(!trigger)continue;
  check(trigger->text=="Trigger - Fire","generic Fire text");
  check(side*glm::dot(trigger->centre-trigger->from,x)<-.02f,"trigger extends inward");
  check(std::abs(glm::dot(trigger->centre-trigger->from,y))<.025f,"trigger stays beside actual anchor and clear of Stick");
  for(const auto& other:l.placed)if(&other!=trigger){
   const auto delta=trigger->centre-other.centre;
   const bool clear=std::abs(glm::dot(delta,x))>=trigger->hx+other.hx+.001f || std::abs(glm::dot(delta,y))>=trigger->hy+other.hy+.001f;
   if(!clear&&pose==0)std::cerr<<"OVERLAP "<<words.profile<<" hand "<<hand<<" vs "<<other.text<<" dx "<<glm::dot(delta,x)<<" dy "<<glm::dot(delta,y)<<" widths "<<trigger->hx+other.hx<<'\n';
   check(clear,"trigger clear of other labels");
  }
 }
 std::cout<<(failures?"FAIL ":"PASS ")<<checks<<" actual-font/controller-layout checks, "<<failures<<" failures\n";return failures?1:0;
}
