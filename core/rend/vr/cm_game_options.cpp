// Confidential Mission USA original Service/Test adapter. Measured 2026-11-02.
#include "cm_game_options.h"
#include "cm_game_options_model.h"
#include "cfg/option.h"
#include "emulator.h"
#include "hw/sh4/sh4_mem.h"
#include "hw/naomi/naomi_flashrom.h"
#include "log/Log.h"
#include <chrono>
#include <mutex>
namespace vr::xr::cm::options {
namespace {
std::mutex mutex;
detail::Controller controller;
uint64_t nativeBoot=0,nativeBootRead=0,nativeSave=0,nativeSaveFailure=0;
bool stateLoaded=false;
detail::Screen previousScreen=detail::Screen::Unknown;
uint64_t nowMs(){return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
uint32_t hashRam(uint32_t start,unsigned count){uint32_t hash=2166136261u;for(unsigned i=0;i<count;++i)hash=(hash^ReadMem8_nommu(0x8c000000u+start+i))*16777619u;return hash;}
bool identity(){return settings.platform.isNaomi() && settings.content.gameId==" CONFIDENTIAL MISSION ---------" && config::Region==1 && !config::GGPOEnable;}
detail::Native observe() {
 detail::Native n{};n.boot=nativeBoot;n.readBoot=nativeBootRead;n.save=nativeSave;n.saveFailure=nativeSaveFailure;
 if(!identity())return n;
 // Native, mirrored EEPROM readback only. No storage or option RAM writes occur here.
 for(unsigned i=0;i<n.eeprom.size();++i)n.eeprom[i]=read_naomi_eeprom(i);
 n.persisted=detail::decodeEeprom(n.eeprom,n.data);
 if(hashRam(0x20800,0x200)==0xbad89ebeu)n.screen=detail::Screen::Game;
 else if(hashRam(0x21d82,0x550)==0x47563a9du && ReadMem32_nommu(0x8c23b26c)==0x0c24ba60 && ReadMem32_nommu(0x8c23b270)==0x0c23afe4) {
  n.cursor=ReadMem32_nommu(0x8c23b27c);if(n.cursor>=1&&n.cursor<=12)n.screen=detail::Screen::Bios;
 }
 else if(hashRam(0x20d9c,0x380)==0xb43576dcu) {
  const auto menu=ReadMem32_nommu(0x8c0622d4),sub=ReadMem32_nommu(0x8c0622d8),dispatch=ReadMem32_nommu(0x8c08cb0c);
  if(dispatch==1&&menu<7){n.screen=detail::Screen::TestMain;n.cursor=menu;}
  else if(dispatch==4&&sub==1&&menu==2) {
   n.screen=detail::Screen::Assignments;n.cursor=ReadMem32_nommu(0x8c0622f0);
   for(unsigned i=0;i<n.data.size();++i)n.data[i]=ReadMem8_nommu(0x8c08d188+i);
  }
 }
 // Only the materialized original SH4 entry hook advances boot generation.
 previousScreen=n.screen;n.boot=nativeBoot;return n;
}
void onEvent(Event event,void*) {
 std::lock_guard<std::mutex> lock(mutex);
 if(event==Event::VBlank) {
  if(stateLoaded){controller.invalidate("Restart the app after loading a state before applying native settings");return;}
  if(!identity()){controller.invalidate("Native options support the measured CM USA revision only");return;}
  auto before=controller.snapshot();controller.tick(observe(),nowMs());auto after=controller.snapshot();
  if(after.status!=before.status)NOTICE_LOG(INPUT,"CM options: %s",after.message);
 } else {
  previousScreen=detail::Screen::Unknown;
  if(event==Event::LoadState)stateLoaded=true;
  else if(event==Event::Start){stateLoaded=false;nativeBootRead=0;}
  controller.invalidate(event==Event::LoadState?"Native state loaded; reopen the game before applying":"Waiting for original Confidential Mission game boot");
 }
}
struct Registration {Registration(){EventManager::listen(Event::VBlank,onEvent);EventManager::listen(Event::Start,onEvent);EventManager::listen(Event::LoadState,onEvent);EventManager::listen(Event::Terminate,onEvent);}} registration;
}
Snapshot snapshot(){std::lock_guard<std::mutex> lock(mutex);return controller.snapshot();}
const char* label(Field f){static const char* names[FieldCount]{"Game difficulty","Boss difficulty","Shifting difficulty","Collision size","Life"};auto i=static_cast<unsigned>(f);return i<FieldCount?names[i]:"";}
const char* valueLabel(Field,int){return nullptr;}
bool stage(Field f,int v){std::lock_guard<std::mutex> lock(mutex);return controller.stage(f,v);}
bool apply(){std::lock_guard<std::mutex> lock(mutex);return controller.apply(nowMs());}
void discard(){std::lock_guard<std::mutex> lock(mutex);controller.discard();}
bool busy(){std::lock_guard<std::mutex> lock(mutex);return controller.snapshot().busy;}
void context(bool allowed,bool focused,bool manual){std::lock_guard<std::mutex> lock(mutex);controller.context(allowed,focused,manual);controller.watchdog(nowMs());}
uint32_t nativeButtons(){std::lock_guard<std::mutex> lock(mutex);return controller.buttons();}
void notifyNativeRead(){std::lock_guard<std::mutex> lock(mutex);nativeBootRead=nativeBoot;}
void notifyNativeSave(bool success){std::lock_guard<std::mutex> lock(mutex);if(success)++nativeSave;else ++nativeSaveFailure;}
void acknowledgeNativeButtons(uint32_t bits){std::lock_guard<std::mutex> lock(mutex);controller.acknowledge(bits);}
void notifyNativeGameBoot(){std::lock_guard<std::mutex> lock(mutex);++nativeBoot;nativeBootRead=0;}
}
