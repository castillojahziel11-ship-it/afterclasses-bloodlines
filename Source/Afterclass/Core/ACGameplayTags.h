// Native gameplay tags used by C++. Designers may add more under the same roots in Config/DefaultGameplayTags.ini.
#pragma once

#include "NativeGameplayTags.h"

namespace ACTags
{
	/** Roster entry is available from the start. */
	AFTERCLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unlock_Default);
	/** Unlocked by completing Chapter 5 (locked decision: Dex Marrow is boss-only, then playable after Ch.5). */
	AFTERCLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unlock_Chapter5Complete);
	/** Never directly playable (boss-only). */
	AFTERCLASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unlock_BossOnly);
}
