"""Rasterize actual production CM scene vertex AND fragment GLSL in hidden WGL."""
from pathlib import Path
import os,re,subprocess,sys
r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
t=(r/'core/rend/gles/gles.cpp').read_text(encoding='utf-8-sig')
v=re.search(r'static const char\* VertexShaderSource = R"\((.*?)\)";',t,re.S)[1]
f=re.search(r'const char\* PixelPipelineShader = R"\((.*?)\)";',t,re.S)[1]
prefix='''#version 330 core
#define TARGET_GL 3
#define GLES2 0
#define GLES3 1
#define GL2 2
#define GL3 3
#define VR_REPROJECT 1
#define DIV_POS_Z 1
#define pp_Gouraud 1
#define pp_FogCtrl 2
#define pp_UseAlpha 1
#define INTERPOLATION
#define lowp
#define mediump
#define highp
#define FOG_CHANNEL r
'''
def block(s):
 a=t.index(s);b=t.index('{',a);e=b+1;d=1
 while d:d+=(t[e]=='{')-(t[e]=='}');e+=1
 return t[a:e]+';'
# Use BOTH production assembly classes as well as their source: a missing
# VR_REPROJECT fragment define must fail rather than being supplied by a test.
assembly='''namespace config {bool VrReproject=true;}
struct PipelineShader {int cp_AlphaTest=0,pp_InsideClipping=0,pp_UseAlpha=1,pp_Texture=0,pp_IgnoreTexA=0,pp_ShadInstr=0,pp_Offset=0,pp_FogCtrl=2,pp_Gouraud=1,pp_BumpMap=0,fog_clamping=0,trilinear=0,palette=0,divPosZ=1,dithering=0,naomi2=0;};
struct OpenGlSource {std::string source="REPLACE";OpenGlSource& addConstant(const char* name,int value){source+="\\n#define "+std::string(name)+" "+std::to_string(value)+"\\n";return *this;}OpenGlSource& addSource(const char* text){source+=text;return *this;}std::string generate(){return source;}};
'''.replace('REPLACE',prefix.replace('#define VR_REPROJECT 1\n','').replace('#define DIV_POS_Z 1\n','').replace('#define pp_Gouraud 1\n','').replace('#define pp_FogCtrl 2\n','').replace('#define pp_UseAlpha 1\n','').replace('#define INTERPOLATION\n','').replace('\n','\\n'))
strings=''
for n in ('VertexCompatShader','PixelCompatShader','GouraudSource','VertexShaderSource','PixelPipelineShader'):
 s=re.search(r'(?:static )?const char\s*\*\s*'+n+r' = R"\((.*?)\)";',t,re.S)[1]
 if n=='PixelCompatShader':s='out vec4 FragColor;\n#define gl_FragColor FragColor\n'+s
 strings+='const char* '+n+'=R"GLSL('+s+')GLSL";\n'
(o/'fade-shaders.inc').write_text(assembly+strings+block('class VertexSource')+'\n'+block('class FragmentShaderSource'))
e=dict(os.environ);e['PATH']='D:/msys64/mingw64/bin;'+e.get('PATH','')
b=o/'view-fade-gl-test.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(o),'-I'+str(r/'core/deps/glm'),str(Path(__file__).with_suffix('.cpp')),'-o',str(b),'-lglew32','-lopengl32','-lgdi32','-luser32'],env=e,check=True)
sys.exit(subprocess.run([str(b)],env=e).returncode)
