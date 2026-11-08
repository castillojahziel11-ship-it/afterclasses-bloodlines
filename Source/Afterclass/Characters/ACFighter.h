// Concrete fighter class. EVERY roster fighter (incl. Dex Marrow) is an AACFighter Blueprint child configured purely by data asset.
#pragma once

#include "CoreMinimal.h"
#include "Characters/ACCharacterBase.h"
#include "ACFighter.generated.h"

UCLASS(Blueprintable)
class AFTERCLASS_API AACFighter : public AACCharacterBase
{
	GENERATED_BODY()

public:
	AACFighter();

	/** Swaps to CharacterData->Outfits[Index].Mesh. Returns false if index/mesh invalid. Unlock checks happen in UI/save layer. */
	UFUNCTION(BlueprintCallable, Category="Afterclass|Fighter")
	bool ApplyOutfit(int32 Index);

	UFUNCTION(BlueprintPure, Category="Afterclass|Fighter")
	int32 GetCurrentOutfit() const { return CurrentOutfit; }

	virtual void ApplyCharacterData() override;

protected:
	/** -1 = base BodyMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Afterclass|Fighter")
	int32 CurrentOutfit = -1;
};
