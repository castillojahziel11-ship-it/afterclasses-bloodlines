// Combat state enum and move-table row. Kept separate from UACCombatComponent so data assets can include it without cycles.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Combat/ACHitTypes.h"
#include "ACCombatTypes.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class EACCombatState : uint8
{
	Idle, Locomotion, Sprint, LightAttack, HeavyAttack, Blocking, Dodging, Stagger, Knockdown, GetUp,
	WeaponEquipped, Aiming, Reloading, Finisher, Dead,
	Grabbing, Special, Grabbed
};

/** One row of a fighter's MoveSet DataTable (CSV source: Content/Data/MoveTables/*.csv). Defaults = Light from ACFrameCore. */
USTRUCT(BlueprintType)
struct AFTERCLASS_API FACMoveRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Move") FName MoveId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Move") EACCombatState State = EACCombatState::LightAttack;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Move") TSoftObjectPtr<UAnimMontage> Montage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Move", meta=(ClampMin="1")) int32 StartupFrames = 7;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Move", meta=(ClampMin="1")) int32 ActiveFrames = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Move", meta=(ClampMin="0")) int32 RecoveryFrames = 11;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Move", meta=(ClampMin="0")) float StaminaCost = 0.f;
	/** Meter spent on start (50 for specials, 100 for finishers). Gains are rule-driven (ACFrameCore). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Move", meta=(ClampMin="0")) float MeterCost = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Move") FACHitData HitData;
	/** Bones swept during active frames when the montage has no AN_HitWindow_Open (or no montage exists). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Move") TArray<FName> DefaultBoneChain;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Move", meta=(ClampMin="1", Units="cm")) float HitRadius = 18.f;

	int32 TotalFrames() const { return StartupFrames + ActiveFrames + RecoveryFrames; }
};
