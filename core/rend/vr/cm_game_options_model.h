// Measured CM USA native Test/Service controller. Pure production logic, tested without an emulator.
#pragma once
#include "cm_game_options.h"
#include <array>
#include <algorithm>
#include <cstring>
namespace vr::xr::cm::options::detail {
using Data = std::array<uint8_t,32>;
using Eeprom = std::array<uint8_t,128>;
enum class Screen { Unknown, Game, Bios, TestMain, Assignments };
struct Native { Screen screen=Screen::Unknown; bool persisted=false; Data data{}; Eeprom eeprom{}; unsigned cursor=0; uint64_t boot=0,readBoot=0,save=0,saveFailure=0; };
inline uint16_t nativeCrc(const uint8_t* b,unsigned size) {
 uint32_t n=0xdebdeb00u;for(unsigned i=0;i<size;++i){n=(n&0xffffff00u)+b[i];for(int k=0;k<8;++k)n=(n<<1)+((n&0x80000000u)?0x10210000u:0u);}
 for(int k=0;k<8;++k)n=(n<<1)+((n&0x80000000u)?0x10210000u:0u);return static_cast<uint16_t>(n>>16);
}
inline bool validData(const Data& d) {
 return std::memcmp(d.data(),"0RCBCMF ",8)==0 && d[10]<=7 && d[11]<=31 && d[24]>=2 && d[24]<=7 && d[25]<=1 && d[26]<=1 && d[27]<=1 && d[28]<=1;
}
inline bool decodeEeprom(const Eeprom& e,Data& d) {
 if(e[38]!=32||e[39]!=32||e[42]!=32||e[43]!=32||std::memcmp(e.data()+44,e.data()+76,32))return false;
 std::copy_n(e.data()+44,32,d.data());auto crc=nativeCrc(d.data(),32);
 return validData(d)&&e[36]==(crc&255)&&e[37]==(crc>>8)&&e[40]==(crc&255)&&e[41]==(crc>>8);
}
inline unsigned value(const Data& d,unsigned f) { switch(f){case 0:return (d[8]>>5)+1;case 1:return (d[9]>>5)+1;case 2:return d[10]+1;case 3:return (d[11]>>2)+1;case 4:return d[24]+1;default:return 0;} }
inline bool protectedEqual(const Data& a,const Data& b,uint32_t edits) {
 Data x=a,y=b;const unsigned off[5]{8,9,10,11,24};const uint8_t mask[5]{0xe0,0xe0,7,0x1c,0xff};
 for(unsigned f=0;f<FieldCount;++f)if(edits&(1u<<f)){x[off[f]]&=static_cast<uint8_t>(~mask[f]);y[off[f]]&=static_cast<uint8_t>(~mask[f]);}
 return x==y;
}
// User approved original expired-ranking1->0 on2026-11-02; no other entry side effects are accepted.
inline bool entryProtectedEqual(const Data& old,const Data& fresh,uint32_t edits) {
 Data expected=old;if(expected[27]==1&&fresh[27]==0)expected[27]=0;
 return protectedEqual(expected,fresh,edits);
}
class Controller {
 enum class Phase { Idle, Enter, WaitBios, BiosToGameTest, WaitMain, ToAssignments, WaitAssignments, Edit, ExitAssignments, WaitMainExit, ExitMain, WaitBiosExit, WaitGame };
 Snapshot s{};Data latest{},baseline{},enteredData{};Eeprom before{};uint8_t target[FieldCount]{};uint32_t editMask=0;
 bool allowed=false,focused=false,manual=false,entered=false,baselineKnown=false,nativeReady=false;Phase phase=Phase::Idle;
 uint64_t started=0,bootAtStart=0,saveAtStart=0,failureAtStart=0,settled=0;unsigned pulseFrames=0,releaseFrames=0,pulseBits=0,biosSteps=0,field=0;
 bool pulseAcknowledged=false,releaseAcknowledged=false;
 bool awaiting=false;unsigned previous=0,expected=0,attempts=0;bool waitValue=false;
 void status(Status state,const char* message){s.status=state;s.message=message;}
 void fail(Status st,const char* message){s.busy=false;s.available=false;s.serviceRecovery=entered;phase=Phase::Idle;pulseFrames=releaseFrames=pulseBits=0;status(st,message);}
 void pulse(unsigned bits){pulseBits=bits;pulseFrames=8;releaseFrames=18;pulseAcknowledged=releaseAcknowledged=false;}
 void readyAvailability() {
  s.available=allowed&&focused&&!manual&&!s.serviceRecovery&&s.supportedMask==31&&nativeReady;
  if(!s.busy && s.status!=Status::Complete && s.status!=Status::Failed && s.status!=Status::Interrupted) {
   if(!s.available)status(Status::Unavailable,"Native options require a ready offline game");
   else status(s.pendingMask?Status::Pending:Status::Ready,latest[27]?"Applying also closes expired internet ranking":s.pendingMask?"Changes are staged until Apply settings and restart":"Original arcade settings");
  }
 }
 bool navigate(unsigned cursor,unsigned destination,unsigned count) {
  if(awaiting){if(cursor==expected){awaiting=false;attempts=0;}else return false;}
  if(cursor>=count){fail(Status::Failed,"Native menu changed; use manual Service");return false;}
  if(cursor==destination)return true;
  previous=cursor;expected=(cursor+1)%count;awaiting=true;pulse(2);return false;
 }
public:
 Snapshot snapshot()const{return s;}
 void acknowledge(unsigned bits){if(!s.busy)return;if(pulseFrames&&bits==pulseBits)pulseAcknowledged=true;if(!pulseFrames&&releaseFrames&&bits==0)releaseAcknowledged=true;}
 void watchdog(uint64_t now){if(s.busy&&now-started>120000)fail(Status::Failed,"Native apply timed out; pending edits retained. Use Service to recover");}
 unsigned buttons()const{return s.busy&&pulseFrames?pulseBits:0;}
 void context(bool a,bool f,bool m){allowed=a;focused=f;manual=m;if(s.busy&&(!a||!f||m))fail(Status::Interrupted,"Native apply interrupted; pending edits retained. Use Service to recover");readyAvailability();}
 bool stage(Field f,int v) {
  unsigned i=static_cast<unsigned>(f);if(!s.available||s.busy||i>=FieldCount||v<(i==4?3:1)||v>8)return false;
  target[i]=static_cast<uint8_t>(v);if(v==static_cast<int>(value(latest,i)))s.pendingMask&=~(1u<<i);else s.pendingMask|=1u<<i;
  s.value[i]=static_cast<uint8_t>(v);status(s.pendingMask?Status::Pending:Status::Ready,latest[27]?"Applying also closes expired internet ranking":s.pendingMask?"Changes are staged until Apply settings and restart":"Original arcade settings");return true;
 }
 void discard(){if(s.busy)return;s.pendingMask=0;for(unsigned f=0;f<FieldCount;++f)s.value[f]=static_cast<uint8_t>(value(latest,f));if(!s.serviceRecovery)status(Status::Ready,"Original arcade settings");readyAvailability();}
 bool apply(uint64_t now) {
  if(!s.available||s.busy||!s.pendingMask)return false;
  s.busy=true;s.serviceRecovery=false;entered=false;editMask=s.pendingMask;baselineKnown=false;started=now;phase=Phase::Enter;awaiting=waitValue=false;attempts=0;pulseFrames=releaseFrames=0;settled=0;
  status(Status::EnteringService,"Ending the round and opening original Service");return true;
 }
 void invalidate(const char* message){nativeReady=false;if(s.busy)fail(Status::Interrupted,message);else{s.available=false;s.supportedMask=0;status(Status::Unavailable,message);}}
 void tick(const Native& n,uint64_t now) {
  if(!s.busy) {
   nativeReady=n.screen==Screen::Game&&n.persisted&&validData(n.data)&&n.boot>0&&n.readBoot==n.boot;
   if(nativeReady) {
    latest=n.data;s.supportedMask=31;
    if(s.serviceRecovery){s.serviceRecovery=false;entered=false;}
    for(unsigned f=0;f<FieldCount;++f){s.minimum[f]=f==4?3:1;s.maximum[f]=8;if(!(s.pendingMask&(1u<<f)))target[f]=static_cast<uint8_t>(value(latest,f));s.value[f]=target[f];}
   }
   readyAvailability();return;
  }
  if(!allowed||!focused||manual){fail(Status::Interrupted,"Native apply interrupted; pending edits retained. Use Service to recover");return;}
  if(now-started>120000){fail(Status::Failed,"Native apply timed out; pending edits retained. Use Service to recover");return;}
  if(phase!=Phase::Enter&&n.saveFailure>failureAtStart){fail(Status::Failed,"Original native save failed; pending edits retained. Use Service to recover");return;}
  if(pulseFrames){if(pulseFrames>1||pulseAcknowledged)--pulseFrames;return;}if(releaseFrames){if(releaseFrames>1||releaseAcknowledged)--releaseFrames;return;}
  if(awaiting&&++attempts>120){fail(Status::Failed,"Native menu did not acknowledge input; use manual Service");return;}
  switch(phase) {
   case Phase::Enter:
    if(n.screen!=Screen::Game||!n.persisted||n.boot==0||n.readBoot!=n.boot){fail(Status::Failed,"Native settings changed before apply; review them again");break;}
    before=n.eeprom;enteredData=n.data;bootAtStart=n.boot;saveAtStart=n.save;failureAtStart=n.saveFailure;entered=true;pulse(1);phase=Phase::WaitBios;break;
   case Phase::WaitBios:
    if(n.screen==Screen::Bios&&n.cursor==12){if(++settled>=60){biosSteps=0;settled=0;awaiting=false;phase=Phase::BiosToGameTest;status(Status::Reading,"Opening Confidential Mission Game Assignments");}}else settled=0;break;
   case Phase::BiosToGameTest:
    if(n.screen!=Screen::Bios){fail(Status::Failed,"Original system menu changed; use manual Service");break;}
    // Measured USA BIOS manager selects rows1..12; readback must acknowledge each Service input.
    if(awaiting){if(n.cursor==expected){awaiting=false;attempts=0;}else break;}
    if(n.cursor<1||n.cursor>12){fail(Status::Failed,"Original system menu changed; use manual Service");break;}
    if(n.cursor==11){pulse(1);phase=Phase::WaitMain;}
    else{expected=n.cursor==12?1:n.cursor+1;awaiting=true;pulse(2);}break;
   case Phase::WaitMain:
    if(n.screen==Screen::TestMain&&n.cursor==6){phase=Phase::ToAssignments;awaiting=false;}break;
   case Phase::ToAssignments:
    if(n.screen!=Screen::TestMain){fail(Status::Failed,"Original game menu changed; use manual Service");break;}
    if(navigate(n.cursor,2,7)){pulse(1);phase=Phase::WaitAssignments;}break;
   case Phase::WaitAssignments:
    if(n.screen==Screen::Assignments&&n.cursor==9) {
     if(!validData(n.data)||!entryProtectedEqual(enteredData,n.data,editMask)){fail(Status::Failed,"Unrelated native settings changed on Service entry; use manual Service");break;}
     // Service may have refreshed data; only touched fields are merged with this latest native snapshot.
     baseline=n.data;baselineKnown=true;field=0;awaiting=waitValue=false;phase=Phase::Edit;status(Status::Applying,"Applying staged settings through original controls");
    }break;
   case Phase::Edit:
    if(n.screen!=Screen::Assignments||!validData(n.data)||!protectedEqual(baseline,n.data,editMask)){fail(Status::Failed,"Unrelated native settings changed; use manual Service");break;}
    if(waitValue){if(value(n.data,field)==expected){waitValue=false;attempts=0;}else{if(++attempts>120)fail(Status::Failed,"Native option did not acknowledge a change; use Service");break;}}
    while(field<FieldCount&&!(editMask&(1u<<field)))++field;
    if(field==FieldCount){phase=Phase::ExitAssignments;awaiting=false;status(Status::Saving,"Saving through the original game Test menu");break;}
    if(value(n.data,field)==target[field]){++field;break;}
    if(navigate(n.cursor,field,10)){unsigned old=value(n.data,field);expected=old==8?(field==4?3:1):old+1;waitValue=true;attempts=0;pulse(1);}break;
   case Phase::ExitAssignments:
    if(n.screen!=Screen::Assignments){fail(Status::Failed,"Native assignments exited unexpectedly; use Service");break;}
    if(navigate(n.cursor,9,10)){pulse(1);phase=Phase::WaitMainExit;}break;
   case Phase::WaitMainExit:
    if(n.screen==Screen::TestMain){phase=Phase::ExitMain;awaiting=false;}break;
   case Phase::ExitMain:
    if(n.screen!=Screen::TestMain){fail(Status::Failed,"Native save menu changed; use manual Service");break;}
    if(navigate(n.cursor,6,7)){pulse(1);phase=Phase::WaitBiosExit;settled=0;}break;
   case Phase::WaitBiosExit:
    if(n.screen==Screen::Bios&&n.cursor==12) {
     if(!n.persisted||n.save<=saveAtStart||!protectedEqual(baseline,n.data,editMask)){fail(Status::Failed,"Native save readback failed; use manual Service");break;}
     bool same=true;for(unsigned f=0;f<FieldCount;++f)if((editMask&(1u<<f))&&value(n.data,f)!=target[f])same=false;
     if(!same){fail(Status::Failed,"Native save was incomplete; pending edits retained");break;}
     // System/region/coin data must not be changed by the gameplay adapter.
     if(!std::equal(before.begin(),before.begin()+36,n.eeprom.begin())){fail(Status::Failed,"Native system data changed; use manual Service");break;}
     if(++settled>=30){pulse(1);phase=Phase::WaitGame;status(Status::Restarting,"Restarting through original system EXIT");}
    }else settled=0;break;
   case Phase::WaitGame:
    if(n.screen==Screen::Game&&n.persisted&&n.boot>bootAtStart) {
     if(n.readBoot!=n.boot){status(Status::Verifying,"Reading native settings after original game boot");break;}
     if(!baselineKnown||!protectedEqual(baseline,n.data,editMask)){fail(Status::Failed,"Restart readback changed unrelated data; use Service");break;}
     bool same=true;for(unsigned f=0;f<FieldCount;++f)if((editMask&(1u<<f))&&value(n.data,f)!=target[f])same=false;
     if(!same){fail(Status::Failed,"Restart readback failed; pending edits retained");break;}
     latest=n.data;s.pendingMask=0;s.busy=false;s.serviceRecovery=false;entered=false;phase=Phase::Idle;
     for(unsigned f=0;f<FieldCount;++f)s.value[f]=static_cast<uint8_t>(value(latest,f));
     status(Status::Complete,"Native settings saved and restarted with verified readback");readyAvailability();
    }break;
   case Phase::Idle:break;
  }
 }
};
}
