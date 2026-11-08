// Shared character base: third-person camera, Enhanced Input routing into UACCombatComponent, data-driven locomotion.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ACCharacterBase.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UACCombatComponent;
class UACCharacterDataAsset;
class UInputAction;
class UEnhancedInputComponent;

UCLASS(Abstract)
class AFTERCLASS_API AACCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AACCharacterBase();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category="Afterclass|Character") UACCombatComponent* GetCombat() const { return Combat; }
	UFUNCTION(BlueprintPure, Category="Afterclass|Character") UACCharacterDataAsset* GetCharacterData() const { return CharacterData; }

	/** Applies mesh, anim class, speeds and vitals from CharacterData. Safe to call repeatedly. */
	UFUNCTION(BlueprintCallable, Category="Afterclass|Character") virtual void ApplyCharacterData();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UACCombatComponent> Combat;

	/** All per-fighter differentiation lives in this asset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Afterclass|Character") TObjectPtr<UACCharacterDataAsset> CharacterData;

	/** Fallbacks used when CharacterData is not set. */
	UPROPERTY(EditDefaultsOnly, Category="Afterclass|Character") float DefaultWalkSpeed = 450.f;
	UPROPERTY(EditDefaultsOnly, Category="Afterclass|Character") float DefaultSprintSpeed = 700.f;

	// Input handlers
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_Light(const FInputActionValue& Value);
	void Input_Heavy(const FInputActionValue& Value);
	void Input_Dodge(const FInputActionValue& Value);
	void Input_BlockStarted(const FInputActionValue& Value);
	void Input_BlockCompleted(const FInputActionValue& Value);
	void Input_Grab(const FInputActionValue& Value);
	void Input_Special(const FInputActionValue& Value);
	void Input_SprintStarted(const FInputActionValue& Value);
	void Input_SprintCompleted(const FInputActionValue& Value);

	void SetSprinting(bool bSprinting);

private:
	/** Loads the soft reference synchronously; logs a clear error naming the expected asset path if missing. */
	static UInputAction* LoadAction(const TSoftObjectPtr<UInputAction>& Ref, const TCHAR* Label);

	FVector2D LastMoveInput = FVector2D::ZeroVector;
	float WalkSpeed = 450.f;
	float SprintSpeed = 700.f;
	bool bIsSprinting = false;
};
