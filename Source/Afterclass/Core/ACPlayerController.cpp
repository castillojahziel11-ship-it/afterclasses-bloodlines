#include "Core/ACPlayerController.h"
#include "Afterclass.h"
#include "Input/ACInputSettings.h"
#include "Save/ACSaveGames.h"
#include "Save/ACSaveSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "GameplayTagContainer.h"
#include "InputMappingContext.h"
#include "UserSettings/EnhancedInputUserSettings.h"

void AACPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalPlayerController())
	{
		AddDefaultMappingContext();
		LoadRemapsFromProfile();
	}
}

void AACPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	// OnPossess runs on the server; this only does work for the listen-server host's local controller.
	if (IsLocalPlayerController())
	{
		AddDefaultMappingContext();
	}
}

UEnhancedInputLocalPlayerSubsystem* AACPlayerController::GetInputSubsystem() const
{
	const ULocalPlayer* LP = GetLocalPlayer();
	return LP ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP) : nullptr;
}

UEnhancedInputUserSettings* AACPlayerController::GetUserSettings() const
{
	UEnhancedInputLocalPlayerSubsystem* Sub = GetInputSubsystem();
	return Sub ? Sub->GetUserSettings() : nullptr;
}

void AACPlayerController::AddDefaultMappingContext()
{
	UEnhancedInputLocalPlayerSubsystem* Sub = GetInputSubsystem();
	if (!Sub) return;

	const UACInputSettings* S = UACInputSettings::Get();
	if (!DefaultContext)
	{
		DefaultContext = S->DefaultMappingContext.LoadSynchronous();
	}
	if (!DefaultContext)
	{
		UE_LOG(LogAfterclass, Error, TEXT("Default mapping context missing at '%s'. See Docs/EDITOR_SETUP.md."),
			*S->DefaultMappingContext.ToSoftObjectPath().ToString());
		return;
	}

	if (!Sub->HasMappingContext(DefaultContext))
	{
		Sub->AddMappingContext(DefaultContext, S->DefaultMappingPriority);
	}

	if (UEnhancedInputUserSettings* Settings = Sub->GetUserSettings())
	{
		Settings->RegisterInputMappingContext(DefaultContext);
	}
	else
	{
		UE_LOG(LogAfterclass, Warning, TEXT("Enhanced Input user settings disabled; key remapping unavailable. Set bEnableUserSettings=True."));
	}
}

bool AACPlayerController::ApplyRemap(UEnhancedInputUserSettings* Settings, FName MappingName, uint8 Slot, const FKey& Key)
{
	if (!Settings || MappingName.IsNone() || !Key.IsValid()) return false;

	FMapPlayerKeyArgs Args;
	Args.MappingName = MappingName;
	Args.Slot = static_cast<EPlayerMappableKeySlot>(Slot);
	Args.NewKey = Key;

	FGameplayTagContainer Failure;
	Settings->MapPlayerKey(Args, Failure);
	if (!Failure.IsEmpty())
	{
		UE_LOG(LogAfterclass, Warning, TEXT("Remap %s[%d] -> %s failed: %s"), *MappingName.ToString(), Slot, *Key.ToString(), *Failure.ToStringSimple());
		return false;
	}
	return true;
}

bool AACPlayerController::RemapKey(FName MappingName, uint8 Slot, FKey NewKey)
{
	UEnhancedInputUserSettings* Settings = GetUserSettings();
	if (!ApplyRemap(Settings, MappingName, Slot, NewKey)) return false;
	Settings->ApplySettings();

	UGameInstance* GI = GetGameInstance();
	UACSaveSubsystem* Save = GI ? GI->GetSubsystem<UACSaveSubsystem>() : nullptr;
	UACProfileSave* Profile = Save ? Save->GetProfile() : nullptr;
	if (!Profile) return true; // applied for this session, not persisted

	FACKeyRemap* Existing = Profile->KeyRemaps.FindByPredicate([&](const FACKeyRemap& R) { return R.MappingName == MappingName && R.Slot == Slot; });
	if (Existing) { Existing->Key = NewKey; }
	else
	{
		FACKeyRemap& Added = Profile->KeyRemaps.AddDefaulted_GetRef();
		Added.MappingName = MappingName;
		Added.Slot = Slot;
		Added.Key = NewKey;
	}
	return Save->SaveProfile() == EACSaveResult::Ok;
}

void AACPlayerController::LoadRemapsFromProfile()
{
	UEnhancedInputUserSettings* Settings = GetUserSettings();
	UGameInstance* GI = GetGameInstance();
	UACSaveSubsystem* Save = GI ? GI->GetSubsystem<UACSaveSubsystem>() : nullptr;
	const UACProfileSave* Profile = Save ? Save->GetProfile() : nullptr;
	if (!Settings || !Profile) return;

	for (const FACKeyRemap& R : Profile->KeyRemaps)
	{
		ApplyRemap(Settings, R.MappingName, R.Slot, R.Key);
	}
	Settings->ApplySettings();
}

void AACPlayerController::ResetRemapsToDefault()
{
	UEnhancedInputUserSettings* Settings = GetUserSettings();
	UGameInstance* GI = GetGameInstance();
	UACSaveSubsystem* Save = GI ? GI->GetSubsystem<UACSaveSubsystem>() : nullptr;
	UACProfileSave* Profile = Save ? Save->GetProfile() : nullptr;

	if (Settings && Profile)
	{
		for (const FACKeyRemap& R : Profile->KeyRemaps)
		{
			// STUB: relies on UnMapPlayerKey restoring the IMC default for that slot; verify against UE 5.5 behaviour (KI-005).
			FMapPlayerKeyArgs Args;
			Args.MappingName = R.MappingName;
			Args.Slot = static_cast<EPlayerMappableKeySlot>(R.Slot);
			FGameplayTagContainer Failure;
			Settings->UnMapPlayerKey(Args, Failure);
		}
		Settings->ApplySettings();
	}
	if (Profile)
	{
		Profile->KeyRemaps.Reset();
		Save->SaveProfile();
	}
}

bool AACPlayerController::InputKey(const FInputKeyParams& Params)
{
	bLastInputWasGamepad = Params.Key.IsGamepadKey();
	return Super::InputKey(Params);
}
