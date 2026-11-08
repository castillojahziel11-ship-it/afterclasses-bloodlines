// Server-side melee hit validation (spec §7): while a hit window is open, sweeps a sphere from each bone's previous to
// current position every tick, dedupes victims per swing ID, and resolves the hit component to a hurt capsule.
// Hurt capsules: any UCapsuleComponent tagged "Hurt" on the victim (object type Pawn); if a victim has none, its root
// character capsule counts. Runs in TG_PostPhysics so bone transforms for this frame are final.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACHitValidation.generated.h"

class USkeletalMeshComponent;
class UPrimitiveComponent;

DECLARE_DELEGATE_FourParams(FACOnHitDetected, AActor* /*Victim*/, const FHitResult& /*Hit*/, int32 /*SwingId*/, FName /*Bone*/);

UCLASS(ClassGroup=(Afterclass), meta=(BlueprintSpawnableComponent))
class AFTERCLASS_API UACHitValidation : public UActorComponent
{
	GENERATED_BODY()

public:
	UACHitValidation();

	static const FName HurtTag;

	/** Opens (or re-targets) the window. Server only; no-op on clients. */
	void BeginWindow(int32 SwingId, const TArray<FName>& BoneChain, float Radius);
	void EndWindow();
	bool IsWindowOpen() const { return bWindowOpen; }

	/** Bound by UACCombatComponent. */
	FACOnHitDetected OnHitDetected;

	/** Draw sweeps in PIE (console: ac.DebugHitSweeps 1). */
	static bool IsDebugDrawEnabled();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Resolves a hit component to a valid hurt capsule owner, or nullptr. Public for automation tests. */
	static AActor* ResolveHurtActor(const UPrimitiveComponent* HitComp, const AActor* Instigator);

private:
	USkeletalMeshComponent* GetMesh() const;
	void SampleBones(TArray<FVector>& Out, TArray<bool>& OutValid) const;
	void Sweep();

	bool bWindowOpen = false;
	int32 CurrentSwingId = INDEX_NONE;
	float SweepRadius = 18.f;
	TArray<FName> Bones;
	TArray<FVector> PrevPositions;
	TArray<bool> PrevValid;
	/** Victims already hit by the current swing. */
	TSet<TWeakObjectPtr<AActor>> HitThisSwing;
	/** Last swing ID that hit each victim; guards notify re-opens of the same swing. */
	TMap<TWeakObjectPtr<AActor>, int32> LastSwingByVictim;
	TSet<FName> WarnedMissingBones;
};
