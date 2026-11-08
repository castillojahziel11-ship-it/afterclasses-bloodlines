// AN_HitWindow_Open: place on an attack montage over the active frames. While open, the owner's UACHitValidation sweeps
// BoneChain from previous to current bone positions every tick (server only). If a move has no montage, the combat
// component opens an equivalent window from the move row's DefaultBoneChain during its active frames instead.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ACAnimNotifyState_HitWindow.generated.h"

UCLASS(meta=(DisplayName="AN_HitWindow_Open"))
class AFTERCLASS_API UACAnimNotifyState_HitWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** Bones or sockets swept as spheres (e.g. hand_r, lowerarm_r). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit Window") TArray<FName> BoneChain;
	/** Move table row this window belongs to (informational; the active move supplies FACHitData). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit Window") FName MoveId;
	/** Distinguishes multiple hits inside one move (0..15). Same index = same swing = one hit per victim. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit Window", meta=(ClampMin="0", ClampMax="15")) int32 SwingIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit Window", meta=(ClampMin="1", Units="cm")) float Radius = 18.f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
