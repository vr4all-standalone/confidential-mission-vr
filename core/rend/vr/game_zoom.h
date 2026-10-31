// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace vr::zoom {
struct Hand { bool tracked{},grip{},click{}; float stickX{},stickY{}; };
class Session {
    std::array<bool,2> waiting_{true,true},down_{},used_{};
    bool enabled_{}; int layout_{-1}; double noticeAt_{-100.};
public:
    bool enabled() const { return enabled_; }
    void suspend(bool clearNotice=true) { waiting_={true,true};down_=used_={};if(clearNotice)noticeAt_=-100.; }
    void reset() { enabled_=false;layout_=-1;suspend(); }
    void set(bool enabled,double now) { if(enabled_!=enabled){enabled_=enabled;noticeAt_=now;} }
    bool notice(double now) const { return now>=noticeAt_ && now-noticeAt_<1.; }
    void consume(int hand) { if(hand>=0&&hand<2&&down_[hand])used_[hand]=true; }
    bool update(const std::array<Hand,2>& hands,bool active,int layout,double now=0.) {
        if(layout!=layout_){suspend();layout_=layout;}
        bool toggle=false;
        for(int h=0;h<2;h++) {
            const auto& in=hands[h];
            if(!active||!in.tracked){waiting_[h]=true;down_[h]=used_[h]=false;continue;}
            if(!in.grip){toggle|=down_[h]&&!used_[h];waiting_[h]=down_[h]=used_[h]=false;continue;}
            if(waiting_[h])continue;
            down_[h]=true;
            if(std::hypot(in.stickX,in.stickY)>=.3f||in.click)used_[h]=true;
        }
        if(!active)noticeAt_=-100.;
        if(toggle)set(!enabled_,now);
        return toggle;
    }
};
// The presentation focal (VrFocal) is calibrated for the first room, not
// the native camera's neutral lens. CM's camera reset loads40degrees into BOTH
// current/target at154ec8/154ed4; the scene script then animates those values.
// Guard the reset instructions and literals, just as VC1/VC2 guard their reset.
template<class Read32> float nativeReferenceFocal(Read32 read) {
    constexpr uint32_t reset[][2]={
        {0x27314,0xd25ac759},{0x27318,0xd357d15a},
        {0x2731c,0xf308f3fa},{0x27324,0xf13af23a},
        {0x2747c,0x42200000},{0x27480,0x0c154ed4},
        {0x27484,0x0c154ec8},{0x27494,0x43a00000}};
    for(const auto& word:reset)if(uint32_t(read(word[0]))!=word[1])return 0.f;
    uint32_t angleBits=read(0x2747c),widthBits=read(0x27494);
    float angle,halfWidth;std::memcpy(&angle,&angleBits,4);std::memcpy(&halfWidth,&widthBits,4);
    return float(double(halfWidth)/std::tan(double(angle)*3.14159265358979323846/360.));
}
// CM gameplay retains the stock hit-test focal while its widened projection uses
// 1.8125x the angle. Its two cinematic camera builders instead double the lens
// tangent and store that widened focal in BOTH places. Equal focal terms identify
// that verified path; undo its factor2 for the authored/native focal reference.
inline float nativeFactor(float reference,float hitFocal,float drawnFocal,bool widened) {
    auto valid=[](float f){return std::isfinite(f)&&f>10.f&&f<20000.f;};
    if(!valid(reference)||!valid(hitFocal)||!valid(drawnFocal))return 1.f;
    const bool wideLens=widened&&std::abs(hitFocal/drawnFocal-1.f)<.01f;
    const float factor=hitFocal*(wideLens?2.f:1.f)/reference;
    return std::isfinite(factor)&&factor>.01f&&factor<100.f?factor:1.f;
}
inline std::array<float,2> worldXY(float x,float y,float factor) { return {x*factor,y*factor}; }
}
