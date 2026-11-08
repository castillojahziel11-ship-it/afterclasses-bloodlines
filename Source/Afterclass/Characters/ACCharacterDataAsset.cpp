#include "Characters/ACCharacterDataAsset.h"

const FPrimaryAssetType UACCharacterDataAsset::FighterAssetType(TEXT("Fighter"));

FPrimaryAssetId UACCharacterDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FighterAssetType, GetFName());
}
