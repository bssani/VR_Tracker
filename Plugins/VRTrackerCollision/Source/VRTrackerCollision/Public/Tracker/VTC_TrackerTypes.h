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
//  단일 Tracker 데이터
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct VRTRACKERCOLLISION_API FVTCTrackerData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Tracker")
	EVTCTrackerRole Role = EVTCTrackerRole::Waist;

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Tracker")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Tracker")
	FRotator WorldRotation = FRotator::ZeroRotator;

	// Tracker가 SteamVR에 연결되어 추적 중인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "VKC|Tracker")
	bool bIsTracked = false;
};

// ─────────────────────────────────────────────
//  신체 측정값 (캘리브레이션 결과)
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct VRTRACKERCOLLISION_API FVTCBodyMeasurements
{
	GENERATED_BODY()

	// 세그먼트 길이 (cm)
	UPROPERTY(BlueprintReadOnly, Category = "VKC|Body")
	float Hip_LeftKnee = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Body")
	float Hip_RightKnee = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Body")
	float LeftKnee_LeftFoot = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Body")
	float RightKnee_RightFoot = 0.0f;

	// 다리 전체 길이 (cm)
	UPROPERTY(BlueprintReadOnly, Category = "VKC|Body")
	float TotalLeftLeg = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Body")
	float TotalRightLeg = 0.0f;

	// HMD 높이 기반 키 추정 (cm) — 바닥 기준
	UPROPERTY(BlueprintReadOnly, Category = "VKC|Body")
	float EstimatedHeight = 0.0f;

	// 캘리브레이션 완료 여부
	UPROPERTY(BlueprintReadOnly, Category = "VKC|Body")
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVKCTrackerUpdated,
	EVTCTrackerRole, InTrackerRole,
	const FVTCTrackerData&, InTrackerData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVKCAllTrackersUpdated);

// ─────────────────────────────────────────────
//  거리 측정 결과 (단일 Knee ↔ Reference Point)
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct VRTRACKERCOLLISION_API FVTCDistanceResult
{
	GENERATED_BODY()

	// 측정한 신체 부위
	UPROPERTY(BlueprintReadOnly, Category = "VKC|Measurement")
	EVTCTrackerRole BodyPart = EVTCTrackerRole::LeftKnee;

	// 기준이 된 차량 부품 이름
	UPROPERTY(BlueprintReadOnly, Category = "VKC|Measurement")
	FString VehiclePartName = TEXT("");

	// 직선 거리 (cm)
	UPROPERTY(BlueprintReadOnly, Category = "VKC|Measurement")
	float Distance = 0.0f;

	// 현재 경고 단계
	UPROPERTY(BlueprintReadOnly, Category = "VKC|Measurement")
	EVTCWarningLevel WarningLevel = EVTCWarningLevel::Safe;

	// 신체 부위 위치
	UPROPERTY(BlueprintReadOnly, Category = "VKC|Measurement")
	FVector BodyPartLocation = FVector::ZeroVector;

	// 차량 기준점 위치
	UPROPERTY(BlueprintReadOnly, Category = "VKC|Measurement")
	FVector ReferencePointLocation = FVector::ZeroVector;
};

// ─────────────────────────────────────────────
//  충돌 이벤트 기록
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct VRTRACKERCOLLISION_API FVTCCollisionEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Collision")
	FString Timestamp = TEXT("");

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Collision")
	EVTCTrackerRole BodyPart = EVTCTrackerRole::LeftKnee;

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Collision")
	FString VehiclePartName = TEXT("");

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Collision")
	FVector CollisionLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Collision")
	float Distance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "VKC|Collision")
	EVTCWarningLevel Level = EVTCWarningLevel::Collision;
};
