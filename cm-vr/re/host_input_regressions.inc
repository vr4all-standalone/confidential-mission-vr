// Focused integration regressions execute the mechanically extracted production
// host paths. Only runtime pose/native snapshot seams are controlled by fixtures.
void hostRegressions(){
 for(bool left:{false,true}){
  reset(left);frame(0);check(menuValue()==(left?"P1 left / P2 right":"P1 right / P2 left"),"real Two players menu shows saved configured dual layout");
  toggleMenu();frame(1);check(menuValue()==(left?"P1 right / P2 left":"P1 left / P2 right")&&p1Hand()==Right,"real Two players menu change preserves current solo owner");
  check(config::savedKey=="vr.TwoPlayerP1Hand"&&config::savedValue==(left?"right":"left"),"real Two players menu saves its configured hand word through normal settings path");
  for(int first:{Left,Right}){
   reset(left);players=0;frame(0);starts[first]=true;frame(1);
   check(!dualWield&&p1Hand()==first&&lastP1==4&&lastP2==0,"first fresh Start in either hand selects original P1");
   check(p1Armed&&guns[0].player==0&&guns[1].player==1,"first Start arms gun without swapping native slot identities");
   players=1;frame(2);check(lastP1==4&&lastP2==0,"held first Start stays original P1 after native acknowledgement");
   starts[first]=false;frame(3);starts[1-first]=true;frame(4);
   check(!dualWield&&lastP1==0&&lastP2==4&&p2JoinPending,"unused hand Start requests missing P2 without speculative dual gun");
   players=3;frame(5);int hp=left?Left:Right;
   check(dualWield&&!p2JoinPending&&guns[0].hand==hp&&guns[1].hand==1-hp,"configured dual layout waits for original native dual acknowledgement");
   check(lastP1==0&&lastP2==4&&startButtonOwner(1-first)==1,"held joining Start stays native P2 after configured hands apply");
   frame(6);check(lastP1==0&&lastP2==4,"held joining Start stays stable over repeated native dual samples");
   starts[1-first]=false;frame(7);starts[hp]=true;frame(8);layout(!left);frame(9);
   check(lastP1==4&&lastP2==0&&guns[0].hand==1-hp,"held P1 Start stays native P1 across live layout change");
   starts[hp]=false;frame(10);starts[hp]=true;frame(11);
   check(lastP1==0&&lastP2==4,"fresh Start follows the new configured dual layout");
  }
  reset(left);players=0;frame(0);starts[0]=starts[1]=true;frame(1);
  check(!dualWield&&p1Hand()==(left?Left:Right)&&lastP1==4&&lastP2==0,"simultaneous first Starts choose configured stable solo hand and only original P1");
  // The real playerInGame returns false while paused. Repeated sampling must
  // retain the last trusted dual roster/layout until the emulator resumes.
  for(int gate=0;gate<4;gate++){
   reset(left);players=3;frame(0);const int oldP2=guns[1].hand;
   menuOpen=true;emuRunning=false;players=2;layout(!left);
   check(!playerInGame(0)&&!playerInGame(1),"production native roster query is empty while emulator is paused");
   bool p1,p2;
   for(int n=0;n<5;n++)sampleStartButtons(n+1,false,false,gate>=2,gate%2==0,p1,p2);
   check(playerOwnership.dual()&&playerOwnership.nativeRoster()==3,"repeated paused/readiness-gated samples preserve trusted dual roster");
   menuOpen=false;emuRunning=true;frame(10);
   check(!dualWield&&primaryGun().player==1&&primaryGun().hand==oldP2,"native P2 survivor retains original slot and its last physical dual hand after menu layout changes");
   check(lastP1==0&&lastP2==0&&guns[0].player==0&&guns[1].player==1,"P2 solo resume releases missing native P1 with canonical slots intact");
   frame(11);starts[oldP2]=true;frame(12);check(lastP1==0&&lastP2==4,"active survivor hand Start continues original native P2");
   starts[oldP2]=false;frame(13);starts[1-oldP2]=true;writes.clear();frame(14);
   check(!dualWield&&lastP1==4&&lastP2==0&&playerOwnership.pendingPlayer()==0,"unused survivor hand requests missing original P1 without inventing a second gun");
   check(writes.size()==2&&writes[0].player==0&&writes[1].player==1,"P2 solo routes missing-player Start and only survivor gun output to their real slots");
   players=3;frame(15);
   check(dualWield&&lastP1==4&&lastP2==0&&guns[0].hand==(left?Right:Left),"rejoined native P1 acknowledgement applies currently configured layout and retains Start owner");
  }
  for(int gate=0;gate<5;gate++){
   reset(left);frame(0);
   if(gate==0)config::VrDualWield.value=false;
   if(gate==1)config::GGPOEnable.value=true;
   if(gate==2)known=false;
   if(gate==3)trackedHands[0]=false;
   if(gate==4)trackedHands[1]=false;
   starts[Left]=true;frameCode(1);
   check(!dualWield&&lastP1==0&&lastP2==0&&playerOwnership.pendingPlayer()<0,"disabled/online/unknown/untracked missing-player join cannot start either native player");
  }
  reset(left);frame(0);starts[Left]=true;frame(1);
  starts[Left]=false;frame(2);frame(4'100'000'002LL);
  check(!dualWield&&!p2JoinPending&&playerOwnership.pendingPlayer()<0&&lastP2==0,"ignored native join expires without speculative second gun or held buttons");
  reset(left);frame(0);starts[Left]=true;frame(1);config::VrDualWield.value=false;frame(2);
  check(!dualWield&&!p2JoinPending&&lastP2==0,"disabling local joining cancels an outstanding missing-player request");
  reset(left);frame(0);starts[Left]=true;frame(1);players=3;frame(2,0,0,false,true);
  check(dualWield&&!p2JoinPending,"native dual acknowledgement survives temporary tracking loss after a sent join");
  reset(left);players=0;gameReady=false;starts[Left]=true;frameCode(0);gameReady=true;frameCode(1);
  check(lastP1==0&&lastP2==0&&!dualWield,"Start held from loading cannot become a first Start or join when native readiness changes");
  starts[Left]=false;frameCode(2);starts[Left]=true;frameCode(3);
  check(lastP1==4&&lastP2==0&&p1Hand()==Left,"fresh Start after loading chooses original P1 in its physical hand");
  reset(left);frame(0);rawPulls[Right]=1;frameCode(1);
  check((lastP1&3)==1&&lastP2==0,"actual gameplay pulls and updateGun dispatch fire only original solo P1");
  rawPulls[Left]=1;writes.clear();frameCode(2);u64 ticket=guns[0].nativeReleaseTicket;
  check(primaryGun().hand==Left&&ticket!=0&&lastP1==0&&lastP2==0,"fresh other trigger transfers while old hand is held and dispatches native release");
  bool neutral=true;for(auto& w:writes)neutral&=(w.buttons&3)==0;
  check(neutral,"every native slot write on transfer frame is neutral rather than transiently reasserting held fire");
  for(int n=0;n<1000;n++)frameCode(3+n);
  check(lastP1==0&&lastP2==0&&guns[0].nativeReleaseTicket==ticket&&!lightgunReleaseObserved(0,ticket),"1000 XR frames cannot replace original native acknowledgement of release");
  lightgunAcknowledgeRelease(1,true);frameCode(1004);
  check(lastP1==0&&!lightgunReleaseObserved(0,ticket),"acknowledgement of other native slot cannot release P1 transfer gate");
  lightgunAcknowledgeRelease(0,false);frameCode(1005);
  check(lastP1==0&&!lightgunReleaseObserved(0,ticket),"native read with fire still asserted cannot acknowledge release");
  nativePoll();frameCode(1006);
  check((lastP1&3)==1&&lastP2==0&&guns[0].nativeReleaseTicket==0,"actual host dispatch resumes transferred held trigger only after original slot consumes neutral release");
  reset(left);frame(0);frame(1,1,1);
  check(primaryGun().hand==Right&&(lastP1&3)==1&&lastP2==0&&guns[0].nativeReleaseTicket==0,"simultaneous fresh pulls preserve solo owner and do not start conflicting transfer");
  reset(left);frame(0);frame(1,0,1);frame(2,0,1,true,false);frame(3,0,.5f);frame(4,0,.7f);
  check((lastP1&3)==0&&!playerOwnership.triggerAllowed(Right),"tracking reacquisition at half pull then full pull cannot rearm held trigger");
  frame(5,0,.3f);frame(6,0,.7f);
  check((lastP1&3)==1&&playerOwnership.triggerAllowed(Right),"tracking recovery rearms only after physical trigger drops below release threshold");
  reset(left);frame(0);frame(1,0,1);focusFailure();frame(2,0,.5f);frame(3,0,.7f);
  check((lastP1&3)==0&&!playerOwnership.triggerAllowed(Right),"focus recovery at half pull then full pull cannot rearm held trigger");
  frame(4,0,.3f);frame(5,0,.7f);check((lastP1&3)==1,"fresh physical release restores fire after focus interruption");
 }
 // Seed every transient field and due queue for both canonical slots. Execute
 // actual focus/sync failure bodies twice, then feed a higher native snapshot.
 for(int failure=0;failure<2;failure++){
  reset();menuOpen=true;lastP1=lastP2=131;
  for(Gun& g:guns){g.triggerWas=false;g.triggerWait=false;g.pullOutside=true;g.pullAt=g.heldAt=g.reloadAt=g.knockEnds=1'000'000'000LL;g.shotsDue=2;g.autosDue=3;g.clicksDue=4;g.freshPull=g.gunKnown=g.pointedDown=g.offArmed=true;g.gunWas={true,6,6,10,10,true};g.shotCount=42;g.nativeReleaseTicket=lightgunRequireRelease(g.player);}
  for(int twice=0;twice<2;twice++){
   writes.clear();if(failure==0)focusFailure();else syncFailure();
   check(writes.size()==2&&lastP1==0&&lastP2==0,"repeated actual focus/sync failure emits zero to both original native slots");
   for(Gun& g:guns)check(g.triggerWas&&g.triggerWait&&!g.pullOutside&&!g.pullAt&&!g.heldAt&&!g.reloadAt&&!g.knockEnds&&!g.shotsDue&&!g.autosDue&&!g.clicksDue&&!g.freshPull&&!g.gunKnown&&!g.pointedDown&&!g.offArmed&&!g.nativeReleaseTicket&&g.shotCount==42,"actual failure clears all transient shot/reload state and keeps monotonic shotCount");
  }
  const int before=knocks;
  for(Gun& g:guns){vr::snapshots[g.player]={true,5,6,11,11,true};gunKnocks(g,1'000'000'100LL,false);gunKnocks(g,1'000'000'200LL,false);}
  check(knocks==before&&guns[0].shotCount==42&&guns[1].shotCount==42,"higher post-interruption native shot snapshots cannot replay stale recoil or queued clicks");
 }
 reset();
 for(Gun& g:guns){g.gunKnown=g.freshPull=true;g.pullAt=g.heldAt=g.knockEnds=1'000'000'000LL;g.shotsDue=g.autosDue=g.clicksDue=1;g.gunWas={true,6,6,10,10,false};g.shotCount=77;const float none[2]{};const bool tracked[2]{true,true};XrSpaceLocation loc[2];updateGun(g,1'000'000'001LL,loc,tracked,none,0,false,false);check(!g.heldAt&&!g.knockEnds&&!g.gunKnown&&!g.freshPull&&!g.shotsDue&&!g.autosDue&&!g.clicksDue&&g.shotCount==77,"actual paused updateGun clears native snapshot and held-time recoil state without resetting shot serial");vr::snapshots[g.player]={true,5,6,11,11,false};gunKnocks(g,1'000'000'002LL,false);}
 check(knocks==0&&guns[0].shotCount==77&&guns[1].shotCount==77,"one-frame menu pause followed by higher snapshot cannot replay a pre-menu shot");
 reset();frame(0);rawPulls[0]=rawPulls[1]=1;newGame=true;resetForNewGame();
 check(guns[0].triggerWas&&guns[0].triggerWait&&guns[1].triggerWas&&guns[1].triggerWait&&!newGame.load(),"actual new-game branch guards held triggers on both native slots");
 frameCode(1);check((lastP1&3)==0&&(lastP2&3)==0,"held triggers across restart cannot fire before release");
}
