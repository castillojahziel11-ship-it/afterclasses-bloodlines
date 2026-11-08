// Build identity. Version comes from Config/DefaultGame.ini ProjectVersion; build number is injected by CI (AC_BUILD_NUMBER env var).
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ACBuildInfo.generated.h"

#ifndef AC_BUILD_NUMBER
#define AC_BUILD_NUMBER 0
#endif

UCLASS()
class AFTERCLASS_API UACBuildInfo : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Afterclass|Build")
	static int32 GetBuildNumber() { return AC_BUILD_NUMBER; }

	/** e.g. "0.1.0" */
	UFUNCTION(BlueprintPure, Category="Afterclass|Build")
	static FString GetProjectVersion();

	/** e.g. "0.1.0 (build 0001, Development)" — debug/log string, not shipping UI text. */
	UFUNCTION(BlueprintPure, Category="Afterclass|Build")
	static FString GetVersionString();

	/** Culture-invariant text for the version watermark (raw-string FText construction is banned in shipping UI). */
	UFUNCTION(BlueprintPure, Category="Afterclass|Build")
	static FText GetVersionText();
};
