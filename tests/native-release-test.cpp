#include <cassert>
#include <cstdio>
#include "../core/rend/vr/native_release.h"
int main(){vr::xr::NativeReleaseGate gate;
 auto one=gate.request(0);assert(!gate.observed(0,one));
 for(int xr=0;xr<1000;xr++)assert(!gate.observed(0,one));
 gate.consume(0,false);assert(!gate.observed(0,one));
 gate.consume(1,true);assert(!gate.observed(0,one));
 gate.consume(0,true);assert(gate.observed(0,one));
 auto two=gate.request(0);assert(!gate.observed(0,two));
 auto three=gate.request(0);assert(!gate.observed(0,three));gate.consume(0,true);
 assert(gate.observed(0,two)&&gate.observed(0,three));
 assert(gate.request(-1)==0&&gate.observed(-1,0));
 std::puts("PASS native-consumed release gate; 1000 stalled XR polls stay neutral");
}
