// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_SessionConfig.h — 세션 전반에서 공유되는 설정 구조체
//
// ProfileManager에서 작성 → GameInstance에 저장 → VRTestLevel에서 읽어 적용.
// INI 파일로 저장/불러오기도 지원 (VTC_GameInstance 참조).

#pragma once

#include "CoreMinimal.h"
#include "VTC_SessionConfig.generated.h"

// ─── 세션 전체 설정 ──────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct VRTRACKERCOLLISION_API FVTCSessionConfig
{
  GENERATED_BODY()

  // ── 피실험자 기본 정보 ──────────────────────────────────────────────────
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Subject")
  FString SubjectID = TEXT("");

  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Subject")
  float Height_cm = 170.0f;

  // ── Mount Offset — 트래커 하드웨어 장착 위치 → 실제 신체 접촉점 보정 ──
  // 트래커 로컬 공간(cm) 기준.
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Offset")
  FVector MountOffset_Waist = FVector::ZeroVector;

  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Offset")
  FVector MountOffset_LeftKnee = FVector::ZeroVector;

  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Offset")
  FVector MountOffset_RightKnee = FVector::ZeroVector;

  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Offset")
  FVector MountOffset_LeftFoot = FVector::ZeroVector;

  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Offset")
  FVector MountOffset_RightFoot = FVector::ZeroVector;

  // ── Vehicle Hip Reference Position ──────────────────────────────────────
  // 차량 설계 기준 Hip 위치 (VRTestLevel 월드 좌표, cm).
  // OperatorMonitor의 [Set Hip Here] 버튼으로 Waist 트래커 현재 위치를 캡처해서 설정.
  // CollisionDetector의 ReferencePoint 중 하나로 자동 등록된다.
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Vehicle")
  FVector VehicleHipPosition = FVector::ZeroVector;

  // ── 거리 임계값 (CollisionDetector에 적용) ────────────────────────────
  // ProfileManagerWidget의 슬라이더로 설정.
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Thresholds")
  float WarningThreshold_cm = 10.0f;    // 이 거리 이하 → Warning

  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Thresholds")
  float CollisionThreshold_cm = 3.0f;   // 이 거리 이하 → Collision

  // ── 차종 프리셋 ──────────────────────────────────────────────────────
  // true면 LoadedPresetJson의 ReferencePoint 데이터로 Level 2에서 스폰
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Preset")
  bool bUseVehiclePreset = false;

  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Preset")
  FString SelectedPresetName = TEXT("");

  // JSON 직렬화된 프리셋 (FVTCVehiclePreset → JsonObjectStringToUStruct 사용)
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Preset")
  FString LoadedPresetJson = TEXT("");

  // ── 가시성 ───────────────────────────────────────────────────────────────
  // Collision Sphere(USphereComponent) 및 VisualSphere 표시 여부
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Visibility")
  bool bShowCollisionSpheres = true;

  // Vive Tracker 하드웨어 3D 메시 표시 여부 (MC_* 의 DeviceModel)
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Visibility")
  bool bShowTrackerMesh = false;

  // ── 프로파일 이름 ─────────────────────────────────────────────────────
  // Saved/VTCProfiles/<ProfileName>.json 파일명으로 사용.
  // 형식: "Subject01_VehicleA" 등 자유롭게 설정.
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Profile")
  FString ProfileName = TEXT("");
};
