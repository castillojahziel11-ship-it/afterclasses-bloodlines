#include "ACEditorAssetLibrary.h"
#include "AfterclassEditor.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/Actor.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

bool UACEditorAssetLibrary::ImportStringTableFromCSV(UStringTable* Table, const FString& CsvFilePath)
{
	if (!Table)
	{
		UE_LOG(LogAfterclassEditor, Error, TEXT("ImportStringTableFromCSV: null table"));
		return false;
	}
	FString Path = CsvFilePath;
	if (FPaths::IsRelative(Path))
	{
		Path = FPaths::Combine(FPaths::ProjectDir(), Path);
	}
	if (!FPaths::FileExists(Path))
	{
		UE_LOG(LogAfterclassEditor, Error, TEXT("ImportStringTableFromCSV: file not found: %s"), *Path);
		return false;
	}

	Table->Modify();
	FStringTableRef Mutable = Table->GetMutableStringTable();
	Mutable->ClearSourceStrings();
	const bool bOk = Mutable->ImportStrings(Path);
	Table->MarkPackageDirty();
	UE_LOG(LogAfterclassEditor, Log, TEXT("ImportStringTableFromCSV: %s <- %s (%s)"), *Table->GetPathName(), *Path, bOk ? TEXT("ok") : TEXT("FAILED"));
	return bOk;
}

bool UACEditorAssetLibrary::SetBlackboardKey(UBlackboardData* Blackboard, FName KeyName, EACBlackboardKeyKind Kind, UClass* BaseClass)
{
	if (!Blackboard || KeyName.IsNone())
	{
		return false;
	}
	Blackboard->Modify();
	switch (Kind)
	{
	case EACBlackboardKeyKind::Float:
		Blackboard->UpdatePersistentKey<UBlackboardKeyType_Float>(KeyName);
		break;
	case EACBlackboardKeyKind::Int:
		Blackboard->UpdatePersistentKey<UBlackboardKeyType_Int>(KeyName);
		break;
	case EACBlackboardKeyKind::Bool:
		Blackboard->UpdatePersistentKey<UBlackboardKeyType_Bool>(KeyName);
		break;
	case EACBlackboardKeyKind::Vector:
		Blackboard->UpdatePersistentKey<UBlackboardKeyType_Vector>(KeyName);
		break;
	case EACBlackboardKeyKind::Object:
	{
		UBlackboardKeyType_Object* Key = Blackboard->UpdatePersistentKey<UBlackboardKeyType_Object>(KeyName);
		if (!Key)
		{
			// Key already existed with the right type; find it to update BaseClass.
			for (FBlackboardEntry& Entry : Blackboard->Keys)
			{
				if (Entry.EntryName == KeyName)
				{
					Key = Cast<UBlackboardKeyType_Object>(Entry.KeyType);
					break;
				}
			}
		}
		if (Key)
		{
			Key->BaseClass = BaseClass ? BaseClass : AActor::StaticClass();
		}
		break;
	}
	}
	Blackboard->MarkPackageDirty();
	return Blackboard->GetKeyID(KeyName) != FBlackboard::InvalidKey;
}

bool UACEditorAssetLibrary::SetBehaviorTreeBlackboard(UBehaviorTree* Tree, UBlackboardData* Blackboard)
{
	if (!Tree || !Blackboard)
	{
		return false;
	}
	Tree->Modify();
	Tree->BlackboardAsset = Blackboard;
	Tree->MarkPackageDirty();
	return true;
}

bool UACEditorAssetLibrary::SetTextPropertyFromStringTable(UObject* Object, FName PropertyName, FName TableId, const FString& Key)
{
	if (!Object)
	{
		return false;
	}
	FTextProperty* Prop = FindFProperty<FTextProperty>(Object->GetClass(), PropertyName);
	if (!Prop)
	{
		UE_LOG(LogAfterclassEditor, Error, TEXT("SetTextPropertyFromStringTable: %s has no FText property %s"), *Object->GetName(), *PropertyName.ToString());
		return false;
	}
	Object->Modify();
	Prop->SetPropertyValue_InContainer(Object, FText::FromStringTable(TableId, Key));
	Object->MarkPackageDirty();
	return true;
}
