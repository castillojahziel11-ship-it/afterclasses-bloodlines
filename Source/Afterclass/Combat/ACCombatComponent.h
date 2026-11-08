// UE wrapper around AC::FrameCore (spec §7). Server-authoritative 60 Hz fixed-step state machine: action timelines,
// dodge i-frames / perfect dodge, block / parry / guard break, grabs + escape, combo scaling with cap and bounce limits,
// stagger / knockdown / get-up, meter (incl. YN "Anonymous" passive), montage dispatch by MoveId, hit windows and
// hit resolution. Clients receive State/vitals via replication and montages/hit FX via multicasts.
// Not compiled in the authoring environment (no UE install; KI-002).
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/ACCombatTypes.h"
#include "Combat/ACFrameCore.h"
#include "ACCombatComponent.generated.h"

class UACCharacterDataAsset;
class UACHitValidation;
class UCameraShakeBase;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FACOnCombatStateChanged, EACCombatState, OldState, EACCombatState, NewState);
/** Fires on every machine when a hit is resolved (camera kick / VFX / VO hook point). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FACOnHitConfirmed, AActor*, Attacker, AActor*, Victim, EACHitOutcome, Outcome, FVector, ImpactPoint);
/** Server: fires on the attacker's component when a move starts (AI history / telemetry). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FACOnMoveStarted, FName, MoveId, EACCombatState, MoveState);

UCLASS(ClassGroup=(Afterclass), meta=(BlueprintSpawnableComponent))
class AFTERCLASS_API UACCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACCombatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Called by the owning character once its data asset is known. Server-authoritative. */
	void InitializeVitals(float InMaxHealth, float InMaxStamina);
	/** Pulls move table, dodge distance and passives from the data asset (null = ACFrameCore defaults). */
	void ConfigureFromData(const UACCharacterDataAsset* Data);

	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestLight();
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestHeavy();
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestDodge(FVector2D Dir);
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestBlock(bool bHeld);
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestGrab();
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestSpecial();
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void RequestFinisher();
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat") void SetSprinting(bool bSprinting);
	/** Starts an arbitrary move-table row (AI / boss telegraphs). Server only. */
	bool StartMoveById(FName MoveId);

	/** Order: legality, stamina/meter, lock, cancel window, facing. Server only; clients route through ServerRequestAction. */
	bool TryStartAction(EACCombatState Next, FName MoveId);

	// ---- Hit pipeline ---------------------------------------------------------------------------------------
	/** AN_HitWindow_Open begin/end. Server only (no-op elsewhere). SwingIndex distinguishes multi-hit swings. */
	void OpenHitWindowFromNotify(const TArray<FName>& BoneChain, float Radius, int32 SwingIndex);
	void CloseHitWindowFromNotify();
	/** Resolves an incoming hit on this (victim) component. Server only. */
	EACHitOutcome ReceiveHit(AActor* Attacker, const FACHitData& Data, const FHitResult& Hit);
	/** Called by UACHitstopSubsystem: pause the simulation for N frames. */
	void AddHitstopFrames(int32 Frames) { PendingHitstopFrames = FMath::Max(PendingHitstopFrames, Frames); }

	// ---- Queries --------------------------------------------------------------------------------------------
	UFUNCTION(BlueprintPure, Category="Afterclass|Combat") EACCombatState GetState() const { return State; }
	UFUNCTION(BlueprintPure, Category="Afterclass|Combat") bool IsInFreeState() const;
	UFUNCTION(BlueprintPure, Category="Afterclass|Combat") bool IsAttacking() const { return IsAttackState(State); }
	UFUNCTION(BlueprintPure, Category="Afterclass|Combat") bool IsFinisherEligible(const AActor* Target) const;
	UFUNCTION(BlueprintPure, Category="Afterclass|Combat") int32 GetComboHitsTaken() const { return ComboTaken.Hits; }
	UFUNCTION(BlueprintPure, Category="Afterclass|Combat") bool IsCounterWindowOpen() const { return CounterWindowFrames > 0; }
	/** Server: frames simulated in the current state. */
	int32 GetStateFrame() const { return StateFrame; }
	/** Server: true while recovering from an attack that has not connected (whiff punish window). */
	bool IsInWhiffRecovery() const;
	/** Server: world time at which the current attack's first active frame began (or -1). */
	double GetActiveStartTime() const { return ActiveStartTime; }
	/** Server: frames until the current attack becomes active (>= 0), or -1 if not in startup. */
	int32 GetFramesUntilActive() const;
	/** Resolves a move row from the fighter's MoveSet, falling back to ACFrameCore defaults. */
	bool FindMove(FName MoveId, FACMoveRow& OutRow) const;
	static bool IsAttackState(EACCombatState S);
	static FACMoveRow MakeDefaultMove(FName MoveId);

	UPROPERTY(BlueprintAssignable, Category="Afterclass|Combat") FACOnCombatStateChanged OnCombatStateChanged;
	UPROPERTY(BlueprintAssignable, Category="Afterclass|Combat") FACOnHitConfirmed OnHitConfirmed;
	UPROPERTY(BlueprintAssignable, Category="Afterclass|Combat") FACOnMoveStarted OnMoveStarted;

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

	/** Set by the match flow; finishers require the final round (spec §7). Defaults true for dev arenas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Afterclass|Combat") bool bFinalRound = true;
	/** Design placeholder (not in spec §7): stamina per dodge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Afterclass|Combat", meta=(ClampMin="0")) float DodgeStaminaCost = 15.f;
	/** Soft lock-on radius used to face the nearest opponent when an action starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Afterclass|Combat", meta=(Units="cm")) float FacingAssistRadius = 400.f;
	/** Optional camera kick played for the locally controlled attacker/victim on a landed hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Afterclass|Combat") TSubclassOf<UCameraShakeBase> HitCameraShake;

	bool bActionLocked = false;
	bool bCancelWindowOpen = false;

protected:
	UFUNCTION(Server, Reliable)
	void ServerRequestAction(EACCombatState Next, FName MoveId);

	UFUNCTION(Server, Reliable)
	void ServerRequestDodge(FVector2D Dir);

	/** Plays the montage of a move row on every machine. NAME_None stops the active montage. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayMove(FName MoveId);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHitConfirmed(AActor* Attacker, AActor* Victim, EACHitOutcome Outcome, int32 HitstopFrames, FVector_NetQuantize ImpactPoint);

	UFUNCTION()
	void OnRep_State(EACCombatState OldState);

private:
	void RouteAction(EACCombatState Next, FName MoveId);
	void SetState(EACCombatState NewState);
	void EnterTimedState(EACCombatState NewState, int32 Frames);
	void SimulateFrame();
	void StepActionTimeline();
	void BeginDodge(FVector2D Dir);
	void FaceNearestOpponent();
	void PlayMoveLocal(FName MoveId);
	void OpenAutoWindow();
	void CloseAllWindows();
	void HandleHitDetected(AActor* Victim, const FHitResult& Hit, int32 SwingId, FName Bone);
	void ApplyDamage(float Amount);
	void ApplyKnockback(AActor* Attacker, const FACHitData& Data, bool bGroundBounce);
	void AddMeter(float Delta);
	void SpendStamina(float Amount);
	void TryGrabEscape();
	void BroadcastHit(AActor* Attacker, AActor* Victim, EACHitOutcome Outcome, int32 HitstopFrames, const FVector& ImpactPoint);

	UPROPERTY(Transient) TObjectPtr<UDataTable> MoveTable;
	UPROPERTY(Transient) TObjectPtr<UACHitValidation> HitValidation;
	TWeakObjectPtr<AActor> GrabbedBy;
	TWeakObjectPtr<AActor> GrabbedVictim;

	FACMoveRow CurrentMove;
	FACHitData PendingGrabHit;
	AC::FComboState ComboTaken;
	double FrameAccumulator = 0.0;
	double ActiveStartTime = -1.0;
	double DodgeStartTime = -1.0;
	float DodgeDistance = 350.f;
	int32 StateFrame = 0;
	int32 StateDurationFrames = 0;
	int32 PendingHitstopFrames = 0;
	int32 CounterWindowFrames = 0;
	int32 FramesSinceLandedHit = 0;
	int32 FramesSinceStaminaUse = 0;
	int32 SwingCounter = 0;
	bool bHasMove = false;
	bool bMoveConnected = false;
	bool bNotifyWindowThisMove = false;
	bool bAutoWindowOpen = false;
	bool bAnonymous = false;
	bool bPerfectDodgeUsed = false;
	TSet<FName> WarnedMissingMontages;
};
