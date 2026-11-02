using namespace vr::xr;
using namespace vr::xr::menu;
int checks = 0;
void check(bool yes, const char* name) { ++checks; if (!yes) { std::cerr << "FAIL: " << name << '\n'; std::exit(1); } }
bool near(float a, float b) { return std::abs(a-b)<.00001f; }
void restart(const std::string& ini) {
 config::Settings::instance().reset(); config::cfgdb=config::IniFile(); config::cfgdb.load(ini);
 config::persisted=ini; config::writeCount=0; config::Settings::instance().load(false);
}
Input input;
glm::vec2 middle(SettingsRect r) { return {r.x+r.w*.5f,r.y+r.h*.5f}; }
void point(int hand, glm::vec2 uv, bool click=false) {
 const auto layout=settingsLayout();
 glm::vec3 at=model*glm::vec4((uv.x-.5f)*layout.width,(.5f-uv.y)*layout.height,0.f,1.f);
 auto& ray=input.ray[hand];ray.valid=true;ray.origin=at+glm::vec3(model[2])*.6f;ray.dir=-glm::vec3(model[2]);ray.click=click;
}
void frame() { update(input);for(auto& h:input.hand)h={};for(auto&r:input.ray)r.click=false; }
void click(int hand, glm::vec2 uv) {point(hand,uv);frame();point(hand,uv,true);frame();}
void rowClick(int hand,int index,float x=.2f) {auto uv=middle(settingsRowRect(panel(),index));uv.x=x;click(hand,uv);}
void tab(int t) {click(activeHand<0?1:activeHand,middle(settingsTabRect(t)));}
void capture(const char* name) {
 paintSettings(panel());std::ofstream file(std::string(name)+".ppm",std::ios::binary);file<<"P6\n"<<canvasWidth<<' '<<canvasHeight<<"\n255\n";
 for(size_t i=0;i<pixels.size();i+=4)file.write((char*)&pixels[i],3);
}
int main() {
 std::ifstream fontFile("Roboto-Bold.ttf",std::ios::binary);
 std::vector<unsigned char> fontBytes((std::istreambuf_iterator<char>(fontFile)),{});
 check(!fontBytes.empty()&&stbtt_InitFont(&font,fontBytes.data(),0),"actual CM font loads for CPU raster capture");
 restart("[config]\nvr.GunAngleZeroVersion=2\nvr.GunAngle=4\nvr.WorldScale=.113456\nvr.ReticleMinSize=.22\nvr.ReticleMaxSize=.75\nvr.ReticleShrinkDistance=7.3\nvr.GunScale=.68\nvr.HandScale=1\nSentinel=73\n");
 input.perHand=true;open(glm::mat4(1.f));point(1,{.5f,.5f});frame();
 check(panel().activeTab==0&&panel().rows.size()==6,"Gun opens on six supported primary controls");
 check(panel().rows.front().label=="Gun model"&&panel().rows.back().label=="Advanced","main Gun page starts with gun model and ends with Advanced");
 check(near(config::VrWorldScale,.113456f)&&near(config::VrReticleMinSize,.22f),"opening menu preserves exact saved numeric tuning");
 capture("gun");
 rowClick(1,1);check(int(config::VrGunAngle)==5,"row body increments Gun angle regardless of left half");
 rowClick(1,1,.755f);check(int(config::VrGunAngle)==4,"explicit minus button decrements");
 rowClick(1,1,.925f);check(int(config::VrGunAngle)==5,"explicit plus button increments");
 rowClick(1,2);check(int(config::VrLaserWidth)==2&&bool(config::VrLaser),"Laser body advances Normal to 2x");
 auto laser=choiceRects(panel(),2);click(1,middle(laser[4]));check(int(config::VrLaserWidth)==4,"10x pill selects 10x exactly");
 click(1,middle(laser[0]));check(!bool(config::VrLaser)&&int(config::VrLaserWidth)==4,"Off preserves chosen beam thickness");
 check(!bool(config::loadBool("config","vr.Laser",true))&&config::loadInt("config","vr.LaserWidth")==4,"menu writes both Laser persistence keys");
 glm::vec3 contact;check(pointer(1,contact)&&!pointer(0,contact),"only active controller exposes real panel contact with Laser Off");
 const auto uv=middle(settingsRowRect(panel(),2));point(1,uv);frame();glm::vec3 wanted=model*glm::vec4((uv.x-.5f)*1.1f,(.5f-uv.y)*(880.f/1200.f*1.1f),0,1);
 check(pointer(1,contact)&&glm::length(contact-wanted)<.00001f,"contact is at measured ray-panel intersection");
 // Inactive input, including stale legacy aggregates, cannot operate the menu.
 input.select=true;input.right=true;input.hand[0].right=true;frame();input.select=input.right=false;
 check(!bool(config::VrLaser)&&int(config::VrLaserWidth)==4,"inactive stick and stale aggregate buttons are ignored");
 point(0,middle(settingsRowRect(panel(),1)),true);frame();
 check(activeHand==0&&int(config::VrGunAngle)==5,"other-hand transfer consumes its trigger press");
 check(pointer(0,contact)&&!pointer(1,contact),"pointer ownership transfers together with controls");
 point(0,middle(settingsRowRect(panel(),1)),true);frame();check(int(config::VrGunAngle)==6,"next fresh owner trigger selects after transfer");
 input.hand[1].right=true;frame();check(int(config::VrGunAngle)==6,"old owner stick cannot change active selection");
 input.hand[0].right=true;frame();check(int(config::VrGunAngle)==7,"active owner stick changes active selection");
 const auto activeRay=input.ray[0];input.ray[0].valid=false;frame();
 check(pointerHand()==-1&&!pointer(0,contact),"loss of owner tracking clears beam owner and contact");input.ray[0]=activeRay;
 point(0,{1.1f,.5f});frame();check(pointerHand()==0&&!pointer(0,contact),"tracked owner survives panel miss without stale contact");
 rowClick(0,4);check(knock==0,"Haptics samples only the controller which owns the menu");
 rowClick(0,5);check(panel().back&&panel().rows.size()==5,"Gun Advanced exposes supported Gun size, reticle sizes and Reload");
 check(panel().rows[0].label=="Gun size"&&panel().rows[4].label=="Reload","CM Advanced preserves Reload and exposes existing sizing");
 capture("gun-advanced");
 rowClick(0,0);check(near(config::VrHandScale,1.05f)&&near(config::VrGunScale,.68f),"native Gun size edits HandScale only");
 nativeGun=false;rowClick(0,0);check(near(config::VrGunScale,.73f)&&near(config::VrHandScale,1.05f),"folder gun sizing preserves separate native scale");nativeGun=true;
 input.back=true;frame();input.back=false;check(isOpen()&&!panel().back&&panel().activeTab==0,"generic back closes Advanced first");
 tab(1);check(panel().activeTab==1&&panel().rows.size()==3&&panel().rows[1].label=="Game zoom","World tab preserves game-specific CM zoom");
 capture("world");auto widen=choiceRects(panel(),0);click(0,middle(widen[2]));check(int(config::VrWidenMode)==2&&bool(config::VrWiden),"Fade pill edits mode and legacy Wider bool");
 check(config::loadInt("config","vr.WidenMode")==2&&config::loadBool("config","vr.Widen"),"Wider mode and legacy key both persist");
 rowClick(0,2);check(panel().rows.size()==3&&panel().rows[0].label=="World size"&&panel().rows[1].label=="HUD size","World Advanced shows exact numeric sizes");capture("world-advanced");
 check(near(config::VrWorldScale,.113456f),"World Advanced leaves exact numeric world value untouched");
 tab(2);check(panel().activeTab==2&&panel().rows.size()==5,"System tab is third and has five actions");capture("system");
 rowClick(0,3);check(panel().dialog&&panel().rows[0].label=="Cancel","VR reset opens safe Cancel-first confirmation");
 input.back=true;frame();input.back=false;check(isOpen()&&!panel().dialog&&near(config::VrReticleMinSize,.22f),"back cancels reset and preserves settings");
 rowClick(0,4);check(panel().dialog&&quits==0,"Quit requires explicit confirmation");capture("quit-confirm");rowClick(0,0);check(!panel().dialog&&quits==0,"Cancel leaves native game open");
 rowClick(0,3);rowClick(0,1);check(!panel().dialog&&near(config::VrReticleMinSize,.20f)&&near(config::VrReticleMaxSize,.45f)&&near(config::VrReticleShrinkDistance,6.5f),"confirmed reset restores captured reticle defaults");
 check(config::loadInt("config","Sentinel")==73,"VR reset preserves unrelated native/operator settings");
 check(config::VrLaser&&int(config::VrLaserWidth)==1&&int(config::VrWidenMode)==1,"confirmed reset restores new laser/widen defaults");
 check(near(config::VrGunScale,.68f)&&near(config::VrHandScale,1.f),"VR reset restores both supported gun scales");
 tab(3);check(panel().activeTab==3&&panel().rows.size()==1&&panel().rows[0].label=="Open Service Menu","unmeasured native options use actual Service fallback without invented choices");
 using namespace cm::options;testNative.available=true;testNative.supportedMask=(1u<<0)|(1u<<4);
 testNative.minimum[0]=1;testNative.maximum[0]=8;testNative.value[0]=4;
 testNative.minimum[4]=3;testNative.maximum[4]=8;testNative.value[4]=5;testNative.status=Status::Ready;testNative.message="Native options ready";
 check(panel().rows.size()==3&&panel().rows[1].label=="Life"&&panel().rows[2].label=="Apply settings and restart","In-game exposes only supported adapter fields and Apply action");
 const int writes=config::writeCount;rowClick(0,0);check(testNative.value[0]==5&&testNative.pendingMask==1&&config::writeCount==writes,"native field edit is staged through adapter without VR preference writes");
 capture("in-game-pending");testNative.supportedMask=31;
 for(int n=1;n<4;++n){testNative.minimum[n]=1;testNative.maximum[n]=8;testNative.value[n]=4;}
 testNative.value[4]=3;capture("in-game-all-fields");testNative.supportedMask=17;
 rowClick(0,2);check(applyCalls==1&&isOpen()&&panel().locked,"accepted Apply keeps progress menu visible and disabled");
 capture("in-game-busy");input.back=true;frame();input.back=false;check(isOpen()&&applyCalls==1,"busy operation cannot be closed or launched twice");
 testNative.busy=false;testNative.status=Status::Failed;testNative.message="Native apply failed; review and retry";
 close();open(glm::mat4(1.f));point(0,{.2f,.4f});frame();tab(3);
 check(testNative.pendingMask==1&&discardCalls==0,"menu close/reopen preserves staged native edits and never discards them");
 // Shared painting geometry: row bodies, precise pills, footer, gaps, rear and nonfinite rays.
 tab(0);auto p=panel();auto r=settingsRowRect(p,1);
 check(settingsAt(p,{.2f,r.y+r.h*.5f}).direction==0,"left row body has no hidden decrement zone");
 check(settingsAt(p,{.2f,r.y+r.h+.005f}).kind==SettingsHit::Kind::None,"row gaps do not click adjacent cards");
 check(settingsAt(p,{.5f,.97f}).kind==SettingsHit::Kind::None,"footer is not an accidental row target");
 check(settingsAt(p,{NAN,.5f}).kind==SettingsHit::Kind::None,"nonfinite pointer positions are rejected");
 check(!settingsPanelHit(model,p,{0,0,-3},{0,0,1}),"rear-facing panel ray is rejected");
 close();const auto persisted=config::persisted;restart(persisted);
 check(near(config::VrReticleMinSize,.2f)&&int(config::VrLaserWidth)==1&&int(config::VrWidenMode)==1,"menu reset survives actual INI cold reload");
 open(glm::mat4(1.f));input={};input.perHand=true;point(1,{.2f,.4f});frame();tab(2);rowClick(1,4);rowClick(1,1);check(quits==1,"confirmed Quit executes exactly once");
 input.hand[1].select=true;frame();check(quits==1,"quit latch ignores subsequent input");
 std::cout<<checks<<" production menu / settings / geometry checks PASS\n";
}
