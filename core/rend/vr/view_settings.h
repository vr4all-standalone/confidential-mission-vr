// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "cfg/cfg.h"
#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
namespace vr::view {
inline bool integer(const std::string& text,int& value) {
 std::istringstream input(text);input.imbue(std::locale::classic());
 if(!(input>>value))return false;input>>std::ws;return input.eof();
}
inline int mode(int value) { return value>=0&&value<=2?value:1; }
inline int nextMode(int value,int dir) { return std::clamp(mode(value)+dir,0,2); }
inline const char* label(int value) { return mode(value)==0?"Off":mode(value)==2?"Fade":"On"; }
// CM's native play camera keeps the original hit focal. Its two cinematic
// builders instead write the factor2 widened focal to both projection and hit
// camera. Equal valid focals identify that measured path (game_zoom.h).
inline float nativeFocal(float hit,float drawn,bool patched) {
 auto valid=[](float f){return std::isfinite(f)&&f>10.f&&f<20000.f;};
 if(!valid(hit)||!valid(drawn))return 0.f;
 const float focal=hit*(patched&&std::abs(hit/drawn-1.f)<.01f?2.f:1.f);
 return valid(focal)?focal:0.f;
}
inline bool fadeActive(int selected,bool activeLarge,bool service,bool native3D) {
 return mode(selected)==2&&activeLarge&&!service&&native3D;
}
template<class OptionType,class LegacyOption> class ModeSetting : public OptionType {
 LegacyOption& legacy;
public:
 explicit ModeSetting(LegacyOption& legacy):OptionType("vr.WidenMode",1),legacy(legacy){}
 void set(int value) {const int selected=mode(value);OptionType::set(selected);legacy.set(selected!=0);}
 void reset() override {OptionType::reset();set(1);}
 void load() override {
  // Missing mode preserves the legacy bool already loaded for this game.
  const auto entries=config::getEntries("config");
  const bool present=std::find(entries.begin(),entries.end(),"vr.WidenMode")!=entries.end();
  int selected=1;
  if(!present)selected=bool(legacy)?1:0;
  else if(!integer(config::loadStr("config","vr.WidenMode",""),selected)||selected<0||selected>2)selected=1;
  set(selected);
 }
 void save() const override {
  const int selected=mode(int(*this));
  config::saveInt("config","vr.WidenMode",selected);
  config::saveBool("config","vr.Widen",selected!=0);
 }
};
}
namespace vr::laser {
inline int width(int value) { return value>=1&&value<=4?value:1; }
inline int nextWidth(int value,int dir) { return std::clamp(width(value)+dir,1,4); }
inline float multiplier(int value) { const float scales[]={1.f,2.f,5.f,10.f};return scales[width(value)-1]; }
inline float halfWidth(int value) { return .0008f*multiplier(value); }
inline const char* label(int value) { const char* names[]={"Normal","2x","5x","10x"};return names[width(value)-1]; }
inline int choice(bool enabled,int value) { return enabled?width(value):0; }
inline int nextChoice(bool enabled,int value,int dir) { return std::clamp(choice(enabled,value)+dir,0,4); }
inline const char* choiceLabel(bool enabled,int value) { return enabled?label(value):"Off"; }
template<class OptionType> class WidthSetting : public OptionType {
public:
 WidthSetting():OptionType("vr.LaserWidth",1){}
 void load() override {
  int selected=1;
  if(!view::integer(config::loadStr("config","vr.LaserWidth",""),selected))selected=1;
  this->set(width(selected));
 }
};
}
