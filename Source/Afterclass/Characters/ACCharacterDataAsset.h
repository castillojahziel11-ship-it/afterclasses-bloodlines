// Per-fighter data. Every fighter is an AACFighter; ALL differentiation lives here (spec: no per-fighter C++ subclasses).
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Combat/ACCombatTypes.h"
#include "ACCharacterDataAsset.generated.h"

class UTexture2D;
class USkeletalMesh;
class UAnimInstance;
class UPhysicsAsset;

USTRUCT(BlueprintType)
struct AFTERCLASS_API FACOutfit
{
	GENERATED_BODY()

	/** Localized via string table (ST_Characters / ST_UI), never a raw literal. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Outfit") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Outfit") TSoftObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Outfit", meta=(Categories="Unlock")) FGameplayTag UnlockCondition;
};

UCLASS(BlueprintType)
class AFTERCLASS_API UACCharacterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Primary asset type used by the Asset Manager (see DefaultGame.ini). */
	static const FPrimaryAssetType FighterAssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") TSoftObjectPtr<UTexture2D> Portrait;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity", meta=(MultiLine=true)) FText Bio;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity") FText Nationality;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity", meta=(ClampMin="1", ClampMax="5")) int32 DifficultyStars = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity", meta=(Categories="Archetype")) FGameplayTag Archetype;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity", meta=(Categories="Unlock")) FGameplayTag UnlockCondition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visuals") TSoftObjectPtr<USkeletalMesh> BodyMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visuals") TSoftClassPtr<UAnimInstance> AnimBP;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visuals") TArray<FACOutfit> Outfits;
	/** Secondary-motion profile (physics asset driving hair/cloth/soft-body bones). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visuals") TSoftObjectPtr<UPhysicsAsset> JiggleProfile;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats", meta=(ClampMin="1")) float MaxHealth = 1000.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0")) float MaxStamina = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0", Units="cm/s")) float WalkSpeed = 450.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0", Units="cm/s")) float SprintSpeed = 700.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats", meta=(ClampMin="0", Units="cm")) float DodgeDistance = 350.f;
	/** YN "Anonymous" passive: meter builds 30% faster, decays after 4 s without landing a hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats") bool bAnonymousPassive = false;
	/** AI preferred engagement distance (written to blackboard PreferredRange). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats", meta=(ClampMin="50", Units="cm")) float PreferredRange = 150.f;
	/** Inclusive invulnerability window in frames; defaults mirror AC::DodgeIFrameStart..DodgeIFrameEnd. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stats") FIntPoint DodgeIFrames = FIntPoint(3, 13);

	/** DataTable with row struct FACMoveRow. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat", meta=(RequiredAssetDataTags="RowStructure=/Script/Afterclass.ACMoveRow"))
	TSoftObjectPtr<UDataTable> MoveSet;

	/** DataTable of VO cues (row struct defined with the VO pipeline; STUB until then — KI-007). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio") TSoftObjectPtr<UDataTable> VoiceProfile;
};
