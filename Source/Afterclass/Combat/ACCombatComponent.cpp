#include "Combat/ACCombatComponent.h"
#include "Afterclass.h"
#include "Characters/ACCharacterDataAsset.h"
#include "Combat/ACHitValidation.h"
#include "Combat/ACHitstopSubsystem.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "CollisionQueryParams.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

namespace
{
	AC::FFrameData ToFrameData(const FACMoveRow& Row)
	{
		return AC::FFrameData{ FMath::Max(1, Row.StartupFrames), FMath::Max(1, Row.ActiveFrames), FMath::Max(0, Row.RecoveryFrames) };
	}

	UACCombatComponent* CombatOf(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<UACCombatComponent>() : nullptr;
	}

	/** Signed yaw (deg) between Defender's forward and the direction to Attacker. */
	float AngleToAttacker(const AActor* Defender, const AActor* Attacker)
	{
		const FVector Fwd = Defender->GetActorForwardVector().GetSafeNormal2D();
		const FVector ToAtt = (Attacker->GetActorLocation() - Defender->GetActorLocation()).GetSafeNormal2D();
		if (ToAtt.IsNearlyZero()) return 0.f;
		const float Dot = FMath::Clamp(FVector::DotProduct(Fwd, ToAtt), -1.f, 1.f);
		const float Sign = FVector::CrossProduct(Fwd, ToAtt).Z >= 0.f ? 1.f : -1.f;
		return Sign * FMath::RadiansToDegrees(FMath::Acos(Dot));
	}

	const FName MoveLight(TEXT("Light"));
	const FName MoveHeavy(TEXT("Heavy"));
	const FName MoveGrab(TEXT("Grab"));
	const FName MoveSpecial(TEXT("Special"));
	const FName MoveFinisher(TEXT("Finisher"));
	const FName MoveDodge(TEXT("Dodge"));
	const FName MoveHitReact(TEXT("HitReact"));
	const FName MoveKnockdown(TEXT("Knockdown"));
	const FName MoveGetUp(TEXT("GetUp"));
}

UACCombatComponent::UACCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void UACCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UACCombatComponent, State);
	DOREPLIFETIME(UACCombatComponent, Health);
	DOREPLIFETIME(UACCombatComponent, MaxHealth);
	DOREPLIFETIME(UACCombatComponent, Stamina);
	DOREPLIFETIME(UACCombatComponent, MaxStamina);
	DOREPLIFETIME(UACCombatComponent, Meter);
}

void UACCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner())
	{
		HitValidation = Owner->FindComponentByClass<UACHitValidation>();
		if (HitValidation)
		{
			HitValidation->OnHitDetected.BindUObject(this, &UACCombatComponent::HandleHitDetected);
		}
		else if (Owner->HasAuthority())
		{
			UE_LOG(LogAfterclass, Warning, TEXT("Combat: %s has no UACHitValidation component; melee hits cannot connect."), *GetNameSafe(Owner));
		}
	}
}

void UACCombatComponent::InitializeVitals(float InMaxHealth, float InMaxStamina)
{
	MaxHealth = FMath::Max(1.f, InMaxHealth);
	MaxStamina = FMath::Max(0.f, InMaxStamina);
	Health = MaxHealth;
	Stamina = MaxStamina;
	Meter = AC::ClampMeter(0.f);
}

void UACCombatComponent::ConfigureFromData(const UACCharacterDataAsset* Data)
{
	if (!Data)
	{
		MoveTable = nullptr;
		bAnonymous = false;
		return;
	}
	bAnonymous = Data->bAnonymousPassive;
	DodgeDistance = Data->DodgeDistance;
	MoveTable = Data->MoveSet.LoadSynchronous(); // STUB: async streaming via AssetManager (KI-006)
	if (!MoveTable && !Data->MoveSet.IsNull())
	{
		UE_LOG(LogAfterclass, Warning, TEXT("Combat: MoveSet '%s' failed to load; using ACFrameCore defaults."), *Data->MoveSet.ToSoftObjectPath().ToString());
	}
	if (MoveTable && MoveTable->GetRowStruct() != FACMoveRow::StaticStruct())
	{
		UE_LOG(LogAfterclass, Error, TEXT("Combat: MoveSet '%s' row struct is not FACMoveRow; ignoring it."), *GetNameSafe(MoveTable));
		MoveTable = nullptr;
	}
}

// ---- Move table ------------------------------------------------------------------------------------------------

FACMoveRow UACCombatComponent::MakeDefaultMove(FName MoveId)
{
	// Mirrors Content/Data/MoveTables/MT_Default.csv. Frame data from spec §7 (Light/Heavy); others are design placeholders.
	FACMoveRow R;
	R.MoveId = MoveId;
	auto SetFrames = [&R](const AC::FFrameData& F) { R.StartupFrames = F.Startup; R.ActiveFrames = F.Active; R.RecoveryFrames = F.Recovery; };
	if (MoveId == MoveHeavy)
	{
		R.State = EACCombatState::HeavyAttack; SetFrames(AC::Heavy); R.StaminaCost = 18.f;
		R.HitData.Damage = 60.f; R.HitData.HitStun = 20; R.HitData.Knockback = 450.f; R.HitData.HitType = EACHitType::Heavy;
		R.HitData.HitstopFrames = 6; R.HitData.bWallBounce = true;
		R.DefaultBoneChain = { TEXT("foot_r"), TEXT("calf_r") }; R.HitRadius = 22.f;
	}
	else if (MoveId == MoveGrab)
	{
		R.State = EACCombatState::Grabbing; SetFrames(AC::GrabMove); R.StaminaCost = 10.f;
		R.HitData.Damage = 80.f; R.HitData.HitStun = 0; R.HitData.Knockback = 300.f; R.HitData.LaunchZ = 200.f;
		R.HitData.HitType = EACHitType::Grab; R.HitData.bUnblockable = true; R.HitData.HitstopFrames = 2;
		R.DefaultBoneChain = { TEXT("hand_l"), TEXT("hand_r") }; R.HitRadius = 20.f;
	}
	else if (MoveId == MoveSpecial)
	{
		R.State = EACCombatState::Special; SetFrames(AC::SpecialMove); R.MeterCost = 50.f;
		R.HitData.Damage = 90.f; R.HitData.HitStun = 24; R.HitData.Knockback = 500.f; R.HitData.LaunchZ = 450.f;
		R.HitData.HitType = EACHitType::Special; R.HitData.HitstopFrames = 8; R.HitData.bGroundBounce = true;
		R.DefaultBoneChain = { TEXT("hand_r"), TEXT("foot_r") }; R.HitRadius = 26.f;
	}
	else if (MoveId == MoveFinisher)
	{
		R.State = EACCombatState::Finisher; SetFrames(AC::FinisherMove); R.MeterCost = 100.f;
		R.HitData.Damage = 250.f; R.HitData.HitStun = 60; R.HitData.Knockback = 600.f; R.HitData.LaunchZ = 500.f;
		R.HitData.HitType = EACHitType::Finisher; R.HitData.bUnblockable = true; R.HitData.HitstopFrames = 12;
		R.DefaultBoneChain = { TEXT("hand_r"), TEXT("foot_r") }; R.HitRadius = 30.f;
	}
	else // Light and anything unknown
	{
		R.State = EACCombatState::LightAttack; SetFrames(AC::Light); R.StaminaCost = 8.f;
		R.HitData.Damage = 30.f; R.HitData.HitStun = 14; R.HitData.Knockback = 200.f; R.HitData.HitType = EACHitType::Light;
		R.HitData.HitstopFrames = AC::DefaultHitstopFrames;
		R.DefaultBoneChain = { TEXT("hand_r"), TEXT("lowerarm_r") }; R.HitRadius = 18.f;
	}
	return R;
}

bool UACCombatComponent::FindMove(FName MoveId, FACMoveRow& OutRow) const
{
	if (MoveTable)
	{
		if (const FACMoveRow* Row = MoveTable->FindRow<FACMoveRow>(MoveId, TEXT("ACCombat"), /*bWarnIfRowMissing*/ false))
		{
			OutRow = *Row;
			if (OutRow.MoveId.IsNone()) OutRow.MoveId = MoveId;
			return true;
		}
	}
	if (MoveId == MoveLight || MoveId == MoveHeavy || MoveId == MoveGrab || MoveId == MoveSpecial || MoveId == MoveFinisher)
	{
		OutRow = MakeDefaultMove(MoveId);
		return true;
	}
	return false;
}

// ---- State queries ---------------------------------------------------------------------------------------------

bool UACCombatComponent::IsInFreeState() const
{
	return State == EACCombatState::Idle || State == EACCombatState::Locomotion || State == EACCombatState::Sprint;
}

bool UACCombatComponent::IsAttackState(EACCombatState S)
{
	return S == EACCombatState::LightAttack || S == EACCombatState::HeavyAttack || S == EACCombatState::Grabbing
		|| S == EACCombatState::Special || S == EACCombatState::Finisher;
}

bool UACCombatComponent::IsInWhiffRecovery() const
{
	return bHasMove && IsAttackState(State) && !bMoveConnected
		&& AC::PhaseAt(ToFrameData(CurrentMove), StateFrame) == AC::EPhase::Recovery;
}

int32 UACCombatComponent::GetFramesUntilActive() const
{
	if (!bHasMove || !IsAttackState(State)) return -1;
	const int32 Remaining = CurrentMove.StartupFrames - StateFrame;
	return Remaining >= 0 ? Remaining : -1;
}

AC::EPhase UACCombatComponent::GetMovePhase() const
{
	return (bHasMove && IsAttackState(State)) ? AC::PhaseAt(ToFrameData(CurrentMove), StateFrame) : AC::EPhase::Done;
}

bool UACCombatComponent::IsFinisherEligible(const AActor* Target) const
{
	const UACCombatComponent* T = CombatOf(Target);
	return T && T != this && T->State != EACCombatState::Dead && AC::FinisherEligible(T->Health, T->MaxHealth, Meter, bFinalRound);
}

// ---- Action start ----------------------------------------------------------------------------------------------

bool UACCombatComponent::TryStartAction(EACCombatState Next, FName MoveId)
{
	if (State == EACCombatState::Dead) return false;
	if (State == EACCombatState::Grabbed)
	{
		if (Next == EACCombatState::Grabbing) TryGrabEscape();
		return false;
	}

	// 1. Legality: free state, hit-confirm cancel, or counter window after parry / perfect dodge.
	const bool bCounter = CounterWindowFrames > 0 && (State == EACCombatState::Blocking || State == EACCombatState::Dodging) && IsAttackState(Next);
	const bool bCancel = bCancelWindowOpen && (IsAttackState(Next) || Next == EACCombatState::Dodging);
	const bool bBlockToDodge = State == EACCombatState::Blocking && Next == EACCombatState::Dodging;
	if (!(IsInFreeState() || bCounter || bCancel || bBlockToDodge)) return false;

	// Pure locomotion transitions carry no move.
	if (Next == EACCombatState::Idle || Next == EACCombatState::Locomotion || Next == EACCombatState::Sprint || Next == EACCombatState::Blocking)
	{
		if (bActionLocked && Next == EACCombatState::Blocking) return false;
		SetState(Next);
		return true;
	}

	// 2-5. Stamina / meter, lock, cancel window, facing, commit.
	if (Next == EACCombatState::Dodging)
	{
		if (Stamina < DodgeStaminaCost || bActionLocked) return false;
		bCancelWindowOpen = false;
		CounterWindowFrames = 0;
		BeginDodge(FVector2D::ZeroVector);
		return true;
	}
	if (!IsAttackState(Next)) return false;
	FACMoveRow Row;
	if (!FindMove(MoveId, Row))
	{
		UE_LOG(LogAfterclass, Warning, TEXT("Combat: move '%s' not in MoveSet and has no default."), *MoveId.ToString());
		return false;
	}
	return CommitMove(Row);
}

bool UACCombatComponent::CommitMove(const FACMoveRow& Row)
{
	if (!IsAttackState(Row.State)) return false;
	// 2. Stamina / meter (+ finisher eligibility against a target in assist range).
	if (Stamina < Row.StaminaCost || Meter < Row.MeterCost) return false;
	if (Row.State == EACCombatState::Finisher)
	{
		bool bAnyEligible = false;
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ACFinisher), false, GetOwner());
		GetWorld()->OverlapMultiByObjectType(Overlaps, GetOwner()->GetActorLocation(), FQuat::Identity,
			FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(FacingAssistRadius), Params);
		for (const FOverlapResult& O : Overlaps) { bAnyEligible |= IsFinisherEligible(O.GetActor()); }
		if (!bAnyEligible) return false;
	}
	// 3. Lock.
	if (bActionLocked) return false;
	// 4. Cancel / counter window consumed.
	bCancelWindowOpen = false;
	CounterWindowFrames = 0;
	// 5. Facing correction, then commit.
	FaceNearestOpponent();
	SpendStamina(Row.StaminaCost);
	if (Row.MeterCost > 0.f) AddMeter(-Row.MeterCost);
	CloseAllWindows();
	CurrentMove = Row;
	bHasMove = true;
	bMoveConnected = false;
	bNotifyWindowThisMove = false;
	ActiveStartTime = -1.0;
	++SwingCounter;
	SetState(Row.State);
	MulticastPlayMove(Row.MoveId);
	OnMoveStarted.Broadcast(Row.MoveId, Row.State);
	return true;
}

bool UACCombatComponent::StartCustomMove(const FACMoveRow& Row)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || State == EACCombatState::Dead || State == EACCombatState::Grabbed) return false;
	if (!(IsInFreeState() || bCancelWindowOpen)) return false;
	return CommitMove(Row);
}

bool UACCombatComponent::StartMoveById(FName MoveId)
{
	FACMoveRow Row;
	if (!GetOwner() || !GetOwner()->HasAuthority() || !FindMove(MoveId, Row)) return false;
	return TryStartAction(Row.State, MoveId);
}

void UACCombatComponent::BeginDodge(FVector2D Dir)
{
	SpendStamina(DodgeStaminaCost);
	CloseAllWindows();
	bHasMove = false;
	bPerfectDodgeUsed = false;
	DodgeStartTime = GetWorld()->GetTimeSeconds();
	SetState(EACCombatState::Dodging);
	MulticastPlayMove(MoveDodge);

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		FVector WorldDir;
		if (Dir.IsNearlyZero())
		{
			WorldDir = -Character->GetActorForwardVector(); // neutral dodge = backstep
		}
		else
		{
			const FRotator Yaw(0.f, Character->GetControlRotation().Yaw, 0.f);
			WorldDir = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X) * Dir.Y + FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y) * Dir.X;
		}
		WorldDir = WorldDir.GetSafeNormal2D();
		// Velocity that covers DodgeDistance over the i-frame span, then ground friction brakes it.
		const float Speed = DodgeDistance / (static_cast<float>(AC::DodgeIFrameEnd) / AC::SimHz);
		Character->LaunchCharacter(WorldDir * Speed, true, false);
	}
}

void UACCombatComponent::FaceNearestOpponent()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ACFacingAssist), false, Owner);
	World->OverlapMultiByObjectType(Overlaps, Owner->GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(FacingAssistRadius), Params);

	const AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (const FOverlapResult& O : Overlaps)
	{
		const AActor* Other = O.GetActor();
		const UACCombatComponent* C = CombatOf(Other);
		if (!Other || Other == Owner || !C || C->State == EACCombatState::Dead) continue;
		// Prefer targets in front: distance weighted by angle.
		const float Angle = FMath::Abs(AngleToAttacker(Owner, Other));
		const float Score = FVector::Dist2D(Owner->GetActorLocation(), Other->GetActorLocation()) * (1.f + Angle / 90.f);
		if (Score < BestScore) { BestScore = Score; Best = Other; }
	}
	if (Best)
	{
		const FVector To = (Best->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
		if (!To.IsNearlyZero()) Owner->SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
	}
}

// ---- Simulation ------------------------------------------------------------------------------------------------

void UACCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return; // clients follow replicated State

	// World delta (not actor-dilated): hitstop is applied exactly once via PendingHitstopFrames.
	const double WorldDt = GetWorld() ? GetWorld()->GetDeltaSeconds() : DeltaTime;
	const int32 Steps = AC::ConsumeFixedSteps(FrameAccumulator, WorldDt);
	for (int32 i = 0; i < Steps; ++i)
	{
		SimulateFrame();
	}
}

void UACCombatComponent::SimulateFrame()
{
	if (PendingHitstopFrames > 0)
	{
		--PendingHitstopFrames;
		return;
	}

	++StateFrame;
	++FramesSinceLandedHit;
	++FramesSinceStaminaUse;
	if (CounterWindowFrames > 0) --CounterWindowFrames;

	if (bAnonymous)
	{
		Meter = AC::AnonymousDecayStep(Meter, FramesSinceLandedHit);
	}
	if (State != EACCombatState::Blocking && State != EACCombatState::Dead && FramesSinceStaminaUse > AC::StaminaRegenDelayFrames)
	{
		Stamina = FMath::Min(MaxStamina, Stamina + AC::StaminaRegenPerSec / AC::SimHz);
	}

	switch (State)
	{
	case EACCombatState::LightAttack:
	case EACCombatState::HeavyAttack:
	case EACCombatState::Grabbing:
	case EACCombatState::Special:
	case EACCombatState::Finisher:
		StepActionTimeline();
		break;
	case EACCombatState::Dodging:
		if (AC::IsDodgeDone(StateFrame)) SetState(EACCombatState::Idle);
		break;
	case EACCombatState::Stagger:
		if (StateFrame >= StateDurationFrames) SetState(EACCombatState::Idle);
		break;
	case EACCombatState::Knockdown:
		if (StateFrame >= StateDurationFrames)
		{
			EnterTimedState(EACCombatState::GetUp, AC::GetUpFrames);
			MulticastPlayMove(MoveGetUp);
		}
		break;
	case EACCombatState::GetUp:
		if (StateFrame >= StateDurationFrames) SetState(EACCombatState::Idle);
		break;
	case EACCombatState::Grabbed:
		if (!AC::IsGrabEscapeOpen(StateFrame))
		{
			// Escape window missed: the throw lands.
			GrabbedBy.Reset();
			ApplyDamage(PendingGrabHit.Damage);
			if (State != EACCombatState::Dead)
			{
				EnterTimedState(EACCombatState::Knockdown, AC::KnockdownFrames);
				MulticastPlayMove(MoveKnockdown);
			}
		}
		break;
	default:
		break;
	}
}

void UACCombatComponent::StepActionTimeline()
{
	if (!bHasMove)
	{
		SetState(EACCombatState::Idle);
		return;
	}
	const AC::FFrameData FD = ToFrameData(CurrentMove);
	if (AC::IsFirstActiveFrame(FD, StateFrame))
	{
		ActiveStartTime = GetWorld()->GetTimeSeconds();
		if (!bNotifyWindowThisMove) OpenAutoWindow(); // no AN_HitWindow_Open (yet): use the row's bone chain
	}
	if (AC::IsFirstRecoveryFrame(FD, StateFrame))
	{
		CloseAllWindows();
	}
	const AC::EPhase Phase = AC::PhaseAt(FD, StateFrame);
	bCancelWindowOpen = Phase == AC::EPhase::Recovery && bMoveConnected; // hit-confirm cancels only
	if (Phase == AC::EPhase::Done)
	{
		SetState(EACCombatState::Idle);
	}
}

void UACCombatComponent::SetState(EACCombatState NewState)
{
	if (NewState == State) return;
	const EACCombatState Old = State;
	State = NewState;
	StateFrame = 0;

	if (IsAttackState(Old) && !IsAttackState(NewState))
	{
		CloseAllWindows();
		bHasMove = false;
		bCancelWindowOpen = false;
		if (Old == EACCombatState::Grabbing) GrabbedVictim.Reset();
	}
	if (NewState == EACCombatState::Idle || NewState == EACCombatState::Locomotion || NewState == EACCombatState::Sprint)
	{
		ComboTaken.Reset(); // the combo against this fighter ends when they recover
	}
	OnCombatStateChanged.Broadcast(Old, State);
}

void UACCombatComponent::EnterTimedState(EACCombatState NewState, int32 Frames)
{
	if (State == NewState) StateFrame = 0; // re-hit while already staggered restarts the timer
	SetState(NewState);
	StateDurationFrames = FMath::Max(1, Frames);
}

void UACCombatComponent::OnRep_State(EACCombatState OldState)
{
	OnCombatStateChanged.Broadcast(OldState, State);
}

// ---- Montages --------------------------------------------------------------------------------------------------

void UACCombatComponent::MulticastPlayMove_Implementation(FName MoveId)
{
	PlayMoveLocal(MoveId);
}

void UACCombatComponent::PlayMoveLocal(FName MoveId)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* Anim = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (MoveId.IsNone())
	{
		if (Anim) Anim->Montage_Stop(0.1f);
		return;
	}

	FACMoveRow Row;
	UAnimMontage* Montage = nullptr;
	if (FindMove(MoveId, Row) && !Row.Montage.IsNull())
	{
		Montage = Row.Montage.LoadSynchronous(); // STUB: preload move montages on fighter load (KI-006)
	}
	else if (MoveTable)
	{
		if (const FACMoveRow* Reaction = MoveTable->FindRow<FACMoveRow>(MoveId, TEXT("ACCombat"), false))
		{
			Montage = Reaction->Montage.LoadSynchronous();
		}
	}

	if (!Montage || !Anim)
	{
		if (!WarnedMissingMontages.Contains(MoveId))
		{
			WarnedMissingMontages.Add(MoveId);
			UE_LOG(LogAfterclass, Log, TEXT("Combat: no montage for move '%s' on %s (or no AnimInstance); gameplay continues on frame data only."),
				*MoveId.ToString(), *GetNameSafe(GetOwner()));
		}
		return;
	}
	// Scale playback so the montage spans the move's frame data when it is a timed action.
	float Rate = 1.f;
	if (IsAttackState(Row.State) && Row.TotalFrames() > 0 && Montage->GetPlayLength() > 0.f)
	{
		Rate = Montage->GetPlayLength() / (static_cast<float>(Row.TotalFrames()) / AC::SimHz);
	}
	Anim->Montage_Play(Montage, Rate);
}

// ---- Hit windows -----------------------------------------------------------------------------------------------

void UACCombatComponent::OpenHitWindowFromNotify(const TArray<FName>& BoneChain, float Radius, int32 SwingIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !HitValidation || !bHasMove) return;
	bNotifyWindowThisMove = true;
	bAutoWindowOpen = false;
	const TArray<FName>& Bones = BoneChain.Num() > 0 ? BoneChain : CurrentMove.DefaultBoneChain;
	HitValidation->BeginWindow((SwingCounter << 4) | (SwingIndex & 0xF), Bones, Radius > 0.f ? Radius : CurrentMove.HitRadius);
}

void UACCombatComponent::CloseHitWindowFromNotify()
{
	if (HitValidation && GetOwner() && GetOwner()->HasAuthority()) HitValidation->EndWindow();
}

void UACCombatComponent::OpenAutoWindow()
{
	if (!HitValidation || CurrentMove.DefaultBoneChain.Num() == 0) return;
	bAutoWindowOpen = true;
	HitValidation->BeginWindow(SwingCounter << 4, CurrentMove.DefaultBoneChain, CurrentMove.HitRadius);
}

void UACCombatComponent::CloseAllWindows()
{
	bAutoWindowOpen = false;
	if (HitValidation && HitValidation->IsWindowOpen()) HitValidation->EndWindow();
}

void UACCombatComponent::HandleHitDetected(AActor* Victim, const FHitResult& Hit, int32 /*SwingId*/, FName Bone)
{
	UACCombatComponent* VictimCombat = CombatOf(Victim);
	if (!bHasMove || !VictimCombat || VictimCombat == this) return;

	FACHitData Data = CurrentMove.HitData;
	Data.InstigatorBone = Bone;
	const bool bWhiffPunish = VictimCombat->IsInWhiffRecovery();
	const EACHitOutcome Outcome = VictimCombat->ReceiveHit(GetOwner(), Data, Hit);

	switch (Outcome)
	{
	case EACHitOutcome::Hit:
		bMoveConnected = true;
		FramesSinceLandedHit = 0;
		AddMeter(AC::MeterForLandedHit(bWhiffPunish, bAnonymous));
		break;
	case EACHitOutcome::Grabbed:
		bMoveConnected = true;
		FramesSinceLandedHit = 0;
		GrabbedVictim = Victim;
		AddMeter(AC::MeterForLandedHit(false, bAnonymous));
		CloseAllWindows();
		break;
	case EACHitOutcome::Blocked:
	case EACHitOutcome::GuardBroken:
		bMoveConnected = true; // blocked hits still allow cancels (pressure), but give no meter
		break;
	default:
		break;
	}
}

// ---- Hit resolution (victim side) ------------------------------------------------------------------------------

EACHitOutcome UACCombatComponent::ReceiveHit(AActor* Attacker, const FACHitData& Data, const FHitResult& Hit)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !Attacker || State == EACCombatState::Dead) return EACHitOutcome::Ignored;
	UACCombatComponent* AttackerCombat = CombatOf(Attacker);
	const FVector Impact = Hit.bBlockingHit ? FVector(Hit.ImpactPoint) : Owner->GetActorLocation();
	const int32 Hitstop = Data.HitstopFrames > 0 ? Data.HitstopFrames : AC::DefaultHitstopFrames;

	// Dodge: invulnerable frames, perfect dodge when the dodge started <= 6f before the attack became active.
	if (State == EACCombatState::Dodging && AC::IsDodgeInvulnerable(StateFrame))
	{
		const double ActiveStart = AttackerCombat ? AttackerCombat->GetActiveStartTime() : -1.0;
		const int32 FramesUntilActive = ActiveStart >= 0.0 ? FMath::RoundToInt((ActiveStart - DodgeStartTime) * AC::SimHz) : -1;
		if (!bPerfectDodgeUsed && AC::IsPerfectDodge(FramesUntilActive))
		{
			bPerfectDodgeUsed = true;
			AddMeter(AC::MeterDelta(AC::MeterEvent::PerfectDodge, bAnonymous));
			CounterWindowFrames = AC::CounterWindow;
			BroadcastHit(Attacker, nullptr, EACHitOutcome::PerfectDodged, AC::PerfectDodgeHitstop, Impact); // attacker only
			return EACHitOutcome::PerfectDodged;
		}
		return EACHitOutcome::Dodged;
	}

	// Combo cap: a knocked-down victim at the cap cannot be extended (OTG protection).
	if (ComboTaken.AtCap() && (State == EACCombatState::Knockdown || State == EACCombatState::GetUp)) return EACHitOutcome::Ignored;

	// Grabs beat block; they are escapable for AC::GrabEscapeFrames.
	if (Data.HitType == EACHitType::Grab)
	{
		if (State == EACCombatState::Knockdown || State == EACCombatState::GetUp || State == EACCombatState::Grabbed) return EACHitOutcome::Ignored;
		CloseAllWindows();
		bHasMove = false;
		PendingGrabHit = Data;
		GrabbedBy = Attacker;
		SetState(EACCombatState::Grabbed);
		MulticastPlayMove(NAME_None);
		BroadcastHit(Attacker, Owner, EACHitOutcome::Grabbed, Hitstop, Impact);
		return EACHitOutcome::Grabbed;
	}

	if (State == EACCombatState::Blocking)
	{
		const AC::EDefense D = AC::ResolveBlock(StateFrame, AngleToAttacker(Owner, Attacker), Stamina, Data.bUnblockable, Data.bGuardBreak);
		switch (D)
		{
		case AC::EDefense::Parried:
			AddMeter(AC::MeterDelta(AC::MeterEvent::Parry, bAnonymous));
			CounterWindowFrames = AC::ParryCounter;
			if (AttackerCombat)
			{
				AttackerCombat->ApplyDamage(AC::ParryReflectDamage(Data.Damage));
				if (AttackerCombat->State != EACCombatState::Dead)
				{
					AttackerCombat->EnterTimedState(EACCombatState::Stagger, AC::ParryCounter);
					AttackerCombat->MulticastPlayMove(MoveHitReact);
				}
			}
			BroadcastHit(Attacker, Owner, EACHitOutcome::Parried, AC::DefaultHitstopFrames, Impact);
			return EACHitOutcome::Parried;
		case AC::EDefense::Blocked:
		{
			SpendStamina(AC::BlockStamina);
			FACHitData Push = Data;
			Push.Knockback *= 0.5f;
			Push.LaunchZ = 0.f;
			ApplyKnockback(Attacker, Push, false);
			BroadcastHit(Attacker, Owner, EACHitOutcome::Blocked, FMath::Max(1, Hitstop / 2), Impact);
			return EACHitOutcome::Blocked;
		}
		case AC::EDefense::GuardBroken:
			Stamina = FMath::Max(0.f, Stamina - AC::BlockStamina);
			FramesSinceStaminaUse = 0;
			EnterTimedState(EACCombatState::Stagger, AC::GuardBreakStagger);
			MulticastPlayMove(MoveHitReact);
			BroadcastHit(Attacker, Owner, EACHitOutcome::GuardBroken, Hitstop, Impact);
			return EACHitOutcome::GuardBroken;
		case AC::EDefense::Hit:
		default:
			break; // unblockable or outside the 120 deg cone
		}
	}

	// Clean hit.
	if (State == EACCombatState::Grabbed) { GrabbedBy.Reset(); }
	if (IsAttackState(State)) { CloseAllWindows(); bHasMove = false; } // trade: our attack is interrupted
	const float Scale = ComboTaken.RegisterHit(Data.ComboScalingIndex);
	ApplyDamage(Data.Damage * Scale);
	if (State == EACCombatState::Dead)
	{
		BroadcastHit(Attacker, Owner, EACHitOutcome::Hit, Hitstop, Impact);
		return EACHitOutcome::Hit;
	}

	const bool bGroundBounce = Data.bGroundBounce && ComboTaken.TryGroundBounce();
	ApplyKnockback(Attacker, Data, bGroundBounce);
	if (Data.LaunchZ > 0.f || bGroundBounce || ComboTaken.AtCap())
	{
		EnterTimedState(EACCombatState::Knockdown, AC::KnockdownFrames); // cap reached: the combo ends in a knockdown
		MulticastPlayMove(MoveKnockdown);
	}
	else
	{
		EnterTimedState(EACCombatState::Stagger, FMath::Max(1, Data.HitStun));
		MulticastPlayMove(MoveHitReact);
	}
	BroadcastHit(Attacker, Owner, EACHitOutcome::Hit, Hitstop, Impact);
	return EACHitOutcome::Hit;
}

void UACCombatComponent::ApplyKnockback(AActor* Attacker, const FACHitData& Data, bool bGroundBounce)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Attacker) return;
	FVector Dir = (Character->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal2D();
	if (Dir.IsNearlyZero()) Dir = -Character->GetActorForwardVector();

	FVector Velocity = Dir * Data.Knockback;
	Velocity.Z = bGroundBounce ? FMath::Max(Data.LaunchZ, 600.f) : Data.LaunchZ;

	// Wall bounce (once per combo): if a wall is right behind the victim, reflect the knockback off it.
	if (Data.bWallBounce && Data.Knockback > 0.f)
	{
		FHitResult Wall;
		const FVector Start = Character->GetActorLocation();
		const FVector End = Start + Dir * 150.f;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ACWallBounce), false, Character);
		Params.AddIgnoredActor(Attacker);
		if (GetWorld()->LineTraceSingleByChannel(Wall, Start, End, ECC_WorldStatic, Params) && ComboTaken.TryWallBounce())
		{
			Velocity = Velocity.MirrorByVector(Wall.ImpactNormal) * 0.6f;
			Velocity.Z = FMath::Max(Velocity.Z, 350.f);
		}
	}
	Character->LaunchCharacter(Velocity, true, Velocity.Z > 0.f);
}

void UACCombatComponent::TryGrabEscape()
{
	if (State != EACCombatState::Grabbed || !AC::IsGrabEscapeOpen(StateFrame)) return;
	UACCombatComponent* Grabber = CombatOf(GrabbedBy.Get());
	GrabbedBy.Reset();
	constexpr int32 EscapePushFrames = 10; // design placeholder
	EnterTimedState(EACCombatState::Stagger, EscapePushFrames);
	if (Grabber && Grabber->State == EACCombatState::Grabbing)
	{
		Grabber->EnterTimedState(EACCombatState::Stagger, EscapePushFrames);
	}
}

// ---- Vitals ----------------------------------------------------------------------------------------------------

void UACCombatComponent::ApplyDamage(float Amount)
{
	if (State == EACCombatState::Dead || Amount <= 0.f) return;
	Health = FMath::Max(0.f, Health - Amount);
	if (Health <= 0.f)
	{
		CloseAllWindows();
		bHasMove = false;
		SetState(EACCombatState::Dead);
		MulticastPlayMove(NAME_None); // STUB: death montage / ragdoll handoff (KI-016)
	}
}

void UACCombatComponent::AddMeter(float Delta)
{
	Meter = AC::ClampMeter(Meter + Delta);
}

void UACCombatComponent::SpendStamina(float Amount)
{
	if (Amount <= 0.f) return;
	Stamina = FMath::Max(0.f, Stamina - Amount);
	FramesSinceStaminaUse = 0;
}

// ---- Hit confirm / hitstop / hooks -----------------------------------------------------------------------------

void UACCombatComponent::BroadcastHit(AActor* Attacker, AActor* Victim, EACHitOutcome Outcome, int32 HitstopFrames, const FVector& ImpactPoint)
{
	MulticastHitConfirmed(Attacker, Victim, Outcome, HitstopFrames, ImpactPoint);
}

void UACCombatComponent::MulticastHitConfirmed_Implementation(AActor* Attacker, AActor* Victim, EACHitOutcome Outcome, int32 HitstopFrames, FVector_NetQuantize ImpactPoint)
{
	if (UWorld* World = GetWorld())
	{
		if (UACHitstopSubsystem* Hitstop = World->GetSubsystem<UACHitstopSubsystem>())
		{
			if (Victim) Hitstop->ApplyHitstop(Attacker, Victim, HitstopFrames);
			else Hitstop->ApplyHitstopToActor(Attacker, HitstopFrames); // perfect dodge: attacker only
		}
	}

	// Camera kick for the local player involved in a landed hit.
	if (HitCameraShake && (Outcome == EACHitOutcome::Hit || Outcome == EACHitOutcome::GuardBroken || Outcome == EACHitOutcome::Parried))
	{
		for (AActor* Involved : { Attacker, Victim })
		{
			const APawn* Pawn = Cast<APawn>(Involved);
			APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
			if (PC && PC->IsLocalController()) PC->ClientStartCameraShake(HitCameraShake);
		}
	}

	// STUB: VFX (Niagara impact by HitType) and VO (VoiceProfile cue) are bound to OnHitConfirmed by content (KI-007, KI-016).
	OnHitConfirmed.Broadcast(Attacker, Victim, Outcome, FVector(ImpactPoint));
}

// ---- Input routing ---------------------------------------------------------------------------------------------

void UACCombatComponent::RouteAction(EACCombatState Next, FName MoveId)
{
	const AActor* Owner = GetOwner();
	if (!Owner) return;
	if (Owner->HasAuthority()) TryStartAction(Next, MoveId);
	else ServerRequestAction(Next, MoveId);
}

void UACCombatComponent::ServerRequestAction_Implementation(EACCombatState Next, FName MoveId)
{
	if (MoveId == FName(TEXT("BlockRelease")))
	{
		if (State == EACCombatState::Blocking) { SetState(EACCombatState::Idle); }
		return;
	}
	// Clients may only request the built-in moves; the server resolves frame data and costs itself.
	if (IsAttackState(Next))
	{
		FACMoveRow Row;
		if (!FindMove(MoveId, Row)) return;
		Next = Row.State;
	}
	TryStartAction(Next, MoveId);
}

void UACCombatComponent::ServerRequestDodge_Implementation(FVector2D Dir)
{
	RequestDodge(Dir.GetClampedToMaxSize(1.f));
}

void UACCombatComponent::RequestLight()    { RouteAction(EACCombatState::LightAttack, MoveLight); }
void UACCombatComponent::RequestHeavy()    { RouteAction(EACCombatState::HeavyAttack, MoveHeavy); }
void UACCombatComponent::RequestGrab()     { RouteAction(EACCombatState::Grabbing, MoveGrab); }
void UACCombatComponent::RequestSpecial()  { RouteAction(EACCombatState::Special, MoveSpecial); }
void UACCombatComponent::RequestFinisher() { RouteAction(EACCombatState::Finisher, MoveFinisher); }

void UACCombatComponent::RequestDodge(FVector2D Dir)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;
	if (!Owner->HasAuthority())
	{
		ServerRequestDodge(Dir);
		return;
	}
	if (State == EACCombatState::Dead || State == EACCombatState::Grabbed || bActionLocked) return;
	const bool bLegal = IsInFreeState() || State == EACCombatState::Blocking || (bCancelWindowOpen && IsAttackState(State));
	if (!bLegal || Stamina < DodgeStaminaCost) return;
	bCancelWindowOpen = false;
	BeginDodge(Dir);
}

void UACCombatComponent::RequestBlock(bool bHeld)
{
	if (bHeld)
	{
		RouteAction(EACCombatState::Blocking, TEXT("Block"));
	}
	else if (State == EACCombatState::Blocking)
	{
		// Releasing block always returns to Idle, bypassing the free-state legality check.
		if (GetOwner() && GetOwner()->HasAuthority()) { SetState(EACCombatState::Idle); }
		else { ServerRequestAction(EACCombatState::Idle, TEXT("BlockRelease")); }
	}
}

void UACCombatComponent::SetSprinting(bool bSprinting)
{
	if (bSprinting && IsInFreeState())
	{
		RouteAction(EACCombatState::Sprint, TEXT("Sprint"));
	}
	else if (!bSprinting && State == EACCombatState::Sprint)
	{
		RouteAction(EACCombatState::Locomotion, TEXT("SprintEnd"));
	}
}
