#include "Core/ACGameInstance.h"
#include "Afterclass.h"
#include "Core/ACBuildInfo.h"
#include "Save/ACSaveSubsystem.h"

void UACGameInstance::Init()
{
	Super::Init();
	UE_LOG(LogAfterclass, Log, TEXT("Afterclass: Bloodline %s"), *UACBuildInfo::GetVersionString());
}

UACSaveSubsystem* UACGameInstance::GetSaveSubsystem() const
{
	return GetSubsystem<UACSaveSubsystem>();
}
