// Project Settings > Game > Afterclass Input. Points C++ at the Enhanced Input assets created in the editor.
// Default paths are documented in Docs/EDITOR_SETUP.md; override here (saved to Config/DefaultGame.ini) if you move them.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ACInputSettings.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Afterclass Input"))
class AFTERCLASS_API UACInputSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UACInputSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Single context containing both keyboard/mouse and gamepad mappings. Keys must be marked Player Mappable to be remappable. */
	UPROPERTY(Config, EditAnywhere, Category="Contexts") TSoftObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY(Config, EditAnywhere, Category="Contexts") int32 DefaultMappingPriority = 0;

	UPROPERTY(Config, EditAnywhere, Category="Actions") TSoftObjectPtr<UInputAction> MoveAction;    // Axis2D
	UPROPERTY(Config, EditAnywhere, Category="Actions") TSoftObjectPtr<UInputAction> LookAction;    // Axis2D
	UPROPERTY(Config, EditAnywhere, Category="Actions") TSoftObjectPtr<UInputAction> LightAction;   // Digital
	UPROPERTY(Config, EditAnywhere, Category="Actions") TSoftObjectPtr<UInputAction> HeavyAction;   // Digital
	UPROPERTY(Config, EditAnywhere, Category="Actions") TSoftObjectPtr<UInputAction> DodgeAction;   // Digital
	UPROPERTY(Config, EditAnywhere, Category="Actions") TSoftObjectPtr<UInputAction> BlockAction;   // Digital (hold)
	UPROPERTY(Config, EditAnywhere, Category="Actions") TSoftObjectPtr<UInputAction> GrabAction;    // Digital
	UPROPERTY(Config, EditAnywhere, Category="Actions") TSoftObjectPtr<UInputAction> SpecialAction; // Digital
	UPROPERTY(Config, EditAnywhere, Category="Actions") TSoftObjectPtr<UInputAction> SprintAction;  // Digital (hold)

	static const UACInputSettings* Get() { return GetDefault<UACInputSettings>(); }
};
