// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_GameInstance.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
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

  // Run mode
  GConfig->SetString(INI_SECTION, TEXT("RunMode"),
      (C.RunMode == EVTCRunMode::VR ? TEXT("VR") : TEXT("Simulation")), Path);

  // Mount offsets
  SaveVector(TEXT("MountOffset_Waist"),      C.MountOffset_Waist,      Path);
  SaveVector(TEXT("MountOffset_LeftKnee"),   C.MountOffset_LeftKnee,   Path);
  SaveVector(TEXT("MountOffset_RightKnee"),  C.MountOffset_RightKnee,  Path);
  SaveVector(TEXT("MountOffset_LeftFoot"),   C.MountOffset_LeftFoot,   Path);
  SaveVector(TEXT("MountOffset_RightFoot"),  C.MountOffset_RightFoot,  Path);

  // Vehicle hip reference
  SaveVector(TEXT("VehicleHipPosition"), C.VehicleHipPosition, Path);

  // Visibility
  GConfig->SetBool(INI_SECTION, TEXT("ShowCollisionSpheres"), C.bShowCollisionSpheres, Path);
  GConfig->SetBool(INI_SECTION, TEXT("ShowTrackerMesh"),      C.bShowTrackerMesh,      Path);

  // Thresholds (Feature A)
  GConfig->SetFloat(INI_SECTION, TEXT("WarningThreshold_cm"),   C.WarningThreshold_cm,   Path);
  GConfig->SetFloat(INI_SECTION, TEXT("CollisionThreshold_cm"), C.CollisionThreshold_cm, Path);

  // Preset — VR 단독 실행 시 자동 복원을 위해 저장
  GConfig->SetBool  (INI_SECTION, TEXT("UseVehiclePreset"),   C.bUseVehiclePreset,   Path);
  GConfig->SetString(INI_SECTION, TEXT("SelectedPresetName"), *C.SelectedPresetName, Path);

  // SubjectID — last-used 값으로 저장 (VR 단독 실행 시 자동 채움)
  GConfig->SetString(INI_SECTION, TEXT("SubjectID"), *C.SubjectID, Path);

  GConfig->Flush(false, Path);
  UE_LOG(LogTemp, Log, TEXT("[VTC] Config saved → %s"), *Path);
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

  FVTCSessionConfig& C = SessionConfig;

  // Run mode
  FString ModeStr;
  if (GConfig->GetString(INI_SECTION, TEXT("RunMode"), ModeStr, Path))
    C.RunMode = (ModeStr == TEXT("VR")) ? EVTCRunMode::VR : EVTCRunMode::Simulation;

  // Mount offsets
  C.MountOffset_Waist     = LoadVector(TEXT("MountOffset_Waist"),     C.MountOffset_Waist,     Path);
  C.MountOffset_LeftKnee  = LoadVector(TEXT("MountOffset_LeftKnee"),  C.MountOffset_LeftKnee,  Path);
  C.MountOffset_RightKnee = LoadVector(TEXT("MountOffset_RightKnee"), C.MountOffset_RightKnee, Path);
  C.MountOffset_LeftFoot  = LoadVector(TEXT("MountOffset_LeftFoot"),  C.MountOffset_LeftFoot,  Path);
  C.MountOffset_RightFoot = LoadVector(TEXT("MountOffset_RightFoot"), C.MountOffset_RightFoot, Path);

  // Vehicle hip reference
  C.VehicleHipPosition = LoadVector(TEXT("VehicleHipPosition"), C.VehicleHipPosition, Path);

  // Visibility
  GConfig->GetBool(INI_SECTION, TEXT("ShowCollisionSpheres"), C.bShowCollisionSpheres, Path);
  GConfig->GetBool(INI_SECTION, TEXT("ShowTrackerMesh"),      C.bShowTrackerMesh,      Path);

  // Thresholds (Feature A)
  GConfig->GetFloat(INI_SECTION, TEXT("WarningThreshold_cm"),   C.WarningThreshold_cm,   Path);
  GConfig->GetFloat(INI_SECTION, TEXT("CollisionThreshold_cm"), C.CollisionThreshold_cm, Path);

  // Preset — 이름으로 디스크에서 JSON 재로드 (VR 단독 실행 지원)
  GConfig->GetBool(INI_SECTION, TEXT("UseVehiclePreset"), C.bUseVehiclePreset, Path);
  FString LoadedPresetName;
  if (GConfig->GetString(INI_SECTION, TEXT("SelectedPresetName"), LoadedPresetName, Path))
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
  if (GConfig->GetString(INI_SECTION, TEXT("SubjectID"), LoadedSubjectID, Path)
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

void UVTC_GameInstance::SaveVector(const TCHAR* Key, const FVector& V,
                                   const FString& Path) const
{
  GConfig->SetFloat(INI_SECTION, *(FString(Key) + TEXT("_X")), V.X, Path);
  GConfig->SetFloat(INI_SECTION, *(FString(Key) + TEXT("_Y")), V.Y, Path);
  GConfig->SetFloat(INI_SECTION, *(FString(Key) + TEXT("_Z")), V.Z, Path);
}

FVector UVTC_GameInstance::LoadVector(const TCHAR* Key, const FVector& Default,
                                      const FString& Path) const
{
  // UE5에서 FVector::X/Y/Z는 double이므로 GetFloat(float&)에 직접 전달 불가.
  // float 임시 변수에 읽어 FVector로 변환한다.
  float X = (float)Default.X, Y = (float)Default.Y, Z = (float)Default.Z;
  GConfig->GetFloat(INI_SECTION, *(FString(Key) + TEXT("_X")), X, Path);
  GConfig->GetFloat(INI_SECTION, *(FString(Key) + TEXT("_Y")), Y, Path);
  GConfig->GetFloat(INI_SECTION, *(FString(Key) + TEXT("_Z")), Z, Path);
  return FVector(X, Y, Z);
}
