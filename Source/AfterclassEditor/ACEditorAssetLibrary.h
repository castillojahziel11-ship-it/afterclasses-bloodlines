// Editor-only helpers exposed to Python (unreal.ACEditorAssetLibrary) for Content/Python/generate_afterclass_assets.py.
// They wrap engine APIs that are not reachable from Python directly (string table CSV import, blackboard key creation,
// protected/non-editable properties such as UBehaviorTree::BlackboardAsset, FText-from-string-table assignment).
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ACEditorAssetLibrary.generated.h"

class UStringTable;
class UBlackboardData;
class UBehaviorTree;

UENUM(BlueprintType)
enum class EACBlackboardKeyKind : uint8
{
	Float,
	Int,
	Bool,
	Vector,
	Object
};

UCLASS()
class AFTERCLASSEDITOR_API UACEditorAssetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Replaces all source strings of Table with the rows of a Key,SourceString[,Comment] CSV (absolute or project-relative path). */
	UFUNCTION(BlueprintCallable, Category="Afterclass|Editor")
	static bool ImportStringTableFromCSV(UStringTable* Table, const FString& CsvFilePath);

	/** Adds or updates a persistent blackboard key. BaseClass is only used for Object keys (defaults to AActor). */
	UFUNCTION(BlueprintCallable, Category="Afterclass|Editor")
	static bool SetBlackboardKey(UBlackboardData* Blackboard, FName KeyName, EACBlackboardKeyKind Kind, UClass* BaseClass);

	/** UBehaviorTree::BlackboardAsset is not editable from Python; set it here. */
	UFUNCTION(BlueprintCallable, Category="Afterclass|Editor")
	static bool SetBehaviorTreeBlackboard(UBehaviorTree* Tree, UBlackboardData* Blackboard);

	/** Sets an FText property on Object to FText::FromStringTable(TableId, Key) (string-table reference, never a literal). */
	UFUNCTION(BlueprintCallable, Category="Afterclass|Editor")
	static bool SetTextPropertyFromStringTable(UObject* Object, FName PropertyName, FName TableId, const FString& Key);
};
