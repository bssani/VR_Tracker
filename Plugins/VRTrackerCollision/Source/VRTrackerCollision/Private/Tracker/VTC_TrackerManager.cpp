// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Tracker/VTC_TrackerManager.h"
#include "DrawDebugHelpers.h"
#include "MotionControllerComponent.h"

AVTC_TrackerManager::AVTC_TrackerManager()
{
	PrimaryActorTick.bCanEverTick = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootSceneComponent);

	// 5개 MotionController 생성 (MotionSource는 BeginPlay에서 설정)
	MC_Waist = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_Waist"));
	MC_Waist->SetupAttachment(RootSceneComponent);

	MC_LeftKnee = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_LeftKnee"));
	MC_LeftKnee->SetupAttachment(RootSceneComponent);

	MC_RightKnee = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_RightKnee"));
	MC_RightKnee->SetupAttachment(RootSceneComponent);

	MC_LeftFoot = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_LeftFoot"));
	MC_LeftFoot->SetupAttachment(RootSceneComponent);

	MC_RightFoot = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_RightFoot"));
	MC_RightFoot->SetupAttachment(RootSceneComponent);

	// TrackerDataMap 초기화
	for (uint8 i = 0; i <= (uint8)EVTCTrackerRole::RightFoot; ++i)
	{
		EVTCTrackerRole TrackerRole = (EVTCTrackerRole)i;
		FVTCTrackerData Data;
		Data.Role = TrackerRole;
		TrackerDataMap.Add(TrackerRole, Data);
	}
}

void AVTC_TrackerManager::BeginPlay()
{
	Super::BeginPlay();

	// MotionSource 이름을 에디터에서 설정한 값으로 적용
	if (MC_Waist)     MC_Waist->MotionSource     = MotionSource_Waist;
	if (MC_LeftKnee)  MC_LeftKnee->MotionSource  = MotionSource_LeftKnee;
	if (MC_RightKnee) MC_RightKnee->MotionSource = MotionSource_RightKnee;
	if (MC_LeftFoot)  MC_LeftFoot->MotionSource  = MotionSource_LeftFoot;
	if (MC_RightFoot) MC_RightFoot->MotionSource = MotionSource_RightFoot;

	UE_LOG(LogTemp, Log, TEXT("[VTC] TrackerManager initialized. Motion Sources: %s / %s / %s / %s / %s"),
		*MotionSource_Waist.ToString(),
		*MotionSource_LeftKnee.ToString(),
		*MotionSource_RightKnee.ToString(),
		*MotionSource_LeftFoot.ToString(),
		*MotionSource_RightFoot.ToString());
}

void AVTC_TrackerManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateAllTrackers();
	OnAllTrackersUpdated.Broadcast();
}

void AVTC_TrackerManager::UpdateAllTrackers()
{
	UpdateTracker(EVTCTrackerRole::Waist,      MC_Waist);
	UpdateTracker(EVTCTrackerRole::LeftKnee,   MC_LeftKnee);
	UpdateTracker(EVTCTrackerRole::RightKnee,  MC_RightKnee);
	UpdateTracker(EVTCTrackerRole::LeftFoot,   MC_LeftFoot);
	UpdateTracker(EVTCTrackerRole::RightFoot,  MC_RightFoot);
}

void AVTC_TrackerManager::UpdateTracker(EVTCTrackerRole TrackerRole, UMotionControllerComponent* MC)
{
	if (!MC) return;

	FVTCTrackerData& Data = TrackerDataMap.FindOrAdd(TrackerRole);
	Data.Role = TrackerRole;
	Data.bIsTracked = MC->IsTracked();

	if (Data.bIsTracked)
	{
		Data.WorldLocation = MC->GetComponentLocation();
		Data.WorldRotation = MC->GetComponentRotation();
	}

	// Debug 시각화
	if (bShowDebugSpheres && Data.bIsTracked)
	{
		FColor DebugColor = FColor::Green;
		switch (TrackerRole)
		{
		case EVTCTrackerRole::Waist:      DebugColor = FColor::Blue;   break;
		case EVTCTrackerRole::LeftKnee:   DebugColor = FColor::Yellow; break;
		case EVTCTrackerRole::RightKnee:  DebugColor = FColor::Orange; break;
		case EVTCTrackerRole::LeftFoot:   DebugColor = FColor::Cyan;   break;
		case EVTCTrackerRole::RightFoot:  DebugColor = FColor::Purple; break;
		}
		DrawDebugSphere(GetWorld(), Data.WorldLocation, DebugSphereRadius, 8, DebugColor, false, -1.0f, 0, 1.0f);
	}

	OnTrackerUpdated.Broadcast(Data);
}

UMotionControllerComponent* AVTC_TrackerManager::GetMotionController(EVTCTrackerRole TrackerRole) const
{
	switch (TrackerRole)
	{
	case EVTCTrackerRole::Waist:     return MC_Waist;
	case EVTCTrackerRole::LeftKnee:  return MC_LeftKnee;
	case EVTCTrackerRole::RightKnee: return MC_RightKnee;
	case EVTCTrackerRole::LeftFoot:  return MC_LeftFoot;
	case EVTCTrackerRole::RightFoot: return MC_RightFoot;
	default: return nullptr;
	}
}

// ─── Blueprint 래퍼 ──────────────────────────────────────────────────────────

FVTCTrackerData AVTC_TrackerManager::BP_GetTrackerData(EVTCTrackerRole TrackerRole) const     { return GetTrackerData(TrackerRole); }
bool            AVTC_TrackerManager::BP_IsTrackerActive(EVTCTrackerRole TrackerRole) const    { return IsTrackerActive(TrackerRole); }
FVector         AVTC_TrackerManager::BP_GetTrackerLocation(EVTCTrackerRole TrackerRole) const { return GetTrackerLocation(TrackerRole); }
bool            AVTC_TrackerManager::BP_AreAllTrackersActive() const                          { return AreAllTrackersActive(); }
int32           AVTC_TrackerManager::BP_GetActiveTrackerCount() const                         { return GetActiveTrackerCount(); }

// ─── IVTC_TrackerInterface 구현 ──────────────────────────────────────────────

FVTCTrackerData AVTC_TrackerManager::GetTrackerData(EVTCTrackerRole TrackerRole) const
{
	if (const FVTCTrackerData* Found = TrackerDataMap.Find(TrackerRole))
	{
		return *Found;
	}
	return FVTCTrackerData();
}

bool AVTC_TrackerManager::IsTrackerActive(EVTCTrackerRole TrackerRole) const
{
	if (const FVTCTrackerData* Found = TrackerDataMap.Find(TrackerRole))
	{
		return Found->bIsTracked;
	}
	return false;
}

FVector AVTC_TrackerManager::GetTrackerLocation(EVTCTrackerRole TrackerRole) const
{
	if (const FVTCTrackerData* Found = TrackerDataMap.Find(TrackerRole))
	{
		return Found->WorldLocation;
	}
	return FVector::ZeroVector;
}

bool AVTC_TrackerManager::AreAllTrackersActive() const
{
	return GetActiveTrackerCount() == 5;
}

int32 AVTC_TrackerManager::GetActiveTrackerCount() const
{
	int32 Count = 0;
	for (const auto& Pair : TrackerDataMap)
	{
		if (Pair.Value.bIsTracked) ++Count;
	}
	return Count;
}
