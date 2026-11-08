// Frame-based hitstop: freezes the attacker and victim with local CustomTimeDilation for N 60 Hz frames of real time,
// and pauses their UACCombatComponent simulation for the same number of frames. Runs on every machine (clients get it
// via UACCombatComponent's hit-confirm multicast) so the freeze is visible everywhere.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ACHitstopSubsystem.generated.h"

UCLASS()
class AFTERCLASS_API UACHitstopSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Dilation used while frozen (0 would stall root motion / movement integration entirely). */
	static constexpr float FrozenDilation = 0.02f;

	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat")
	void ApplyHitstop(AActor* Attacker, AActor* Victim, int32 Frames);

	/** Single-actor variant (e.g. perfect dodge freezes only the attacker). */
	UFUNCTION(BlueprintCallable, Category="Afterclass|Combat")
	void ApplyHitstopToActor(AActor* Actor, int32 Frames);

	UFUNCTION(BlueprintPure, Category="Afterclass|Combat")
	bool IsFrozen(const AActor* Actor) const;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

private:
	struct FFreeze
	{
		float OriginalDilation = 1.f;
		double RemainingSeconds = 0.0;
	};
	TMap<TWeakObjectPtr<AActor>, FFreeze> Frozen;
};
