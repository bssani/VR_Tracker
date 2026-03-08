// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_ProfileLibrary.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"

FString UVTC_ProfileLibrary::GetProfileDirectory()
{
	return FPaths::ProjectSavedDir() / TEXT("VTCProfiles");
}

bool UVTC_ProfileLibrary::SaveProfile(const FString& ProfileName, const FVTCSessionConfig& Config)
{
	if (ProfileName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[VTC] Profile 이름이 비어있어 저장 불가."));
		return false;
	}

	const FString Dir = GetProfileDirectory();
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (!PF.DirectoryExists(*Dir)) PF.CreateDirectoryTree(*Dir);

	// ProfileName을 Config에도 반영한 뒤 직렬화
	FVTCSessionConfig ToSave = Config;
	ToSave.ProfileName = ProfileName;

	FString JsonStr;
	if (!FJsonObjectConverter::UStructToJsonObjectString(ToSave, JsonStr))
	{
		UE_LOG(LogTemp, Error, TEXT("[VTC] Profile 직렬화 실패: %s"), *ProfileName);
		return false;
	}

	const FString FilePath = Dir / (ProfileName + TEXT(".json"));
	const bool bOK = FFileHelper::SaveStringToFile(JsonStr, *FilePath);
	UE_LOG(LogTemp, Log, TEXT("[VTC] Profile %s → %s"),
		bOK ? TEXT("저장됨") : TEXT("저장 실패"), *FilePath);
	return bOK;
}

bool UVTC_ProfileLibrary::LoadProfile(const FString& ProfileName, FVTCSessionConfig& OutConfig)
{
	const FString FilePath = GetProfileDirectory() / (ProfileName + TEXT(".json"));

	FString JsonStr;
	if (!FFileHelper::LoadFileToString(JsonStr, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("[VTC] Profile 파일 없음: %s"), *FilePath);
		return false;
	}

	FVTCSessionConfig Loaded;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonStr, &Loaded))
	{
		UE_LOG(LogTemp, Error, TEXT("[VTC] Profile 파싱 실패: %s"), *ProfileName);
		return false;
	}

	OutConfig = Loaded;
	OutConfig.ProfileName = ProfileName;  // 파일명을 정식 이름으로 보장
	UE_LOG(LogTemp, Log, TEXT("[VTC] Profile 로드됨: %s  Subject=%s"),
		*ProfileName, *OutConfig.SubjectID);
	return true;
}

TArray<FString> UVTC_ProfileLibrary::GetAvailableProfileNames()
{
	TArray<FString> Names;
	const FString Dir = GetProfileDirectory();

	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Dir / TEXT("*.json")), true, false);

	for (const FString& File : Files)
	{
		Names.Add(FPaths::GetBaseFilename(File));
	}

	Names.Sort();
	return Names;
}

bool UVTC_ProfileLibrary::DeleteProfile(const FString& ProfileName)
{
	const FString FilePath = GetProfileDirectory() / (ProfileName + TEXT(".json"));
	const bool bOK = IFileManager::Get().Delete(*FilePath);
	UE_LOG(LogTemp, Log, TEXT("[VTC] Profile %s: %s"),
		bOK ? TEXT("삭제됨") : TEXT("삭제 실패"), *FilePath);
	return bOK;
}
