#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#if __has_include("game_zoom.h")
#include "game_zoom.h"
#define HAS_ZOOM 1
#else
#define HAS_ZOOM 0
#endif
static int count;
#define CHECK(x) do{++count;if(!(x)){std::cerr<<"FAIL " #x " line "<<__LINE__<<'\n';return 1;}}while(0)
int main(){
 CHECK(HAS_ZOOM);
#if HAS_ZOOM
 using namespace vr::zoom;
 Session s; std::array<Hand,2> h{};for(auto& v:h)v.tracked=true;
 auto tick=[&](bool active=true,int layout=0){return s.update(h,active,layout);};
 tick();CHECK(!s.enabled());
 for(int hand:{0,1}){h[hand].grip=true;CHECK(!tick());for(int i=0;i<50;i++)CHECK(!tick());h[hand].grip=false;CHECK(tick()&&s.enabled());CHECK(!tick());h[hand].grip=true;tick();h[hand].grip=false;CHECK(tick()&&!s.enabled());}
 h[0].grip=h[1].grip=true;tick();h[0].grip=h[1].grip=false;CHECK(tick()&&s.enabled());
 for(int consume=0;consume<3;consume++){h[0].grip=true;tick();if(consume==0)h[0].stickX=.31f;if(consume==1)h[0].click=true;if(consume==2)s.consume(0);tick();h[0].stickX=0;h[0].click=false;tick();h[0].grip=false;CHECK(!tick()&&s.enabled());}
 h[0].grip=true;tick();tick(false);tick();h[0].grip=false;CHECK(!tick()&&s.enabled());
 h[1].grip=true;tick();h[1].tracked=false;tick();h[1].tracked=true;tick();h[1].grip=false;CHECK(!tick()&&s.enabled());
 h[0].grip=true;tick();tick(true,1);h[0].grip=false;CHECK(!tick(true,1)&&s.enabled());
 s.suspend();h[0].grip=true;tick();h[0].grip=false;CHECK(!tick()&&s.enabled());
 s.reset();CHECK(!s.enabled());s.set(true,2);CHECK(s.enabled()&&s.notice(2)&&s.notice(2.999)&&!s.notice(3));s.suspend();CHECK(!s.notice(2.2)&&s.enabled());
 CHECK(nativeFactor(540.8f,540.8f,219.7f,true)==1.f);
 CHECK(std::abs(nativeFactor(540.8f,879.2f,436.4f,true)-879.2f/540.8f)<1e-6f);
 CHECK(std::abs(nativeFactor(540.8f,228.2f,228.2f,true)-456.4f/540.8f)<1e-6f);
 CHECK(std::abs(nativeFactor(540.8f,456.4f,456.4f,false)-456.4f/540.8f)<1e-6f);
 for(float bad:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){CHECK(nativeFactor(bad,879.2f,436.4f,true)==1);CHECK(nativeFactor(540.8f,bad,436.4f,true)==1);CHECK(nativeFactor(540.8f,879.2f,bad,true)==1);}
 for(float factor:{.5f,1.f,2.f}){auto p=worldXY(3.f,-4.f,factor);auto q=worldXY(p[0],p[1],1.f/factor);CHECK(q[0]==3&&q[1]==-4);}
#endif
 std::cout<<count<<" zoom session/native camera checks PASS\n";
}
