from pathlib import Path
import re,subprocess,os,sys
r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
deps=Path('E:/___flattovrdev/hotd2-vr/core/deps/glm')
t=(r/'core/rend/gles/gles.cpp').read_text(encoding='utf-8-sig')
s=re.search(r'static const char\* VertexShaderSource = R"\((.*?)\)";',t,re.S)[1]
prefix='#version 330 core\n#define TARGET_GL 3\n#define GLES2 0\n#define GLES3 1\n#define GL2 2\n#define GL3 3\n#define VR_REPROJECT 1\n#define DIV_POS_Z 1\n#define pp_Gouraud 1\n#define INTERPOLATION\n'
(o/'shader.inc').write_text('const char* shader=R"ZOOM('+prefix+s+')ZOOM";',encoding='utf8')
e=dict(os.environ);e['PATH']='D:/msys64/mingw64/bin;'+e.get('PATH','')
b=o/'game-zoom-gl-test.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(o),'-I'+str(deps),str(Path(__file__).with_suffix('.cpp')),'-o',str(b),'-lglew32','-lopengl32','-lgdi32','-luser32'],check=True,env=e)
sys.exit(subprocess.run([str(b)],env=e).returncode)
