// Confidential Mission: original native operator settings. No direct option RAM writes.
#pragma once
#include <cstdint>
namespace vr::xr::cm::options {
enum class Field : unsigned { GameDifficulty, BossDifficulty, ShiftingDifficulty, CollisionSize, Life, Count };
constexpr unsigned FieldCount = static_cast<unsigned>(Field::Count);
enum class Status { Unavailable, Ready, Pending, EnteringService, Reading, Applying, Saving, Restarting, Verifying, Complete, Interrupted, Failed };
struct Snapshot {
 using Status = options::Status;
 bool available = false, busy = false, serviceRecovery = false;
 uint32_t supportedMask = 0, pendingMask = 0;
 uint8_t value[FieldCount]{}, minimum[FieldCount]{}, maximum[FieldCount]{};
 Status status = Status::Unavailable;
 const char* message = "Native options are unavailable until the game is ready";
};
Snapshot snapshot();
const char* label(Field field);
// nullptr means display the verified numeric value, without invented difficulty names.
const char* valueLabel(Field field, int value);
bool stage(Field field, int value);
bool apply();
void discard();
bool busy();
// Render thread reports current safe context; native work runs on the engine thread.
void context(bool allowed, bool focused, bool manualService);
// Called only by original engine hooks. Native input bits:1 Test,2 Service.
uint32_t nativeButtons();
void notifyNativeGameBoot();
void acknowledgeNativeButtons(uint32_t bits);
void notifyNativeSave(bool success);
void notifyNativeRead();
}
