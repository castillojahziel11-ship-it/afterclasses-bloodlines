#include "Input/ACInputSettings.h"
#include "InputAction.h"
#include "InputMappingContext.h"

namespace
{
	template <typename T>
	TSoftObjectPtr<T> AssetAt(const TCHAR* Path)
	{
		return TSoftObjectPtr<T>(FSoftObjectPath(Path));
	}
}

UACInputSettings::UACInputSettings()
{
	DefaultMappingContext = AssetAt<UInputMappingContext>(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	MoveAction    = AssetAt<UInputAction>(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	LookAction    = AssetAt<UInputAction>(TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	LightAction   = AssetAt<UInputAction>(TEXT("/Game/Input/Actions/IA_Light.IA_Light"));
	HeavyAction   = AssetAt<UInputAction>(TEXT("/Game/Input/Actions/IA_Heavy.IA_Heavy"));
	DodgeAction   = AssetAt<UInputAction>(TEXT("/Game/Input/Actions/IA_Dodge.IA_Dodge"));
	BlockAction   = AssetAt<UInputAction>(TEXT("/Game/Input/Actions/IA_Block.IA_Block"));
	GrabAction    = AssetAt<UInputAction>(TEXT("/Game/Input/Actions/IA_Grab.IA_Grab"));
	SpecialAction = AssetAt<UInputAction>(TEXT("/Game/Input/Actions/IA_Special.IA_Special"));
	SprintAction  = AssetAt<UInputAction>(TEXT("/Game/Input/Actions/IA_Sprint.IA_Sprint"));
}
