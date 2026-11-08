#include "Core/ACBuildInfo.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/App.h"

FString UACBuildInfo::GetProjectVersion()
{
	FString Version;
	if (!GConfig || !GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), Version, GGameIni))
	{
		Version = TEXT("0.0.0");
	}
	return Version;
}

FString UACBuildInfo::GetVersionString()
{
	return FString::Printf(TEXT("%s (build %04d, %s)"), *GetProjectVersion(), GetBuildNumber(), LexToString(FApp::GetBuildConfiguration()));
}

FText UACBuildInfo::GetVersionText()
{
	return FText::AsCultureInvariant(GetVersionString());
}
