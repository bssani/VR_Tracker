// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_CalibrationComponent.h — T-Pose 기반 신체 측정 캘리브레이션

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sound/SoundBase.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "Tracker/VTC_TrackerInterface.h"
#include "VTC_CalibrationComponent.generated.h"

// 캘리브레이션 완료 시 브로드캐스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVTCCalibrationComplete, const FVTCBodyMeasurements&, Measurements);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVTCCalibrationFailed, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVTCCalibrationCountdown, int32, SecondsRemaining);

UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="VTC Calibration"))
class VRTRACKERCOLLISION_API UVTC_CalibrationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVTC_CalibrationComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ─── 설정 ────────────────────────────────────────────────────────────────

	// Tracker 공급자 (TrackerPawn)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration")
	TScriptInterface<IVTC_TrackerInterface> TrackerSource;

	// T-Pose를 유지해야 하는 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration",
		meta=(ClampMin=1.0f, ClampMax=10.0f))
	float CalibrationHoldTime = 3.0f;

	// HMD 높이로 키를 추정할 때의 보정 계수 (서 있을 때 HMD 위치는 실제 키의 약 92%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration",
		meta=(ClampMin=0.8f, ClampMax=1.0f))
	float HeightCorrectionFactor = 0.92f;

	// ─── 유효성 검사 임계값 (Feature D) ──────────────────────────────────────

	// 좌우 비대칭 허용 비율 (0.25 = 25% 이상 차이 시 경고)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration|Validation",
		meta=(ClampMin=0.05f, ClampMax=0.5f))
	float AsymmetryWarningThreshold = 0.25f;

	// 전체 다리 길이 허용 범위 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration|Validation",
		meta=(ClampMin=30.0f, ClampMax=100.0f))
	float MinTotalLegLength = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration|Validation",
		meta=(ClampMin=80.0f, ClampMax=200.0f))
	float MaxTotalLegLength = 130.0f;

	// ─── 음성 카운트다운 (Feature H) ─────────────────────────────────────────

	// 인덱스 0 = "3", 1 = "2", 2 = "1", 3 = "완료!" (BP에서 할당)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration|Audio")
	TArray<TObjectPtr<USoundBase>> CountdownSFX;

	// 음성 볼륨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration|Audio",
		meta=(ClampMin=0.0f, ClampMax=2.0f))
	float VoiceVolume = 1.0f;

	// ─── 상태 ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Calibration")
	bool bIsCalibrating = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Calibration")
	FVTCBodyMeasurements LastMeasurements;

	// ─── 함수 ────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "VTC|Calibration")
	void StartCalibration();

	UFUNCTION(BlueprintCallable, Category = "VTC|Calibration")
	void CancelCalibration();

	UFUNCTION(BlueprintCallable, Category = "VTC|Calibration")
	bool SnapCalibrate();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Calibration")
	FVTCBodyMeasurements GetBodyMeasurements() const;

	UFUNCTION(BlueprintCallable, Category = "VTC|Calibration")
	void SetManualMeasurements(float HipToLKnee, float HipToRKnee,
		float LKneeToLFoot, float RKneeToRFoot, float SubjectHeight);

	// ─── Delegates ──────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "VTC|Calibration|Events")
	FOnVTCCalibrationComplete OnCalibrationComplete;

	UPROPERTY(BlueprintAssignable, Category = "VTC|Calibration|Events")
	FOnVTCCalibrationFailed OnCalibrationFailed;

	UPROPERTY(BlueprintAssignable, Category = "VTC|Calibration|Events")
	FOnVTCCalibrationCountdown OnCalibrationCountdown;

private:
	float CalibrationTimer = 0.0f;
	int32 LastBroadcastedSecond = -1;

	FVTCBodyMeasurements CalculateMeasurements() const;
	bool ValidateMeasurements(const FVTCBodyMeasurements& Measurements, FString& OutReason) const;
};
