// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Pawn/VTC_TrackerPawn.h"
#include "DrawDebugHelpers.h"

AVTC_TrackerPawn::AVTC_TrackerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// ── 계층 구조 생성 ─────────────────────────────────────────────────────
	// Root → VROrigin → Camera + 5 MotionControllers
	// VROrigin을 별도로 두는 이유:
	//   SteamVR/OpenXR은 바닥을 트래킹 원점으로 사용한다.
	//   Pawn 루트는 고정하고 VROrigin 이하만 HMD 추적 공간을 공유하게 하면
	//   나중에 좌표계 오프셋 조정이 VROrigin 위치 변경만으로 해결된다.

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	VROrigin->SetupAttachment(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VROrigin);

	// ── 5개 MotionController: 모두 VROrigin 하위에 붙임 ────────────────────
	// VROrigin 기준 좌표계를 공유하므로 HMD와 Tracker 위치가 항상 동일 공간에서 계산됨

	MC_Waist = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_Waist"));
	MC_Waist->SetupAttachment(VROrigin);

	MC_LeftKnee = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_LeftKnee"));
	MC_LeftKnee->SetupAttachment(VROrigin);

	MC_RightKnee = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_RightKnee"));
	MC_RightKnee->SetupAttachment(VROrigin);

	MC_LeftFoot = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_LeftFoot"));
	MC_LeftFoot->SetupAttachment(VROrigin);

	MC_RightFoot = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MC_RightFoot"));
	MC_RightFoot->SetupAttachment(VROrigin);

	// TrackerDataMap 초기화
	for (uint8 i = 0; i <= static_cast<uint8>(EVTCTrackerRole::RightFoot); ++i)
	{
		FVTCTrackerData Data;
		Data.Role = static_cast<EVTCTrackerRole>(i);
		TrackerDataMap.Add(static_cast<EVTCTrackerRole>(i), Data);
	}
}

void AVTC_TrackerPawn::BeginPlay()
{
	Super::BeginPlay();

	// BeginPlay에서 MotionSource 이름 적용 (에디터에서 변경 가능한 값)
	if (MC_Waist)     MC_Waist->MotionSource     = MotionSource_Waist;
	if (MC_LeftKnee)  MC_LeftKnee->MotionSource  = MotionSource_LeftKnee;
	if (MC_RightKnee) MC_RightKnee->MotionSource = MotionSource_RightKnee;
	if (MC_LeftFoot)  MC_LeftFoot->MotionSource  = MotionSource_LeftFoot;
	if (MC_RightFoot) MC_RightFoot->MotionSource = MotionSource_RightFoot;

	UE_LOG(LogTemp, Log, TEXT("[VTC] TrackerPawn initialized. MotionSources: %s / %s / %s / %s / %s"),
		*MotionSource_Waist.ToString(),
		*MotionSource_LeftKnee.ToString(),
		*MotionSource_RightKnee.ToString(),
		*MotionSource_LeftFoot.ToString(),
		*MotionSource_RightFoot.ToString());
}

void AVTC_TrackerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateAllTrackers();
	OnAllTrackersUpdated.Broadcast();
}

// ─── IVTC_TrackerInterface 구현 ─────────────────────────────────────────────

FVTCTrackerData AVTC_TrackerPawn::GetTrackerData(EVTCTrackerRole Role) const
{
	if (const FVTCTrackerData* Found = TrackerDataMap.Find(Role))
	{
		return *Found;
	}
	return FVTCTrackerData();
}

FVector AVTC_TrackerPawn::GetTrackerLocation(EVTCTrackerRole Role) const
{
	if (const FVTCTrackerData* Found = TrackerDataMap.Find(Role))
	{
		return Found->WorldLocation;
	}
	return FVector::ZeroVector;
}

bool AVTC_TrackerPawn::IsTrackerActive(EVTCTrackerRole Role) const
{
	if (const FVTCTrackerData* Found = TrackerDataMap.Find(Role))
	{
		return Found->bIsTracked;
	}
	return false;
}

bool AVTC_TrackerPawn::AreAllTrackersActive() const
{
	return GetActiveTrackerCount() == 5;
}

int32 AVTC_TrackerPawn::GetActiveTrackerCount() const
{
	int32 Count = 0;
	for (const auto& Pair : TrackerDataMap)
	{
		if (Pair.Value.bIsTracked) ++Count;
	}
	return Count;
}

// ─── Blueprint 래퍼 ──────────────────────────────────────────────────────────

FVTCTrackerData AVTC_TrackerPawn::BP_GetTrackerData(EVTCTrackerRole TrackerRole) const     { return GetTrackerData(TrackerRole); }
FVector         AVTC_TrackerPawn::BP_GetTrackerLocation(EVTCTrackerRole TrackerRole) const { return GetTrackerLocation(TrackerRole); }
bool            AVTC_TrackerPawn::BP_IsTrackerActive(EVTCTrackerRole TrackerRole) const    { return IsTrackerActive(TrackerRole); }
bool            AVTC_TrackerPawn::BP_AreAllTrackersActive() const                   { return AreAllTrackersActive(); }
int32           AVTC_TrackerPawn::BP_GetActiveTrackerCount() const                  { return GetActiveTrackerCount(); }

// ─── Private ─────────────────────────────────────────────────────────────────

void AVTC_TrackerPawn::UpdateAllTrackers()
{
	UpdateTracker(EVTCTrackerRole::Waist,      MC_Waist);
	UpdateTracker(EVTCTrackerRole::LeftKnee,   MC_LeftKnee);
	UpdateTracker(EVTCTrackerRole::RightKnee,  MC_RightKnee);
	UpdateTracker(EVTCTrackerRole::LeftFoot,   MC_LeftFoot);
	UpdateTracker(EVTCTrackerRole::RightFoot,  MC_RightFoot);
}

void AVTC_TrackerPawn::UpdateTracker(EVTCTrackerRole Role, UMotionControllerComponent* MC)
{
	if (!MC) return;

	FVTCTrackerData& Data = TrackerDataMap.FindOrAdd(Role);
	Data.Role      = Role;
	Data.bIsTracked = MC->IsTracked();

	if (Data.bIsTracked)
	{
		Data.WorldLocation = MC->GetComponentLocation();
		Data.WorldRotation = MC->GetComponentRotation();
	}

	if (bShowDebugSpheres && Data.bIsTracked)
	{
		FColor Color = FColor::Green;
		switch (Role)
		{
		case EVTCTrackerRole::Waist:      Color = FColor::Blue;   break;
		case EVTCTrackerRole::LeftKnee:   Color = FColor::Yellow; break;
		case EVTCTrackerRole::RightKnee:  Color = FColor::Orange; break;
		case EVTCTrackerRole::LeftFoot:   Color = FColor::Cyan;   break;
		case EVTCTrackerRole::RightFoot:  Color = FColor::Purple; break;
		}
		DrawDebugSphere(GetWorld(), Data.WorldLocation, DebugSphereRadius, 8, Color, false, -1.0f, 0, 1.0f);
	}

	OnTrackerUpdated.Broadcast(Role, Data);
}

UMotionControllerComponent* AVTC_TrackerPawn::GetMotionController(EVTCTrackerRole Role) const
{
	switch (Role)
	{
	case EVTCTrackerRole::Waist:     return MC_Waist;
	case EVTCTrackerRole::LeftKnee:  return MC_LeftKnee;
	case EVTCTrackerRole::RightKnee: return MC_RightKnee;
	case EVTCTrackerRole::LeftFoot:  return MC_LeftFoot;
	case EVTCTrackerRole::RightFoot: return MC_RightFoot;
	default: return nullptr;
	}
}
