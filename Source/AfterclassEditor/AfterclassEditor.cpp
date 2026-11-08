#include "AfterclassEditor.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogAfterclassEditor);

void FAfterclassEditorModule::StartupModule()
{
	UE_LOG(LogAfterclassEditor, Log, TEXT("AfterclassEditor module started."));
}

void FAfterclassEditorModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FAfterclassEditorModule, AfterclassEditor);
