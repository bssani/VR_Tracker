// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_GameInstance.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "VTC_VehiclePreset.h"
#include "VTC_ProfileLibrary.h"

// ─────────────────────────────────────────────────────────────────────────────
//  라이프사이클
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_GameInstance::Init()
{
  Super::Init();
  // VRTestLevel 실행 전에 이전에 저장한 프로파일(MountOffset / VehicleHipPosition / Threshold 등)이 복원된다.
  LoadConfigFromINI();
  UE_LOG(LogTemp, Log, TEXT("[VTC] GameInstance::Init — config loaded. Profile=%s"),
      *SessionConfig.ProfileName);
}

// ─────────────────────────────────────────────────────────────────────────────
//  JSON 저장
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_GameInstance::SaveConfigToINI()
{
  const FString Path = GetConfigPath();

  // Saved/VTCConfig/ 폴더 자동 생성
  IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), /*Tree=*/true);

  FString JsonStr;
  if (!FJsonObjectConverter::UStructToJsonObjectString(SessionConfig, JsonStr))
  {
    UE_LOG(LogTemp, Error, TEXT("[VTC] Config serialize failed"));
    return;
  }

  const bool bOK = FFileHelper::SaveStringToFile(JsonStr, *Path);
  UE_LOG(LogTemp, Log, TEXT("[VTC] Config %s → %s"),
      bOK ? TEXT("saved") : TEXT("SAVE FAILED"), *Path);

  // LastSelectedProfileName 별도 저장 (VTCConfig/LastProfile.txt)
  if (!LastSelectedProfileName.IsEmpty())
  {
    const FString ProfilePath = FPaths::ProjectSavedDir() / TEXT("VTCConfig/LastProfile.txt");
    FFileHelper::SaveStringToFile(LastSelectedProfileName, *ProfilePath);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  JSON 불러오기
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_GameInstance::LoadConfigFromINI()
{
  const FString Path = GetConfigPath();

  if (!FPaths::FileExists(Path))
  {
    UE_LOG(LogTemp, Warning, TEXT("[VTC] Config 파일 없음 — 기본값 사용: %s"), *Path);
    return;
  }

  FString JsonStr;
  if (!FFileHelper::LoadFileToString(JsonStr, *Path))
  {
    UE_LOG(LogTemp, Error, TEXT("[VTC] Config read failed: %s"), *Path);
    return;
  }

  FVTCSessionConfig Loaded;
  if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonStr, &Loaded))
  {
    UE_LOG(LogTemp, Error, TEXT("[VTC] Config parse failed — 기본값 유지"));
    return;
  }
  SessionConfig = Loaded;

  // Preset JSON 재로드 (이름으로 디스크에서 다시 읽기)
  if (SessionConfig.bUseVehiclePreset && !SessionConfig.SelectedPresetName.IsEmpty())
  {
    FVTCVehiclePreset Preset;
    if (UVTC_VehiclePresetLibrary::LoadPreset(SessionConfig.SelectedPresetName, Preset))
    {
      SessionConfig.LoadedPresetJson = UVTC_VehiclePresetLibrary::PresetToJsonString(Preset);

      // VehicleHipPosition이 zero면 preset의 Vehicle_Hip에서 채움
      if (SessionConfig.VehicleHipPosition.IsNearlyZero())
      {
        for (const FVTCPresetRefPoint& P : Preset.ReferencePoints)
        {
          if (P.PartName == TEXT("Vehicle_Hip"))
          {
            SessionConfig.VehicleHipPosition = P.Location;
            UE_LOG(LogTemp, Log,
                TEXT("[VTC] VehicleHipPosition ← preset '%s': %s"),
                *SessionConfig.SelectedPresetName,
                *SessionConfig.VehicleHipPosition.ToString());
            break;
          }
        }
      }
      UE_LOG(LogTemp, Log,
          TEXT("[VTC] Preset '%s' reloaded from disk (%d ref points)"),
          *SessionConfig.SelectedPresetName, Preset.ReferencePoints.Num());
    }
    else
    {
      UE_LOG(LogTemp, Warning,
          TEXT("[VTC] Preset '%s' not found on disk — preset not applied."),
          *SessionConfig.SelectedPresetName);
      SessionConfig.bUseVehiclePreset = false;
    }
  }

  UE_LOG(LogTemp, Log, TEXT("[VTC] Config loaded ← %s  Subject=%s"),
      *Path, *SessionConfig.SubjectID);

  // LastSelectedProfileName 복원
  const FString ProfilePath = FPaths::ProjectSavedDir() / TEXT("VTCConfig/LastProfile.txt");
  FFileHelper::LoadFileToString(LastSelectedProfileName, *ProfilePath);
}

// ─────────────────────────────────────────────────────────────────────────────
//  프로파일 적용
// ─────────────────────────────────────────────────────────────────────────────
bool UVTC_GameInstance::ApplyProfileByName(const FString& ProfileName)
{
  FVTCSessionConfig Loaded;
  if (!UVTC_ProfileLibrary::LoadProfile(ProfileName, Loaded))
  {
    UE_LOG(LogTemp, Warning, TEXT("[VTC] ApplyProfileByName 실패: %s"), *ProfileName);
    return false;
  }

  SessionConfig            = Loaded;
  LastSelectedProfileName  = ProfileName;

  // 즉시 INI에도 반영 (VRTestLevel 재시작 시 유지)
  SaveConfigToINI();

  UE_LOG(LogTemp, Log, TEXT("[VTC] Profile 적용됨: %s  Subject=%s"),
      *ProfileName, *SessionConfig.SubjectID);
  return true;
}

TArray<FString> UVTC_GameInstance::GetAvailableProfileNames() const
{
  return UVTC_ProfileLibrary::GetAvailableProfileNames();
}

// ─────────────────────────────────────────────────────────────────────────────
//  내부 헬퍼
// ─────────────────────────────────────────────────────────────────────────────
FString UVTC_GameInstance::GetConfigPath() const
{
  return FPaths::ConvertRelativePathToFull(
      FPaths::ProjectSavedDir() / TEXT("VTCConfig/VTCSettings.json"));
}
