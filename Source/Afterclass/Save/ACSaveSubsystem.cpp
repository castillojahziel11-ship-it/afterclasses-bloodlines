#include "Save/ACSaveSubsystem.h"
#include "Afterclass.h"
#include "Save/ACSaveGames.h"

#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

// ---------------------------------------------------------------------------------------------------------------------
// Paths

FString UACSaveSubsystem::GetSaveDir()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("Afterclass"));
}

FString UACSaveSubsystem::GetQuarantineDir()
{
	return FPaths::Combine(GetSaveDir(), TEXT("Quarantine"));
}

FString UACSaveSubsystem::GetSlotName(EACSaveSlot Slot)
{
	switch (Slot)
	{
	case EACSaveSlot::Auto:    return TEXT("Auto");
	case EACSaveSlot::Manual1: return TEXT("Manual_1");
	case EACSaveSlot::Manual2: return TEXT("Manual_2");
	case EACSaveSlot::Manual3: return TEXT("Manual_3");
	case EACSaveSlot::Manual4: return TEXT("Manual_4");
	case EACSaveSlot::Manual5: return TEXT("Manual_5");
	case EACSaveSlot::Profile: return TEXT("Profile");
	default:                   return TEXT("Invalid");
	}
}

FString UACSaveSubsystem::GetSlotPath(EACSaveSlot Slot)
{
	return FPaths::Combine(GetSaveDir(), GetSlotName(Slot) + TEXT(".acsav"));
}

// ---------------------------------------------------------------------------------------------------------------------
// Container: [uint32 magic][int32 containerVersion][int32 payloadSize][uint32 crc32(payload)][payload...]

bool UACSaveSubsystem::WrapPayload(const TArray<uint8>& Payload, TArray<uint8>& OutFile)
{
	if (Payload.Num() <= 0) return false;

	uint32 Magic = FileMagic;
	int32 Version = ContainerVersion;
	int32 Size = Payload.Num();
	uint32 Crc = FCrc::MemCrc32(Payload.GetData(), Payload.Num());

	OutFile.Reset();
	FMemoryWriter Writer(OutFile, /*bIsPersistent=*/true);
	Writer << Magic << Version << Size << Crc;
	Writer.Serialize(const_cast<uint8*>(Payload.GetData()), Payload.Num());
	return !Writer.IsError();
}

bool UACSaveSubsystem::UnwrapPayload(const TArray<uint8>& File, TArray<uint8>& OutPayload)
{
	constexpr int32 HeaderSize = sizeof(uint32) + sizeof(int32) + sizeof(int32) + sizeof(uint32);
	if (File.Num() < HeaderSize) return false;

	FMemoryReader Reader(File, /*bIsPersistent=*/true);
	uint32 Magic = 0, Crc = 0;
	int32 Version = 0, Size = 0;
	Reader << Magic << Version << Size << Crc;

	if (Reader.IsError() || Magic != FileMagic) return false;
	if (Version <= 0 || Version > ContainerVersion) return false;
	if (Size <= 0 || Size != File.Num() - HeaderSize) return false;

	OutPayload.SetNumUninitialized(Size);
	Reader.Serialize(OutPayload.GetData(), Size);
	if (Reader.IsError()) return false;

	return FCrc::MemCrc32(OutPayload.GetData(), OutPayload.Num()) == Crc;
}

// ---------------------------------------------------------------------------------------------------------------------
// Lifecycle

void UACSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IFileManager::Get().MakeDirectory(*GetSaveDir(), /*Tree=*/true);

	EACSaveResult Result = EACSaveResult::NotFound;
	Profile = Cast<UACProfileSave>(ReadSlot(EACSaveSlot::Profile, Result));
	if (!Profile)
	{
		UE_LOG(LogAfterclass, Log, TEXT("Profile not loaded (result %d); creating defaults."), static_cast<int32>(Result));
		Profile = Cast<UACProfileSave>(UGameplayStatics::CreateSaveGameObject(UACProfileSave::StaticClass()));
		if (Result != EACSaveResult::VersionTooNew)
		{
			SaveProfile(); // never overwrite a file written by a newer build
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Public API

EACSaveResult UACSaveSubsystem::SaveCampaign(EACSaveSlot Slot, UACCampaignSave* Data)
{
	if (Slot == EACSaveSlot::Profile || !Data) return EACSaveResult::WrongType;
	Data->SaveVersion = UACCampaignSave::CAMPAIGN_VERSION;
	Data->SavedAtUtc = FDateTime::UtcNow();
	return WriteSlot(Slot, Data);
}

UACCampaignSave* UACSaveSubsystem::LoadCampaign(EACSaveSlot Slot, EACSaveResult& OutResult)
{
	if (Slot == EACSaveSlot::Profile) { OutResult = EACSaveResult::WrongType; return nullptr; }
	USaveGame* Loaded = ReadSlot(Slot, OutResult);
	UACCampaignSave* Campaign = Cast<UACCampaignSave>(Loaded);
	if (Loaded && !Campaign) { OutResult = EACSaveResult::WrongType; }
	return Campaign;
}

EACSaveResult UACSaveSubsystem::SaveProfile()
{
	if (!Profile) return EACSaveResult::SerializeFailed;
	Profile->SaveVersion = UACProfileSave::PROFILE_VERSION;
	return WriteSlot(EACSaveSlot::Profile, Profile);
}

bool UACSaveSubsystem::DoesSlotExist(EACSaveSlot Slot) const
{
	return IFileManager::Get().FileExists(*GetSlotPath(Slot));
}

bool UACSaveSubsystem::DeleteSlot(EACSaveSlot Slot)
{
	return IFileManager::Get().Delete(*GetSlotPath(Slot), /*RequireExists=*/false, /*EvenReadOnly=*/true);
}

// ---------------------------------------------------------------------------------------------------------------------
// IO

EACSaveResult UACSaveSubsystem::WriteSlot(EACSaveSlot Slot, USaveGame* Data)
{
	TArray<uint8> Payload;
	if (!UGameplayStatics::SaveGameToMemory(Data, Payload)) return EACSaveResult::SerializeFailed;

	TArray<uint8> FileBytes;
	if (!WrapPayload(Payload, FileBytes)) return EACSaveResult::SerializeFailed;

	const FString FinalPath = GetSlotPath(Slot);
	const FString TempPath = FinalPath + TEXT(".tmp");

	IFileManager& FM = IFileManager::Get();
	FM.MakeDirectory(*GetSaveDir(), true);

	if (!FFileHelper::SaveArrayToFile(FileBytes, *TempPath))
	{
		UE_LOG(LogAfterclass, Error, TEXT("Save: failed to write temp file %s"), *TempPath);
		FM.Delete(*TempPath, false, true);
		return EACSaveResult::IOError;
	}

	// Verify what actually hit the disk before replacing the previous good save.
	TArray<uint8> Verify, VerifyPayload;
	if (!FFileHelper::LoadFileToArray(Verify, *TempPath) || !UnwrapPayload(Verify, VerifyPayload))
	{
		UE_LOG(LogAfterclass, Error, TEXT("Save: temp file failed verification %s"), *TempPath);
		FM.Delete(*TempPath, false, true);
		return EACSaveResult::IOError;
	}

	if (!FM.Move(*FinalPath, *TempPath, /*Replace=*/true, /*EvenIfReadOnly=*/true))
	{
		UE_LOG(LogAfterclass, Error, TEXT("Save: failed to move %s -> %s"), *TempPath, *FinalPath);
		FM.Delete(*TempPath, false, true);
		return EACSaveResult::IOError;
	}

	UE_LOG(LogAfterclass, Log, TEXT("Save: wrote slot %s (%d bytes)"), *GetSlotName(Slot), FileBytes.Num());
	return EACSaveResult::Ok;
}

USaveGame* UACSaveSubsystem::ReadSlot(EACSaveSlot Slot, EACSaveResult& OutResult)
{
	const FString Path = GetSlotPath(Slot);
	if (!IFileManager::Get().FileExists(*Path)) { OutResult = EACSaveResult::NotFound; return nullptr; }

	TArray<uint8> FileBytes, Payload;
	if (!FFileHelper::LoadFileToArray(FileBytes, *Path)) { OutResult = EACSaveResult::IOError; return nullptr; }

	if (!UnwrapPayload(FileBytes, Payload))
	{
		Quarantine(Path, TEXT("header/CRC mismatch"));
		OutResult = EACSaveResult::CorruptQuarantined;
		return nullptr;
	}

	USaveGame* Loaded = UGameplayStatics::LoadGameFromMemory(Payload);
	if (!Loaded)
	{
		Quarantine(Path, TEXT("USaveGame deserialization failed"));
		OutResult = EACSaveResult::CorruptQuarantined;
		return nullptr;
	}

	int32 FileVersion = 0, CurrentVersion = 0;
	if (const UACProfileSave* P = Cast<UACProfileSave>(Loaded)) { FileVersion = P->SaveVersion; CurrentVersion = UACProfileSave::PROFILE_VERSION; }
	else if (const UACCampaignSave* C = Cast<UACCampaignSave>(Loaded)) { FileVersion = C->SaveVersion; CurrentVersion = UACCampaignSave::CAMPAIGN_VERSION; }
	else { OutResult = EACSaveResult::WrongType; return nullptr; }

	if (FileVersion > CurrentVersion)
	{
		UE_LOG(LogAfterclass, Warning, TEXT("Save: slot %s version %d is newer than build (%d); not loading."), *GetSlotName(Slot), FileVersion, CurrentVersion);
		OutResult = EACSaveResult::VersionTooNew;
		return nullptr;
	}
	if (FileVersion < CurrentVersion)
	{
		// STUB: per-version migrations go here (KI-009). v1 is the first version, so nothing to migrate yet.
		UE_LOG(LogAfterclass, Log, TEXT("Save: slot %s migrating v%d -> v%d"), *GetSlotName(Slot), FileVersion, CurrentVersion);
	}

	OutResult = EACSaveResult::Ok;
	return Loaded;
}

void UACSaveSubsystem::Quarantine(const FString& Path, const TCHAR* Reason)
{
	IFileManager& FM = IFileManager::Get();
	FM.MakeDirectory(*GetQuarantineDir(), true);
	const FString Dest = FPaths::Combine(GetQuarantineDir(),
		FString::Printf(TEXT("%s_%s.acsav"), *FPaths::GetBaseFilename(Path), *FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))));
	const bool bMoved = FM.Move(*Dest, *Path, /*Replace=*/true, /*EvenIfReadOnly=*/true);
	UE_LOG(LogAfterclass, Error, TEXT("Save: corrupt file %s (%s) -> %s [%s]"), *Path, Reason, *Dest, bMoved ? TEXT("quarantined") : TEXT("MOVE FAILED"));
}
