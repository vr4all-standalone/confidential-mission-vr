"""Replay private RIP1 native polygons through real placement and production GLSL.

Usage: python tests/game-zoom-native-test.py SOURCE OUTPUT PRIVATE_RIP_DIRECTORY
No game data is embedded in this test or written into the source tree.
"""
from pathlib import Path
import os,re,subprocess,sys
r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
def block(t,s):
 a=t.index(s);b=t.index('{',a);e=b+1;d=1
 while d:d+=(t[e]=='{')-(t[e]=='}');e+=1
 return t[a:e]
v=(r/'core/rend/vr_reproject.cpp').read_text(encoding='utf-8-sig')
gles=(r/'core/rend/gles/gles.cpp').read_text(encoding='utf-8-sig')
gltest=(r/'tests/game-zoom-gl-test.cpp').read_text(encoding='utf-8-sig')
shader=re.search(r'static const char\* VertexShaderSource = R"\((.*?)\)";',gles,re.S)[1]
prefix='#version 330 core\n#define TARGET_GL 3\n#define GLES2 0\n#define GLES3 1\n#define GL2 2\n#define GL3 3\n#define VR_REPROJECT 1\n#define DIV_POS_Z 1\n#define pp_Gouraud 1\n#define INTERPOLATION\n'
(o/'shader.inc').write_text('const char* shader=R"ZOOM('+prefix+shader+')ZOOM";',encoding='utf8')
functions='\n'.join(block(v,s) for s in ['static bool onOverlay(', 'static void hideVertices(', 'static void cropToScreen(', 'static void placeSights(', 'static bool nativeZoomCutsceneBars(', 'bool zoomCutsceneFrame(', 'float hudScaleForPolygon('])
(o/'native-production.inc').write_text('namespace vr {\n'+functions+'\n}\n'+block(gltest,'static void context(')+'\n'+block(gltest,'GLuint program('),encoding='utf8')
e=dict(os.environ);e['PATH']='D:/msys64/mingw64/bin;'+e.get('PATH','')
exe=o/'game-zoom-native-test.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(o),'-I'+str(r/'core/deps/glm'),str(Path(__file__).with_suffix('.cpp')),'-o',str(exe),'-lglew32','-lopengl32','-lgdi32','-luser32'],env=e,check=True)
sys.exit(subprocess.run([str(exe),sys.argv[3],str(o)],env=e).returncode)
