#include "../Source/Afterclass/Combat/ACFrameCore.h"
#include <cstdio>
#include <cmath>
static int fails=0;
#define CHECK(c) do{ if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);fails++;} }while(0)
int main(){
  using namespace AC;
  // Frame data (spec §7)
  CHECK(Light.Startup==7&&Light.Active==3&&Light.Recovery==11&&Light.Total()==21);
  CHECK(Heavy.Startup==13&&Heavy.Active==4&&Heavy.Recovery==19&&Heavy.Total()==36);
  CHECK(!IsDodgeInvulnerable(2)&&IsDodgeInvulnerable(3)&&IsDodgeInvulnerable(13)&&!IsDodgeInvulnerable(14));
  CHECK(!IsDodgeDone(22)&&IsDodgeDone(23));
  CHECK(IsPerfectDodge(6)&&!IsPerfectDodge(7)&&!IsPerfectDodge(-1)&&IsPerfectDodge(0));
  CHECK(CounterWindow==15&&PerfectDodgeHitstop==6&&ParryCounter==12&&GuardBreakStagger==40&&GrabEscapeFrames==18);

  // Timeline progression: Light frames 1-7 startup, 8-10 active, 11-21 recovery, 22 done
  CHECK(PhaseAt(Light,1)==EPhase::Startup&&PhaseAt(Light,7)==EPhase::Startup);
  CHECK(PhaseAt(Light,8)==EPhase::Active&&PhaseAt(Light,10)==EPhase::Active);
  CHECK(PhaseAt(Light,11)==EPhase::Recovery&&PhaseAt(Light,21)==EPhase::Recovery&&PhaseAt(Light,22)==EPhase::Done);
  CHECK(PhaseAt(Heavy,13)==EPhase::Startup&&PhaseAt(Heavy,14)==EPhase::Active&&PhaseAt(Heavy,17)==EPhase::Active&&PhaseAt(Heavy,18)==EPhase::Recovery&&PhaseAt(Heavy,37)==EPhase::Done);
  CHECK(IsFirstActiveFrame(Light,8)&&!IsFirstActiveFrame(Light,7)&&IsFirstRecoveryFrame(Light,11));
  { int active=0; for(int f=1;f<=40;f++) if(PhaseAt(Light,f)==EPhase::Active) active++; CHECK(active==3); }

  // Fixed 60 Hz stepping
  { double acc=0; CHECK(ConsumeFixedSteps(acc,1.0/60)==1); CHECK(ConsumeFixedSteps(acc,1.0/120)==0); CHECK(ConsumeFixedSteps(acc,1.0/120)==1);
    int total=0; for(int i=0;i<144;i++) total+=ConsumeFixedSteps(acc,1.0/144); CHECK(total==60||total==59);
    acc=0; CHECK(ConsumeFixedSteps(acc,1.0,5)==5); CHECK(acc==0.0); }

  // Scaling
  float exp[]={1,.95f,.88f,.8f,.72f,.65f,.6f,.6f,.6f,.6f,.6f,.6f};
  for(int i=0;i<ComboCap;i++) CHECK(std::fabs(ComboScale(i)-exp[i])<1e-6);

  // Meter
  CHECK(MeterDelta(MeterEvent::PerfectDodge)==25&&MeterDelta(MeterEvent::Parry)==12&&MeterDelta(MeterEvent::WhiffPunish)==8);
  CHECK(std::fabs(MeterDelta(MeterEvent::Hit,true)-3.9f)<1e-5);
  CHECK(MeterDelta(MeterEvent::Special,true)==-50&&MeterDelta(MeterEvent::Finisher,true)==-100); // costs unaffected by Anonymous
  CHECK(ClampMeter(MeterDelta(MeterEvent::Special)+10)==0&&ClampMeter(130)==100);
  CHECK(MeterForLandedHit(false)==3&&MeterForLandedHit(true)==11);
  CHECK(std::fabs(MeterForLandedHit(true,true)-14.3f)<1e-4);
  CHECK(CanAffordSpecial(50)&&!CanAffordSpecial(49.9f));
  // YN decay: none for 4 s, then decays
  CHECK(AnonymousDecayStep(50,240)==50);
  CHECK(AnonymousDecayStep(50,241)<50&&std::fabs(AnonymousDecayStep(50,241)-(50-10.f/60))<1e-4);
  CHECK(AnonymousDecayStep(0,1000)==0);
  CHECK(FinisherEligible(14,100,100,true)&&!FinisherEligible(16,100,100,true)&&!FinisherEligible(10,100,99,true)&&!FinisherEligible(10,100,100,false));

  // Defence: cone, parry window, block stamina, guard break
  CHECK(InBlockCone(60)&&!InBlockCone(61)&&InBlockCone(-60)&&!InBlockCone(-61));
  CHECK(IsParryFrame(0)&&IsParryFrame(3)&&!IsParryFrame(4)&&!IsParryFrame(-1));
  CHECK(ResolveBlock(2,0,100,false,false)==EDefense::Parried);
  CHECK(ResolveBlock(4,0,100,false,false)==EDefense::Blocked);
  CHECK(ResolveBlock(10,0,8,false,false)==EDefense::Blocked);
  CHECK(ResolveBlock(10,0,7.9f,false,false)==EDefense::GuardBroken);
  CHECK(ResolveBlock(10,0,100,false,true)==EDefense::GuardBroken);
  CHECK(ResolveBlock(1,0,100,true,false)==EDefense::Hit);   // unblockable beats parry too
  CHECK(ResolveBlock(1,90,100,false,false)==EDefense::Hit); // outside 120° cone
  CHECK(std::fabs(ParryReflectDamage(100)-20)<1e-5);
  { float st=100; int blocks=0; while(ResolveBlock(10,0,st,false,false)==EDefense::Blocked){ st-=BlockStamina; blocks++; } CHECK(blocks==12); }

  // Grab escape
  CHECK(IsGrabEscapeOpen(0)&&IsGrabEscapeOpen(18)&&!IsGrabEscapeOpen(19));

  // Combo: scaling, cap, bounce limits
  { FComboState c; CHECK(c.RegisterHit()==1.0f); CHECK(std::fabs(c.RegisterHit()-0.95f)<1e-6);
    CHECK(c.TryWallBounce()&&!c.TryWallBounce()); CHECK(c.TryGroundBounce()&&!c.TryGroundBounce());
    for(int i=0;i<10;i++){ c.RegisterHit(); }
    CHECK(c.AtCap()&&c.Hits==12);
    c.Reset(); CHECK(c.Hits==0&&!c.AtCap()&&c.TryWallBounce());
    FComboState d; CHECK(std::fabs(d.RegisterHit(2)-0.88f)<1e-6); }

  printf(fails?"FAILED (%d)\n":"framecore: ALL PASS\n",fails); return fails;
}
