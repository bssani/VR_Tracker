// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_TrackerInterface.h — TrackerPawn이 구현하는 Tracker 데이터 접근 인터페이스
//
// 이 인터페이스를 통해 BodySegmentComponent, CalibrationComponent 등은
// TrackerPawn으로부터 동일한 방법으로 트래커 데이터에 접근할 수 있다.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "VTC_TrackerInterface.generated.h"

UINTERFACE(BlueprintType, MinimalAPI, meta=(DisplayName="VKC Tracker Interface"))
class UVTC_TrackerInterface : public UInterface
{
	GENERATED_BODY()
};

class VRTRACKERCOLLISION_API IVTC_TrackerInterface
{
	GENERATED_BODY()

public:
	// 특정 역할의 전체 TrackerData 반환
	virtual FVTCTrackerData GetTrackerData(EVTCTrackerRole Role) const = 0;

	// 특정 역할의 월드 위치만 빠르게 반환
	virtual FVector GetTrackerLocation(EVTCTrackerRole Role) const = 0;

	// 특정 역할 트래커가 현재 추적 중인지
	virtual bool IsTrackerActive(EVTCTrackerRole Role) const = 0;

	// 5개 트래커 모두 활성 상태인지
	virtual bool AreAllTrackersActive() const = 0;

	// 현재 활성 트래커 수 (0~5)
	virtual int32 GetActiveTrackerCount() const = 0;
};
