#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ACGameInstance.generated.h"

class UACSaveSubsystem;

UCLASS()
class AFTERCLASS_API UACGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintPure, Category="Afterclass")
	UACSaveSubsystem* GetSaveSubsystem() const;
};
