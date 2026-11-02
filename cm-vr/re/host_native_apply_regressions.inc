// Actual host integration at the backend API boundary. Persistence/service
// protocol behavior is verified by the separate native original-engine suite.
void nativeApplyHostRegressions(){
 using cm::options::Status;
 reset();menuOpen=true;emuRunning=false;nativeOptionsWasBusy=false;pauseRequest=0;focused=true;
 cm::options::fixture={};cm::options::fixture.busy=true;cm::options::fixture.status=Status::Saving;
 cm::options::fixture.pendingMask=5;cm::options::fixture.value[0]=7;
 gameReady=false;drains.clear();int startsBefore=emu.starts;
 for(int n=0;n<5;n++){
  applyPause();drainFrame();idleFrame();
  check(menuOpen&&emuRunning&&nativeOptionsWasBusy&&pauseRequest==-1,"actual host keeps native save/restart progress card visible with engine running across multiple frames");
  check(cm::options::lastAllowed&&cm::options::lastFocused&&!cm::options::lastManual,"in-progress native apply retains permitted context across original boot/loading readiness changes");
 }
 check(emu.starts==startsBefore+1&&drains.size()==10,"busy menu starts the engine only once and continues original engine frame consumption");
 bool alternates=drains.size()==10;for(size_t n=0;n<drains.size();n++)alternates&=drains[n]==(n%2?10:0);
 check(alternates,"actual visible and idle frame guards drain original engine while native apply owns the open menu");
 check(cm::options::fixture.pendingMask==5&&cm::options::fixture.value[0]==7,"host progress frames preserve backend staged native fields");
 cm::options::fixture.busy=false;cm::options::fixture.status=Status::Complete;
 int resetsBefore=menu::resets;newGame=false;applyPause();
 check(!menuOpen&&emuRunning&&!nativeOptionsWasBusy&&newGame.load()&&menu::resets==resetsBefore+1,"actual completed native restart closes/reset menu and schedules fresh gun/player state");
 check(lastP1==0&&lastP2==0&&startSwallow&&skipSwallow&&!serviceControls.active&&!serviceRequest,"native apply completion neutralizes both native slots and consumes held menu/gameplay buttons");
 for(Status status:{Status::Failed,Status::Interrupted}){
  reset();menuOpen=true;emuRunning=true;nativeOptionsWasBusy=true;pauseRequest=0;menuFresh=false;
  cm::options::fixture={};cm::options::fixture.status=status;cm::options::fixture.pendingMask=9;cm::options::fixture.value[0]=6;
  cm::options::fixture.serviceRecovery=status==Status::Failed;
  startsBefore=emu.starts;int stopsBefore=emu.stops;newGame=false;applyPause();
  check(menuOpen&&!emuRunning&&menuFresh&&!nativeOptionsWasBusy&&pauseRequest==-1&&!newGame.load(),"actual failed/interrupted native apply keeps progress/result menu open with game paused");
  check(emu.starts==startsBefore&&emu.stops==stopsBefore+1&&cm::options::fixture.pendingMask==9&&cm::options::fixture.value[0]==6,"host failure retains staged native edits and stops the engine without restarting it");
  check(serviceControls.active==cm::options::fixture.serviceRecovery&&!serviceControls.pending&&!serviceRequest&&!serviceNextRequest&&!serviceSelectRequest,"failed native Service recovery adopts manual mode without injecting another Test/Next/Select pulse");
  if(serviceControls.active)check(serviceControls.bootGeneration==serviceGameBootGeneration.load(),"manual native recovery observes the current real game boot generation");
  drains.clear();drainFrame();idleFrame();check(drains.empty(),"ordinary paused result menu resumes last image without draining a stopped engine");
 }
 cm::options::fixture={};nativeOptionsWasBusy=false;reset();
}
