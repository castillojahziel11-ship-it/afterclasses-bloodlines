#include "Characters/ACFighter.h"
#include "Characters/ACCharacterDataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

AACFighter::AACFighter()
{
}

void AACFighter::ApplyCharacterData()
{
	Super::ApplyCharacterData();
	if (CurrentOutfit >= 0)
	{
		ApplyOutfit(CurrentOutfit);
	}
}

bool AACFighter::ApplyOutfit(int32 Index)
{
	if (!CharacterData || !CharacterData->Outfits.IsValidIndex(Index))
	{
		return false;
	}
	USkeletalMesh* OutfitMesh = CharacterData->Outfits[Index].Mesh.LoadSynchronous(); // STUB: async load (KI-006)
	if (!OutfitMesh)
	{
		return false;
	}
	GetMesh()->SetSkeletalMeshAsset(OutfitMesh);
	CurrentOutfit = Index;
	return true;
}
