// Hit payload types shared by move tables, hit validation and the combat component (spec §7 FACHitData).
#pragma once

#include "CoreMinimal.h"
#include "ACHitTypes.generated.h"

UENUM(BlueprintType)
enum class EACHitType : uint8
{
	Light, Heavy, Launcher, Sweep, Grab, Special, Finisher, Projectile
};

/** Result of resolving one hit against a defender. */
UENUM(BlueprintType)
enum class EACHitOutcome : uint8
{
	Ignored, Hit, Blocked, Parried, GuardBroken, Dodged, PerfectDodged, Grabbed
};

USTRUCT(BlueprintType)
struct AFTERCLASS_API FACHitData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit", meta=(ClampMin="0")) float Damage = 30.f;
	/** Frames of stagger on hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit", meta=(ClampMin="0")) int32 HitStun = 14;
	/** Horizontal knockback speed (cm/s) applied away from the attacker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit", meta=(ClampMin="0")) float Knockback = 250.f;
	/** Vertical launch speed (cm/s). > 0 puts the victim into knockdown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit", meta=(ClampMin="0")) float LaunchZ = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit") EACHitType HitType = EACHitType::Light;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit") bool bUnblockable = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit") bool bGuardBreak = false;
	/** Requests a wall bounce (granted once per combo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit") bool bWallBounce = false;
	/** Requests a ground bounce (granted once per combo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit") bool bGroundBounce = false;
	/** Extra offset into the combo scaling table for this move. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit", meta=(ClampMin="0")) int32 ComboScalingIndex = 0;
	/** Hitstop applied to BOTH attacker and victim, in 60 Hz frames. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit", meta=(ClampMin="0")) int32 HitstopFrames = 4;
	/** Bone/socket that delivered the hit (filled by hit validation). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit") FName InstigatorBone;
};
