// Engine-independent AI combat evaluator (spec §8). Header-only; unit-tested in Tests/test_evaluator.cpp.
// UACCombatEvaluator wraps this. The evaluator only sees OBSERVED state (current target phase + past actions):
// it never reads pending/future player input.
//
// score(action) = distance curve + stamina term + pattern bias (12-entry history) + difficulty modifier + jitter
// then: deliberate error (Easy/Normal/Hard/Nightmare 14/10/6/3 %) and reaction latency (320/220/150/90 ms).
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace AC::Eval {

enum class Action : uint8_t { Approach, Retreat, Block, Parry, DodgeL, DodgeR, DodgeBack, Light, Heavy, Grab, Special, Reposition, Count };
inline constexpr int NumActions = static_cast<int>(Action::Count);
enum class Difficulty : uint8_t { Easy, Normal, Hard, Nightmare, Count };
/** What the player did, as observed after the fact. Feint = an attack cancelled before its active frames. */
enum class PlayerAct : uint8_t { None, Light, Heavy, Feint, Grab, Special, Dodge, Block };
/** Observed target phase right now. */
enum class TargetPhase : uint8_t { Neutral, AttackStartup, AttackActive, AttackRecovery, Blocking, Dodging, Stunned, Knockdown };

inline bool IsDodge(Action a){ return a==Action::DodgeL||a==Action::DodgeR||a==Action::DodgeBack; }
inline bool IsAttack(Action a){ return a==Action::Light||a==Action::Heavy||a==Action::Grab||a==Action::Special; }
inline bool IsAttackAct(PlayerAct p){ return p==PlayerAct::Light||p==PlayerAct::Heavy||p==PlayerAct::Grab||p==PlayerAct::Special; }

struct DifficultyRow { int CadenceMs; float ErrorRate; int LatencyMs; float AdaptWeight; float Aggression; float Jitter; };
// Cadence/error/latency are spec values; AdaptWeight/Aggression/Jitter are tuning placeholders.
inline constexpr std::array<DifficultyRow,4> DifficultyTable{{
  {250, 0.14f, 320, 0.25f, 0.70f, 0.40f},  // Easy
  {100, 0.10f, 220, 0.55f, 0.85f, 0.25f},  // Normal
  {100, 0.06f, 150, 1.00f, 1.00f, 0.15f},  // Hard
  {100, 0.03f,  90, 1.30f, 1.15f, 0.08f}}}; // Nightmare
inline const DifficultyRow& Row(Difficulty d){ return DifficultyTable[static_cast<int>(d)]; }
inline constexpr int PunishDelayMs = 180; // extra wait before punishing a predictable post-dodge attack

inline bool ShouldEvaluate(int64_t nowMs,int64_t lastEvalMs,Difficulty d){ return lastEvalMs<0 || nowMs-lastEvalMs>=Row(d).CadenceMs; }

/** Deterministic splitmix64 RNG (seedable for tests and replays). */
struct Rng {
  uint64_t S;
  explicit Rng(uint64_t Seed=0x5EEDull):S(Seed){}
  uint64_t Next(){ uint64_t z=(S+=0x9E3779B97F4A7C15ull); z=(z^(z>>30))*0xBF58476D1CE4E5B9ull; z=(z^(z>>27))*0x94D049BB133111EBull; return z^(z>>31); }
  float Unit(){ return static_cast<float>(Next()>>40)*(1.0f/16777216.0f); } // [0,1)
  int Range(int n){ return n<=0?0:static_cast<int>(Next()%static_cast<uint64_t>(n)); }
};

struct HistoryEntry { PlayerAct Act=PlayerAct::None; bool bAfterDodge=false; };

/** 12-entry rolling history of observed player actions. Recent(0) = newest. */
class History {
public:
  static constexpr int Capacity=12;
  void Push(HistoryEntry e){ Buf[Head]=e; Head=(Head+1)%Capacity; if(Count<Capacity) ++Count; }
  void Push(PlayerAct a,bool afterDodge=false){ Push(HistoryEntry{a,afterDodge}); }
  int Size() const { return Count; }
  void Clear(){ Count=0; Head=0; }
  const HistoryEntry& Recent(int i) const { return Buf[(Head-1-i+Capacity*2)%Capacity]; }
  /** Re-labels the newest entry as a feint (attack observed to cancel before active frames). */
  void MarkNewestAsFeint(){ if(Count>0) Buf[(Head-1+Capacity)%Capacity].Act=PlayerAct::Feint; }
  int CountOf(PlayerAct a) const { int n=0; for(int i=0;i<Count;i++) if(Recent(i).Act==a) ++n; return n; }
  /** Consecutive newest entries equal to a (ignores Block/Dodge entries between attacks). */
  int TrailingRun(PlayerAct a) const { int n=0; for(int i=0;i<Count;i++){ PlayerAct p=Recent(i).Act; if(p==a) ++n; else if(p==PlayerAct::Block||p==PlayerAct::Dodge) continue; else break; } return n; }
  int PostDodgeAttacks() const { int n=0; for(int i=0;i<Count;i++) if(Recent(i).bAfterDodge && IsAttackAct(Recent(i).Act)) ++n; return n; }
private:
  std::array<HistoryEntry,Capacity> Buf{}; int Count=0, Head=0;
};

struct Context {
  float DistanceUU=500.f, PreferredRangeUU=150.f;
  float OwnStamina01=1.f, TargetStamina01=1.f, OwnHealth01=1.f, OwnMeter=0.f;
  TargetPhase Target=TargetPhase::Neutral;
  PlayerAct TargetAttack=PlayerAct::None; // which attack the target is performing (valid during Attack* phases)
  bool bTargetHasWeapon=false;
};

struct Decision { Action Chosen=Action::Approach; int DelayMs=0; bool bDeliberateError=false; bool bDelayedPunish=false; float BestScore=0.f; };

using Scores = std::array<float,NumActions>;
inline float& S(Scores& s,Action a){ return s[static_cast<int>(a)]; }

/** Deterministic part of the score (no jitter / error). Exposed for tests and debugging. */
inline Scores ScoreAll(const Context& c,const History& h,Difficulty d){
  const DifficultyRow& R=Row(d);
  Scores s{}; s.fill(0.f);
  const float r=std::max(50.f,c.PreferredRangeUU), dist=c.DistanceUU;
  const bool inRange=dist<=r*1.3f, close=dist<=r*0.8f, far=dist>r*2.5f;

  // 1) Distance curve
  S(s,Action::Approach)=std::clamp((dist-r)/r,0.f,2.f);
  S(s,Action::Retreat)=close?0.15f:-0.3f;
  S(s,Action::Reposition)=(!inRange&&!far)?0.3f:0.05f;
  S(s,Action::Light)=inRange?0.6f:-1.f;
  S(s,Action::Heavy)=inRange?0.45f:-1.f;
  S(s,Action::Grab)=close?0.35f:-1.f;
  S(s,Action::Special)=(inRange&&c.OwnMeter>=50.f)?0.55f:-2.f;
  S(s,Action::Block)=inRange?0.2f:-0.2f;
  S(s,Action::Parry)=inRange?0.05f:-0.5f;
  for(Action a:{Action::DodgeL,Action::DodgeR,Action::DodgeBack}) S(s,a)=inRange?0.1f:-0.4f;
  S(s,Action::DodgeBack)+=0.02f; // tie-break preference

  // 2) Stamina
  if(c.OwnStamina01<0.25f){ for(Action a:{Action::Heavy,Action::DodgeL,Action::DodgeR,Action::DodgeBack,Action::Block}) S(s,a)-=0.6f; S(s,Action::Retreat)+=0.5f; }
  if(c.TargetStamina01<0.25f){ S(s,Action::Heavy)+=0.4f; S(s,Action::Grab)+=0.3f; }

  // 3) Reaction to the OBSERVED target phase
  switch(c.Target){
  case TargetPhase::AttackStartup: case TargetPhase::AttackActive:
    if(inRange){ S(s,Action::Block)+=0.6f; S(s,Action::Parry)+=0.35f; for(Action a:{Action::DodgeL,Action::DodgeR,Action::DodgeBack}) S(s,a)+=0.45f; S(s,Action::Light)-=0.4f; S(s,Action::Heavy)-=0.6f; S(s,Action::Grab)-=0.5f; }
    break;
  case TargetPhase::AttackRecovery:
    if(inRange){ S(s,Action::Light)+=1.2f; S(s,Action::Heavy)+=0.9f; S(s,Action::Grab)+=0.5f; }
    else S(s,Action::Approach)+=0.6f;
    break;
  case TargetPhase::Blocking: S(s,Action::Grab)+=close?0.9f:0.f; S(s,Action::Heavy)+=0.3f; S(s,Action::Light)-=0.2f; break;
  case TargetPhase::Dodging: S(s,Action::Block)+=0.3f; S(s,Action::Light)-=0.4f; S(s,Action::Heavy)-=0.4f; break;
  case TargetPhase::Stunned: S(s,Action::Light)+=0.8f; S(s,Action::Heavy)+=0.6f; break;
  case TargetPhase::Knockdown: S(s,Action::Reposition)+=0.5f; S(s,Action::Approach)+=0.3f; for(Action a:{Action::Light,Action::Heavy,Action::Grab,Action::Special}) S(s,a)-=0.5f; break;
  default: break;
  }

  // 4) Pattern bias from the 12-entry history (adaptation)
  const float A=R.AdaptWeight;
  const int heavyRun=h.TrailingRun(PlayerAct::Heavy), heavies=h.CountOf(PlayerAct::Heavy), feints=h.CountOf(PlayerAct::Feint);
  const bool reactingToHeavy=(c.Target==TargetPhase::AttackStartup||c.Target==TargetPhase::AttackActive)&&c.TargetAttack==PlayerAct::Heavy;
  if(reactingToHeavy){
    const float dodgeBias=A*(0.35f*std::min(heavyRun,4)+0.05f*heavies);
    for(Action a:{Action::DodgeL,Action::DodgeR,Action::DodgeBack}) S(s,a)+=dodgeBias;
    S(s,Action::Parry)+=A*0.15f*std::min(heavyRun,4);
  }
  if(feints>0){ // repeated feints make dodging less attractive (they bait the dodge)
    for(Action a:{Action::DodgeL,Action::DodgeR,Action::DodgeBack}) S(s,a)-=A*0.3f*std::min(feints,4);
    S(s,Action::Block)+=A*0.15f*std::min(feints,4);
  }
  if(c.Target==TargetPhase::Dodging && h.PostDodgeAttacks()>=2) S(s,Action::Block)+=A*0.8f; // absorb predictable post-dodge attack, then punish

  // 5) Difficulty modifier: aggression scales positive offensive scores
  for(Action a:{Action::Light,Action::Heavy,Action::Grab,Action::Special}) if(S(s,a)>0) S(s,a)*=R.Aggression;
  return s;
}

inline Decision Evaluate(const Context& c,const History& h,Difficulty d,Rng& rng){
  const DifficultyRow& R=Row(d);
  Scores s=ScoreAll(c,h,d);
  for(float& v:s) v+=(rng.Unit()*2.f-1.f)*R.Jitter;                 // jitter
  int best=0; for(int i=1;i<NumActions;i++) if(s[i]>s[best]) best=i;
  Decision out; out.BestScore=s[best]; out.Chosen=static_cast<Action>(best);
  if(rng.Unit()<R.ErrorRate){                                        // deliberate error: any other action
    int pick=rng.Range(NumActions-1); if(pick>=best) ++pick;
    out.Chosen=static_cast<Action>(pick); out.bDeliberateError=true;
  }
  out.DelayMs=R.LatencyMs;                                           // reaction latency
  if(!out.bDeliberateError && out.Chosen==Action::Block && c.Target==TargetPhase::Dodging && h.PostDodgeAttacks()>=2){
    out.bDelayedPunish=true; out.DelayMs+=PunishDelayMs;
  }
  return out;
}

// ---- Boss phases (authored at 66 % and 33 %, one telegraphed unblockable per phase) ----------------------------
inline constexpr float BossPhase2At=0.66f, BossPhase3At=0.33f;
inline int BossPhaseForHealth(float hp01){ return hp01>BossPhase2At?0:(hp01>BossPhase3At?1:2); }
struct BossPhaseTracker {
  int Phase=0; bool UnblockableReady=true;
  /** Returns true if the phase advanced. Phases never go backwards (healing does not re-arm). */
  bool Update(float hp01){ const int p=BossPhaseForHealth(hp01); if(p>Phase){ Phase=p; UnblockableReady=true; return true; } return false; }
  bool ConsumeUnblockable(){ if(!UnblockableReady) return false; UnblockableReady=false; return true; }
};
}
