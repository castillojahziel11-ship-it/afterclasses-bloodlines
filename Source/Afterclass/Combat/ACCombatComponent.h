// STUB: UE wrapper. State enum + TryStartAction legality order are real; replication, montage and sweep wiring are NOT yet implemented (see KNOWN_ISSUES KI-001). Not compiled: no UE install in this environment.
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACFrameCore.h"
#include "ACCombatComponent.generated.h"
UENUM(BlueprintType) enum class EACCombatState : uint8 { Idle, Locomotion, Sprint, LightAttack, HeavyAttack, Blocking, Dodging, Stagger, Knockdown, GetUp, WeaponEquipped, Aiming, Reloading, Finisher, Dead };
UCLASS(ClassGroup=(Afterclass), meta=(BlueprintSpawnableComponent))
class AFTERCLASS_API UACCombatComponent : public UActorComponent {
  GENERATED_BODY()
public:
  void RequestLight(); void RequestHeavy(); void RequestDodge(FVector2D Dir); void RequestBlock(bool bHeld);
  bool TryStartAction(EACCombatState Next, FName MoveId); // order: legality, stamina, lock, cancel window, facing
  UPROPERTY(Replicated) EACCombatState State = EACCombatState::Idle;
  UPROPERTY(Replicated) float Health=100, Stamina=100, Meter=0;
  bool bActionLocked=false, bCancelWindowOpen=false;
};
