// Blackboard key names (spec §8). Content/Python/generate_afterclass_assets.py creates BB_Fighter with exactly these keys.
#pragma once

#include "CoreMinimal.h"

namespace ACBB
{
	inline const FName TargetActor(TEXT("TargetActor"));             // Object (Actor)
	inline const FName TargetDist(TEXT("TargetDist"));               // Float, uu
	inline const FName TargetStamina(TEXT("TargetStamina"));         // Float, 0..1
	inline const FName TargetWeapon(TEXT("TargetWeapon"));           // Bool
	inline const FName TargetState(TEXT("TargetState"));             // Int (AC::Eval::TargetPhase)
	inline const FName LastPlayerAttack(TEXT("LastPlayerAttack"));   // Int (AC::Eval::PlayerAct)
	inline const FName AttackPatternBias(TEXT("AttackPatternBias")); // Float, -1..1 (+ = heavy-repeating, - = feinting)
	inline const FName OwnHealth(TEXT("OwnHealth"));                 // Float, 0..1
	inline const FName OwnStamina(TEXT("OwnStamina"));               // Float, 0..1
	inline const FName PreferredRange(TEXT("PreferredRange"));       // Float, uu
	inline const FName CoverPoint(TEXT("CoverPoint"));               // Vector
}
