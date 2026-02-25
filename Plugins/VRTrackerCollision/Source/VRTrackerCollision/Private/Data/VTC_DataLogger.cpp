// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Data/VTC_DataLogger.h"
#include "Misc/FileHelper.h"
#include "Tracker/VTC_TrackerInterface.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

UVTC_DataLogger::UVTC_DataLogger()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVTC_DataLogger::StartLogging(const FString& SubjectID)
{
	bIsLogging = true;
	CurrentSubjectID = SubjectID;
	LoggedRowCount = 0;
	SessionStartTime = GetTimestampString();
	LogBuffer.Empty();
	CollisionEvents.Empty();

	UE_LOG(LogTemp, Log, TEXT("[VTC] Logging started for subject: %s"), *SubjectID);
}

void UVTC_DataLogger::StopLogging()
{
	bIsLogging = false;
	UE_LOG(LogTemp, Log, TEXT("[VTC] Logging stopped. Total rows: %d"), LoggedRowCount);
}

void UVTC_DataLogger::LogFrame(const FVTCBodyMeasurements& Measurements,
	const TArray<FVTCDistanceResult>& DistanceResults)
{
	if (!bIsLogging) return;

	FVTCLogRow Row;
	Row.Timestamp       = GetTimestampString();
	Row.SubjectID       = CurrentSubjectID;
	Row.Height          = Measurements.EstimatedHeight;
	Row.UpperLeftLeg    = Measurements.Hip_LeftKnee;
	Row.UpperRightLeg   = Measurements.Hip_RightKnee;
	Row.LowerLeftLeg    = Measurements.LeftKnee_LeftFoot;
	Row.LowerRightLeg   = Measurements.RightKnee_RightFoot;
	Row.DistanceResults = DistanceResults;
	Row.bCollisionOccurred = false;

	// ── 신체 부위 월드 위치 기록 ──────────────────────────────────────────
	// TrackerSource가 있으면 직접 조회 (가장 정확).
	// 없으면 DistanceResults에 포함된 BodyPartLocation으로 폴백.
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
		// DistanceResults에서 각 신체 부위 위치 추출 (중복 제거, 첫 번째 값 사용)
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

	// 현재 프레임에 충돌이 있으면 표시
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
}

void UVTC_DataLogger::LogCollisionEvent(const FVTCCollisionEvent& Event)
{
	CollisionEvents.Add(Event);
	UE_LOG(LogTemp, Warning, TEXT("[VTC] COLLISION: %s hit %s at distance %.1f cm"),
		*StaticEnum<EVTCTrackerRole>()->GetValueAsString(Event.BodyPart),
		*Event.VehiclePartName,
		Event.Distance);
}

FString UVTC_DataLogger::ExportToCSV()
{
	if (LogBuffer.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VTC] No log data to export."));
		return TEXT("");
	}

	// 저장 경로 결정
	if (LogDirectory.IsEmpty())
	{
		LogDirectory = FPaths::ProjectSavedDir() / TEXT("VTCLogs");
	}

	// 디렉토리 생성
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*LogDirectory))
	{
		PlatformFile.CreateDirectoryTree(*LogDirectory);
	}

	// 파일명: SubjectID_YYYYMMDD_HHMMSS.csv
	const FString FileName = FString::Printf(TEXT("%s_%s.csv"),
		*CurrentSubjectID, *SessionStartTime.Replace(TEXT(":"), TEXT("-")).Replace(TEXT(" "), TEXT("_")));
	const FString FilePath = LogDirectory / FileName;

	// CSV 내용 빌드
	FString CSVContent = BuildCSVHeader() + LINE_TERMINATOR;
	for (const FVTCLogRow& Row : LogBuffer)
	{
		CSVContent += RowToCSVLine(Row) + LINE_TERMINATOR;
	}

	// 충돌 이벤트 섹션 추가
	if (CollisionEvents.Num() > 0)
	{
		CSVContent += LINE_TERMINATOR;
		CSVContent += TEXT("=== COLLISION EVENTS ===") + FString(LINE_TERMINATOR);
		CSVContent += TEXT("Timestamp,BodyPart,VehiclePart,Distance(cm),Location") + FString(LINE_TERMINATOR);
		for (const FVTCCollisionEvent& Event : CollisionEvents)
		{
			CSVContent += FString::Printf(TEXT("%s,%s,%s,%.2f,\"(%.1f %.1f %.1f)\""),
				*Event.Timestamp,
				*StaticEnum<EVTCTrackerRole>()->GetValueAsString(Event.BodyPart),
				*Event.VehiclePartName,
				Event.Distance,
				Event.CollisionLocation.X, Event.CollisionLocation.Y, Event.CollisionLocation.Z);
			CSVContent += LINE_TERMINATOR;
		}
	}

	// 파일 저장
	if (FFileHelper::SaveStringToFile(CSVContent, *FilePath))
	{
		UE_LOG(LogTemp, Log, TEXT("[VTC] Exported CSV: %s (%d rows)"), *FilePath, LogBuffer.Num());
		OnLogExported.Broadcast(FilePath);
		return FilePath;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[VTC] Failed to export CSV to: %s"), *FilePath);
		return TEXT("");
	}
}

FString UVTC_DataLogger::BuildCSVHeader() const
{
	return TEXT("Timestamp,SubjectID,Height(cm),UpperLeft(cm),UpperRight(cm),LowerLeft(cm),LowerRight(cm)")
		TEXT(",HipX,HipY,HipZ,LKneeX,LKneeY,LKneeZ,RKneeX,RKneeY,RKneeZ")
		TEXT(",LFootX,LFootY,LFootZ,RFootX,RFootY,RFootZ")
		TEXT(",PartName,BodyPart,Distance(cm),WarningLevel")
		TEXT(",CollisionOccurred,CollisionPart");
}

FString UVTC_DataLogger::RowToCSVLine(const FVTCLogRow& Row) const
{
	// 거리 정보 (여러 기준점이 있으면 첫 번째 것만 메인 컬럼에 기록)
	FString PartName, BodyPartStr, DistStr, LevelStr;
	if (Row.DistanceResults.Num() > 0)
	{
		const FVTCDistanceResult& D = Row.DistanceResults[0];
		PartName    = D.VehiclePartName;
		BodyPartStr = StaticEnum<EVTCTrackerRole>()->GetValueAsString(D.BodyPart);
		DistStr     = FString::SanitizeFloat(D.Distance);
		LevelStr    = StaticEnum<EVTCWarningLevel>()->GetValueAsString(D.WarningLevel);
	}

	return FString::Printf(
		TEXT("%s,%s,%.1f,%.1f,%.1f,%.1f,%.1f")
		TEXT(",%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f")
		TEXT(",%.1f,%.1f,%.1f,%.1f,%.1f,%.1f")
		TEXT(",%s,%s,%s,%s")
		TEXT(",%s,%s"),
		*Row.Timestamp, *Row.SubjectID,
		Row.Height, Row.UpperLeftLeg, Row.UpperRightLeg, Row.LowerLeftLeg, Row.LowerRightLeg,
		Row.HipLocation.X, Row.HipLocation.Y, Row.HipLocation.Z,
		Row.LeftKneeLocation.X, Row.LeftKneeLocation.Y, Row.LeftKneeLocation.Z,
		Row.RightKneeLocation.X, Row.RightKneeLocation.Y, Row.RightKneeLocation.Z,
		Row.LeftFootLocation.X, Row.LeftFootLocation.Y, Row.LeftFootLocation.Z,
		Row.RightFootLocation.X, Row.RightFootLocation.Y, Row.RightFootLocation.Z,
		*PartName, *BodyPartStr, *DistStr, *LevelStr,
		Row.bCollisionOccurred ? TEXT("TRUE") : TEXT("FALSE"), *Row.CollisionPartName
	);
}

void UVTC_DataLogger::ClearLog()
{
	LogBuffer.Empty();
	CollisionEvents.Empty();
	LoggedRowCount = 0;
}

FString UVTC_DataLogger::GetTimestampString()
{
	return FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S.%s"));
}
