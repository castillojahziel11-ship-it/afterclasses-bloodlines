#include "Combat/ACAnimNotifyState_HitWindow.h"
#include "Combat/ACCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	UACCombatComponent* FindCombat(const USkeletalMeshComponent* MeshComp)
	{
		const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
		return Owner ? Owner->FindComponentByClass<UACCombatComponent>() : nullptr;
	}
}

void UACAnimNotifyState_HitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	if (UACCombatComponent* Combat = FindCombat(MeshComp))
	{
		Combat->OpenHitWindowFromNotify(BoneChain, Radius, SwingIndex);
	}
}

void UACAnimNotifyState_HitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UACCombatComponent* Combat = FindCombat(MeshComp))
	{
		Combat->CloseHitWindowFromNotify();
	}
}

FString UACAnimNotifyState_HitWindow::GetNotifyName_Implementation() const
{
	return MoveId.IsNone() ? TEXT("AN_HitWindow_Open") : FString::Printf(TEXT("AN_HitWindow_Open (%s)"), *MoveId.ToString());
}
