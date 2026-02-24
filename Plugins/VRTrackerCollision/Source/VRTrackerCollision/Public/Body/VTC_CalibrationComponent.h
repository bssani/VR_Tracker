// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_CalibrationComponent.h — T-Pose 기반 신체 측정 캘리브레이션

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "Tracker/VTC_TrackerInterface.h"
#include "VTC_CalibrationComponent.generated.h"

// 캘리브레이션 완료 시 브로드캐스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVKCCalibrationComplete, const FVTCBodyMeasurements&, Measurements);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVKCCalibrationFailed, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVKCCalibrationCountdown, int32, SecondsRemaining);

UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="VKC Calibration"))
class VRTRACKERCOLLISION_API UVTC_CalibrationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVTC_CalibrationComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ─── 설정 ────────────────────────────────────────────────────────────────

	// Tracker 공급자 — TrackerPawn 또는 TrackerManager 모두 할당 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Calibration")
	TScriptInterface<IVTC_TrackerInterface> TrackerManager;

	// T-Pose를 유지해야 하는 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Calibration",
		meta=(ClampMin=1.0f, ClampMax=10.0f))
	float CalibrationHoldTime = 3.0f;

	// HMD 높이로 키를 추정할 때의 보정 계수 (서 있을 때 HMD 위치는 실제 키의 약 92%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Calibration",
		meta=(ClampMin=0.8f, ClampMax=1.0f))
	float HeightCorrectionFactor = 0.92f;

	// ─── 상태 ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VKC|Calibration")
	bool bIsCalibrating = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VKC|Calibration")
	FVTCBodyMeasurements LastMeasurements;

	// ─── 함수 ────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "VKC|Calibration")
	void StartCalibration();

	UFUNCTION(BlueprintCallable, Category = "VKC|Calibration")
	void CancelCalibration();

	UFUNCTION(BlueprintCallable, Category = "VKC|Calibration")
	bool SnapCalibrate();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Calibration")
	FVTCBodyMeasurements GetBodyMeasurements() const { return LastMeasurements; }

	UFUNCTION(BlueprintCallable, Category = "VKC|Calibration")
	void SetManualMeasurements(float HipToLKnee, float HipToRKnee,
		float LKneeToLFoot, float RKneeToRFoot, float SubjectHeight);

	// ─── Delegates ──────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "VKC|Calibration|Events")
	FOnVKCCalibrationComplete OnCalibrationComplete;

	UPROPERTY(BlueprintAssignable, Category = "VKC|Calibration|Events")
	FOnVKCCalibrationFailed OnCalibrationFailed;

	UPROPERTY(BlueprintAssignable, Category = "VKC|Calibration|Events")
	FOnVKCCalibrationCountdown OnCalibrationCountdown;

private:
	float CalibrationTimer = 0.0f;
	int32 LastBroadcastedSecond = -1;

	FVTCBodyMeasurements CalculateMeasurements() const;
	bool ValidateMeasurements(const FVTCBodyMeasurements& Measurements, FString& OutReason) const;
};
