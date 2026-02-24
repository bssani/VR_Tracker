// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Body/VTC_CalibrationComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"

UVTC_CalibrationComponent::UVTC_CalibrationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UVTC_CalibrationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsCalibrating) return;

	CalibrationTimer += DeltaTime;
	int32 SecondsRemaining = FMath::CeilToInt(CalibrationHoldTime - CalibrationTimer);
	SecondsRemaining = FMath::Max(0, SecondsRemaining);

	// 초 단위 카운트다운 브로드캐스트
	if (SecondsRemaining != LastBroadcastedSecond)
	{
		LastBroadcastedSecond = SecondsRemaining;
		OnCalibrationCountdown.Broadcast(SecondsRemaining);
	}

	// 지정 시간 도달 → 측정 실행
	if (CalibrationTimer >= CalibrationHoldTime)
	{
		SnapCalibrate();
		bIsCalibrating = false;
		PrimaryComponentTick.SetTickFunctionEnable(false);
	}
}

void UVTC_CalibrationComponent::StartCalibration()
{
	if (!TrackerManager)
	{
		OnCalibrationFailed.Broadcast(TEXT("TrackerManager is not set."));
		return;
	}
	if (!TrackerManager->AreAllTrackersActive())
	{
		const int32 Count = TrackerManager->GetActiveTrackerCount();
		OnCalibrationFailed.Broadcast(
			FString::Printf(TEXT("Not all trackers are active. Active: %d/5"), Count));
		return;
	}

	bIsCalibrating = true;
	CalibrationTimer = 0.0f;
	LastBroadcastedSecond = -1;
	PrimaryComponentTick.SetTickFunctionEnable(true);

	UE_LOG(LogTemp, Log, TEXT("[VTC] Calibration started. Hold T-Pose for %.1f seconds."), CalibrationHoldTime);
}

void UVTC_CalibrationComponent::CancelCalibration()
{
	bIsCalibrating = false;
	CalibrationTimer = 0.0f;
	PrimaryComponentTick.SetTickFunctionEnable(false);
	UE_LOG(LogTemp, Log, TEXT("[VTC] Calibration cancelled."));
}

bool UVTC_CalibrationComponent::SnapCalibrate()
{
	if (!TrackerManager) return false;

	FVTCBodyMeasurements Measurements = CalculateMeasurements();

	FString FailReason;
	if (!ValidateMeasurements(Measurements, FailReason))
	{
		OnCalibrationFailed.Broadcast(FailReason);
		UE_LOG(LogTemp, Warning, TEXT("[VTC] Calibration failed: %s"), *FailReason);
		return false;
	}

	Measurements.bIsCalibrated = true;
	LastMeasurements = Measurements;

	OnCalibrationComplete.Broadcast(Measurements);

	UE_LOG(LogTemp, Log, TEXT("[VTC] Calibration complete!\n"
		"  Hip→LKnee: %.1f cm | Hip→RKnee: %.1f cm\n"
		"  LKnee→LFoot: %.1f cm | RKnee→RFoot: %.1f cm\n"
		"  Total Left: %.1f cm | Total Right: %.1f cm\n"
		"  Est. Height: %.1f cm"),
		Measurements.Hip_LeftKnee, Measurements.Hip_RightKnee,
		Measurements.LeftKnee_LeftFoot, Measurements.RightKnee_RightFoot,
		Measurements.TotalLeftLeg, Measurements.TotalRightLeg,
		Measurements.EstimatedHeight);

	return true;
}

FVTCBodyMeasurements UVTC_CalibrationComponent::CalculateMeasurements() const
{
	FVTCBodyMeasurements M;
	if (!TrackerManager) return M;

	const FVector HipPos    = TrackerManager->GetTrackerLocation(EVTCTrackerRole::Waist);
	const FVector LKneePos  = TrackerManager->GetTrackerLocation(EVTCTrackerRole::LeftKnee);
	const FVector RKneePos  = TrackerManager->GetTrackerLocation(EVTCTrackerRole::RightKnee);
	const FVector LFootPos  = TrackerManager->GetTrackerLocation(EVTCTrackerRole::LeftFoot);
	const FVector RFootPos  = TrackerManager->GetTrackerLocation(EVTCTrackerRole::RightFoot);

	M.Hip_LeftKnee         = FVector::Dist(HipPos, LKneePos);
	M.Hip_RightKnee        = FVector::Dist(HipPos, RKneePos);
	M.LeftKnee_LeftFoot    = FVector::Dist(LKneePos, LFootPos);
	M.RightKnee_RightFoot  = FVector::Dist(RKneePos, RFootPos);
	M.TotalLeftLeg         = M.Hip_LeftKnee + M.LeftKnee_LeftFoot;
	M.TotalRightLeg        = M.Hip_RightKnee + M.RightKnee_RightFoot;

	// HMD 높이 기반 키 추정 (HMD가 Pawn에 붙어 있다면 Pawn의 카메라 위치 활용)
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			// HMD Camera 높이 (월드 Z)
			const float CameraZ = Pawn->GetActorLocation().Z + 160.0f; // 기본값 fallback
			if (UActorComponent* CamComp = Pawn->FindComponentByClass(UCameraComponent::StaticClass()))
			{
				const float HMDHeight = Cast<UCameraComponent>(CamComp)->GetComponentLocation().Z;
				M.EstimatedHeight = HMDHeight / HeightCorrectionFactor;
			}
			else
			{
				M.EstimatedHeight = CameraZ / HeightCorrectionFactor;
			}
		}
	}

	return M;
}

bool UVTC_CalibrationComponent::ValidateMeasurements(const FVTCBodyMeasurements& M,
	FString& OutReason) const
{
	if (M.Hip_LeftKnee < 10.0f)
	{
		OutReason = TEXT("Hip→LeftKnee distance too small. Check Waist or LeftKnee tracker.");
		return false;
	}
	if (M.Hip_RightKnee < 10.0f)
	{
		OutReason = TEXT("Hip→RightKnee distance too small. Check Waist or RightKnee tracker.");
		return false;
	}
	if (M.LeftKnee_LeftFoot < 10.0f)
	{
		OutReason = TEXT("LeftKnee→LeftFoot distance too small. Check LeftKnee or LeftFoot tracker.");
		return false;
	}
	if (M.RightKnee_RightFoot < 10.0f)
	{
		OutReason = TEXT("RightKnee→RightFoot distance too small. Check RightKnee or RightFoot tracker.");
		return false;
	}
	// 비현실적으로 큰 값 체크 (100cm 이상이면 Tracker 역할 잘못 설정된 것)
	if (M.TotalLeftLeg > 150.0f || M.TotalRightLeg > 150.0f)
	{
		OutReason = TEXT("Leg length exceeds 150cm. Check tracker role assignments in SteamVR.");
		return false;
	}
	return true;
}

void UVTC_CalibrationComponent::SetManualMeasurements(float HipToLKnee, float HipToRKnee,
	float LKneeToLFoot, float RKneeToRFoot, float SubjectHeight)
{
	LastMeasurements.Hip_LeftKnee        = HipToLKnee;
	LastMeasurements.Hip_RightKnee       = HipToRKnee;
	LastMeasurements.LeftKnee_LeftFoot   = LKneeToLFoot;
	LastMeasurements.RightKnee_RightFoot = RKneeToRFoot;
	LastMeasurements.TotalLeftLeg        = HipToLKnee + LKneeToLFoot;
	LastMeasurements.TotalRightLeg       = HipToRKnee + RKneeToRFoot;
	LastMeasurements.EstimatedHeight     = SubjectHeight;
	LastMeasurements.bIsCalibrated       = true;

	OnCalibrationComplete.Broadcast(LastMeasurements);
	UE_LOG(LogTemp, Log, TEXT("[VTC] Manual calibration set. Height: %.1f cm"), SubjectHeight);
}
