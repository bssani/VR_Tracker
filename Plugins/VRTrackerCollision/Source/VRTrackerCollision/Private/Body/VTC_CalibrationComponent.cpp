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

	// 초 단위 카운트다운 브로드캐스트 + 음성 재생
	if (SecondsRemaining != LastBroadcastedSecond)
	{
		LastBroadcastedSecond = SecondsRemaining;
		OnCalibrationCountdown.Broadcast(SecondsRemaining);

		// 음성 카운트다운 (Feature H)
		// 예: HoldTime=3.0 → TotalTicks=3, 3초→idx0, 2초→idx1, 1초→idx2
		if (SecondsRemaining > 0 && CountdownSFX.Num() > 0)
		{
			const int32 TotalTicks = FMath::CeilToInt(CalibrationHoldTime);
			const int32 SFXIndex = TotalTicks - SecondsRemaining;
			if (SFXIndex >= 0 && CountdownSFX.IsValidIndex(SFXIndex)
				&& CountdownSFX[SFXIndex])
			{
				UGameplayStatics::PlaySound2D(this, CountdownSFX[SFXIndex],
											  VoiceVolume);
			}
		}
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
	if (!TrackerSource)
	{
		OnCalibrationFailed.Broadcast(TEXT("TrackerSource (TrackerPawn) is not set."));
		return;
	}
	if (!TrackerSource->AreAllTrackersActive())
	{
		const int32 Count = TrackerSource->GetActiveTrackerCount();
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

FVTCBodyMeasurements UVTC_CalibrationComponent::GetBodyMeasurements() const
{
	return LastMeasurements;
}

bool UVTC_CalibrationComponent::SnapCalibrate()
{
	if (!TrackerSource) return false;

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

	// 완료 사운드 재생 (CountdownSFX 배열의 마지막 요소)
	if (CountdownSFX.Num() > 0)
	{
		const int32 CompletionIndex = CountdownSFX.Num() - 1;
		if (CountdownSFX[CompletionIndex])
		{
			UGameplayStatics::PlaySound2D(this, CountdownSFX[CompletionIndex],
										  VoiceVolume);
		}
	}

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
	if (!TrackerSource) return M;

	const FVector HipPos    = TrackerSource->GetTrackerLocation(EVTCTrackerRole::Waist);
	const FVector LKneePos  = TrackerSource->GetTrackerLocation(EVTCTrackerRole::LeftKnee);
	const FVector RKneePos  = TrackerSource->GetTrackerLocation(EVTCTrackerRole::RightKnee);
	const FVector LFootPos  = TrackerSource->GetTrackerLocation(EVTCTrackerRole::LeftFoot);
	const FVector RFootPos  = TrackerSource->GetTrackerLocation(EVTCTrackerRole::RightFoot);

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
	// ── 기본 최소값 체크 ──
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

	// ── 전체 다리 길이 범위 체크 (Feature D) ──
	if (M.TotalLeftLeg < MinTotalLegLength || M.TotalRightLeg < MinTotalLegLength)
	{
		OutReason = FString::Printf(
			TEXT("Leg too short (L:%.1f / R:%.1f cm). Min: %.0f cm. Check tracker placement."),
			M.TotalLeftLeg, M.TotalRightLeg, MinTotalLegLength);
		return false;
	}
	if (M.TotalLeftLeg > MaxTotalLegLength || M.TotalRightLeg > MaxTotalLegLength)
	{
		OutReason = FString::Printf(
			TEXT("Leg too long (L:%.1f / R:%.1f cm). Max: %.0f cm. Check tracker role assignments."),
			M.TotalLeftLeg, M.TotalRightLeg, MaxTotalLegLength);
		return false;
	}

	// ── 좌우 비대칭 체크 (Feature D) ──
	const float AvgLeg = (M.TotalLeftLeg + M.TotalRightLeg) * 0.5f;
	if (AvgLeg > 0.0f)
	{
		const float Asymmetry =
			FMath::Abs(M.TotalLeftLeg - M.TotalRightLeg) / AvgLeg;
		if (Asymmetry > AsymmetryWarningThreshold)
		{
			OutReason = FString::Printf(
				TEXT("Left/Right leg asymmetry %.0f%% exceeds %.0f%% threshold. "
				     "L:%.1f / R:%.1f cm. Re-check tracker positions."),
				Asymmetry * 100.0f, AsymmetryWarningThreshold * 100.0f,
				M.TotalLeftLeg, M.TotalRightLeg);
			return false;
		}
	}

	// ── 상/하 비율 체크 (대퇴 vs 하퇴 비율 0.7~1.5 범위) ──
	auto CheckSegmentRatio = [&](float Upper, float Lower,
								 const TCHAR* SideName) -> bool
	{
		if (Lower > 0.0f)
		{
			const float Ratio = Upper / Lower;
			if (Ratio < 0.7f || Ratio > 1.5f)
			{
				OutReason = FString::Printf(
					TEXT("%s thigh/shin ratio (%.2f) out of range [0.7–1.5]. "
					     "Upper:%.1f / Lower:%.1f cm."),
					SideName, Ratio, Upper, Lower);
				return false;
			}
		}
		return true;
	};

	if (!CheckSegmentRatio(M.Hip_LeftKnee, M.LeftKnee_LeftFoot, TEXT("Left")))
		return false;
	if (!CheckSegmentRatio(M.Hip_RightKnee, M.RightKnee_RightFoot, TEXT("Right")))
		return false;

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
