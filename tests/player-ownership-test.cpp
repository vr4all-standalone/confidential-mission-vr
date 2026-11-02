#include <cassert>
#include <cstdio>
#include "../core/rend/vr/player_ownership.h"
using vr::xr::PlayerOwnership;
int checks=0;
void check(bool x){++checks;if(!x){std::fprintf(stderr,"FAIL check %d\n",checks);std::abort();}}
int main(){
 for(int layout=0;layout<2;layout++)for(int first=0;first<2;first++){
  PlayerOwnership p;PlayerOwnership::Input in;in.layout=layout;in.known=true;in.allowed=true;in.canJoin=true;in.tracked[0]=in.tracked[1]=true;
  p.update(in);in.start[first]=true;auto out=p.update(in);
  check(out.start[0]&&!out.start[1]);check(p.soloHand()==first&&p.soloPlayer()==0);
  in.roster=1;in.start[first]=false;p.update(in);
  in.pull[first]=1;p.update(in);in.pull[1-first]=1;out=p.update(in);
  check(p.soloHand()==1-first&&out.release[0]);
  in.start[1-first]=true;out=p.update(in);check(out.start[0]&&!out.start[1]);
  in.start[1-first]=false;p.update(in);in.start[first]=true;out=p.update(in);
  check(!p.dual()&&out.start[1]&&p.pendingPlayer()==1);
  in.roster=3;out=p.update(in);check(p.dual()&&p.hand(0)==layout&&p.hand(1)==1-layout);
  check(out.start[1]&&!out.start[0]);
  in.start[first]=false;in.pull[0]=in.pull[1]=0;p.update(in);
  p.suspend();p.suspend();in.roster=2;in.layout=1-layout;out=p.update(in);
  check(!p.dual()&&p.soloPlayer()==1&&p.soloHand()==1-layout);
  in.layout=layout;out=p.update(in);check(p.soloHand()==1-layout);
  // Re-arm after focus, then swap a sole native P2 without renaming it.
  in.pull[layout]=1;out=p.update(in);check(p.soloPlayer()==1&&p.soloHand()==layout&&out.release[1]);
  in.start[1-layout]=true;out=p.update(in);check(out.start[0]&&p.pendingPlayer()==0);
 }
 {
  PlayerOwnership p;PlayerOwnership::Input in;in.known=in.allowed=in.canJoin=true;in.tracked[0]=in.tracked[1]=true;in.layout=0;p.update(in);
  in.start[0]=in.start[1]=true;auto out=p.update(in);check(out.start[0]&&!out.start[1]&&p.soloHand()==0);
  in.start[0]=in.start[1]=false;in.roster=1;p.update(in);in.pull[0]=in.pull[1]=1;p.update(in);check(p.soloHand()==0);
  p.suspend();out=p.update(in);check(!out.release[0]&&p.soloHand()==0);in.pull[0]=in.pull[1]=0;p.update(in);in.pull[1]=1;p.update(in);check(p.soloHand()==1);
  in.layout=1;p.update(in);check(p.soloHand()==1);
  in.start[0]=true;in.canJoin=false;out=p.update(in);check(!out.start[1]&&p.pendingPlayer()==-1);
  in.start[0]=false;in.canJoin=true;p.update(in);in.start[0]=true;in.now=1;out=p.update(in);check(out.start[1]);in.now=5000000002ULL;out=p.update(in);check(!out.start[1]&&p.pendingPlayer()==-1);
  in.roster=3;p.update(in);check(p.dual());
  in.allowed=false;in.roster=0;p.update(in);check(p.dual());
  in.allowed=true;in.roster=2;in.known=false;p.suspend();p.update(in);check(p.dual());in.known=true;p.update(in);check(!p.dual()&&p.soloPlayer()==1&&p.soloHand()==0);
 }
 std::printf("PASS %d ownership transition checks\n",checks);
}
