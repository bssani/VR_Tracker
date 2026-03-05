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

	const int32 ActiveCount = TrackerSource->GetActiveTrackerCount();
	if (ActiveCount == 0)
	{
		OnCalibrationFailed.Broadcast(TEXT("No trackers active. Connect at least one tracker and try again."));
		return;
	}

	if (ActiveCount < 5)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[VTC] Calibration: %d/5 trackers active. "
			     "Missing trackers will use mirrored values from the opposite side."),
			ActiveCount);
	}

	bIsCalibrating = true;
	CalibrationTimer = 0.0f;
	LastBroadcastedSecond = -1;
	PrimaryComponentTick.SetTickFunctionEnable(true);

	UE_LOG(LogTemp, Log, TEXT("[VTC] Calibration started (%d/5 trackers). Hold T-Pose for %.1f seconds."),
		ActiveCount, CalibrationHoldTime);
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

	UE_LOG(LogTemp, Log, TEXT("[VTC] Calibration complete! (%d/5 trackers)\n"
		"  Hip→LKnee: %.1f cm | Hip→RKnee: %.1f cm\n"
		"  LKnee→LFoot: %.1f cm | RKnee→RFoot: %.1f cm\n"
		"  Total Left: %.1f cm | Total Right: %.1f cm\n"
		"  Est. Height: %.1f cm"),
		TrackerSource ? TrackerSource->GetActiveTrackerCount() : 0,
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

	// 트래커별 활성 여부 확인 — 비활성 트래커는 거리 계산에서 제외
	const bool bWaist = TrackerSource->IsTrackerActive(EVTCTrackerRole::Waist);
	const bool bLKnee = TrackerSource->IsTrackerActive(EVTCTrackerRole::LeftKnee);
	const bool bRKnee = TrackerSource->IsTrackerActive(EVTCTrackerRole::RightKnee);
	const bool bLFoot = TrackerSource->IsTrackerActive(EVTCTrackerRole::LeftFoot);
	const bool bRFoot = TrackerSource->IsTrackerActive(EVTCTrackerRole::RightFoot);

	const FVector HipPos   = bWaist ? TrackerSource->GetTrackerLocation(EVTCTrackerRole::Waist)     : FVector::ZeroVector;
	const FVector LKneePos = bLKnee ? TrackerSource->GetTrackerLocation(EVTCTrackerRole::LeftKnee)  : FVector::ZeroVector;
	const FVector RKneePos = bRKnee ? TrackerSource->GetTrackerLocation(EVTCTrackerRole::RightKnee) : FVector::ZeroVector;
	const FVector LFootPos = bLFoot ? TrackerSource->GetTrackerLocation(EVTCTrackerRole::LeftFoot)  : FVector::ZeroVector;
	const FVector RFootPos = bRFoot ? TrackerSource->GetTrackerLocation(EVTCTrackerRole::RightFoot) : FVector::ZeroVector;

	// 직접 측정: 양쪽 트래커 모두 활성인 세그먼트만
	const float DirectHipL = (bWaist && bLKnee) ? FVector::Dist(HipPos, LKneePos)  : 0.0f;
	const float DirectHipR = (bWaist && bRKnee) ? FVector::Dist(HipPos, RKneePos)  : 0.0f;
	const float DirectLLow = (bLKnee && bLFoot) ? FVector::Dist(LKneePos, LFootPos) : 0.0f;
	const float DirectRLow = (bRKnee && bRFoot) ? FVector::Dist(RKneePos, RFootPos) : 0.0f;

	// 미러링: 한쪽이 없으면 반대쪽 측정값으로 좌우 대칭 추정
	M.Hip_LeftKnee        = (DirectHipL > 0.0f) ? DirectHipL : DirectHipR;
	M.Hip_RightKnee       = (DirectHipR > 0.0f) ? DirectHipR : DirectHipL;
	M.LeftKnee_LeftFoot   = (DirectLLow > 0.0f) ? DirectLLow : DirectRLow;
	M.RightKnee_RightFoot = (DirectRLow > 0.0f) ? DirectRLow : DirectLLow;

	M.TotalLeftLeg  = M.Hip_LeftKnee + M.LeftKnee_LeftFoot;
	M.TotalRightLeg = M.Hip_RightKnee + M.RightKnee_RightFoot;

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
	// ── 세그먼트별 최소값 체크 (0.0f = 해당 트래커 미연결 → 건너뜀) ──
	if (M.Hip_LeftKnee > 0.0f && M.Hip_LeftKnee < 10.0f)
	{
		OutReason = TEXT("Hip→LeftKnee distance too small. Check Waist or LeftKnee tracker.");
		return false;
	}
	if (M.Hip_RightKnee > 0.0f && M.Hip_RightKnee < 10.0f)
	{
		OutReason = TEXT("Hip→RightKnee distance too small. Check Waist or RightKnee tracker.");
		return false;
	}
	if (M.LeftKnee_LeftFoot > 0.0f && M.LeftKnee_LeftFoot < 10.0f)
	{
		OutReason = TEXT("LeftKnee→LeftFoot distance too small. Check LeftKnee or LeftFoot tracker.");
		return false;
	}
	if (M.RightKnee_RightFoot > 0.0f && M.RightKnee_RightFoot < 10.0f)
	{
		OutReason = TEXT("RightKnee→RightFoot distance too small. Check RightKnee or RightFoot tracker.");
		return false;
	}

	// ── 전체 다리 길이 범위 체크 (측정된 경우만) ──
	if (M.TotalLeftLeg > 0.0f)
	{
		if (M.TotalLeftLeg < MinTotalLegLength || M.TotalLeftLeg > MaxTotalLegLength)
		{
			OutReason = FString::Printf(
				TEXT("Left leg length (%.1f cm) out of range [%.0f–%.0f cm]. Check tracker placement."),
				M.TotalLeftLeg, MinTotalLegLength, MaxTotalLegLength);
			return false;
		}
	}
	if (M.TotalRightLeg > 0.0f)
	{
		if (M.TotalRightLeg < MinTotalLegLength || M.TotalRightLeg > MaxTotalLegLength)
		{
			OutReason = FString::Printf(
				TEXT("Right leg length (%.1f cm) out of range [%.0f–%.0f cm]. Check tracker placement."),
				M.TotalRightLeg, MinTotalLegLength, MaxTotalLegLength);
			return false;
		}
	}

	// ── 좌우 비대칭 체크 (양쪽 모두 측정된 경우만) ──
	if (M.TotalLeftLeg > 0.0f && M.TotalRightLeg > 0.0f)
	{
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
	}

	// ── 상/하 비율 체크 (세그먼트 모두 측정된 경우만) ──
	auto CheckSegmentRatio = [&](float Upper, float Lower,
								 const TCHAR* SideName) -> bool
	{
		if (Upper > 0.0f && Lower > 0.0f)
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
