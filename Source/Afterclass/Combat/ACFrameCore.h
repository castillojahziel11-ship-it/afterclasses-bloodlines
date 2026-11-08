// Engine-independent combat rules (spec §7). Header-only so it is unit-testable without UE (Tests/test_framecore.cpp).
// UE wrappers (UACCombatComponent, UACHitstopSubsystem) call into this; numbers live in ONE place.
//
// Frame conventions (fixed 60 Hz simulation):
//  * Action timelines are 1-based: frame 1 is the first simulated frame after the action starts.
//    Light 7/3/11 => frames 1-7 startup, 8-10 active, 11-21 recovery, frame 22 => Done.
//  * Dodge: frames 3..13 invulnerable, dodge ends after frame DodgeEnd (22).
//  * Parry / block timers are 0-based "frames elapsed since block started" (0 = block began this frame).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace AC {
inline constexpr int SimHz = 60;
inline constexpr double SimStep = 1.0 / SimHz;

struct FFrameData { int Startup, Active, Recovery; constexpr int Total() const { return Startup + Active + Recovery; } };
inline constexpr FFrameData Light{7,3,11}, Heavy{13,4,19};
// Design placeholders (not in spec §7; tune in the move table): grab, special, finisher timelines.
inline constexpr FFrameData GrabMove{5,3,22}, SpecialMove{15,5,25}, FinisherMove{20,10,40};

inline constexpr int DodgeWindup=2, DodgeIFrameStart=3, DodgeIFrameEnd=13, DodgeEnd=22;
inline constexpr int PerfectDodgeLead=6, CounterWindow=15, PerfectDodgeHitstop=6, ParryWindow=4, ParryCounter=12;
inline constexpr int GuardBreakStagger=40, GrabEscapeFrames=18, ComboCap=12;
inline constexpr float BlockStamina=8.f, BlockConeDeg=120.f, ParryReflect=0.20f;
// Design placeholders (not in spec §7): reaction durations and stamina economy.
inline constexpr int KnockdownFrames=45, GetUpFrames=30, DefaultHitstopFrames=4;
inline constexpr float StaminaRegenPerSec=25.f; inline constexpr int StaminaRegenDelayFrames=30;
// YN "Anonymous" passive: +30% meter gain; meter decays after 4 s without landing a hit (decay rate is a placeholder).
inline constexpr float AnonymousGainMul=1.3f; inline constexpr int AnonymousDecayDelayFrames=4*SimHz;
inline constexpr float AnonymousDecayPerSec=10.f;

inline constexpr std::array<float,6> ScaleTable{1.0f,0.95f,0.88f,0.8f,0.72f,0.65f};
inline float ComboScale(int hitIndex){ return hitIndex<0? 1.0f : (hitIndex<6? ScaleTable[hitIndex] : 0.6f); } // 0-based

// ---- Timelines ---------------------------------------------------------------------------------------------
enum class EPhase{Startup,Active,Recovery,Done};
inline EPhase PhaseAt(const FFrameData& fd,int frame){
  if(frame<=fd.Startup) return EPhase::Startup;
  if(frame<=fd.Startup+fd.Active) return EPhase::Active;
  if(frame<=fd.Total()) return EPhase::Recovery;
  return EPhase::Done; }
inline bool IsFirstActiveFrame(const FFrameData& fd,int frame){ return frame==fd.Startup+1; }
inline bool IsFirstRecoveryFrame(const FFrameData& fd,int frame){ return frame==fd.Startup+fd.Active+1; }
inline bool IsDodgeInvulnerable(int f){ return f>=DodgeIFrameStart && f<=DodgeIFrameEnd; }
inline bool IsDodgeDone(int f){ return f>DodgeEnd; }
// framesUntilActive: frames before the attacker's first active frame when dodge starts.
inline bool IsPerfectDodge(int framesUntilActive){ return framesUntilActive>=0 && framesUntilActive<=PerfectDodgeLead; }
inline bool IsGrabEscapeOpen(int framesGrabbed){ return framesGrabbed>=0 && framesGrabbed<=GrabEscapeFrames; }

// Fixed-step accumulator. Adds dt, returns how many 60 Hz frames to simulate (capped, excess time dropped to avoid spiral).
inline int ConsumeFixedSteps(double& acc,double dt,int maxSteps=5){
  if(dt>0) acc+=dt;
  int n=0; while(acc>=SimStep-1e-9 && n<maxSteps){ acc-=SimStep; ++n; }
  if(n==maxSteps && acc>=SimStep) acc=0.0;
  if(acc<0) acc=0.0;
  return n; }

// ---- Meter ----------------------------------------------------------------------------------------------------
enum class MeterEvent{Hit,PerfectDodge,Parry,WhiffPunish,Special,Finisher};
inline float MeterDelta(MeterEvent e,bool anonymous=false){
  float d=0; switch(e){case MeterEvent::Hit:d=3;break;case MeterEvent::PerfectDodge:d=25;break;
   case MeterEvent::Parry:d=12;break;case MeterEvent::WhiffPunish:d=8;break;
   case MeterEvent::Special:return -50;case MeterEvent::Finisher:return -100;}
  return anonymous? d*AnonymousGainMul : d; }
inline float ClampMeter(float m){ return std::clamp(m,0.f,100.f); }
inline bool CanAffordSpecial(float meter){ return meter>=50.f; }
// Meter for landing a hit: +3, plus +8 if the victim was recovering from an attack that whiffed.
inline float MeterForLandedHit(bool victimInWhiffRecovery,bool anonymous=false){
  return MeterDelta(MeterEvent::Hit,anonymous)+(victimInWhiffRecovery? MeterDelta(MeterEvent::WhiffPunish,anonymous):0.f); }
// Per-frame meter after YN decay. framesSinceLanded counts simulated frames since the last landed hit.
inline float AnonymousDecayStep(float meter,int framesSinceLanded){
  if(framesSinceLanded<=AnonymousDecayDelayFrames) return meter;
  return ClampMeter(meter-AnonymousDecayPerSec/SimHz); }
inline bool FinisherEligible(float hp,float maxHp,float meter,bool finalRound){
  return hp<=maxHp*0.15f && meter>=100.f && finalRound; }

// ---- Defence ------------------------------------------------------------------------------------------------
inline bool InBlockCone(float angleDeg){ return std::fabs(angleDeg)<=BlockConeDeg*0.5f; }
inline bool IsParryFrame(int framesSinceBlockStart){ return framesSinceBlockStart>=0 && framesSinceBlockStart<ParryWindow; }
enum class EDefense{Hit,Blocked,Parried,GuardBroken};
// Resolves an incoming hit against a defender who is holding block.
inline EDefense ResolveBlock(int framesSinceBlockStart,float angleDeg,float stamina,bool unblockable,bool guardBreakMove){
  if(unblockable || !InBlockCone(angleDeg)) return EDefense::Hit;
  if(IsParryFrame(framesSinceBlockStart)) return EDefense::Parried;
  if(guardBreakMove || stamina<BlockStamina) return EDefense::GuardBroken;
  return EDefense::Blocked; }
inline float ParryReflectDamage(float incoming){ return incoming*ParryReflect; }

// ---- Combo --------------------------------------------------------------------------------------------------
// Tracks hits a victim takes in one combo: scaling index, cap, and the one-wall-bounce / one-ground-bounce rule.
struct FComboState {
  int Hits=0; bool WallBounceUsed=false, GroundBounceUsed=false;
  void Reset(){ *this=FComboState{}; }
  // Registers a hit; returns the damage multiplier for it. extraScaling = per-move ComboScalingIndex offset.
  float RegisterHit(int extraScaling=0){ const float s=ComboScale(Hits+std::max(0,extraScaling)); ++Hits; return s; }
  bool AtCap() const { return Hits>=ComboCap; }
  bool TryWallBounce(){ if(WallBounceUsed) return false; WallBounceUsed=true; return true; }
  bool TryGroundBounce(){ if(GroundBounceUsed) return false; GroundBounceUsed=true; return true; }
};
}
