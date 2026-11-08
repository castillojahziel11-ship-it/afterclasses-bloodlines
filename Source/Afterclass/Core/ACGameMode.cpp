#include "Core/ACGameMode.h"
#include "Characters/ACFighter.h"
#include "Core/ACPlayerController.h"

AACGameMode::AACGameMode()
{
	// Override DefaultPawnClass in a Blueprint child (e.g. BP_GM_Dev -> BP_Fighter_Placeholder) to get a visible mesh.
	DefaultPawnClass = AACFighter::StaticClass();
	PlayerControllerClass = AACPlayerController::StaticClass();
}
