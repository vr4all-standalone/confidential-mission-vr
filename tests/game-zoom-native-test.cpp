#include <GL/glew.h>
#include <windows.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>
using u32=unsigned;using u8=unsigned char;
static int checks;
static void check(bool ok,const char* msg){++checks;if(!ok){std::cerr<<"FAIL: "<<msg<<'\n';std::exit(1);}}
#define NOTICE_LOG(...) ((void)0)
struct Vertex{float x,y,z;unsigned char col[4],spc[4];float u,v;unsigned char col1[4]{},spc1[4]{};float u1=0,v1=0;};
struct PolyParam{struct{bool Texture=false;}pcw;struct{unsigned TexAddr=0;}tcw;u32 first=0,count=0;bool vrSight=false;u32 rawPcw=0;};
struct rend_context{bool isRTT=false,vrZoomCutscene=false;std::vector<Vertex> verts;std::vector<PolyParam> global_param_op,global_param_pt,global_param_tr;};
namespace config{float VrFocal=540.8f,VrHudW=1.f;}
namespace vr{struct GameProfile{const char*gameId=" CONFIDENTIAL MISSION ---------";u32 sightTextures[8]{0x400000};float sightMinW=.95f,sightMaxW=1.005f,snapMinW=.005f,snapMaxW=10.f;u32 letterboxTextures[4]{0x400680};float letterboxW=.2f;}profile;const GameProfile*activeProfile=&profile;u32 markerTextures[4]{0x5e8000,0x5e7800};}
#include "shader.inc"
#include "native-production.inc"
struct NativeFrame{rend_context ctx;float focalX,focalY;unsigned width,height;};
template<class T>T read(std::istream& in){T value{};in.read(reinterpret_cast<char*>(&value),sizeof(value));if(!in){std::cerr<<"FAIL native fixture field read\n";std::exit(1); }return value;}
static NativeFrame load(const std::string& path){
 std::ifstream in(path,std::ios::binary);char magic[4];in.read(magic,4);check(std::memcmp(magic,"RIP1",4)==0,"native fixture has RIP1 provenance");NativeFrame f;
 f.width=read<u32>(in);f.height=read<u32>(in);f.focalX=read<float>(in);f.focalY=read<float>(in);read<float>(in);read<float>(in);auto lists=read<u32>(in);check(lists==3,"native fixture retains all TA lists");
 for(auto* list:{&f.ctx.global_param_op,&f.ctx.global_param_pt,&f.ctx.global_param_tr}){
  auto count=read<u32>(in);for(u32 j=0;j<count;j++){
   read<u32>(in);read<u32>(in);auto tcw=read<u32>(in),pcw=read<u32>(in),nv=read<u32>(in);PolyParam p;p.pcw.Texture=(pcw>>3)&1;p.tcw.TexAddr=tcw&0x1fffff;p.rawPcw=pcw;p.first=f.ctx.verts.size();p.count=nv;
   for(u32 k=0;k<nv;k++){Vertex v;v.x=read<float>(in);v.y=read<float>(in);v.z=read<float>(in);v.u=read<float>(in);v.v=read<float>(in);in.read(reinterpret_cast<char*>(v.col),4);in.read(reinterpret_cast<char*>(v.spc),4);if(!in){std::cerr<<"FAIL native fixture vertex read\n";std::exit(1); }f.ctx.verts.push_back(v);}list->push_back(p);
  }
 }
 return f;
}
static void ppm(const std::string& path,const std::vector<unsigned char>& pixels){std::ofstream out(path,std::ios::binary);out<<"P6\n512 512\n255\n";for(int y=511;y>=0;y--)for(int x=0;x<512;x++)out.write(reinterpret_cast<const char*>(&pixels[(y*512+x)*4]),3);}
int main(int argc,char**argv){check(argc==3,"private fixture directory and output supplied");context();
 GLuint tex,fbo,vao,vbo,feedback;glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,512,512,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);check(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"native ring raster target ready");glViewport(0,0,512,512);
 auto prog=program(shader);glUseProgram(prog);glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glGenBuffers(1,&feedback);glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,feedback);
 auto projection=glm::mat4(0);projection[0][0]=projection[1][1]=1;projection[2][2]=projection[2][3]=-1;projection[3][2]=-.2f;auto identity=glm::mat4(1);
 glUniformMatrix4fv(glGetUniformLocation(prog,"ndcMat"),1,GL_FALSE,&identity[0][0]);glUniformMatrix4fv(glGetUniformLocation(prog,"vrMat"),1,GL_FALSE,&projection[0][0]);glUniform3f(glGetUniformLocation(prog,"vrComfort"),0,0,0);glUniform3f(glGetUniformLocation(prog,"vrNativeRaster"),1,1,0);
 unsigned ringPolygons=0,knownNonSights=0,correctedPoints=0,offCentre=0;double worst=0,oldWorst=0;
 for(int n:{2,3,4,5}){
  auto name="pass000"+std::to_string(n)+".bin";auto raw=load(std::string(argv[1])+"/"+name);auto f=raw;bool bars=false;
  for(auto* list:{&f.ctx.global_param_op,&f.ctx.global_param_pt,&f.ctx.global_param_tr})bars|=vr::nativeZoomCutsceneBars(f.ctx,*list,0,list->size(),f.width,f.height);
  check(bars==(n==2||n==3),"real native intro strips detected; gameplay full-screen fade rejected");f.ctx.vrZoomCutscene=bars;
  if(n==2||n==3){for(auto& v:f.ctx.verts)v.x=v.y=NAN;check(vr::zoomCutsceneFrame(f.ctx),"native bar evidence survives hidden XY");continue;}
  glm::vec2 unwiden(f.focalX/config::VrFocal,f.focalY/config::VrFocal);unsigned placed=0;
  vr::placeSights(f.ctx,f.ctx.global_param_op,1,f.ctx.global_param_op.size(),1,f.width,f.height,unwiden,placed,false);vr::placeSights(f.ctx,f.ctx.global_param_pt,0,f.ctx.global_param_pt.size(),1,f.width,f.height,unwiden,placed,false);vr::placeSights(f.ctx,f.ctx.global_param_tr,0,f.ctx.global_param_tr.size(),1,f.width,f.height,unwiden,placed,false);
  check(placed==16,"all16 actual native target polygons qualify in each gameplay capture");
  glUniform2f(glGetUniformLocation(prog,"vrTan"),f.width*.5f/f.focalX,f.height*.5f/f.focalY);glUniform4f(glGetUniformLocation(prog,"vrOverlay"),f.width*.5f/config::VrFocal,f.height*.5f/config::VrFocal,40,1);
  for(auto* list:{&f.ctx.global_param_op,&f.ctx.global_param_pt,&f.ctx.global_param_tr})for(auto& p:*list){
   if(p.pcw.Texture&&p.tcw.TexAddr==(0x400000>>3)&&!p.vrSight){++knownNonSights;check(vr::hudScaleForPolygon(p,1.4f,false,.615f)==1,"shared native texture has no zoom effect without actual sight qualification");}
   if(!p.vrSight)continue;++ringPolygons;check(((p.rawPcw>>16)&3)==0,"real native rings carry User_Clip0, native clip modeOff");
   std::vector<float> input;for(u32 i=0;i<p.count;i++){auto& v=f.ctx.verts[p.first+i];input.insert(input.end(),{(v.x-f.width*.5f)/(f.width*.5f),(v.y-f.height*.5f)/(f.height*.5f),v.z,1,v.u,v.v});}
   glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,input.size()*sizeof(float),input.data(),GL_STREAM_DRAW);glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,6*sizeof(float),nullptr);glEnableVertexAttribArray(0);glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(4*sizeof(float)));glEnableVertexAttribArray(3);
   glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER,feedback);glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,p.count*7*sizeof(float),nullptr,GL_STREAM_READ);
   for(float zoom:{.5f,.615f,1.f,1.5f}){
    glUniform1f(glGetUniformLocation(prog,"vrWorldZoom"),zoom);glUniform1f(glGetUniformLocation(prog,"vrHudSize"),vr::hudScaleForPolygon(p,1.4f,false,zoom));
    glEnable(GL_RASTERIZER_DISCARD);glBeginTransformFeedback(GL_POINTS);glDrawArrays(GL_POINTS,0,p.count);glEndTransformFeedback();glDisable(GL_RASTERIZER_DISCARD);std::vector<float> actual(p.count*7);glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER,0,actual.size()*sizeof(float),actual.data());
    for(u32 i=0;i<p.count;i++){
     auto& original=raw.ctx.verts[p.first+i];auto& placedVertex=f.ctx.verts[p.first+i];if(!std::isfinite(placedVertex.x)||!std::isfinite(placedVertex.y))continue;
     // Independent native ray angle: original PVR pixel offset / live camera focal.
     // Drop only vertices actually cropped by the original native screen boundary.
     const float expectedX=(original.x-f.width*.5f)/f.focalX*zoom,expectedY=(original.y-f.height*.5f)/f.focalY*zoom;
     const float unzoomX=(original.x-f.width*.5f)/f.focalX,unzoomY=(original.y-f.height*.5f)/f.focalY;
     if(std::abs(placedVertex.x-(f.width*.5f+(original.x-f.width*.5f)/unwiden.x))>.001f||std::abs(placedVertex.y-(f.height*.5f+(original.y-f.height*.5f)/unwiden.y))>.001f)continue;
     const auto x=actual[i*7]/actual[i*7+3],y=actual[i*7+1]/actual[i*7+3];auto error=std::max(std::abs(x-expectedX),std::abs(y-expectedY));worst=std::max(worst,double(error));check(error<2e-6f,"production placement plus production GLSL aligns real marker vertex with zoomed native ray");++correctedPoints;
     if(zoom!=1&&std::max(std::abs(unzoomX),std::abs(unzoomY))>.01f){oldWorst=std::max(oldWorst,double(std::max(std::abs(unzoomX-expectedX),std::abs(unzoomY-expectedY))));++offCentre;}
    }
   }
  }
  // Actual native ring geometry is rasterized twice through the unchanged production
  // scene shader: old overlay scale1 and the corrected per-polygon frame factor.
  for(bool corrected:{false,true}){
   glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glUniform1f(glGetUniformLocation(prog,"vrWorldZoom"),.615f);
   for(auto* list:{&f.ctx.global_param_op,&f.ctx.global_param_pt,&f.ctx.global_param_tr})for(auto& p:*list)if(p.vrSight){std::vector<float> input;for(u32 i=0;i<p.count;i++){auto& v=f.ctx.verts[p.first+i];input.insert(input.end(),{(v.x-f.width*.5f)/(f.width*.5f),(v.y-f.height*.5f)/(f.height*.5f),v.z,1,v.u,v.v});}glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,input.size()*sizeof(float),input.data(),GL_STREAM_DRAW);glUniform1f(glGetUniformLocation(prog,"vrHudSize"),corrected?vr::hudScaleForPolygon(p,1.4f,false,.615f):1.f);glDrawArrays(GL_TRIANGLE_STRIP,0,p.count);}
   std::vector<unsigned char> pixels(512*512*4);glReadPixels(0,0,512,512,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());ppm(std::string(argv[2])+"/native-rings-"+std::to_string(n)+(corrected?"-corrected.ppm":"-before.ppm"),pixels);
  }
 }
 check(ringPolygons==32&&knownNonSights>0&&correctedPoints>100&&offCentre>100,"native replay covers positive markers, shared-texture negatives and off-centre zoom cases");check(oldWorst>.05,"old marker transform demonstrably diverges from zoomed native ray");check(glGetError()==GL_NO_ERROR,"native production shader replay has no GL errors");std::cout<<checks<<" native capture/production placement/GLSL checks PASS; rings="<<ringPolygons<<" other400000="<<knownNonSights<<" projected="<<correctedPoints<<" worstCorrected="<<worst<<" oldWorst="<<oldWorst<<"\n";
}
