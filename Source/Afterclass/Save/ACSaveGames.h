// USaveGame payloads. Bump the matching *_VERSION constant whenever a field is added/removed and add a migration in UACSaveSubsystem.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "ACSaveGames.generated.h"

/** A single player key override applied through UEnhancedInputUserSettings. Remaps live in the PROFILE slot (spec). */
USTRUCT(BlueprintType)
struct AFTERCLASS_API FACKeyRemap
{
	GENERATED_BODY()

	/** Player Mappable Key Settings "Name" on the mapping/action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input") FName MappingName;
	/** EPlayerMappableKeySlot as uint8 (0 = First/primary, 1 = Second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input") uint8 Slot = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input") FKey Key;
};

UCLASS(BlueprintType)
class AFTERCLASS_API UACProfileSave : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 PROFILE_VERSION = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="Save") int32 SaveVersion = PROFILE_VERSION;

	// Settings
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Settings") float MasterVolume = 1.f;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Settings") float MusicVolume = 0.8f;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Settings") float SfxVolume = 1.f;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Settings") float VoiceVolume = 1.f;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Settings") float LookSensitivity = 1.f;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Settings") bool bInvertY = false;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Settings") bool bSubtitles = true;
	/** Culture code, e.g. "en", "es-419", "ja". Empty = OS default. */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Settings") FString Culture;

	// Control remaps
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Input") TArray<FACKeyRemap> KeyRemaps;

	// Meta-progression shared across campaign slots (roster/outfit unlocks)
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Progress") FGameplayTagContainer Unlocks;
};

UCLASS(BlueprintType)
class AFTERCLASS_API UACCampaignSave : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CAMPAIGN_VERSION = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category="Save") int32 SaveVersion = CAMPAIGN_VERSION;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Campaign") int32 Chapter = 1;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Campaign") int32 Mission = 1;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Campaign") FName CurrentMap;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Campaign") FPrimaryAssetId SelectedFighter;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Campaign") float PlaytimeSeconds = 0.f;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Campaign") FDateTime SavedAtUtc;
	UPROPERTY(SaveGame, BlueprintReadWrite, Category="Campaign") FGameplayTagContainer CompletedMissions;
};
