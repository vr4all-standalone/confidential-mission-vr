// Physical controller ownership, independent from the original NAOMI player slots.
// Copyright 2026 VR4ALL. GNU GPL v2 or later.
#pragma once
#include <algorithm>
#include <cstdint>

namespace vr::xr {
class PlayerOwnership {
public:
    struct Input {
        bool start[2]{}, tracked[2]{};
        float pull[2]{};
        unsigned roster=0;
        bool known=false, allowed=false, canJoin=false;
        int layout=0;
        uint64_t now=0;
    };
    struct Output { bool start[2]{}, release[2]{}, transfer[2]{}; };
private:
    int soloHand_=1, soloPlayer_=0, layout_=0, pending_=-1;
    unsigned roster_=0;
    bool dual_=false, starts_[2]{}, startReady_[2]{true,true};
    bool triggers_[2]{}, triggerReady_[2]{};
    int owners_[2]{-1,-1};
    uint64_t pendingAt_=0;
    Output out_{};

    void roster(const Input& in) {
        if(!in.known)return;
        const unsigned mask=in.roster&3;
        const bool nextDual=mask==3 && in.canJoin;
        if(nextDual) {
            if(!dual_ || layout_!=in.layout)out_.release[0]=out_.release[1]=true;
        } else {
            const int player=mask==2?1:0;
            if(dual_)soloHand_=player?1-layout_:layout_;
            if(soloPlayer_!=player || dual_)out_.release[0]=out_.release[1]=true;
            soloPlayer_=player;
        }
        dual_=nextDual;layout_=in.layout==1?1:0;roster_=mask;
        if(pending_>=0 && (mask&(1u<<pending_)))pending_=-1;
    }
public:
    int soloHand()const{return soloHand_;}
    int soloPlayer()const{return soloPlayer_;}
    bool dual()const{return dual_;}
    int hand(int player)const{return dual_?(player?1-layout_:layout_):(player==soloPlayer_?soloHand_:1-soloHand_);}
    int pendingPlayer()const{return pending_;}
    int startOwner(int hand)const{return hand>=0&&hand<2?owners_[hand]:-1;}
    unsigned nativeRoster()const{return roster_;}
    bool triggerAllowed(int hand)const{return hand>=0&&hand<2&&triggerReady_[hand];}
    const Output& output()const{return out_;}
    void reset(){*this=PlayerOwnership();}
    // Keep trusted roster/layout and solo identity; repeated suspension is idempotent.
    void suspend(){
        pending_=-1;out_={};
        for(int h=0;h<2;h++){starts_[h]=triggers_[h]=false;startReady_[h]=triggerReady_[h]=false;owners_[h]=-1;}
    }
    Output update(const Input& in) {
        out_={};
        if(!in.allowed){suspend();return out_;}
        roster(in);
        bool freshStart[2]{},freshTrigger[2]{};
        for(int h=0;h<2;h++){
            if(!in.start[h])startReady_[h]=true;
            freshStart[h]=in.start[h]&&startReady_[h]&&!starts_[h];
            if(!in.start[h])owners_[h]=-1;
            starts_[h]=in.start[h];
            if(!in.tracked[h]){triggerReady_[h]=false;triggers_[h]=false;continue;}
            const bool held=triggers_[h]?in.pull[h]>.35f:in.pull[h]>.6f;
            if(in.pull[h]<=.35f)triggerReady_[h]=true;
            freshTrigger[h]=held&&triggerReady_[h]&&!triggers_[h];
            triggers_[h]=held;
        }
        if(!dual_){
            // Both initial Starts select one stable owner, both target original P1.
            if(roster_==0 && (freshStart[0]||freshStart[1])){
                const int first=freshStart[0]&&freshStart[1]?layout_:(freshStart[0]?0:1);
                if(soloHand_!=first)out_.release[soloPlayer_]=true;
                soloHand_=first;
            }
            const int other=1-soloHand_;
            if(freshTrigger[other]&&!freshTrigger[soloHand_]){
                soloHand_=other;out_.release[soloPlayer_]=out_.transfer[soloPlayer_]=true;
            }
        }
        if(pending_>=0 && (!in.canJoin || (in.now>=pendingAt_ && in.now-pendingAt_>4000000000ULL)))pending_=-1;
        for(int h=0;h<2;h++){
            if(freshStart[h]){
                int player=dual_?(h==layout_?0:1):roster_==0?0:h==soloHand_?soloPlayer_:1-soloPlayer_;
                if(!dual_ && roster_!=0 && player!=soloPlayer_){
                    if(in.canJoin&&in.known&&in.tracked[0]&&in.tracked[1]){pending_=player;pendingAt_=in.now;}
                    else player=-1;
                }
                owners_[h]=player;
            }
            const int player=owners_[h];
            if(in.start[h]&&player>=0&&(dual_||player==soloPlayer_||pending_==player))out_.start[player]=true;
        }
        return out_;
    }
};
}
