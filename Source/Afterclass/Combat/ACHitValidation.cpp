#include "Combat/ACHitValidation.h"
#include "Afterclass.h"

#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"

const FName UACHitValidation::HurtTag(TEXT("Hurt"));

static TAutoConsoleVariable<int32> CVarDebugHitSweeps(TEXT("ac.DebugHitSweeps"), 0, TEXT("Draw melee hit-validation sweeps (1 = on)."));

bool UACHitValidation::IsDebugDrawEnabled()
{
	return CVarDebugHitSweeps.GetValueOnGameThread() != 0;
}

UACHitValidation::UACHitValidation()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	SetIsReplicatedByDefault(false);
}

USkeletalMeshComponent* UACHitValidation::GetMesh() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	return Character ? Character->GetMesh() : (GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr);
}

void UACHitValidation::BeginWindow(int32 SwingId, const TArray<FName>& BoneChain, float Radius)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	if (SwingId != CurrentSwingId)
	{
		HitThisSwing.Reset();
	}
	CurrentSwingId = SwingId;
	Bones = BoneChain;
	SweepRadius = FMath::Max(1.f, Radius);
	SampleBones(PrevPositions, PrevValid);
	bWindowOpen = Bones.Num() > 0;
	SetComponentTickEnabled(bWindowOpen);
	if (!bWindowOpen)
	{
		UE_LOG(LogAfterclass, Warning, TEXT("HitValidation: window opened with empty bone chain on %s"), *GetNameSafe(Owner));
	}
}

void UACHitValidation::EndWindow()
{
	if (bWindowOpen)
	{
		Sweep(); // final sweep so a fast swing ending this frame is not lost
	}
	bWindowOpen = false;
	SetComponentTickEnabled(false);
}

void UACHitValidation::SampleBones(TArray<FVector>& Out, TArray<bool>& OutValid) const
{
	Out.SetNum(Bones.Num());
	OutValid.SetNum(Bones.Num());
	const USkeletalMeshComponent* Mesh = GetMesh();
	for (int32 i = 0; i < Bones.Num(); ++i)
	{
		const bool bValid = Mesh && Mesh->DoesSocketExist(Bones[i]);
		OutValid[i] = bValid;
		Out[i] = bValid ? Mesh->GetSocketLocation(Bones[i]) : FVector::ZeroVector;
	}
}

void UACHitValidation::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bWindowOpen)
	{
		Sweep();
	}
}

AActor* UACHitValidation::ResolveHurtActor(const UPrimitiveComponent* HitComp, const AActor* Instigator)
{
	if (!HitComp) return nullptr;
	AActor* Victim = HitComp->GetOwner();
	if (!Victim || Victim == Instigator) return nullptr;

	if (HitComp->ComponentHasTag(HurtTag)) return Victim;

	// No explicit hurt capsules on the victim -> the character capsule is the hurt volume.
	TArray<UCapsuleComponent*> Capsules;
	Victim->GetComponents<UCapsuleComponent>(Capsules);
	const bool bHasHurtCapsules = Capsules.ContainsByPredicate([](const UCapsuleComponent* C) { return C && C->ComponentHasTag(HurtTag); });
	if (bHasHurtCapsules) return nullptr;

	const ACharacter* Character = Cast<ACharacter>(Victim);
	return (Character && HitComp == Character->GetCapsuleComponent()) ? Victim : nullptr;
}

void UACHitValidation::Sweep()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner) return;

	TArray<FVector> Cur;
	TArray<bool> CurValid;
	SampleBones(Cur, CurValid);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ACHitValidation), /*bTraceComplex=*/false, Owner);
	const FCollisionObjectQueryParams ObjectParams(ECC_Pawn);
	const FCollisionShape Shape = FCollisionShape::MakeSphere(SweepRadius);
	const bool bDebug = IsDebugDrawEnabled();

	for (int32 i = 0; i < Bones.Num(); ++i)
	{
		if (!CurValid[i])
		{
			if (!WarnedMissingBones.Contains(Bones[i]))
			{
				WarnedMissingBones.Add(Bones[i]);
				UE_LOG(LogAfterclass, Warning, TEXT("HitValidation: bone/socket '%s' not found on %s; skipped."), *Bones[i].ToString(), *GetNameSafe(Owner));
			}
			continue;
		}
		const FVector Start = PrevValid.IsValidIndex(i) && PrevValid[i] ? PrevPositions[i] : Cur[i];
		const FVector End = Cur[i];

		TArray<FHitResult> Hits;
		World->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjectParams, Shape, Params);
		if (bDebug)
		{
			DrawDebugCapsule(World, (Start + End) * 0.5f, FMath::Max(SweepRadius, (End - Start).Size() * 0.5f), SweepRadius,
				FRotationMatrix::MakeFromZ(End - Start).ToQuat(), Hits.Num() ? FColor::Red : FColor::Green, false, 0.5f);
		}

		for (const FHitResult& Hit : Hits)
		{
			AActor* Victim = ResolveHurtActor(Hit.GetComponent(), Owner);
			if (!Victim) continue;

			const TWeakObjectPtr<AActor> Key(Victim);
			if (HitThisSwing.Contains(Key)) continue;
			if (const int32* Last = LastSwingByVictim.Find(Key); Last && *Last == CurrentSwingId) continue;

			HitThisSwing.Add(Key);
			LastSwingByVictim.Add(Key, CurrentSwingId);
			OnHitDetected.ExecuteIfBound(Victim, Hit, CurrentSwingId, Bones[i]);
		}
	}

	PrevPositions = MoveTemp(Cur);
	PrevValid = MoveTemp(CurValid);
}
