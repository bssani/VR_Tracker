// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_SessionConfig.h — 세션 전반에서 공유되는 설정 구조체
//
// Level 1(Setup)에서 작성 → GameInstance에 저장 → Level 2(Test)에서 읽어 적용.
// INI 파일로 저장/불러오기도 지원 (VTC_GameInstance 참조).

#pragma once

#include "CoreMinimal.h"
#include "VTC_SessionConfig.generated.h"

// ─── 실행 모드 ───────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EVTCRunMode : uint8
{
  VR         UMETA(DisplayName = "VR (HMD + Trackers)"),
  Simulation UMETA(DisplayName = "Simulation (Desktop Only)"),
};

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

  // ── 실행 모드 ────────────────────────────────────────────────────────────
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Mode")
  EVTCRunMode RunMode = EVTCRunMode::VR;

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
  // 차량 설계 기준 Hip 위치 (Level 2 월드 좌표, cm).
  // 실제 피실험자 Hip 트래커와 이 좌표 사이의 거리를 측정/비교하는 용도.
  // CollisionDetector의 ReferencePoint 중 하나로 자동 등록된다.
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Vehicle")
  FVector VehicleHipPosition = FVector::ZeroVector;

  // ── 가시성 ───────────────────────────────────────────────────────────────
  // Collision Sphere(USphereComponent) 및 VisualSphere 표시 여부
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Visibility")
  bool bShowCollisionSpheres = true;

  // Vive Tracker 하드웨어 3D 메시 표시 여부 (MC_* 의 DeviceModel)
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Visibility")
  bool bShowTrackerMesh = false;
};
