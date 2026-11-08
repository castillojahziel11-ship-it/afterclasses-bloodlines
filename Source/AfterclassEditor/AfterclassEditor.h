// Afterclass: Bloodline — editor-only module (asset validators, tooling). Intentionally minimal for now.
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAfterclassEditor, Log, All);

class FAfterclassEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
