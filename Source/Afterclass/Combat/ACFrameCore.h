// Engine-independent combat rules (spec §7). Header-only so it is unit-testable without UE.
// UE wrappers (UACCombatComponent) call into this; numbers live in ONE place.
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
namespace AC {
struct FFrameData { int Startup, Active, Recovery; };
inline constexpr FFrameData Light{7,3,11}, Heavy{13,4,19};
inline constexpr int DodgeWindup=2, DodgeIFrameStart=3, DodgeIFrameEnd=13, DodgeEnd=22;
inline constexpr int PerfectDodgeLead=6, CounterWindow=15, ParryWindow=4, ParryCounter=12;
inline constexpr int GuardBreakStagger=40, GrabEscapeFrames=18, ComboCap=12;
inline constexpr float BlockStamina=8.f, BlockConeDeg=120.f, ParryReflect=0.20f;
inline constexpr std::array<float,6> ScaleTable{1.0f,0.95f,0.88f,0.8f,0.72f,0.65f};
inline float ComboScale(int hitIndex){ return hitIndex<6? ScaleTable[hitIndex] : 0.6f; } // 0-based
inline bool IsDodgeInvulnerable(int f){ return f>=DodgeIFrameStart && f<=DodgeIFrameEnd; }
// framesUntilActive: frames before the attacker's first active frame when dodge starts.
inline bool IsPerfectDodge(int framesUntilActive){ return framesUntilActive>=0 && framesUntilActive<=PerfectDodgeLead; }
enum class MeterEvent{Hit,PerfectDodge,Parry,WhiffPunish,Special,Finisher};
inline float MeterDelta(MeterEvent e,bool anonymous=false){
  float d=0; switch(e){case MeterEvent::Hit:d=3;break;case MeterEvent::PerfectDodge:d=25;break;
   case MeterEvent::Parry:d=12;break;case MeterEvent::WhiffPunish:d=8;break;
   case MeterEvent::Special:return -50;case MeterEvent::Finisher:return -100;}
  return anonymous? d*1.3f : d; }
inline float ClampMeter(float m){ return std::clamp(m,0.f,100.f); }
inline bool InBlockCone(float angleDeg){ return angleDeg<=BlockConeDeg*0.5f; }
inline bool FinisherEligible(float hp,float maxHp,float meter,bool finalRound){
  return hp<=maxHp*0.15f && meter>=100.f && finalRound; }
}
