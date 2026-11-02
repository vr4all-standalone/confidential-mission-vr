/* CM one-time World size update migration; GPL v2 or later. */
#pragma once
#include "cfg/cfg.h"
namespace vr::world_scale {
inline constexpr float Default = 0.10f;
// Global menu/grip preferences must win over stale per-game scale overrides.
template<class OptionType> class Setting : public OptionType {
public:
    Setting():OptionType("vr.WorldScale",Default){}
    using OptionType::operator=;
    void load() override {
        OptionType::load();
        if(config::loadInt("config","vr.CmWorldSizeMigration",0)>=1)return;
        this->set(Default);
        config::setAutoSave(false);
        config::saveFloat("config","vr.WorldScale",Default);
        config::saveInt("config","vr.CmWorldSizeMigration",1);
        config::setAutoSave(true);
    }
};
}
