#include "rend/vr/cm_game_options_model.h"
#include <cassert>
#include <iostream>
using namespace vr::xr::cm::options;
using namespace vr::xr::cm::options::detail;
Data original() { Data d{}; const char id[]="0RCBCMF "; for (int i=0;i<8;++i)d[i]=id[i]; d[8]=0x63;d[9]=0x65;d[10]=3;d[11]=0x0d;d[24]=2;d[25]=d[26]=1;d[28]=1;return d; }
struct OriginalMenuFixture {
 Native n{};Data working{};unsigned oldButtons=0;bool partial=false,protectedFault=false,holdRead=false;unsigned boots=1,tests=0,changes=0;
 OriginalMenuFixture(bool ranking=false){n.screen=Screen::Game;n.persisted=true;n.data=original();if(ranking)n.data[27]=1;n.data[9]=0xa5;n.boot=n.readBoot=boots;save();}
 void save(){for(unsigned i=0;i<32;++i)n.eeprom[44+i]=n.eeprom[76+i]=n.data[i];n.eeprom[38]=n.eeprom[39]=n.eeprom[42]=n.eeprom[43]=32;auto c=nativeCrc(n.data.data(),32);n.eeprom[36]=n.eeprom[40]=c&255;n.eeprom[37]=n.eeprom[41]=c>>8;}
 void consume(unsigned buttons){unsigned edge=buttons&~oldButtons;oldButtons=buttons;if(!edge)return;
  if(n.screen==Screen::Game){assert(edge==1);n.screen=Screen::Bios;n.cursor=12;++tests;}
  else if(n.screen==Screen::Bios){if(edge==2)n.cursor=n.cursor==12?1:n.cursor+1;else if(n.cursor==11){n.screen=Screen::TestMain;n.cursor=6;working=n.data;}else{assert(n.cursor==12);n.screen=Screen::Game;n.boot=++boots;n.readBoot=holdRead?0:n.boot;}}
  else if(n.screen==Screen::TestMain){if(edge==2)n.cursor=(n.cursor+1)%7;else if(n.cursor==2){n.screen=Screen::Assignments;n.cursor=9;n.data=working;n.data[27]=0;if(protectedFault)n.data[25]^=1;}else{assert(n.cursor==6);n.screen=Screen::Bios;n.cursor=12;n.data=working;save();++n.save;if(partial)++n.saveFailure;}}
  else if(n.screen==Screen::Assignments){if(edge==2)n.cursor=(n.cursor+1)%10;else if(n.cursor==9){working=n.data;n.screen=Screen::TestMain;n.cursor=2;}else{assert(n.cursor<5);auto f=n.cursor;unsigned v=value(n.data,f);v=v==8?(f==4?3:1):v+1;switch(f){case 0:n.data[8]=(n.data[8]&31)|((v-1)<<5);break;case 1:n.data[9]=(n.data[9]&31)|((v-1)<<5);break;case 2:n.data[10]=v-1;break;case 3:n.data[11]=(n.data[11]&3)|((v-1)<<2);break;case 4:n.data[24]=v-1;break;}++changes;}}
 }
};
void originalProtocol(bool partial=false,bool ranking=false){OriginalMenuFixture native(ranking);native.partial=partial;Controller c;c.context(true,true,false);c.tick(native.n,0);auto before=native.n.data;assert(c.stage(Field::GameDifficulty,6));assert(c.stage(Field::Life,7));assert(c.apply(1));
 for(unsigned frame=1;frame<8000&&c.snapshot().busy;++frame){c.tick(native.n,frame*17);native.consume(c.buttons());c.acknowledge(c.buttons());}
 auto s=c.snapshot();assert(!s.busy);if(partial){assert(s.status==Status::Failed&&s.pendingMask==17&&s.serviceRecovery);return;}
 assert(s.status==Status::Complete&&s.pendingMask==0&&native.boots==2&&native.tests==1&&native.changes==6);assert(s.value[0]==6&&s.value[1]==6&&s.value[2]==4&&s.value[3]==4&&s.value[4]==7);assert(entryProtectedEqual(before,native.n.data,17));
 // A second reset reuses the same original service lifecycle, rather than resetting emulator/audio machinery.
 assert(c.stage(Field::Life,6));assert(c.apply(200000));for(unsigned frame=1;frame<8000&&c.snapshot().busy;++frame){c.tick(native.n,200000+frame*17);native.consume(c.buttons());c.acknowledge(c.buttons());}assert(c.snapshot().status==Status::Complete&&native.boots==3&&native.tests==2);
}
void originalFaults(){
 for(unsigned fault=0;fault<2;++fault){OriginalMenuFixture n;n.protectedFault=fault==0;n.holdRead=fault==1;Controller c;c.context(true,true,false);c.tick(n.n,0);assert(c.stage(Field::GameDifficulty,5));assert(c.apply(1));
  for(unsigned frame=1;frame<5000&&c.snapshot().busy;++frame){c.tick(n.n,frame*17);n.consume(c.buttons());c.acknowledge(c.buttons());if(fault==1&&n.boots==2&&c.snapshot().status==Status::Verifying)break;}
  if(fault==0)assert(c.snapshot().status==Status::Failed&&c.snapshot().pendingMask==1&&c.snapshot().serviceRecovery);
  else{assert(c.snapshot().busy&&c.snapshot().status==Status::Verifying&&c.snapshot().pendingMask==1);n.n.readBoot=n.n.boot;c.tick(n.n,90000);assert(c.snapshot().status==Status::Complete);}
 }
 OriginalMenuFixture n;Controller c;c.context(true,true,false);c.tick(n.n,0);assert(c.stage(Field::GameDifficulty,5));assert(c.apply(1));c.tick(n.n,2);
 for(unsigned f=1;f<60;++f)c.tick(n.n,f*17);assert(c.buttons()==1&&c.snapshot().busy); // no cabinet poll: never pretend a press was consumed
 c.watchdog(120002);assert(!c.snapshot().busy&&c.buttons()==0&&c.snapshot().status==Status::Failed&&c.snapshot().pendingMask==1);
}
int main() {
 Data zeros{};assert(nativeCrc(zeros.data(),zeros.size())==0x4231);
 Controller c; Native n{}; c.context(true,true,false); c.tick(n,0); assert(!c.snapshot().available); assert(!c.apply(0));
 n.screen=Screen::Game;n.persisted=true;n.boot=n.readBoot=1;n.data=original(); c.tick(n,1); auto s=c.snapshot(); assert(s.available&&s.supportedMask==31&&s.value[0]==4&&s.value[4]==3);n.persisted=false;c.tick(n,2);assert(!c.snapshot().available);n.persisted=true;c.tick(n,3);
 assert(!c.stage(Field::Life,2)); assert(!c.stage(Field::Life,9)); assert(c.stage(Field::GameDifficulty,6)); assert(c.stage(Field::Life,8));
 n.data[9]=0xa5;c.tick(n,2);s=c.snapshot();assert(s.value[0]==6&&s.value[1]==6&&s.pendingMask==17);
 assert(c.apply(3));assert(c.snapshot().busy);assert(!c.stage(Field::Life,3));
 c.context(true,false,false);s=c.snapshot();assert(!s.busy&&s.status==Status::Interrupted&&s.pendingMask==17);
 c.context(true,true,false); c.tick(n,4); assert(c.apply(5)); c.tick(n,6);c.context(true,false,false);assert(c.snapshot().serviceRecovery);
 Controller ranking;ranking.context(true,true,false);n.data=original();n.data[27]=1; ranking.tick(n,0);assert(ranking.snapshot().available);assert(ranking.stage(Field::GameDifficulty,6));assert(ranking.apply(0));
 Controller timeout;timeout.context(true,true,false);n.data=original();timeout.tick(n,0);assert(timeout.stage(Field::Life,4));assert(timeout.apply(0));timeout.tick(n,120001);assert(timeout.snapshot().status==Status::Failed&&!timeout.snapshot().busy);
 Data a=original(),b=a;b[8]=0xc3;assert(protectedEqual(a,b,1));assert(!protectedEqual(a,b,0));b[25]=0;assert(!protectedEqual(a,b,1));
 // A native save must retain every untouched bit, including calibration/ranking/cabinet.
 Eeprom e{};for(int i=0;i<32;++i)e[44+i]=e[76+i]=a[i];e[38]=e[39]=e[42]=e[43]=32;auto crc=nativeCrc(a.data(),32);e[36]=e[40]=crc&255;e[37]=e[41]=crc>>8;Data decoded{};assert(decodeEeprom(e,decoded)&&decoded==a);e[90]^=1;assert(!decodeEeprom(e,decoded));
 originalProtocol();originalProtocol(true);originalProtocol(false,true);originalFaults();
 Data rankingOld=original(),rankingNew=rankingOld;rankingOld[27]=1;assert(entryProtectedEqual(rankingOld,rankingNew,0));assert(!entryProtectedEqual(rankingNew,rankingOld,0));rankingNew[25]=0;assert(!entryProtectedEqual(rankingOld,rankingNew,0));
 std::cout<<"CM native model: full original protocol/repeated restart/partial save, availability, authorized original ranking closure, touched merge, busy, focus, timeout, protected bits and mirrored native CRC checks passed\n";
}
