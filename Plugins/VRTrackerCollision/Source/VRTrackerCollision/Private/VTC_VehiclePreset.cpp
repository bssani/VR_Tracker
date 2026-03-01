// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_VehiclePreset.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "JsonObjectConverter.h"

FString UVTC_VehiclePresetLibrary::GetPresetDirectory()
{
	return FPaths::ProjectSavedDir() / TEXT("VTCPresets");
}

bool UVTC_VehiclePresetLibrary::SavePreset(const FVTCVehiclePreset& Preset)
{
	if (Preset.PresetName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[VTC] Cannot save preset with empty name."));
		return false;
	}

	const FString Dir = GetPresetDirectory();
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (!PF.DirectoryExists(*Dir)) PF.CreateDirectoryTree(*Dir);

	const FString FilePath = Dir / (Preset.PresetName + TEXT(".json"));
	const FString JsonString = PresetToJsonString(Preset);

	if (FFileHelper::SaveStringToFile(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Log, TEXT("[VTC] Preset saved: %s"), *FilePath);
		return true;
	}

	UE_LOG(LogTemp, Error, TEXT("[VTC] Failed to save preset: %s"), *FilePath);
	return false;
}

bool UVTC_VehiclePresetLibrary::LoadPreset(const FString& PresetName, FVTCVehiclePreset& OutPreset)
{
	const FString FilePath = GetPresetDirectory() / (PresetName + TEXT(".json"));
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("[VTC] Preset file not found: %s"), *FilePath);
		return false;
	}

	return JsonStringToPreset(JsonString, OutPreset);
}

TArray<FString> UVTC_VehiclePresetLibrary::GetAvailablePresetNames()
{
	TArray<FString> Names;
	const FString Dir = GetPresetDirectory();

	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Dir / TEXT("*.json")), true, false);

	for (const FString& File : Files)
	{
		Names.Add(FPaths::GetBaseFilename(File));
	}

	return Names;
}

bool UVTC_VehiclePresetLibrary::DeletePreset(const FString& PresetName)
{
	const FString FilePath = GetPresetDirectory() / (PresetName + TEXT(".json"));
	return IFileManager::Get().Delete(*FilePath);
}

FString UVTC_VehiclePresetLibrary::PresetToJsonString(const FVTCVehiclePreset& Preset)
{
	FString Output;
	FJsonObjectConverter::UStructToJsonObjectString(Preset, Output);
	return Output;
}

bool UVTC_VehiclePresetLibrary::JsonStringToPreset(const FString& JsonString, FVTCVehiclePreset& OutPreset)
{
	return FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &OutPreset);
}
