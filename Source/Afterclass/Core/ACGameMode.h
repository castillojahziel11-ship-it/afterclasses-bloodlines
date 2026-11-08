// Default game mode: AACFighter pawn + AACPlayerController. Per-map/mode subclasses (Story, Versus, Arena) come later.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ACGameMode.generated.h"

UCLASS()
class AFTERCLASS_API AACGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AACGameMode();
};
