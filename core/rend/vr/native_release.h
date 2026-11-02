// A release must reach an original engine input poll before XR reasserts fire.
#pragma once
#include <atomic>
#include <cstdint>
namespace vr::xr {
class NativeReleaseGate {
    std::atomic<uint64_t> requested_[2]{},consumed_[2]{};
public:
    uint64_t request(int player){return player>=0&&player<2?requested_[player].fetch_add(1,std::memory_order_acq_rel)+1:0;}
    void consume(int player,bool released){
        if(player>=0&&player<2&&released)
            consumed_[player].store(requested_[player].load(std::memory_order_acquire),std::memory_order_release);
    }
    bool observed(int player,uint64_t ticket)const{
        return ticket==0||(player>=0&&player<2&&consumed_[player].load(std::memory_order_acquire)>=ticket);
    }
};
}
