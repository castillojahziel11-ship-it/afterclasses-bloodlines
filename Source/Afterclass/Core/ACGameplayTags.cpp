#include "Core/ACGameplayTags.h"

namespace ACTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unlock_Default, "Unlock.Default", "Available from the start.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unlock_Chapter5Complete, "Unlock.Chapter5Complete", "Unlocked after completing Chapter 5.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unlock_SeveredEnding, "Unlock.SeveredEnding", "Unlocked by reaching the SEVERED ending (YN).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unlock_BossOnly, "Unlock.BossOnly", "Boss-only, not selectable.");
}
