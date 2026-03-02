// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_TrackerTypes.h — 공통 Enum, Struct 정의 (다른 모든 모듈이 이 파일을 참조)

#pragma once

#include "CoreMinimal.h"
#include "VTC_TrackerTypes.generated.h"

// ─────────────────────────────────────────────
//  Tracker 역할 (5개 Vive Tracker)
// ─────────────────────────────────────────────
UENUM(BlueprintType)
enum class EVTCTrackerRole : uint8
{
	Waist       UMETA(DisplayName = "Waist / Hip"),
	LeftKnee    UMETA(DisplayName = "Left Knee"),
	RightKnee   UMETA(DisplayName = "Right Knee"),
	LeftFoot    UMETA(DisplayName = "Left Foot"),
	RightFoot   UMETA(DisplayName = "Right Foot"),
};

// ─────────────────────────────────────────────
//  경고 단계
// ─────────────────────────────────────────────
UENUM(BlueprintType)
enum class EVTCWarningLevel : uint8
{
	Safe        UMETA(DisplayName = "Safe (Green)"),       // 거리 > WarningThreshold
	Warning     UMETA(DisplayName = "Warning (Yellow)"),   // CollisionThreshold < 거리 <= WarningThreshold
	Collision   UMETA(DisplayName = "Collision (Red)"),    // 거리 <= CollisionThreshold 또는 Overlap
};

// ─────────────────────────────────────────────
//  세션 상태
// ─────────────────────────────────────────────
UENUM(BlueprintType)
enum class EVTCSessionState : uint8
{
	Idle        UMETA(DisplayName = "Idle"),
	Calibrating UMETA(DisplayName = "Calibrating"),
	Testing     UMETA(DisplayName = "Testing"),
	Reviewing   UMETA(DisplayName = "Reviewing"),
};

// ─────────────────────────────────────────────
//  이동 단계 (진입/착석/이탈 자동 분류)
// ─────────────────────────────────────────────
UENUM(BlueprintType)
enum class EVTCMovementPhase : uint8
{
	Unknown     UMETA(DisplayName = "Unknown"),
	Stationary  UMETA(DisplayName = "Stationary"),   // Hip 거의 안 움직임
	Entering    UMETA(DisplayName = "Entering"),      // Hip Z 빠르게 내려감 (탑승 중)
	Seated      UMETA(DisplayName = "Seated"),        // Hip 안정 + 낮은 높이 (착석 완료)
	Exiting     UMETA(DisplayName = "Exiting"),       // Hip Z 빠르게 올라감 (하차 중)
};

// ─────────────────────────────────────────────
//  단일 Tracker 데이터
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct VRTRACKERCOLLISION_API FVTCTrackerData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Tracker")
	EVTCTrackerRole Role = EVTCTrackerRole::Waist;

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Tracker")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Tracker")
	FRotator WorldRotation = FRotator::ZeroRotator;

	// Tracker가 SteamVR에 연결되어 추적 중인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Tracker")
	bool bIsTracked = false;

	// true = 실제 추적이 아닌 드롭아웃 보간값을 사용 중
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Tracker")
	bool bIsInterpolated = false;
};

// ─────────────────────────────────────────────
//  신체 측정값 (캘리브레이션 결과)
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct VRTRACKERCOLLISION_API FVTCBodyMeasurements
{
	GENERATED_BODY()

	// 세그먼트 길이 (cm)
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Body")
	float Hip_LeftKnee = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Body")
	float Hip_RightKnee = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Body")
	float LeftKnee_LeftFoot = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Body")
	float RightKnee_RightFoot = 0.0f;

	// 다리 전체 길이 (cm)
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Body")
	float TotalLeftLeg = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Body")
	float TotalRightLeg = 0.0f;

	// HMD 높이 기반 키 추정 (cm) — 바닥 기준 (자동 보정, ±5cm 오차)
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Body")
	float EstimatedHeight = 0.0f;

	// 위젯에서 직접 입력한 키 (cm) — 0이면 EstimatedHeight 사용
	UPROPERTY(BlueprintReadWrite, Category = "VTC|Body")
	float ManualHeight_cm = 0.0f;

	// 캘리브레이션 완료 여부
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Body")
	bool bIsCalibrated = false;

	// 유효성 검사 (모든 세그먼트가 최소 10cm 이상인지)
	bool IsValid() const
	{
		return Hip_LeftKnee > 10.0f && Hip_RightKnee > 10.0f
			&& LeftKnee_LeftFoot > 10.0f && RightKnee_RightFoot > 10.0f;
	}
};

// ─────────────────────────────────────────────
//  Tracker 업데이트 델리게이트
// ─────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVTCTrackerUpdated,
	EVTCTrackerRole, InTrackerRole,
	const FVTCTrackerData&, InTrackerData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVTCAllTrackersUpdated);

// Phase 변경 델리게이트 (진입/착석/이탈 자동 감지)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVTCPhaseChanged,
	EVTCMovementPhase, OldPhase,
	EVTCMovementPhase, NewPhase);

// ─────────────────────────────────────────────
//  거리 측정 결과 (단일 Knee ↔ Reference Point)
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct VRTRACKERCOLLISION_API FVTCDistanceResult
{
	GENERATED_BODY()

	// 측정한 신체 부위
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Measurement")
	EVTCTrackerRole BodyPart = EVTCTrackerRole::LeftKnee;

	// 기준이 된 차량 부품 이름
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Measurement")
	FString VehiclePartName = TEXT("");

	// 직선 거리 (cm)
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Measurement")
	float Distance = 0.0f;

	// 현재 경고 단계
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Measurement")
	EVTCWarningLevel WarningLevel = EVTCWarningLevel::Safe;

	// 신체 부위 위치
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Measurement")
	FVector BodyPartLocation = FVector::ZeroVector;

	// 차량 기준점 위치
	UPROPERTY(BlueprintReadOnly, Category = "VTC|Measurement")
	FVector ReferencePointLocation = FVector::ZeroVector;
};

// ─────────────────────────────────────────────
//  충돌 이벤트 기록
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct VRTRACKERCOLLISION_API FVTCCollisionEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Collision")
	FString Timestamp = TEXT("");

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Collision")
	EVTCTrackerRole BodyPart = EVTCTrackerRole::LeftKnee;

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Collision")
	FString VehiclePartName = TEXT("");

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Collision")
	FVector CollisionLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Collision")
	float Distance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VTC|Collision")
	EVTCWarningLevel Level = EVTCWarningLevel::Collision;
};
