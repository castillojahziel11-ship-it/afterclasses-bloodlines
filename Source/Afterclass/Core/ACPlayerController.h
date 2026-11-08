// Adds the default Enhanced Input mapping context for local players and owns control remapping.
// Remaps are applied through UEnhancedInputUserSettings (requires bEnableUserSettings=True, see Config/DefaultInput.ini)
// and persisted in the PROFILE save slot via UACSaveSubsystem (spec: settings & control remaps live in profile).
// Keyboard/mouse and gamepad are both handled by the single default IMC; Steam Input maps controllers onto the gamepad keys.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "ACPlayerController.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UEnhancedInputUserSettings;
class UInputMappingContext;

UCLASS()
class AFTERCLASS_API AACPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	/**
	 * Remaps a Player Mappable key. MappingName = the "Name" in the mapping's Player Mappable Key Settings.
	 * Slot: 0 = primary, 1 = secondary. Persists to the profile save immediately. Returns false on failure.
	 */
	UFUNCTION(BlueprintCallable, Category="Afterclass|Input")
	bool RemapKey(FName MappingName, uint8 Slot, FKey NewKey);

	/** Re-applies every remap stored in the profile (called automatically on BeginPlay/OnPossess). */
	UFUNCTION(BlueprintCallable, Category="Afterclass|Input")
	void LoadRemapsFromProfile();

	/** Clears stored remaps from the profile and restores IMC defaults. */
	UFUNCTION(BlueprintCallable, Category="Afterclass|Input")
	void ResetRemapsToDefault();

	/** Returns true if the last input came from a gamepad (for button-prompt swapping). */
	UFUNCTION(BlueprintPure, Category="Afterclass|Input")
	bool IsUsingGamepad() const { return bLastInputWasGamepad; }

	virtual bool InputKey(const FInputKeyParams& Params) override;

protected:
	void AddDefaultMappingContext();
	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem() const;
	UEnhancedInputUserSettings* GetUserSettings() const;
	bool ApplyRemap(UEnhancedInputUserSettings* Settings, FName MappingName, uint8 Slot, const FKey& Key);

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> DefaultContext;

	bool bLastInputWasGamepad = false;
};
