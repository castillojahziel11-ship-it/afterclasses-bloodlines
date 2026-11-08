// UE wrapper around AC::Eval (AI/ACEvaluatorCore.h). Lives on AACEnemyController (server only).
// Observes the target's combat component (moves started, state changes) to build the 12-entry history, evaluates on the
// difficulty cadence, holds each decision for its reaction latency, then exposes it to the BT (or the controller's
// no-BT fallback). Never reads player input: only replicated/observed combat state.
// Not compiled in the authoring environment (no UE install; KI-002).
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/ACEvaluatorCore.h"
#include "Combat/ACCombatTypes.h"
#include "ACCombatEvaluator.generated.h"

class AAIController;
class APawn;
class UACCombatComponent;

UENUM(BlueprintType)
enum class EACDifficulty : uint8
{
	Easy, Normal, Hard, Nightmare
};

UENUM(BlueprintType)
enum class EACAIAction : uint8
{
	Approach, Retreat, Block, Parry, DodgeL, DodgeR, DodgeBack, Light, Heavy, Grab, Special, Reposition
};

USTRUCT(BlueprintType)
struct AFTERCLASS_API FACAIDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="AI") EACAIAction Action = EACAIAction::Approach;
	UPROPERTY(BlueprintReadOnly, Category="AI") bool bDeliberateError = false;
	UPROPERTY(BlueprintReadOnly, Category="AI") bool bDelayedPunish = false;
	UPROPERTY(BlueprintReadOnly, Category="AI") float Score = 0.f;
	/** World time at which the decision may be executed (evaluation time + reaction latency). */
	UPROPERTY(BlueprintReadOnly, Category="AI") double ReadyTime = 0.0;
};

UCLASS(ClassGroup=(Afterclass), meta=(BlueprintSpawnableComponent))
class AFTERCLASS_API UACCombatEvaluator : public UActorComponent
{
	GENERATED_BODY()

public:
	UACCombatEvaluator();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI") EACDifficulty Difficulty = EACDifficulty::Normal;
	/** 0 = seed from world time; any other value gives deterministic decisions (tests / replays). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI") int64 Seed = 0;
	/** How long Block is held (Parry releases after AC::ParryWindow frames). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI", meta=(Units="s")) float BlockHoldSeconds = 0.45f;
	/** Window after a target dodge in which its next attack counts as a "post-dodge" attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI", meta=(Units="s")) float PostDodgeWindowSeconds = 0.6f;

	UFUNCTION(BlueprintCallable, Category="AI") void SetTarget(AActor* NewTarget);
	UFUNCTION(BlueprintPure, Category="AI") AActor* GetTarget() const { return Target.Get(); }

	/** Pops the pending decision if its reaction latency has elapsed. */
	bool ConsumeReadyDecision(FACAIDecision& Out);
	bool HasPendingDecision() const { return bHasPending; }

	/** Executes a decision on the controlled pawn's combat component / path following. Server only. */
	void ExecuteDecision(AAIController* Controller, const FACAIDecision& Decision);

	/** Latest observed context (also written to the blackboard by UACBTService_UpdateCombatBlackboard). */
	const AC::Eval::Context& GetContext() const { return LastContext; }
	const AC::Eval::History& GetHistory() const { return History; }
	AC::Eval::PlayerAct GetLastPlayerAttack() const { return LastPlayerAttack; }
	/** +1 = target spams heavies, -1 = target feints a lot. */
	float GetAttackPatternBias() const;
	FVector GetCoverPoint() const { return CoverPoint; }

	static AC::Eval::Difficulty ToCore(EACDifficulty D) { return static_cast<AC::Eval::Difficulty>(D); }
	static EACAIAction FromCore(AC::Eval::Action A) { return static_cast<EACAIAction>(A); }
	static AC::Eval::PlayerAct ActFromState(EACCombatState S);
	static AC::Eval::TargetPhase PhaseOf(const UACCombatComponent* Combat);

private:
	UFUNCTION() void HandleTargetMoveStarted(FName MoveId, EACCombatState MoveState);
	UFUNCTION() void HandleTargetStateChanged(EACCombatState OldState, EACCombatState NewState);

	APawn* GetPawn() const;
	UACCombatComponent* GetOwnCombat() const;
	UACCombatComponent* GetTargetCombat() const;
	void BuildContext();
	void UpdateCoverPoint();
	FVector PickMoveGoal(bool bAway, bool bLateral) const;

	TWeakObjectPtr<AActor> Target;
	AC::Eval::History History;
	AC::Eval::Context LastContext;
	AC::Eval::Rng Rng;
	AC::Eval::PlayerAct LastPlayerAttack = AC::Eval::PlayerAct::None;
	FACAIDecision Pending;
	FVector CoverPoint = FVector::ZeroVector;
	double LastEvalTime = -1.0;
	double LastTargetDodgeEnd = -100.0;
	double BlockReleaseTime = -1.0;
	bool bHasPending = false;
	bool bRngSeeded = false;
};
