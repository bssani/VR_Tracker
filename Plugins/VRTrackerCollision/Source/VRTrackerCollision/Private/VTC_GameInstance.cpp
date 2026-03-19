// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

// ─────────────────────────────────────────────────────────────────────────────
//  레벨 전환
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_GameInstance::OpenTestLevel_Implementation()
{
  UGameplayStatics::OpenLevel(this, FName(*TestLevelName));
}

void UVTC_GameInstance::OpenSetupLevel_Implementation()
{
  UGameplayStatics::OpenLevel(this, FName(*SetupLevelName));
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
