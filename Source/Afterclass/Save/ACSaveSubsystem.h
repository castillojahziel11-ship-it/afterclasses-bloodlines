// Save system (spec): 1 auto + 5 manual campaign slots + 1 profile slot. Files are written atomically (temp file + move),
// wrapped in a header with magic, container format version and CRC32 over the serialized payload; corrupt files are moved
// to <Saved>/SaveGames/Afterclass/Quarantine instead of being deleted or silently overwritten.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ACSaveSubsystem.generated.h"

class USaveGame;
class UACProfileSave;
class UACCampaignSave;

UENUM(BlueprintType)
enum class EACSaveSlot : uint8
{
	Auto,
	Manual1,
	Manual2,
	Manual3,
	Manual4,
	Manual5,
	Profile
};

UENUM(BlueprintType)
enum class EACSaveResult : uint8
{
	Ok,
	NotFound,
	/** File failed validation and was moved to the Quarantine folder. */
	CorruptQuarantined,
	/** Written by a newer build; left untouched. */
	VersionTooNew,
	WrongType,
	SerializeFailed,
	IOError
};

UCLASS()
class AFTERCLASS_API UACSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Container header constants. */
	static constexpr uint32 FileMagic = 0x56534341; // 'ACSV' little-endian
	static constexpr int32 ContainerVersion = 1;
	static constexpr int32 NumManualSlots = 5;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Campaign
	UFUNCTION(BlueprintCallable, Category="Afterclass|Save")
	EACSaveResult SaveCampaign(EACSaveSlot Slot, UACCampaignSave* Data);

	UFUNCTION(BlueprintCallable, Category="Afterclass|Save")
	UACCampaignSave* LoadCampaign(EACSaveSlot Slot, EACSaveResult& OutResult);

	// Profile (settings, control remaps, unlocks). Loaded or created at Initialize; always non-null afterwards.
	UFUNCTION(BlueprintPure, Category="Afterclass|Save")
	UACProfileSave* GetProfile() const { return Profile; }

	UFUNCTION(BlueprintCallable, Category="Afterclass|Save")
	EACSaveResult SaveProfile();

	UFUNCTION(BlueprintPure, Category="Afterclass|Save")
	bool DoesSlotExist(EACSaveSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category="Afterclass|Save")
	bool DeleteSlot(EACSaveSlot Slot);

	UFUNCTION(BlueprintPure, Category="Afterclass|Save")
	static FString GetSlotName(EACSaveSlot Slot);

	/** Absolute path helpers (exposed for tests/tools). */
	static FString GetSaveDir();
	static FString GetQuarantineDir();
	static FString GetSlotPath(EACSaveSlot Slot);

	/** Low-level, engine-only API: wraps/validates the container. Public for automation tests. */
	static bool WrapPayload(const TArray<uint8>& Payload, TArray<uint8>& OutFile);
	static bool UnwrapPayload(const TArray<uint8>& File, TArray<uint8>& OutPayload);

private:
	EACSaveResult WriteSlot(EACSaveSlot Slot, USaveGame* Data);
	USaveGame* ReadSlot(EACSaveSlot Slot, EACSaveResult& OutResult);
	void Quarantine(const FString& Path, const TCHAR* Reason);

	UPROPERTY(Transient)
	TObjectPtr<UACProfileSave> Profile;
};
