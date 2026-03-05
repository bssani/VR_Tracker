// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_GameInstance.h"
#include "Misc/ConfigFile.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "VTC_VehiclePreset.h"

// ─────────────────────────────────────────────────────────────────────────────
//  라이프사이클
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_GameInstance::Init()
{
  Super::Init();
  // 어떤 레벨보다 먼저 실행되므로, Level 1 없이 VR/Sim 레벨을 직접 실행해도
  // 이전에 저장한 MountOffset / VehicleHipPosition / Threshold 등이 복원된다.
  LoadConfigFromINI();
  UE_LOG(LogTemp, Log, TEXT("[VTC] GameInstance::Init — config loaded from INI. RunMode=%s"),
      SessionConfig.RunMode == EVTCRunMode::VR ? TEXT("VR") : TEXT("Simulation"));
}

// ─────────────────────────────────────────────────────────────────────────────
//  INI 저장
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_GameInstance::SaveConfigToINI()
{
  const FString Path = GetINIPath();
  const FVTCSessionConfig& C = SessionConfig;

  // Config 디렉터리가 없으면 생성
  IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), /*Tree=*/true);

  FConfigFile Config;
  // 기존 파일이 있으면 읽어서 병합
  if (FPaths::FileExists(Path))
    Config.Read(Path);

  auto SetVec = [&](const FString& Key, const FVector& V)
  {
    Config.SetString(INI_SECTION, *(Key + TEXT("_X")), *FString::SanitizeFloat(V.X));
    Config.SetString(INI_SECTION, *(Key + TEXT("_Y")), *FString::SanitizeFloat(V.Y));
    Config.SetString(INI_SECTION, *(Key + TEXT("_Z")), *FString::SanitizeFloat(V.Z));
  };

  // Run mode
  Config.SetString(INI_SECTION, TEXT("RunMode"),
      C.RunMode == EVTCRunMode::VR ? TEXT("VR") : TEXT("Simulation"));

  // Mount offsets
  SetVec(TEXT("MountOffset_Waist"),      C.MountOffset_Waist);
  SetVec(TEXT("MountOffset_LeftKnee"),   C.MountOffset_LeftKnee);
  SetVec(TEXT("MountOffset_RightKnee"),  C.MountOffset_RightKnee);
  SetVec(TEXT("MountOffset_LeftFoot"),   C.MountOffset_LeftFoot);
  SetVec(TEXT("MountOffset_RightFoot"),  C.MountOffset_RightFoot);

  // Vehicle hip reference
  SetVec(TEXT("VehicleHipPosition"), C.VehicleHipPosition);

  // Visibility
  Config.SetString(INI_SECTION, TEXT("ShowCollisionSpheres"), C.bShowCollisionSpheres ? TEXT("True") : TEXT("False"));
  Config.SetString(INI_SECTION, TEXT("ShowTrackerMesh"),      C.bShowTrackerMesh      ? TEXT("True") : TEXT("False"));

  // Thresholds (Feature A)
  Config.SetString(INI_SECTION, TEXT("WarningThreshold_cm"),   *FString::SanitizeFloat(C.WarningThreshold_cm));
  Config.SetString(INI_SECTION, TEXT("CollisionThreshold_cm"), *FString::SanitizeFloat(C.CollisionThreshold_cm));

  // Preset — VR 단독 실행 시 자동 복원을 위해 저장
  Config.SetString(INI_SECTION, TEXT("UseVehiclePreset"),   C.bUseVehiclePreset ? TEXT("True") : TEXT("False"));
  Config.SetString(INI_SECTION, TEXT("SelectedPresetName"), *C.SelectedPresetName);

  // SubjectID — last-used 값으로 저장 (VR 단독 실행 시 자동 채움)
  Config.SetString(INI_SECTION, TEXT("SubjectID"), *C.SubjectID);

  const bool bOK = Config.Write(Path);
  UE_LOG(LogTemp, Log, TEXT("[VTC] Config %s → %s"),
      bOK ? TEXT("saved") : TEXT("SAVE FAILED"), *Path);
}

// ─────────────────────────────────────────────────────────────────────────────
//  INI 불러오기
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_GameInstance::LoadConfigFromINI()
{
  const FString Path = GetINIPath();

  if (!FPaths::FileExists(Path))
  {
    UE_LOG(LogTemp, Warning, TEXT("[VTC] Config 파일 없음 — 기본값 사용: %s"), *Path);
    return;
  }

  FConfigFile Config;
  Config.Read(Path);

  FVTCSessionConfig& C = SessionConfig;

  auto GetVec = [&](const FString& Key, const FVector& Default) -> FVector
  {
    FString SX, SY, SZ;
    float X = (float)Default.X, Y = (float)Default.Y, Z = (float)Default.Z;
    if (Config.GetString(INI_SECTION, *(Key + TEXT("_X")), SX)) LexTryParseString(X, *SX);
    if (Config.GetString(INI_SECTION, *(Key + TEXT("_Y")), SY)) LexTryParseString(Y, *SY);
    if (Config.GetString(INI_SECTION, *(Key + TEXT("_Z")), SZ)) LexTryParseString(Z, *SZ);
    return FVector(X, Y, Z);
  };

  auto GetBool = [&](const TCHAR* Key, bool Default) -> bool
  {
    FString Val;
    return Config.GetString(INI_SECTION, Key, Val) ? (Val == TEXT("True")) : Default;
  };

  auto GetFloat = [&](const TCHAR* Key, float Default) -> float
  {
    FString Val;
    float Out = Default;
    if (Config.GetString(INI_SECTION, Key, Val)) LexTryParseString(Out, *Val);
    return Out;
  };

  // Run mode
  FString ModeStr;
  if (Config.GetString(INI_SECTION, TEXT("RunMode"), ModeStr))
    C.RunMode = (ModeStr == TEXT("VR")) ? EVTCRunMode::VR : EVTCRunMode::Simulation;

  // Mount offsets
  C.MountOffset_Waist     = GetVec(TEXT("MountOffset_Waist"),     C.MountOffset_Waist);
  C.MountOffset_LeftKnee  = GetVec(TEXT("MountOffset_LeftKnee"),  C.MountOffset_LeftKnee);
  C.MountOffset_RightKnee = GetVec(TEXT("MountOffset_RightKnee"), C.MountOffset_RightKnee);
  C.MountOffset_LeftFoot  = GetVec(TEXT("MountOffset_LeftFoot"),  C.MountOffset_LeftFoot);
  C.MountOffset_RightFoot = GetVec(TEXT("MountOffset_RightFoot"), C.MountOffset_RightFoot);

  // Vehicle hip reference
  C.VehicleHipPosition = GetVec(TEXT("VehicleHipPosition"), C.VehicleHipPosition);

  // Visibility
  C.bShowCollisionSpheres = GetBool(TEXT("ShowCollisionSpheres"), C.bShowCollisionSpheres);
  C.bShowTrackerMesh      = GetBool(TEXT("ShowTrackerMesh"),      C.bShowTrackerMesh);

  // Thresholds (Feature A)
  C.WarningThreshold_cm   = GetFloat(TEXT("WarningThreshold_cm"),   C.WarningThreshold_cm);
  C.CollisionThreshold_cm = GetFloat(TEXT("CollisionThreshold_cm"), C.CollisionThreshold_cm);

  // Preset — 이름으로 디스크에서 JSON 재로드 (VR 단독 실행 지원)
  C.bUseVehiclePreset = GetBool(TEXT("UseVehiclePreset"), C.bUseVehiclePreset);
  FString LoadedPresetName;
  if (Config.GetString(INI_SECTION, TEXT("SelectedPresetName"), LoadedPresetName))
    C.SelectedPresetName = LoadedPresetName;

  if (C.bUseVehiclePreset && !C.SelectedPresetName.IsEmpty())
  {
    FVTCVehiclePreset Preset;
    if (UVTC_VehiclePresetLibrary::LoadPreset(C.SelectedPresetName, Preset))
    {
      C.LoadedPresetJson = UVTC_VehiclePresetLibrary::PresetToJsonString(Preset);

      // VehicleHipPosition이 INI에 없으면(zero) preset의 Vehicle_Hip에서 채움
      if (C.VehicleHipPosition.IsNearlyZero())
      {
        for (const FVTCPresetRefPoint& P : Preset.ReferencePoints)
        {
          if (P.PartName == TEXT("Vehicle_Hip"))
          {
            C.VehicleHipPosition = P.Location;
            UE_LOG(LogTemp, Log,
                TEXT("[VTC] VehicleHipPosition ← preset '%s': %s"),
                *C.SelectedPresetName, *C.VehicleHipPosition.ToString());
            break;
          }
        }
      }
      UE_LOG(LogTemp, Log,
          TEXT("[VTC] Preset '%s' reloaded from disk (%d ref points)"),
          *C.SelectedPresetName, Preset.ReferencePoints.Num());
    }
    else
    {
      UE_LOG(LogTemp, Warning,
          TEXT("[VTC] Preset '%s' not found on disk — preset not applied."),
          *C.SelectedPresetName);
      C.bUseVehiclePreset = false;
    }
  }

  // SubjectID last-used 복원 (비어 있으면 기본값 유지)
  FString LoadedSubjectID;
  if (Config.GetString(INI_SECTION, TEXT("SubjectID"), LoadedSubjectID)
      && !LoadedSubjectID.IsEmpty())
    C.SubjectID = LoadedSubjectID;

  UE_LOG(LogTemp, Log, TEXT("[VTC] Config loaded ← %s"), *Path);
}

// ─────────────────────────────────────────────────────────────────────────────
//  내부 헬퍼
// ─────────────────────────────────────────────────────────────────────────────
FString UVTC_GameInstance::GetINIPath() const
{
  return FPaths::ConvertRelativePathToFull(
      FPaths::ProjectConfigDir() / TEXT("VTCSettings.ini"));
}
