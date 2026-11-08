#include "Combat/ACHitstopSubsystem.h"
#include "Combat/ACCombatComponent.h"
#include "Combat/ACFrameCore.h"
#include "GameFramework/Actor.h"

void UACHitstopSubsystem::ApplyHitstop(AActor* Attacker, AActor* Victim, int32 Frames)
{
	ApplyHitstopToActor(Attacker, Frames);
	if (Victim != Attacker)
	{
		ApplyHitstopToActor(Victim, Frames);
	}
}

void UACHitstopSubsystem::ApplyHitstopToActor(AActor* Actor, int32 Frames)
{
	if (!IsValid(Actor) || Frames <= 0) return;

	const double Seconds = static_cast<double>(Frames) / AC::SimHz;
	FFreeze* Existing = Frozen.Find(Actor);
	if (Existing)
	{
		Existing->RemainingSeconds = FMath::Max(Existing->RemainingSeconds, Seconds); // overlapping hitstops don't stack
	}
	else
	{
		FFreeze& F = Frozen.Add(Actor);
		F.OriginalDilation = Actor->CustomTimeDilation;
		F.RemainingSeconds = Seconds;
		Actor->CustomTimeDilation = FrozenDilation;
	}

	if (UACCombatComponent* Combat = Actor->FindComponentByClass<UACCombatComponent>())
	{
		Combat->AddHitstopFrames(Frames);
	}
}

bool UACHitstopSubsystem::IsFrozen(const AActor* Actor) const
{
	return Actor && Frozen.Contains(TWeakObjectPtr<AActor>(const_cast<AActor*>(Actor)));
}

void UACHitstopSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	for (auto It = Frozen.CreateIterator(); It; ++It)
	{
		AActor* Actor = It.Key().Get();
		if (!Actor)
		{
			It.RemoveCurrent();
			continue;
		}
		It.Value().RemainingSeconds -= DeltaTime; // world delta: unaffected by the actor's own dilation
		if (It.Value().RemainingSeconds <= 0.0)
		{
			Actor->CustomTimeDilation = It.Value().OriginalDilation;
			It.RemoveCurrent();
		}
	}
}

TStatId UACHitstopSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UACHitstopSubsystem, STATGROUP_Tickables);
}

void UACHitstopSubsystem::Deinitialize()
{
	for (auto& Pair : Frozen)
	{
		if (AActor* Actor = Pair.Key.Get())
		{
			Actor->CustomTimeDilation = Pair.Value.OriginalDilation;
		}
	}
	Frozen.Reset();
	Super::Deinitialize();
}
