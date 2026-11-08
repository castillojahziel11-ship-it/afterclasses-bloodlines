#include "Afterclass.h"
#include "Modules/ModuleManager.h"
#include "UI/ACText.h"

DEFINE_LOG_CATEGORY(LogAfterclass);

/** Primary game module: registers CSV string tables before any asset referencing them is loaded. */
class FAfterclassModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		ACText::RegisterStringTables();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FAfterclassModule, Afterclass, "Afterclass");
