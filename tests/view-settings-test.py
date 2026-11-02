"""Production Option/Settings/INI fixture, independent of changing menu layout."""
from pathlib import Path
import os,re,subprocess,sys
r=Path(sys.argv[1]);o=Path(sys.argv[2]);o.mkdir(parents=True,exist_ok=True)
def src(n):return (r/n).read_text(encoding='utf-8-sig')
def block(t,s):
 a=t.index(s);b=t.index('{',a);e=b+1;d=1
 while d:d+=(t[e]=='{')-(t[e]=='}');e+=1
 return t[a:e]
def line(t,p):
 m=re.search(p,t,re.M);return m[0] if m else ''
ih=src('core/cfg/ini.h').replace('#include "oslib/storage.h"','').replace('#pragma once','')
ic=re.sub(r'^#include[^\n]*\n','',src('core/cfg/ini.cpp'),flags=re.M)
for s in ('void IniFile::load(hostfs::File *file','void IniFile::save(hostfs::File *file'):ic=ic.replace(block(ic,s),'')
trim=line(src('core/stdclass.cpp'),r'^const std::string defaultWs[^\n]+')+'\n'+block(src('core/stdclass.h'),'static inline std::string trim_ws(')
cfg='namespace config {static IniFile cfgdb;static bool autoSaving=true;std::string persisted;int writeCount=0;void saveFile(){cfgdb.save(persisted);++writeCount;}void autoSave(){if(autoSaving)saveFile();}\n'
for s in ('void saveStr(','std::string loadStr(','void saveInt(','int loadInt(','void saveBool(','bool loadBool(','void setTransient(','bool isTransient(','bool hasSection(','void deleteSection(','void deleteEntry(','void setAutoSave(','void saveFloat(','float loadFloat(','std::vector<std::string> getEntries('):cfg+=block(src('core/cfg/cfg.cpp'),s)+'\n'
cfg+='}'
option=src('core/cfg/option.h');definitions=src('core/cfg/option.cpp');has='VrLaserWidth' in definitions and 'VrWidenMode' in definitions
code='#define HAS_VIEW_SETTINGS '+str(int(has))+'\n'
code+='namespace config {'+';\n'.join(block(option,s) for s in ('class BaseOption','class Settings','template<typename T, bool PerGameOption = true>'))+';\n'
code+=line(option,r'^using WorldScaleOption[^\n]+')+'\n'
for n in ('VrWiden','VrLaser','VrWorldScale','VrHudDepth'):code+=line(definitions,r'^\w+(?:<[^>]+>)? '+n+r'[^\n]+')+'\n'
if has:
 for n in ('LaserWidthOption','WidenModeOption'):code+=line(option,r'^using '+n+r'[^\n]+')+'\n'
 for n in ('VrLaserWidth','VrWidenMode'):code+=line(definitions,r'^\w+ '+n+r'[^\n]+')+'\n'
code+='}'
policy='#include "rend/vr/world_scale_settings.h"\n'+('#include "rend/vr/view_settings.h"\n' if has else '')
(o/'view-settings.inc').write_text('\n'.join((trim,ih,ic,'#include "cfg/cfg.h"',cfg,policy,code)))
env=dict(os.environ);env['PATH']='D:/msys64/mingw64/bin;'+env.get('PATH','')
b=o/'view-settings-test.exe'
subprocess.run(['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(o),'-I'+str(r/'core'),str(Path(__file__).with_suffix('.cpp')),'-o',str(b)],check=True,env=env)
sys.exit(subprocess.run([str(b)],env=env).returncode)
