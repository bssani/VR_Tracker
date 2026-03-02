// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_CollisionDetector.h — 신체 Sphere ↔ 차량 Mesh 충돌 감지 + 거리 측정 통합

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "VTC_CollisionDetector.generated.h"

class AVTC_BodyActor;
class AVTC_ReferencePoint;

// 경고 레벨 변경 시 브로드캐스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVTCWarningLevelChanged,
	EVTCTrackerRole, BodyPart,
	FString, VehiclePartName,
	EVTCWarningLevel, NewLevel);

// 새 거리 측정값 브로드캐스트 (HUD 업데이트용)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVTCDistanceUpdated,
	const FVTCDistanceResult&, DistanceResult);

UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="VTC Collision Detector"))
class VRTRACKERCOLLISION_API UVTC_CollisionDetector : public UActorComponent
{
	GENERATED_BODY()

public:
	UVTC_CollisionDetector();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ─── 설정 ────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Collision")
	TObjectPtr<AVTC_BodyActor> BodyActor;

	// 거리 측정 대상 ReferencePoint 목록 (레벨에 배치된 것들)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Collision")
	TArray<TObjectPtr<AVTC_ReferencePoint>> ReferencePoints;

	// 경고 거리 임계값 (cm) — 이 거리 이하가 되면 Warning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Collision",
		meta=(ClampMin=0.0f, ClampMax=50.0f))
	float WarningThreshold = 10.0f;

	// 충돌 거리 임계값 (cm) — 이 거리 이하가 되면 Collision
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Collision",
		meta=(ClampMin=0.0f, ClampMax=20.0f))
	float CollisionThreshold = 3.0f;

	// 거리 계산 틱 레이트 (초당 횟수, 성능 최적화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Collision",
		meta=(ClampMin=10.0f, ClampMax=90.0f))
	float MeasurementHz = 30.0f;

	// ─── 결과 조회 ───────────────────────────────────────────────────────────

	// 현재 모든 거리 측정 결과
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Collision")
	TArray<FVTCDistanceResult> CurrentDistanceResults;

	// 세션 중 감지된 최소 거리
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Collision")
	float SessionMinDistance = TNumericLimits<float>::Max();

	// 현재 전체 경고 단계 (가장 심각한 단계 기준)
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Collision")
	EVTCWarningLevel OverallWarningLevel = EVTCWarningLevel::Safe;

	// ─── 함수 ────────────────────────────────────────────────────────────────

	// 특정 신체 부위 ↔ 특정 기준점 거리 반환 (cm)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Collision")
	float GetDistance(EVTCTrackerRole BodyPart, const FString& PartName) const;

	// 세션 통계 초기화
	UFUNCTION(BlueprintCallable, Category = "VTC|Collision")
	void ResetSessionStats();

	// ReferencePoint를 씬에서 자동 탐색
	UFUNCTION(BlueprintCallable, Category = "VTC|Collision")
	void AutoFindReferencePoints();

	// ─── Debug 시각화 ────────────────────────────────────────────────────────

	// VR 라인 중간에 거리 수치를 텍스트로 표시 (DrawDebugString)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Debug")
	bool bShowDistanceLabels = true;

	// Debug 라인 두께 (VR에서 가독성)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Debug",
		meta=(ClampMin=0.5f, ClampMax=5.0f))
	float DebugLineThickness = 1.5f;

	// ─── 자동 스크린샷 ───────────────────────────────────────────────────────

	// 최악 클리어런스 갱신 시 자동 스크린샷
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Screenshot")
	bool bAutoScreenshotOnWorstClearance = true;

	// 스크린샷 저장 경로 (비워두면 Saved/VTCLogs/Screenshots/)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Screenshot")
	FString ScreenshotDirectory = TEXT("");

	// 가장 최근 스크린샷 경로
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Screenshot")
	FString LastScreenshotPath = TEXT("");

	// ─── Delegates ──────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "VTC|Collision|Events")
	FOnVTCWarningLevelChanged OnWarningLevelChanged;

	UPROPERTY(BlueprintAssignable, Category = "VTC|Collision|Events")
	FOnVTCDistanceUpdated OnDistanceUpdated;

#if WITH_EDITOR
	// 에디터에서 값 변경 시 Warning/Collision 임계값 상호 클램프
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	float MeasurementTimer = 0.0f;

	// 이전 경고 레벨 저장 (변경 감지용)
	TMap<FString, EVTCWarningLevel> PreviousWarningLevels;

	// 경고 레벨 계산
	EVTCWarningLevel CalculateWarningLevel(float Distance) const;

	// 거리 측정 실행
	void PerformDistanceMeasurements();
};
