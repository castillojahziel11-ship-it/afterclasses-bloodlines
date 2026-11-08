// STUB: UE wrapper around AC::FrameCore. State enum, TryStartAction legality order, replication of State/Health/Stamina/Meter
// and server-routed requests are real; montage dispatch, hit sweeps, stamina cost lookup and facing correction are NOT yet
// implemented (see Docs/KNOWN_ISSUES.md KI-001). Not compiled in the authoring environment (no UE install; KI-002).
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACFrameCore.h"
#include "ACCombatComponent.generated.h"

UENUM(BlueprintType)
enum class EACCombatState : uint8
{
	Idle, Locomotion, Sprint, LightAttack, HeavyAttack, Blocking, Dodging, Stagger, Knockdown, GetUp,
	WeaponEquipped, Aiming, Reloading, Finisher, Dead,
	Grabbing, Special
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FACOnCombatStateChanged, EACCombatState, OldState, EACCombatState, NewState);

UCLASS(ClassGroup=(Afterclass), meta=(BlueprintSpawnableComponent))
class AFTERCLASS_API UACCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACCombatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called by the owning character once its data asset is known. Server-authoritative. */
	void InitializeVitals(float InMaxHealth, float InMaxStamina);

	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestLight();
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestHeavy();
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestDodge(FVector2D Dir);
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestBlock(bool bHeld);
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestGrab();
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestSpecial();
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void SetSprinting(bool bSprinting);

	/** Order: legality, stamina, lock, cancel window, facing. Server only; clients route through ServerRequestAction. */
	bool TryStartAction(EACCombatState Next, FName MoveId);

	UFUNCTION(BlueprintPure, Category="Afterclass|Combat") EACCombatState GetState() const { return State; }
	UFUNCTION(BlueprintPure, Category="Afterclass|Combat") bool IsInFreeState() const;

	UPROPERTY(BlueprintAssignable, Category="Afterclass|Combat")
	FACOnCombatStateChanged OnCombatStateChanged;

	UPROPERTY(ReplicatedUsing=OnRep_State, VisibleInstanceOnly, BlueprintReadOnly, Category="Afterclass|Combat")
	EACCombatState State = EACCombatState::Idle;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Afterclass|Combat")
	float Health = 100.f;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Afterclass|Combat")
	float MaxHealth = 100.f;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Afterclass|Combat")
	float Stamina = 100.f;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Afterclass|Combat")
	float MaxStamina = 100.f;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Afterclass|Combat")
	float Meter = 0.f;

	bool bActionLocked = false;
	bool bCancelWindowOpen = false;

protected:
	UFUNCTION(Server, Reliable)
	void ServerRequestAction(EACCombatState Next, FName MoveId);

	UFUNCTION()
	void OnRep_State(EACCombatState OldState);

private:
	/** Routes to TryStartAction on the server, or to the server RPC on owning clients. */
	void RouteAction(EACCombatState Next, FName MoveId);
	void SetState(EACCombatState NewState);
};
