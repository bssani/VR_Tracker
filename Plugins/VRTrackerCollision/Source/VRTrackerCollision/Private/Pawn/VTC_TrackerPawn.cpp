// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Pawn/VTC_TrackerPawn.h"
#include "DrawDebugHelpers.h"

AVTC_TrackerPawn::AVTC_TrackerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// ── 계층 구조 생성 ─────────────────────────────────────────────────────
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	VROrigin->SetupAttachment(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VROrigin);

	// ── 5개 MotionController: 모두 VROrigin 하위에 붙임 ────────────────────
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

	UE_LOG(LogTemp, Log,
		TEXT("[VTC] TrackerPawn initialized. MotionSources: %s / %s / %s / %s / %s"),
		*MotionSource_Waist.ToString(),
		*MotionSource_LeftKnee.ToString(),
		*MotionSource_RightKnee.ToString(),
		*MotionSource_LeftFoot.ToString(),
		*MotionSource_RightFoot.ToString());

	// ── 착석 자동 스냅 ────────────────────────────────────────────────────
	if (bAutoSnapOnBeginPlay)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			SnapWaistTo(SeatHipWorldPosition);
		});
	}
}

void AVTC_TrackerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// VR 모드: 실제 Tracker에서 데이터 읽기
	UpdateAllTrackers();

	// Movement Phase 감지 (Feature E)
	DetectMovementPhase(DeltaTime);

	OnAllTrackersUpdated.Broadcast();
}

// ─── IVTC_TrackerInterface 구현 ─────────────────────────────────────────────

FVTCTrackerData AVTC_TrackerPawn::GetTrackerData(EVTCTrackerRole TrackerRole) const
{
	if (const FVTCTrackerData* Found = TrackerDataMap.Find(TrackerRole))
	{
		return *Found;
	}
	return FVTCTrackerData();
}

FVector AVTC_TrackerPawn::GetTrackerLocation(EVTCTrackerRole TrackerRole) const
{
	if (const FVTCTrackerData* Found = TrackerDataMap.Find(TrackerRole))
	{
		return Found->WorldLocation;
	}
	return FVector::ZeroVector;
}

bool AVTC_TrackerPawn::IsTrackerActive(EVTCTrackerRole TrackerRole) const
{
	if (const FVTCTrackerData* Found = TrackerDataMap.Find(TrackerRole))
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
bool            AVTC_TrackerPawn::BP_AreAllTrackersActive() const                          { return AreAllTrackersActive(); }
int32           AVTC_TrackerPawn::BP_GetActiveTrackerCount() const                         { return GetActiveTrackerCount(); }

// ─── VR 실제 Tracker 갱신 (Private) ─────────────────────────────────────────

void AVTC_TrackerPawn::UpdateAllTrackers()
{
	UpdateTracker(EVTCTrackerRole::Waist,      MC_Waist);
	UpdateTracker(EVTCTrackerRole::LeftKnee,   MC_LeftKnee);
	UpdateTracker(EVTCTrackerRole::RightKnee,  MC_RightKnee);
	UpdateTracker(EVTCTrackerRole::LeftFoot,   MC_LeftFoot);
	UpdateTracker(EVTCTrackerRole::RightFoot,  MC_RightFoot);
}

void AVTC_TrackerPawn::UpdateTracker(EVTCTrackerRole TrackerRole, UMotionControllerComponent* MC)
{
	if (!MC) return;

	FVTCTrackerData& Data = TrackerDataMap.FindOrAdd(TrackerRole);
	Data.Role       = TrackerRole;
	Data.bIsTracked = MC->IsTracked();
	Data.bIsInterpolated = false;

	if (Data.bIsTracked)
	{
		Data.WorldLocation = MC->GetComponentLocation();
		Data.WorldRotation = MC->GetComponentRotation();

		// Dropout 보간: 추적 성공 시 히스토리 갱신 + 카운터 리셋
		TArray<FVector>& History = TrackerLocationHistory.FindOrAdd(TrackerRole);
		History.Add(Data.WorldLocation);
		if (History.Num() > 2) History.RemoveAt(0);
		DropoutFrameCount.FindOrAdd(TrackerRole) = 0;
	}
	else if (MaxDropoutFrames > 0)
	{
		// Dropout 보간 (Feature C): 추적 실패 시 히스토리 기반 선형 외삽
		int32& FrameCount = DropoutFrameCount.FindOrAdd(TrackerRole);
		const TArray<FVector>* History = TrackerLocationHistory.Find(TrackerRole);

		if (History && History->Num() >= 2 && FrameCount < MaxDropoutFrames)
		{
			const FVector& Prev  = (*History)[History->Num() - 2];
			const FVector& Last  = (*History)[History->Num() - 1];
			const FVector Velocity = Last - Prev;
			Data.WorldLocation = Last + Velocity * (FrameCount + 1);
			Data.bIsTracked = true;
			Data.bIsInterpolated = true;
			FrameCount++;
		}
	}

	if (bShowDebugSpheres && Data.bIsTracked)
	{
		FColor Color = FColor::Green;
		switch (TrackerRole)
		{
		case EVTCTrackerRole::Waist:      Color = FColor::Blue;   break;
		case EVTCTrackerRole::LeftKnee:   Color = FColor::Yellow; break;
		case EVTCTrackerRole::RightKnee:  Color = FColor::Orange; break;
		case EVTCTrackerRole::LeftFoot:   Color = FColor::Cyan;   break;
		case EVTCTrackerRole::RightFoot:  Color = FColor::Purple; break;
		}
		const float LineThickness = Data.bIsInterpolated ? 0.5f : 1.0f;
		DrawDebugSphere(GetWorld(), Data.WorldLocation, DebugSphereRadius, 8, Color, false, -1.0f, 0, LineThickness);
	}

	OnTrackerUpdated.Broadcast(TrackerRole, Data);
}

UMotionControllerComponent* AVTC_TrackerPawn::GetMotionController(EVTCTrackerRole TrackerRole) const
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

// ─── Movement Phase 감지 (Feature E) ─────────────────────────────────────────

void AVTC_TrackerPawn::DetectMovementPhase(float DeltaTime)
{
	const FVector CurrentHip = GetTrackerLocation(EVTCTrackerRole::Waist);
	if (CurrentHip.IsZero()) return;

	if (!bHasPreviousHipLocation)
	{
		PreviousHipLocation = CurrentHip;
		bHasPreviousHipLocation = true;
		return;
	}

	const float HipZVelocity = (DeltaTime > SMALL_NUMBER)
		? (CurrentHip.Z - PreviousHipLocation.Z) / DeltaTime
		: 0.0f;

	PreviousHipLocation = CurrentHip;

	EVTCMovementPhase NewPhase = CurrentPhase;

	if (FMath::Abs(HipZVelocity) < PhaseVelocityThreshold)
	{
		if (CurrentPhase == EVTCMovementPhase::Entering)
			NewPhase = EVTCMovementPhase::Seated;
		else if (CurrentPhase == EVTCMovementPhase::Unknown)
			NewPhase = EVTCMovementPhase::Stationary;
	}
	else if (HipZVelocity < -PhaseVelocityThreshold)
	{
		NewPhase = EVTCMovementPhase::Entering;
	}
	else if (HipZVelocity > PhaseVelocityThreshold)
	{
		NewPhase = EVTCMovementPhase::Exiting;
	}

	if (NewPhase != CurrentPhase)
	{
		const EVTCMovementPhase OldPhase = CurrentPhase;
		CurrentPhase = NewPhase;
		OnPhaseChanged.Broadcast(OldPhase, NewPhase);
	}
}

// ─── 착석 정렬 ────────────────────────────────────────────────────────────────

void AVTC_TrackerPawn::SnapWaistTo(const FVector& WorldPos)
{
	const FVector WaistWorld = GetTrackerLocation(EVTCTrackerRole::Waist);
	const FVector Delta = WorldPos - WaistWorld;
	SetActorLocation(GetActorLocation() + Delta);

	UE_LOG(LogTemp, Log,
		TEXT("[VTC] SnapWaistTo: Waist %.1f,%.1f,%.1f → Target %.1f,%.1f,%.1f (delta %.1f,%.1f,%.1f)"),
		WaistWorld.X, WaistWorld.Y, WaistWorld.Z,
		WorldPos.X,   WorldPos.Y,   WorldPos.Z,
		Delta.X,      Delta.Y,      Delta.Z);
}

// ─── Tracker Mesh 가시성 ─────────────────────────────────────────────────────

void AVTC_TrackerPawn::SetTrackerMeshVisible(bool bVisible)
{
	TArray<UMotionControllerComponent*> MCs = {
		MC_Waist, MC_LeftKnee, MC_RightKnee, MC_LeftFoot, MC_RightFoot
	};
	for (UMotionControllerComponent* MC : MCs)
	{
		if (MC) MC->SetVisibility(bVisible, true);
	}
}
