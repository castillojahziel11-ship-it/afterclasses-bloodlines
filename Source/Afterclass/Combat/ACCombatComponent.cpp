// STUB: see header / KI-001.
#include "ACCombatComponent.h"
bool UACCombatComponent::TryStartAction(EACCombatState Next, FName MoveId){
  const bool bFree = State==EACCombatState::Idle||State==EACCombatState::Locomotion||State==EACCombatState::Sprint;
  if(!(bFree || bCancelWindowOpen)) return false;   // legality + cancel window
  if(bActionLocked) return false;
  // STUB: stamina cost lookup, facing correction, montage dispatch.
  State = Next; return true; }
void UACCombatComponent::RequestLight(){ TryStartAction(EACCombatState::LightAttack,"Light"); }
void UACCombatComponent::RequestHeavy(){ TryStartAction(EACCombatState::HeavyAttack,"Heavy"); }
void UACCombatComponent::RequestDodge(FVector2D){ TryStartAction(EACCombatState::Dodging,"Dodge"); }
void UACCombatComponent::RequestBlock(bool b){ TryStartAction(b?EACCombatState::Blocking:EACCombatState::Idle,"Block"); }
