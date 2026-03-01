// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Data/VTC_DataLogger.h"
#include "Misc/FileHelper.h"
#include "Tracker/VTC_TrackerInterface.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

// ─── 초기화 ──────────────────────────────────────────────────────────────────

UVTC_DataLogger::UVTC_DataLogger()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ─── 세션 제어 ───────────────────────────────────────────────────────────────

void UVTC_DataLogger::StartLogging(const FString& SubjectID)
{
	bIsLogging       = true;
	CurrentSubjectID = SubjectID;
	LoggedRowCount   = 0;
	SessionStartTime = GetTimestampString();
	LogBuffer.Empty();
	CollisionEvents.Empty();

	// 세션 요약 추적 필드 초기화
	CachedMeasurements      = FVTCBodyMeasurements();
	HipPosSum               = FVector::ZeroVector;
	HipPosSampleCount       = 0;
	MinClearance_Hip        = TNumericLimits<float>::Max();
	MinClearance_LKnee      = TNumericLimits<float>::Max();
	MinClearance_RKnee      = TNumericLimits<float>::Max();
	MinClearance_Overall    = TNumericLimits<float>::Max();
	MinClearance_BodyPart   = TEXT("");
	MinClearance_RefPoint   = TEXT("");
	HipPosAtMinClearance    = FVector::ZeroVector;
	OverallWorstStatus      = EVTCWarningLevel::Safe;
	WarningFrameCount       = 0;

	UE_LOG(LogTemp, Log, TEXT("[VTC] Logging started for subject: %s"), *SubjectID);
}

void UVTC_DataLogger::StopLogging()
{
	bIsLogging = false;
	UE_LOG(LogTemp, Log, TEXT("[VTC] Logging stopped. Total frames: %d"), LoggedRowCount);
}

// ─── 프레임 기록 ─────────────────────────────────────────────────────────────

void UVTC_DataLogger::LogFrame(const FVTCBodyMeasurements& Measurements,
	const TArray<FVTCDistanceResult>& DistanceResults)
{
	if (!bIsLogging) return;

	// ── 행 구성 ──────────────────────────────────────────────────────────────
	FVTCLogRow Row;
	Row.Timestamp      = GetTimestampString();
	Row.SubjectID      = CurrentSubjectID;
	Row.Height         = Measurements.EstimatedHeight;
	Row.UpperLeftLeg   = Measurements.Hip_LeftKnee;
	Row.UpperRightLeg  = Measurements.Hip_RightKnee;
	Row.LowerLeftLeg   = Measurements.LeftKnee_LeftFoot;
	Row.LowerRightLeg  = Measurements.RightKnee_RightFoot;
	Row.DistanceResults = DistanceResults;
	Row.bCollisionOccurred = false;

	// 신체 부위 월드 위치 기록
	if (TrackerSource)
	{
		Row.HipLocation       = TrackerSource->GetTrackerLocation(EVTCTrackerRole::Waist);
		Row.LeftKneeLocation  = TrackerSource->GetTrackerLocation(EVTCTrackerRole::LeftKnee);
		Row.RightKneeLocation = TrackerSource->GetTrackerLocation(EVTCTrackerRole::RightKnee);
		Row.LeftFootLocation  = TrackerSource->GetTrackerLocation(EVTCTrackerRole::LeftFoot);
		Row.RightFootLocation = TrackerSource->GetTrackerLocation(EVTCTrackerRole::RightFoot);
	}
	else
	{
		for (const FVTCDistanceResult& D : DistanceResults)
		{
			switch (D.BodyPart)
			{
			case EVTCTrackerRole::Waist:     if (Row.HipLocation.IsZero())       Row.HipLocation       = D.BodyPartLocation; break;
			case EVTCTrackerRole::LeftKnee:  if (Row.LeftKneeLocation.IsZero())  Row.LeftKneeLocation  = D.BodyPartLocation; break;
			case EVTCTrackerRole::RightKnee: if (Row.RightKneeLocation.IsZero()) Row.RightKneeLocation = D.BodyPartLocation; break;
			case EVTCTrackerRole::LeftFoot:  if (Row.LeftFootLocation.IsZero())  Row.LeftFootLocation  = D.BodyPartLocation; break;
			case EVTCTrackerRole::RightFoot: if (Row.RightFootLocation.IsZero()) Row.RightFootLocation = D.BodyPartLocation; break;
			}
		}
	}

	// 충돌 플래그
	for (const FVTCDistanceResult& D : DistanceResults)
	{
		if (D.WarningLevel == EVTCWarningLevel::Collision)
		{
			Row.bCollisionOccurred = true;
			Row.CollisionPartName  = D.VehiclePartName;
			break;
		}
	}

	LogBuffer.Add(Row);
	LoggedRowCount++;

	// ── 세션 요약 갱신 ───────────────────────────────────────────────────────

	// 측정값 캐시 (첫 유효 캘리브레이션 데이터 저장)
	if (!CachedMeasurements.bIsCalibrated && Measurements.bIsCalibrated)
	{
		CachedMeasurements = Measurements;
	}

	// Hip 위치 누적
	HipPosSum += Row.HipLocation;
	HipPosSampleCount++;

	// 신체 부위별 최소 클리어런스 및 전체 최악 상태 추적
	bool bAnyWarning = false;
	for (const FVTCDistanceResult& D : DistanceResults)
	{
		// 신체 부위별 최소값 갱신
		switch (D.BodyPart)
		{
		case EVTCTrackerRole::Waist:
			if (D.Distance < MinClearance_Hip)   MinClearance_Hip   = D.Distance; break;
		case EVTCTrackerRole::LeftKnee:
			if (D.Distance < MinClearance_LKnee) MinClearance_LKnee = D.Distance; break;
		case EVTCTrackerRole::RightKnee:
			if (D.Distance < MinClearance_RKnee) MinClearance_RKnee = D.Distance; break;
		default: break;
		}

		// 전체 최소 클리어런스 (Hip 위치 포함)
		if (D.Distance < MinClearance_Overall)
		{
			MinClearance_Overall  = D.Distance;
			MinClearance_BodyPart = StaticEnum<EVTCTrackerRole>()->GetDisplayValueAsText(D.BodyPart).ToString();
			MinClearance_RefPoint = D.VehiclePartName;
			HipPosAtMinClearance  = Row.HipLocation;
		}

		// 전체 최악 경고 단계
		if (static_cast<uint8>(D.WarningLevel) > static_cast<uint8>(OverallWorstStatus))
		{
			OverallWorstStatus = D.WarningLevel;
		}

		if (D.WarningLevel != EVTCWarningLevel::Safe) bAnyWarning = true;
	}
	if (bAnyWarning) WarningFrameCount++;
}

void UVTC_DataLogger::LogCollisionEvent(const FVTCCollisionEvent& Event)
{
	CollisionEvents.Add(Event);
	UE_LOG(LogTemp, Warning, TEXT("[VTC] COLLISION: %s hit %s at distance %.1f cm"),
		*StaticEnum<EVTCTrackerRole>()->GetValueAsString(Event.BodyPart),
		*Event.VehiclePartName,
		Event.Distance);
}

// ─── CSV 내보내기 — 요약 (메인) ──────────────────────────────────────────────

FString UVTC_DataLogger::ExportToCSV()
{
	if (LogBuffer.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VTC] No log data to export."));
		return TEXT("");
	}

	FString Content = BuildSummaryHeader() + LINE_TERMINATOR;
	Content        += BuildSummaryRow()    + LINE_TERMINATOR;

	// 충돌 이벤트 섹션
	if (CollisionEvents.Num() > 0)
	{
		Content += LINE_TERMINATOR;
		Content += TEXT("=== COLLISION EVENTS ===") + FString(LINE_TERMINATOR);
		Content += TEXT("Timestamp,BodyPart,VehiclePart,Distance(cm),Location") + FString(LINE_TERMINATOR);
		for (const FVTCCollisionEvent& Event : CollisionEvents)
		{
			Content += FString::Printf(TEXT("%s,%s,%s,%.2f,\"(%.1f %.1f %.1f)\""),
				*Event.Timestamp,
				*StaticEnum<EVTCTrackerRole>()->GetValueAsString(Event.BodyPart),
				*Event.VehiclePartName,
				Event.Distance,
				Event.CollisionLocation.X, Event.CollisionLocation.Y, Event.CollisionLocation.Z);
			Content += LINE_TERMINATOR;
		}
	}

	const FString FilePath = SaveFile(TEXT("summary"), Content);
	if (!FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("[VTC] Exported summary CSV: %s"), *FilePath);
		OnLogExported.Broadcast(FilePath);
	}
	return FilePath;
}

// ─── CSV 내보내기 — 원시 프레임 (선택) ──────────────────────────────────────

FString UVTC_DataLogger::ExportFrameDataCSV()
{
	if (LogBuffer.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VTC] No frame data to export."));
		return TEXT("");
	}

	FString Content = BuildFrameHeader() + LINE_TERMINATOR;
	for (const FVTCLogRow& Row : LogBuffer)
	{
		Content += FrameRowToCSVLine(Row) + LINE_TERMINATOR;
	}

	const FString FilePath = SaveFile(TEXT("frames"), Content);
	if (!FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("[VTC] Exported frame CSV: %s (%d rows)"), *FilePath, LogBuffer.Num());
	}
	return FilePath;
}

// ─── 초기화 ──────────────────────────────────────────────────────────────────

void UVTC_DataLogger::ClearLog()
{
	LogBuffer.Empty();
	CollisionEvents.Empty();
	LoggedRowCount      = 0;
	HipPosSum           = FVector::ZeroVector;
	HipPosSampleCount   = 0;
	MinClearance_Hip    = TNumericLimits<float>::Max();
	MinClearance_LKnee  = TNumericLimits<float>::Max();
	MinClearance_RKnee  = TNumericLimits<float>::Max();
	MinClearance_Overall = TNumericLimits<float>::Max();
	MinClearance_BodyPart = TEXT("");
	MinClearance_RefPoint = TEXT("");
	HipPosAtMinClearance  = FVector::ZeroVector;
	OverallWorstStatus    = EVTCWarningLevel::Safe;
	WarningFrameCount     = 0;
	CachedMeasurements    = FVTCBodyMeasurements();
}

// ─── Summary CSV 빌더 ────────────────────────────────────────────────────────

FString UVTC_DataLogger::BuildSummaryHeader() const
{
	return
		TEXT("SubjectID,Date")
		TEXT(",Height_cm")
		TEXT(",WaistToKnee_L_cm,WaistToKnee_R_cm")
		TEXT(",KneeToFoot_L_cm,KneeToFoot_R_cm")
		TEXT(",WaistToFoot_L_cm,WaistToFoot_R_cm")
		TEXT(",HipPos_avg_X,HipPos_avg_Y,HipPos_avg_Z")
		TEXT(",HipDist_to_Ref_min_cm")
		TEXT(",LKnee_to_Ref_min_cm")
		TEXT(",RKnee_to_Ref_min_cm")
		TEXT(",MinClearance_cm,NearestBodyPart,NearestRefPoint")
		TEXT(",HipX_atMinClearance,HipY_atMinClearance,HipZ_atMinClearance")
		TEXT(",OverallStatus,CollisionCount,WarningFrames,TotalFrames");
}

FString UVTC_DataLogger::BuildSummaryRow() const
{
	// 평균 Hip 위치
	const FVector HipAvg = (HipPosSampleCount > 0)
		? HipPosSum / static_cast<float>(HipPosSampleCount)
		: FVector::ZeroVector;

	// 미측정 항목은 -1로 표시
	auto SafeMinStr = [](float Val) -> FString
	{
		return (Val >= TNumericLimits<float>::Max() * 0.5f)
			? TEXT("-1") : FString::Printf(TEXT("%.1f"), Val);
	};

	return FString::Printf(
		TEXT("%s,%s"
		     ",%.1f"
		     ",%.1f,%.1f"
		     ",%.1f,%.1f"
		     ",%.1f,%.1f"
		     ",%.1f,%.1f,%.1f"
		     ",%s,%s,%s"
		     ",%s,%s,%s"
		     ",%.1f,%.1f,%.1f"
		     ",%s,%d,%d,%d"),
		*CurrentSubjectID, *SessionStartTime,
		// ManualHeight_cm > 0이면 직접 입력값 우선, 아니면 HMD 추정값 사용
		CachedMeasurements.ManualHeight_cm > 0.0f
			? CachedMeasurements.ManualHeight_cm
			: CachedMeasurements.EstimatedHeight,
		CachedMeasurements.Hip_LeftKnee,         CachedMeasurements.Hip_RightKnee,
		CachedMeasurements.LeftKnee_LeftFoot,     CachedMeasurements.RightKnee_RightFoot,
		CachedMeasurements.TotalLeftLeg,          CachedMeasurements.TotalRightLeg,
		HipAvg.X, HipAvg.Y, HipAvg.Z,
		*SafeMinStr(MinClearance_Hip),
		*SafeMinStr(MinClearance_LKnee),
		*SafeMinStr(MinClearance_RKnee),
		*SafeMinStr(MinClearance_Overall),
		*MinClearance_BodyPart,
		*MinClearance_RefPoint,
		HipPosAtMinClearance.X, HipPosAtMinClearance.Y, HipPosAtMinClearance.Z,
		*WarningLevelToStatus(OverallWorstStatus),
		CollisionEvents.Num(), WarningFrameCount, LoggedRowCount
	);
}

// ─── Frame CSV 빌더 ──────────────────────────────────────────────────────────

FString UVTC_DataLogger::BuildFrameHeader() const
{
	// 거리 정보는 기준점별로 전부 기록 (BodyPart|RefPoint|Distance|Status 반복 컬럼)
	return
		TEXT("Timestamp,SubjectID,Height_cm")
		TEXT(",WaistToKnee_L,WaistToKnee_R,KneeToFoot_L,KneeToFoot_R")
		TEXT(",HipX,HipY,HipZ")
		TEXT(",LKneeX,LKneeY,LKneeZ,RKneeX,RKneeY,RKneeZ")
		TEXT(",LFootX,LFootY,LFootZ,RFootX,RFootY,RFootZ")
		TEXT(",BodyPart,RefPoint,Distance_cm,Status[repeated per ref]")
		TEXT(",CollisionOccurred,CollisionPart");
}

FString UVTC_DataLogger::FrameRowToCSVLine(const FVTCLogRow& Row) const
{
	// 기본 컬럼
	FString Line = FString::Printf(
		TEXT("%s,%s,%.1f,%.1f,%.1f,%.1f,%.1f"
		     ",%.1f,%.1f,%.1f"
		     ",%.1f,%.1f,%.1f,%.1f,%.1f,%.1f"
		     ",%.1f,%.1f,%.1f,%.1f,%.1f,%.1f"),
		*Row.Timestamp, *Row.SubjectID,
		Row.Height, Row.UpperLeftLeg, Row.UpperRightLeg, Row.LowerLeftLeg, Row.LowerRightLeg,
		Row.HipLocation.X,        Row.HipLocation.Y,        Row.HipLocation.Z,
		Row.LeftKneeLocation.X,   Row.LeftKneeLocation.Y,   Row.LeftKneeLocation.Z,
		Row.RightKneeLocation.X,  Row.RightKneeLocation.Y,  Row.RightKneeLocation.Z,
		Row.LeftFootLocation.X,   Row.LeftFootLocation.Y,   Row.LeftFootLocation.Z,
		Row.RightFootLocation.X,  Row.RightFootLocation.Y,  Row.RightFootLocation.Z
	);

	// 기준점별 거리 컬럼 (모두 출력)
	for (const FVTCDistanceResult& D : Row.DistanceResults)
	{
		Line += FString::Printf(TEXT(",%s,%s,%.1f,%s"),
			*StaticEnum<EVTCTrackerRole>()->GetValueAsString(D.BodyPart),
			*D.VehiclePartName,
			D.Distance,
			*WarningLevelToStatus(D.WarningLevel));
	}

	// 충돌 플래그
	Line += FString::Printf(TEXT(",%s,%s"),
		Row.bCollisionOccurred ? TEXT("TRUE") : TEXT("FALSE"),
		*Row.CollisionPartName);

	return Line;
}

// ─── 공통 유틸 ───────────────────────────────────────────────────────────────

FString UVTC_DataLogger::SaveFile(const FString& Suffix, const FString& Content) const
{
	FString Dir = LogDirectory.IsEmpty()
		? FPaths::ProjectSavedDir() / TEXT("VTCLogs")
		: LogDirectory;

	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (!PF.DirectoryExists(*Dir)) PF.CreateDirectoryTree(*Dir);

	const FString SafeTime = SessionStartTime
		.Replace(TEXT(":"), TEXT("-"))
		.Replace(TEXT(" "), TEXT("_"));
	const FString FilePath = Dir / FString::Printf(TEXT("%s_%s_%s.csv"),
		*CurrentSubjectID, *SafeTime, *Suffix);

	if (FFileHelper::SaveStringToFile(Content, *FilePath))
	{
		return FilePath;
	}
	UE_LOG(LogTemp, Error, TEXT("[VTC] Failed to save file: %s"), *FilePath);
	return TEXT("");
}

FString UVTC_DataLogger::WarningLevelToStatus(EVTCWarningLevel Level)
{
	switch (Level)
	{
	case EVTCWarningLevel::Safe:      return TEXT("GREEN");
	case EVTCWarningLevel::Warning:   return TEXT("YELLOW");
	case EVTCWarningLevel::Collision: return TEXT("RED");
	}
	return TEXT("UNKNOWN");
}

FString UVTC_DataLogger::GetTimestampString()
{
	return FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S.%s"));
}
