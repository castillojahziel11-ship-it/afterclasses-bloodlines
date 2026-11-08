#include "UI/ACText.h"
#include "Afterclass.h"

#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"

namespace ACText
{
	void RegisterStringTables()
	{
		// Paths are relative to the project Content directory. Packaging stages them (DefaultGame.ini
		// DirectoriesToAlwaysStageAsUFS=Localization/StringTables); the text gatherer reads these macros too.
		if (!FStringTableRegistry::Get().FindStringTable(UITable).IsValid())
		{
			LOCTABLE_FROMFILE_GAME("ST_UI", "ST_UI", "Localization/StringTables/ST_UI.csv");
		}
		if (!FStringTableRegistry::Get().FindStringTable(CharactersTable).IsValid())
		{
			LOCTABLE_FROMFILE_GAME("ST_Characters", "ST_Characters", "Localization/StringTables/ST_Characters.csv");
		}
		if (!FStringTableRegistry::Get().FindStringTable(CombatTable).IsValid())
		{
			LOCTABLE_FROMFILE_GAME("ST_Combat", "ST_Combat", "Localization/StringTables/ST_Combat.csv");
		}
		for (const FName Id : { UITable, CharactersTable, CombatTable })
		{
			FStringTableConstPtr Table = FStringTableRegistry::Get().FindStringTable(Id);
			int32 Count = 0;
			if (Table.IsValid()) Table->EnumerateSourceStrings([&Count](const FString&, const FString&) { ++Count; return true; });
			if (Count == 0)
			{
				UE_LOG(LogAfterclass, Error, TEXT("String table %s is empty or failed to load (Content/Localization/StringTables)."), *Id.ToString());
			}
		}
	}

	FText UI(const TCHAR* Key)         { return FText::FromStringTable(UITable, Key); }
	FText Characters(const TCHAR* Key) { return FText::FromStringTable(CharactersTable, Key); }
	FText Combat(const TCHAR* Key)     { return FText::FromStringTable(CombatTable, Key); }

	FText FighterName(const FString& FighterId)
	{
		return FText::FromStringTable(CharactersTable, FString::Printf(TEXT("Fighter.%s.Name"), *FighterId));
	}
}
