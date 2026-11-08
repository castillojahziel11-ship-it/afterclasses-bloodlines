// Localized text access. All user-facing strings come from the CSV string tables in
// Content/Localization/StringTables/ (ST_UI, ST_Characters, ST_Combat), registered at module startup with
// LOCTABLE_FROMFILE_GAME so they exist before any asset that references them loads, in editor and packaged builds.
// Usage:  ACText::UI(TEXT("Menu.Story"))   ACText::FighterName(TEXT("Akari"))
// Build/check_loc.sh verifies every literal key passed to these helpers exists in the CSVs.
#pragma once

#include "CoreMinimal.h"

namespace ACText
{
	inline const FName UITable(TEXT("ST_UI"));
	inline const FName CharactersTable(TEXT("ST_Characters"));
	inline const FName CombatTable(TEXT("ST_Combat"));

	/** Registers the three CSV string tables (idempotent). Called from FAfterclassModule::StartupModule. */
	AFTERCLASS_API void RegisterStringTables();

	AFTERCLASS_API FText UI(const TCHAR* Key);
	AFTERCLASS_API FText Characters(const TCHAR* Key);
	AFTERCLASS_API FText Combat(const TCHAR* Key);
	/** FighterId = roster id used for assets (DA_Fighter_<Id>), e.g. "Akari", "EunChae", "DexMarrow". */
	AFTERCLASS_API FText FighterName(const FString& FighterId);
}
