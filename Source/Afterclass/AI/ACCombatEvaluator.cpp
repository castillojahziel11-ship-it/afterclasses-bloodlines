#include "AI/ACCombatEvaluator.h"
#include "Afterclass.h"
#include "AI/ACBossPhaseComponent.h"
#include "Characters/ACCharacterBase.h"
#include "Characters/ACCharacterDataAsset.h"
#include "Combat/ACCombatComponent.h"

#include "AIController.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"

using namespace AC::Eval;

UACCombatEvaluator::UACCombatEvaluator()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f; // cadence handled by ShouldEvaluate (100 ms / 250 ms Easy)
	SetIsReplicatedByDefault(false);
}

APawn* UACCombatEvaluator::GetPawn() const
{
	const AAIController* C = Cast<AAIController>(GetOwner());
	return C ? C->GetPawn() : Cast<APawn>(GetOwner());
}

UACCombatComponent* UACCombatEvaluator::GetOwnCombat() const
{
	const APawn* P = GetPawn();
	return P ? P->FindComponentByClass<UACCombatComponent>() : nullptr;
}

UACCombatComponent* UACCombatEvaluator::GetTargetCombat() const
{
	const AActor* T = Target.Get();
	return T ? T->FindComponentByClass<UACCombatComponent>() : nullptr;
}

void UACCombatEvaluator::SetTarget(AActor* NewTarget)
{
	if (Target.Get() == NewTarget) return;
	if (UACCombatComponent* Old = GetTargetCombat())
	{
		Old->OnMoveStarted.RemoveDynamic(this, &UACCombatEvaluator::HandleTargetMoveStarted);
		Old->OnCombatStateChanged.RemoveDynamic(this, &UACCombatEvaluator::HandleTargetStateChanged);
	}
	Target = NewTarget;
	History.Clear();
	bHasPending = false;
	if (UACCombatComponent* New = GetTargetCombat())
	{
		New->OnMoveStarted.AddDynamic(this, &UACCombatEvaluator::HandleTargetMoveStarted);
		New->OnCombatStateChanged.AddDynamic(this, &UACCombatEvaluator::HandleTargetStateChanged);
	}
}

void UACCombatEvaluator::EndPlay(const EEndPlayReason::Type Reason)
{
	SetTarget(nullptr);
	Super::EndPlay(Reason);
}

PlayerAct UACCombatEvaluator::ActFromState(EACCombatState S)
{
	switch (S)
	{
	case EACCombatState::LightAttack: return PlayerAct::Light;
	case EACCombatState::HeavyAttack: return PlayerAct::Heavy;
	case EACCombatState::Grabbing:    return PlayerAct::Grab;
	case EACCombatState::Special:
	case EACCombatState::Finisher:    return PlayerAct::Special;
	case EACCombatState::Dodging:     return PlayerAct::Dodge;
	case EACCombatState::Blocking:    return PlayerAct::Block;
	default:                          return PlayerAct::None;
	}
}

TargetPhase UACCombatEvaluator::PhaseOf(const UACCombatComponent* C)
{
	if (!C) return TargetPhase::Neutral;
	switch (C->GetState())
	{
	case EACCombatState::Blocking: return TargetPhase::Blocking;
	case EACCombatState::Dodging:  return TargetPhase::Dodging;
	case EACCombatState::Stagger:
	case EACCombatState::Grabbed:  return TargetPhase::Stunned;
	case EACCombatState::Knockdown:
	case EACCombatState::GetUp:
	case EACCombatState::Dead:     return TargetPhase::Knockdown;
	default: break;
	}
	if (C->IsAttacking())
	{
		switch (C->GetMovePhase())
		{
		case AC::EPhase::Startup:  return TargetPhase::AttackStartup;
		case AC::EPhase::Active:   return TargetPhase::AttackActive;
		case AC::EPhase::Recovery: return TargetPhase::AttackRecovery;
		default: break;
		}
	}
	return TargetPhase::Neutral;
}

void UACCombatEvaluator::HandleTargetMoveStarted(FName /*MoveId*/, EACCombatState MoveState)
{
	const PlayerAct Act = ActFromState(MoveState);
	if (!IsAttackAct(Act)) return;
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	History.Push(Act, (Now - LastTargetDodgeEnd) <= PostDodgeWindowSeconds);
	LastPlayerAttack = Act;
}

void UACCombatEvaluator::HandleTargetStateChanged(EACCombatState OldState, EACCombatState NewState)
{
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (NewState == EACCombatState::Dodging) History.Push(PlayerAct::Dodge);
	if (NewState == EACCombatState::Blocking) History.Push(PlayerAct::Block);
	if (OldState == EACCombatState::Dodging) LastTargetDodgeEnd = Now;

	// Feint: an attack left voluntarily before its first active frame (not because the target was hit).
	const bool bInterrupted = NewState == EACCombatState::Stagger || NewState == EACCombatState::Knockdown
		|| NewState == EACCombatState::Grabbed || NewState == EACCombatState::Dead;
	if (UACCombatComponent::IsAttackState(OldState) && !bInterrupted)
	{
		const UACCombatComponent* TC = GetTargetCombat();
		if (TC && TC->GetActiveStartTime() < 0.0 && History.Size() > 0 && IsAttackAct(History.Recent(0).Act))
		{
			History.MarkNewestAsFeint();
		}
	}
}

float UACCombatEvaluator::GetAttackPatternBias() const
{
	const float Heavy = static_cast<float>(FMath::Min(History.TrailingRun(PlayerAct::Heavy), 4)) / 4.f;
	const float Feint = static_cast<float>(FMath::Min(History.CountOf(PlayerAct::Feint), 4)) / 4.f;
	return FMath::Clamp(Heavy - Feint, -1.f, 1.f);
}

void UACCombatEvaluator::BuildContext()
{
	Context C;
	const APawn* Self = GetPawn();
	const UACCombatComponent* Own = GetOwnCombat();
	const UACCombatComponent* TC = GetTargetCombat();
	if (const AACCharacterBase* Char = Cast<AACCharacterBase>(Self))
	{
		if (const UACCharacterDataAsset* Data = Char->GetCharacterData()) C.PreferredRangeUU = Data->PreferredRange;
	}
	if (Self && Target.IsValid()) C.DistanceUU = FVector::Dist2D(Self->GetActorLocation(), Target->GetActorLocation());
	if (Own)
	{
		C.OwnStamina01 = Own->MaxStamina > 0.f ? Own->Stamina / Own->MaxStamina : 0.f;
		C.OwnHealth01 = Own->Health / FMath::Max(1.f, Own->MaxHealth);
		C.OwnMeter = Own->Meter;
	}
	if (TC)
	{
		C.TargetStamina01 = TC->MaxStamina > 0.f ? TC->Stamina / TC->MaxStamina : 0.f;
		C.Target = PhaseOf(TC);
		C.TargetAttack = TC->IsAttacking() ? ActFromState(TC->GetState()) : PlayerAct::None;
		const EACCombatState S = TC->GetState();
		C.bTargetHasWeapon = S == EACCombatState::WeaponEquipped || S == EACCombatState::Aiming || S == EACCombatState::Reloading;
	}
	LastContext = C;
}

void UACCombatEvaluator::UpdateCoverPoint()
{
	// Cover = navigable point behind self, away from the target (used by Retreat / ranged threats).
	CoverPoint = PickMoveGoal(/*bAway*/ true, /*bLateral*/ false);
}

FVector UACCombatEvaluator::PickMoveGoal(bool bAway, bool bLateral) const
{
	const APawn* Self = GetPawn();
	if (!Self) return FVector::ZeroVector;
	const FVector Origin = Self->GetActorLocation();
	FVector Dir = Target.IsValid() ? (Origin - Target->GetActorLocation()).GetSafeNormal2D() : -Self->GetActorForwardVector();
	if (!bAway) Dir = -Dir;
	if (bLateral) Dir = FVector::CrossProduct(Dir, FVector::UpVector) * (Rng.S & 1 ? 1.f : -1.f);
	const FVector Goal = Origin + Dir * FMath::Max(200.f, LastContext.PreferredRangeUU);
	if (const UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Projected;
		if (Nav->ProjectPointToNavigation(Goal, Projected, FVector(200.f, 200.f, 300.f))) return Projected.Location;
	}
	return Goal;
}

void UACCombatEvaluator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !Owner->HasAuthority() || !World) return;

	if (!bRngSeeded)
	{
		Rng = AC::Eval::Rng(Seed != 0 ? static_cast<uint64>(Seed) : static_cast<uint64>(FPlatformTime::Cycles64()));
		bRngSeeded = true;
	}

	const double Now = World->GetTimeSeconds();
	if (BlockReleaseTime >= 0.0 && Now >= BlockReleaseTime)
	{
		BlockReleaseTime = -1.0;
		if (UACCombatComponent* Own = GetOwnCombat()) Own->RequestBlock(false);
	}

	if (!Target.IsValid()) return;
	const int64 NowMs = static_cast<int64>(Now * 1000.0);
	const int64 LastMs = LastEvalTime < 0.0 ? -1 : static_cast<int64>(LastEvalTime * 1000.0);
	if (!ShouldEvaluate(NowMs, LastMs, ToCore(Difficulty))) return;
	LastEvalTime = Now;

	BuildContext();
	UpdateCoverPoint();
	if (bHasPending) return; // still waiting out reaction latency for the previous decision

	const Decision D = Evaluate(LastContext, History, ToCore(Difficulty), Rng);
	Pending.Action = FromCore(D.Chosen);
	Pending.bDeliberateError = D.bDeliberateError;
	Pending.bDelayedPunish = D.bDelayedPunish;
	Pending.Score = D.BestScore;
	Pending.ReadyTime = Now + D.DelayMs / 1000.0;
	bHasPending = true;
}

bool UACCombatEvaluator::ConsumeReadyDecision(FACAIDecision& Out)
{
	if (!bHasPending || !GetWorld() || GetWorld()->GetTimeSeconds() < Pending.ReadyTime) return false;
	Out = Pending;
	bHasPending = false;
	return true;
}

void UACCombatEvaluator::ExecuteDecision(AAIController* Controller, const FACAIDecision& Decision)
{
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	UACCombatComponent* Own = GetOwnCombat();
	if (!Pawn || !Own) return;
	if (Target.IsValid()) Controller->SetFocus(Target.Get()); // keeps control rotation on the target for dodge directions

	const double Now = GetWorld()->GetTimeSeconds();
	switch (Decision.Action)
	{
	case EACAIAction::Approach:
		if (Target.IsValid()) Controller->MoveToActor(Target.Get(), FMath::Max(50.f, LastContext.PreferredRangeUU * 0.8f));
		break;
	case EACAIAction::Retreat:
		Controller->MoveToLocation(CoverPoint, 50.f);
		break;
	case EACAIAction::Reposition:
		Controller->MoveToLocation(PickMoveGoal(false, true), 50.f);
		break;
	case EACAIAction::Block:
		Controller->StopMovement();
		Own->RequestBlock(true);
		BlockReleaseTime = Now + BlockHoldSeconds;
		break;
	case EACAIAction::Parry:
		Controller->StopMovement();
		Own->RequestBlock(true); // parry = block started inside the 4f window; release right after it
		BlockReleaseTime = Now + static_cast<double>(AC::ParryWindow + 1) / AC::SimHz;
		break;
	case EACAIAction::DodgeL:    Own->RequestDodge(FVector2D(-1.f, 0.f)); break;
	case EACAIAction::DodgeR:    Own->RequestDodge(FVector2D(1.f, 0.f)); break;
	case EACAIAction::DodgeBack: Own->RequestDodge(FVector2D(0.f, -1.f)); break;
	case EACAIAction::Light:     Own->RequestLight(); break;
	case EACAIAction::Heavy:
		if (UACBossPhaseComponent* Boss = Pawn->FindComponentByClass<UACBossPhaseComponent>())
		{
			if (Boss->TryTelegraphUnblockable()) break; // boss swaps one heavy per phase for its telegraphed unblockable
		}
		Own->RequestHeavy();
		break;
	case EACAIAction::Grab:      Own->RequestGrab(); break;
	case EACAIAction::Special:   Own->RequestSpecial(); break;
	default: break;
	}
}
