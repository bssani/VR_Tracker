// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Collision/VTC_CollisionDetector.h"
#include "Body/VTC_BodyActor.h"
#include "Vehicle/VTC_ReferencePoint.h"
#include "Tracker/VTC_TrackerManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UVTC_CollisionDetector::UVTC_CollisionDetector()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UVTC_CollisionDetector::BeginPlay()
{
	Super::BeginPlay();

	// BodyActor 자동 탐색
	if (!BodyActor)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVTC_BodyActor::StaticClass(), Found);
		if (Found.Num() > 0) BodyActor = Cast<AVTC_BodyActor>(Found[0]);
	}

	// ReferencePoint 자동 탐색
	if (ReferencePoints.Num() == 0)
	{
		AutoFindReferencePoints();
	}
}

void UVTC_CollisionDetector::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	MeasurementTimer += DeltaTime;
	const float MeasurementInterval = 1.0f / MeasurementHz;

	if (MeasurementTimer >= MeasurementInterval)
	{
		MeasurementTimer = 0.0f;
		PerformDistanceMeasurements();
	}
}

void UVTC_CollisionDetector::PerformDistanceMeasurements()
{
	if (!BodyActor) return;

	CurrentDistanceResults.Empty();
	EVTCWarningLevel NewOverallLevel = EVTCWarningLevel::Safe;

	// 측정할 신체 부위: 양쪽 무릎 (필요 시 발, 엉덩이도 추가 가능)
	const TArray<EVTCTrackerRole> BodyPartsToCheck = {
		EVTCTrackerRole::LeftKnee,
		EVTCTrackerRole::RightKnee,
		EVTCTrackerRole::LeftFoot,
		EVTCTrackerRole::RightFoot,
		EVTCTrackerRole::Waist
	};

	for (const EVTCTrackerRole TrackerRole : BodyPartsToCheck)
	{
		const FVector BodyLocation = BodyActor->GetBodyPartLocation(TrackerRole);
		if (BodyLocation.IsZero()) continue;

		for (const TObjectPtr<AVTC_ReferencePoint>& RefPoint : ReferencePoints)
		{
			if (!RefPoint || !RefPoint->bActive) continue;

			// 이 기준점이 현재 신체 부위와 관련 있는지 체크
			if (!RefPoint->RelevantBodyParts.Contains(TrackerRole)) continue;

			const float Distance = FVector::Dist(BodyLocation, RefPoint->GetReferenceLocation());
			const EVTCWarningLevel Level = CalculateWarningLevel(Distance);

			// 결과 저장
			FVTCDistanceResult Result;
			Result.BodyPart             = TrackerRole;
			Result.VehiclePartName      = RefPoint->PartName;
			Result.Distance             = Distance;
			Result.WarningLevel         = Level;
			Result.BodyPartLocation     = BodyLocation;
			Result.ReferencePointLocation = RefPoint->GetReferenceLocation();
			CurrentDistanceResults.Add(Result);

			// 최소 거리 갱신
			if (Distance < SessionMinDistance) SessionMinDistance = Distance;

			// 전체 경고 레벨 갱신 (더 심각한 쪽으로)
			if ((int32)Level > (int32)NewOverallLevel) NewOverallLevel = Level;

			// 경고 레벨 변경 감지 → Delegate 브로드캐스트
			const FString Key = RefPoint->PartName + FString::FromInt((int32)TrackerRole);
			EVTCWarningLevel* Prev = PreviousWarningLevels.Find(Key);
			if (!Prev || *Prev != Level)
			{
				PreviousWarningLevels.FindOrAdd(Key) = Level;
				OnWarningLevelChanged.Broadcast(TrackerRole, RefPoint->PartName, Level);

				// 기준점 마커 색상 업데이트
				RefPoint->SetMarkerWarningColor(Level);
			}

			// 거리 업데이트 브로드캐스트
			OnDistanceUpdated.Broadcast(Result);

			// Debug 라인 시각화
			FColor LineColor = FColor::Green;
			if (Level == EVTCWarningLevel::Warning)   LineColor = FColor::Yellow;
			if (Level == EVTCWarningLevel::Collision)  LineColor = FColor::Red;
			DrawDebugLine(GetWorld(), BodyLocation, RefPoint->GetReferenceLocation(),
				LineColor, false, -1.0f, 0, 0.5f);
		}
	}

	OverallWarningLevel = NewOverallLevel;
}

EVTCWarningLevel UVTC_CollisionDetector::CalculateWarningLevel(float Distance) const
{
	if (Distance <= CollisionThreshold) return EVTCWarningLevel::Collision;
	if (Distance <= WarningThreshold)   return EVTCWarningLevel::Warning;
	return EVTCWarningLevel::Safe;
}

float UVTC_CollisionDetector::GetDistance(EVTCTrackerRole BodyPart, const FString& PartName) const
{
	for (const FVTCDistanceResult& Result : CurrentDistanceResults)
	{
		if (Result.BodyPart == BodyPart && Result.VehiclePartName == PartName)
		{
			return Result.Distance;
		}
	}
	return -1.0f; // 미측정
}

void UVTC_CollisionDetector::ResetSessionStats()
{
	SessionMinDistance = TNumericLimits<float>::Max();
	PreviousWarningLevels.Empty();
	OverallWarningLevel = EVTCWarningLevel::Safe;

	// 모든 기준점 색상 초기화
	for (const TObjectPtr<AVTC_ReferencePoint>& RefPoint : ReferencePoints)
	{
		if (RefPoint) RefPoint->ResetMarkerColor();
	}
}

void UVTC_CollisionDetector::AutoFindReferencePoints()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVTC_ReferencePoint::StaticClass(), Found);
	ReferencePoints.Empty();
	for (AActor* Actor : Found)
	{
		if (AVTC_ReferencePoint* Ref = Cast<AVTC_ReferencePoint>(Actor))
		{
			ReferencePoints.Add(Ref);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[VTC] CollisionDetector found %d ReferencePoints."), ReferencePoints.Num());
}
