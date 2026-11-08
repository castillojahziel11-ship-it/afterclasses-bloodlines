#include "AI/ACBossPhaseComponent.h"
#include "Afterclass.h"
#include "Combat/ACCombatComponent.h"
#include "Combat/ACFrameCore.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

UACBossPhaseComponent::UACBossPhaseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	UnblockableMove.MoveId = TEXT("BossUnblockable");
	UnblockableMove.State = EACCombatState::HeavyAttack;
	UnblockableMove.StartupFrames = AC::Heavy.Startup;
	UnblockableMove.ActiveFrames = AC::Heavy.Active;
	UnblockableMove.RecoveryFrames = AC::Heavy.Recovery + 6; // extra recovery = punish window after the big hit
	UnblockableMove.HitData.Damage = 120.f;
	UnblockableMove.HitData.HitStun = 30;
	UnblockableMove.HitData.Knockback = 600.f;
	UnblockableMove.HitData.LaunchZ = 350.f;
	UnblockableMove.HitData.HitType = EACHitType::Heavy;
	UnblockableMove.HitData.bUnblockable = true;
	UnblockableMove.HitData.HitstopFrames = 10;
	UnblockableMove.DefaultBoneChain = { TEXT("hand_r"), TEXT("lowerarm_r") };
	UnblockableMove.HitRadius = 30.f;
}

UACCombatComponent* UACBossPhaseComponent::GetCombat() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UACCombatComponent>() : nullptr;
}

void UACBossPhaseComponent::BeginPlay()
{
	Super::BeginPlay();
	Tracker = AC::Eval::BossPhaseTracker{};
}

void UACBossPhaseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	UACCombatComponent* Combat = GetCombat();
	if (!Combat || Combat->GetState() == EACCombatState::Dead) return;

	if (Tracker.Update(Combat->Health / FMath::Max(1.f, Combat->MaxHealth)))
	{
		UE_LOG(LogAfterclass, Log, TEXT("Boss %s entered phase %d"), *GetNameSafe(GetOwner()), Tracker.Phase + 1);
		OnPhaseChanged.Broadcast(Tracker.Phase);
	}

	if (TelegraphEndTime >= 0.0 && GetWorld()->GetTimeSeconds() >= TelegraphEndTime)
	{
		// Telegraph finished: start the attack as soon as the boss is free (it may be mid-reaction).
		if (Combat->StartCustomMove(UnblockableMove))
		{
			TelegraphEndTime = -1.0;
		}
	}
}

bool UACBossPhaseComponent::TryTelegraphUnblockable()
{
	UACCombatComponent* Combat = GetCombat();
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Combat || !Combat->IsInFreeState() || TelegraphEndTime >= 0.0) return false;
	if (!Tracker.ConsumeUnblockable()) return false;
	TelegraphEndTime = GetWorld()->GetTimeSeconds() + TelegraphSeconds;
	MulticastTelegraph(Tracker.Phase, TelegraphSeconds);
	return true;
}

void UACBossPhaseComponent::MulticastTelegraph_Implementation(int32 Phase, float Seconds)
{
	// STUB: telegraph flash VFX / VO line / audio sting are content bound to OnTelegraph (KI-016).
	OnTelegraph.Broadcast(Phase, Seconds);
}
