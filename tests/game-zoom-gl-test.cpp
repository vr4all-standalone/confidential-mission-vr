#include <GL/glew.h>
#include <windows.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cmath>
static int checks;
static void check(bool ok,const char* msg){++checks;if(!ok){std::cerr<<"FAIL: "<<msg<<"\n";std::exit(1);}}
#include "shader.inc"
static void context(){
 WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"FlycastReticleTest";RegisterClassW(&wc);
 auto w=CreateWindowW(wc.lpszClassName,L"Flycast reticle production raster test",WS_OVERLAPPEDWINDOW,0,0,32,32,nullptr,nullptr,wc.hInstance,nullptr);check(w!=nullptr,"hidden test window creates");
 auto dc=GetDC(w);PIXELFORMATDESCRIPTOR p{};p.nSize=sizeof(p);p.nVersion=1;p.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;p.iPixelType=PFD_TYPE_RGBA;p.cColorBits=32;p.cDepthBits=24;
 auto format=ChoosePixelFormat(dc,&p);check(format&&SetPixelFormat(dc,format,&p),"OpenGL test pixel format creates");auto rc=wglCreateContext(dc);check(rc&&wglMakeCurrent(dc,rc),"hidden WGL context creates");glewExperimental=GL_TRUE;check(glewInit()==GLEW_OK,"OpenGL entry points load");while(glGetError()!=GL_NO_ERROR){}
}

GLuint program(const std::string& src){
 auto compile=[](GLenum kind,const char* s){GLuint v=glCreateShader(kind);glShaderSource(v,1,&s,nullptr);glCompileShader(v);GLint ok;glGetShaderiv(v,GL_COMPILE_STATUS,&ok);if(!ok){char log[8192];glGetShaderInfoLog(v,sizeof(log),nullptr,log);std::cerr<<log;std::exit(2);}return v;};
 auto v=compile(GL_VERTEX_SHADER,src.c_str()),f=compile(GL_FRAGMENT_SHADER,"#version 330 core\nin vec3 vtx_uv;out vec4 color;void main(){color=vec4(vtx_uv.xy,0.5,1);}");
 auto p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glBindAttribLocation(p,0,"in_pos");glBindAttribLocation(p,1,"in_base");glBindAttribLocation(p,2,"in_offs");glBindAttribLocation(p,3,"in_uv");
 const char* varyings[]={"gl_Position","vtx_uv"};glTransformFeedbackVaryings(p,2,varyings,GL_INTERLEAVED_ATTRIBS);glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);check(ok,"actual production vertex GLSL links");return p;
}
int main(){context();
 GLuint tex,fbo,vao,vbo,feedback;glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1024,1024,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);check(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"raster FBO ready");glViewport(0,0,1024,1024);
 auto p=program(shader);std::string old=shader;auto at=old.find(" * vrHudSize");check(at!=std::string::npos,"production overlay scaling expression exists");old.erase(at,std::string(" * vrHudSize").size());auto reference=program(old);
 glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);
 float data[]={-.5f,-.5f,1,1,0,0, .5f,-.5f,1,1,1,0, -.5f,.5f,1,1,0,1, .5f,.5f,1,1,1,1};
 glBufferData(GL_ARRAY_BUFFER,sizeof(data),data,GL_DYNAMIC_DRAW);glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);glEnableVertexAttribArray(0);glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(4*sizeof(float)));glEnableVertexAttribArray(3);
 glGenBuffers(1,&feedback);glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER,feedback);glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,7*4*sizeof(float),nullptr,GL_DYNAMIC_READ);glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,feedback);
 auto render=[&](GLuint prog,float scale,float z,float zoom=1.f){glUseProgram(prog);auto matrix=glm::ortho(-2.f,2.f,-2.f,2.f,.1f,100.f),identity=glm::mat4(1);glUniformMatrix4fv(glGetUniformLocation(prog,"ndcMat"),1,GL_FALSE,&identity[0][0]);glUniformMatrix4fv(glGetUniformLocation(prog,"vrMat"),1,GL_FALSE,&matrix[0][0]);glUniform2f(glGetUniformLocation(prog,"vrTan"),1,1);glUniform4f(glGetUniformLocation(prog,"vrOverlay"),.75f,.75f,1,1);glUniform3f(glGetUniformLocation(prog,"vrComfort"),0,0,0);glUniform1f(glGetUniformLocation(prog,"vrHudSize"),scale);glUniform1f(glGetUniformLocation(prog,"vrWorldZoom"),zoom);for(int i=0;i<4;i++)data[i*6+2]=z;glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(data),data);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLE_STRIP,0,4);std::vector<unsigned char> pix(1024*1024*4);glReadPixels(0,0,1024,1024,GL_RGBA,GL_UNSIGNED_BYTE,pix.data());return pix;};
 auto area=[](const auto& pix){int n=0;for(size_t i=0;i<pix.size();i+=4)n+=pix[i+2]>0;return n;};
 auto normal=render(p,1,1);check(normal==render(p,1,1,2),"zoom preserves native HUD raster");check(normal==render(reference,1,1),"100 percent raster exactly matches prior production shader");
 auto small=render(p,.5f,1),large=render(p,2,1);check(area(small)*4==area(normal)&&area(normal)*4==area(large),"50/200 percent scale actual native UI physical XY by half/double");
 auto world=render(p,1,.5f);check(world==render(p,.5f,.5f)&&world==render(p,2,.5f),"world raster invariant for all HUD sizes");
 check(normal==render(p,1,1),"reset next frame/eye to100 has no uniform leakage");
 auto zoomed=render(p,1,.5f,2);check(area(zoomed)==4*area(world),"world zoom doubles actual production world XY");
 std::string offShader=shader;auto zoomAt=offShader.find("p.xy *= vrWorldZoom;");if(zoomAt!=std::string::npos)offShader.erase(zoomAt,std::string("p.xy *= vrWorldZoom;").size());auto oldWorld=program(offShader);check(world==render(oldWorld,1,.5f),"zoom Off world raster equals original");
 auto capture=[&](float scale,float z=1.f,float zoom=1.f){render(p,scale,z,zoom);glEnable(GL_RASTERIZER_DISCARD);glBeginTransformFeedback(GL_POINTS);glDrawArrays(GL_POINTS,0,4);glEndTransformFeedback();glDisable(GL_RASTERIZER_DISCARD);std::vector<float> out(28);glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER,0,out.size()*sizeof(float),out.data());return out;};
 auto before=capture(1),after=capture(2);for(int i=0;i<4;i++){check(std::abs(after[i*7]-2*before[i*7])<1e-6f&&std::abs(after[i*7+1]-2*before[i*7+1])<1e-6f,"transform feedback proves XY-only scale");for(int j=2;j<7;j++)check(after[i*7+j]==before[i*7+j],"depth/homogeneous W/UV unchanged");}
 auto plain=capture(1,.5f,1),magnified=capture(1,.5f,2);for(int i=0;i<4;i++){check(std::abs(magnified[i*7]-2*plain[i*7])<1e-6f&&std::abs(magnified[i*7+1]-2*plain[i*7+1])<1e-6f,"world transform XY magnified");for(int j=2;j<7;j++)check(magnified[i*7+j]==plain[i*7+j],"world zoom preserves depth/W/UV");}
 check(glGetError()==GL_NO_ERROR,"native overlay shader GL run has no errors");std::cout<<checks<<" production HUD GLSL raster/transform checks PASS\n";
}
