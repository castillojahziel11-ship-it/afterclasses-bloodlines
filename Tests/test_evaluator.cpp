#include "../Source/Afterclass/AI/ACEvaluatorCore.h"
#include <cstdio>
#include <cmath>
static int fails=0;
#define CHECK(c) do{ if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);fails++;} }while(0)
using namespace AC::Eval;

static bool IsPunishOrDodge(Action a){ return IsDodge(a)||a==Action::Parry; }
static Context HeavyIncoming(){ Context c; c.DistanceUU=120; c.PreferredRangeUU=150; c.Target=TargetPhase::AttackStartup; c.TargetAttack=PlayerAct::Heavy; return c; }
static History ThreeHeavies(){ History h; for(int i=0;i<3;i++) h.Push(PlayerAct::Heavy); return h; }
static double Rate(Difficulty d,const Context& c,const History& h,bool(*pred)(Action),int n=2000,uint64_t seed0=1){
  int k=0; for(int i=0;i<n;i++){ Rng r(seed0+i); if(pred(Evaluate(c,h,d,r).Chosen)) k++; } return double(k)/n; }

int main(){
  // Tables
  CHECK(Row(Difficulty::Easy).ErrorRate==0.14f&&Row(Difficulty::Normal).ErrorRate==0.10f&&Row(Difficulty::Hard).ErrorRate==0.06f&&Row(Difficulty::Nightmare).ErrorRate==0.03f);
  CHECK(Row(Difficulty::Easy).LatencyMs==320&&Row(Difficulty::Normal).LatencyMs==220&&Row(Difficulty::Hard).LatencyMs==150&&Row(Difficulty::Nightmare).LatencyMs==90);
  CHECK(Row(Difficulty::Easy).CadenceMs==250&&Row(Difficulty::Hard).CadenceMs==100);
  CHECK(ShouldEvaluate(0,-1,Difficulty::Easy)&&!ShouldEvaluate(200,0,Difficulty::Easy)&&ShouldEvaluate(250,0,Difficulty::Easy)&&ShouldEvaluate(100,0,Difficulty::Hard));

  // History ring buffer
  { History h; for(int i=0;i<15;i++) h.Push(i%2?PlayerAct::Light:PlayerAct::Heavy); CHECK(h.Size()==12); CHECK(h.Recent(0).Act==PlayerAct::Heavy);
    History g; g.Push(PlayerAct::Light); g.Push(PlayerAct::Heavy); g.Push(PlayerAct::Dodge); g.Push(PlayerAct::Heavy); g.Push(PlayerAct::Heavy);
    CHECK(g.TrailingRun(PlayerAct::Heavy)==3&&g.CountOf(PlayerAct::Heavy)==3);
    g.MarkNewestAsFeint(); CHECK(g.Recent(0).Act==PlayerAct::Feint&&g.TrailingRun(PlayerAct::Heavy)==0);
    History p; p.Push(PlayerAct::Light,true); p.Push(PlayerAct::Dodge,false); p.Push(PlayerAct::Heavy,true); CHECK(p.PostDodgeAttacks()==2);
    History q; for(int i=0;i<12;i++) q.Push(PlayerAct::Heavy); q.Push(PlayerAct::Light); CHECK(q.CountOf(PlayerAct::Heavy)==11); }

  // Determinism
  { Rng a(42),b(42); for(int i=0;i<50;i++){ Decision x=Evaluate(HeavyIncoming(),ThreeHeavies(),Difficulty::Hard,a), y=Evaluate(HeavyIncoming(),ThreeHeavies(),Difficulty::Hard,b); CHECK(x.Chosen==y.Chosen&&x.DelayMs==y.DelayMs); } }

  // GATE: Hard punishes/dodges after three repeated heavies (deterministic seed)
  { Rng r(1337); Decision d=Evaluate(HeavyIncoming(),ThreeHeavies(),Difficulty::Hard,r);
    CHECK(IsPunishOrDodge(d.Chosen)); CHECK(d.DelayMs==150); CHECK(!d.bDeliberateError);
    double withH=Rate(Difficulty::Hard,HeavyIncoming(),ThreeHeavies(),IsPunishOrDodge), noH=Rate(Difficulty::Hard,HeavyIncoming(),History{},IsPunishOrDodge);
    printf("  gate: Hard dodge/parry vs 3rd heavy = %.1f%% (no history %.1f%%)\n",withH*100,noH*100);
    CHECK(withH>=0.90); CHECK(withH>noH+0.25);
    // ...and punishes the heavy's recovery
    Context rec=HeavyIncoming(); rec.Target=TargetPhase::AttackRecovery;
    double pun=Rate(Difficulty::Hard,rec,ThreeHeavies(),[](Action a){ return a==Action::Light||a==Action::Heavy||a==Action::Grab; });
    printf("  gate: Hard punishes heavy recovery = %.1f%%\n",pun*100); CHECK(pun>=0.90);
    // Easy adapts less than Hard
    double easy=Rate(Difficulty::Easy,HeavyIncoming(),ThreeHeavies(),IsPunishOrDodge); CHECK(easy<withH); }

  // Feints lower dodge odds
  { History hf=ThreeHeavies(); for(int i=0;i<4;i++) hf.Push(PlayerAct::Feint); for(int i=0;i<3;i++) hf.Push(PlayerAct::Heavy);
    double plain=Rate(Difficulty::Hard,HeavyIncoming(),ThreeHeavies(),[](Action a){return IsDodge(a);});
    double feinted=Rate(Difficulty::Hard,HeavyIncoming(),hf,[](Action a){return IsDodge(a);});
    CHECK(feinted<plain); }

  // Predictable post-dodge attacks -> delayed punish (block, extra delay)
  { History h; h.Push(PlayerAct::Dodge); h.Push(PlayerAct::Light,true); h.Push(PlayerAct::Dodge); h.Push(PlayerAct::Heavy,true);
    Context c; c.DistanceUU=130; c.Target=TargetPhase::Dodging; Rng r(7); Decision d=Evaluate(c,h,Difficulty::Hard,r);
    CHECK(d.Chosen==Action::Block&&d.bDelayedPunish&&d.DelayMs==150+PunishDelayMs);
    History none; Rng r2(7); Decision e=Evaluate(c,none,Difficulty::Hard,r2); CHECK(!e.bDelayedPunish); }

  // Distance: far -> approach; out of meter -> never special as best
  { Context c; c.DistanceUU=900; Rng r(3); CHECK(Evaluate(c,History{},Difficulty::Nightmare,r).Chosen==Action::Approach);
    Context m; m.DistanceUU=100; m.OwnMeter=0; Scores s=ScoreAll(m,History{},Difficulty::Hard); int b=0; for(int i=1;i<NumActions;i++) if(s[i]>s[b]) b=i; CHECK(static_cast<Action>(b)!=Action::Special); }

  // Deliberate error rates within ±1.5 % of the table
  for(int di=0;di<4;di++){ Difficulty d=static_cast<Difficulty>(di); int err=0; const int N=20000; Rng r(99+di);
    for(int i=0;i<N;i++) if(Evaluate(HeavyIncoming(),ThreeHeavies(),d,r).bDeliberateError) err++;
    double rate=double(err)/N; CHECK(std::fabs(rate-Row(d).ErrorRate)<0.015); }

  // Boss phases: 66 % / 33 %, one unblockable per phase
  { CHECK(BossPhaseForHealth(1.0f)==0&&BossPhaseForHealth(0.67f)==0&&BossPhaseForHealth(0.66f)==1&&BossPhaseForHealth(0.34f)==1&&BossPhaseForHealth(0.33f)==2&&BossPhaseForHealth(0.f)==2);
    BossPhaseTracker t; CHECK(t.ConsumeUnblockable()&&!t.ConsumeUnblockable());
    CHECK(!t.Update(0.8f)); CHECK(t.Update(0.6f)&&t.Phase==1); CHECK(t.ConsumeUnblockable()&&!t.ConsumeUnblockable());
    CHECK(!t.Update(0.9f)&&t.Phase==1); CHECK(t.Update(0.1f)&&t.Phase==2&&t.ConsumeUnblockable()); CHECK(!t.Update(0.f)); }

  printf(fails?"FAILED (%d)\n":"evaluator: ALL PASS\n",fails); return fails;
}
