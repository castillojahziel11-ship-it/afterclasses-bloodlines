#include "../Source/Afterclass/Combat/ACFrameCore.h"
#include <cstdio>
#include <cmath>
static int fails=0;
#define CHECK(c) do{ if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);fails++;} }while(0)
int main(){
  using namespace AC;
  CHECK(Light.Startup==7&&Light.Active==3&&Light.Recovery==11);
  CHECK(Heavy.Startup==13&&Heavy.Active==4&&Heavy.Recovery==19);
  CHECK(!IsDodgeInvulnerable(2)&&IsDodgeInvulnerable(3)&&IsDodgeInvulnerable(13)&&!IsDodgeInvulnerable(14));
  CHECK(IsPerfectDodge(6)&&!IsPerfectDodge(7)&&!IsPerfectDodge(-1));
  float exp[]={1,.95f,.88f,.8f,.72f,.65f,.6f,.6f,.6f,.6f,.6f,.6f};
  for(int i=0;i<ComboCap;i++) CHECK(std::fabs(ComboScale(i)-exp[i])<1e-6);
  CHECK(MeterDelta(MeterEvent::PerfectDodge)==25);
  CHECK(std::fabs(MeterDelta(MeterEvent::Hit,true)-3.9f)<1e-5);
  CHECK(ClampMeter(MeterDelta(MeterEvent::Special)+10)==0);
  CHECK(InBlockCone(60)&&!InBlockCone(61));
  CHECK(FinisherEligible(14,100,100,true)&&!FinisherEligible(16,100,100,true)&&!FinisherEligible(10,100,99,true));
  printf(fails?"FAILED (%d)\n":"ALL PASS\n",fails); return fails;
}
