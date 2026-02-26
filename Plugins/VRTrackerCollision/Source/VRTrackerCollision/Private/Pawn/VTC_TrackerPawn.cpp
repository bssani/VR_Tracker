// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Pawn/VTC_TrackerPawn.h"
#include "DrawDebugHelpers.h"
#include "IXRTrackingSystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AVTC_TrackerPawn::AVTC_TrackerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// ── 계층 구조 생성 ─────────────────────────────────────────────────────
	// Root → VROrigin → Camera + 5 MotionControllers
	// VROrigin을 별도로 두는 이유:
	//   SteamVR/OpenXR은 바닥을 트래킹 원점으로 사용한다.
	//   Pawn 루트는 고정하고 VROrigin 이하만 HMD 추적 공간을 공유하게 하면
	//   좌표계 오프셋 조정이 VROrigin 위치 변경만으로 해결된다.

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

	// ── 시뮬레이션 이동 컴포넌트 ──────────────────────────────────────────────
	// VR 장비 없이 데스크탑에서 테스트할 때 WASD로 Pawn을 이동시킨다.
	SimMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("SimMovement"));

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

	// ── HMD 자동 감지 → 시뮬레이션 모드 전환 ──────────────────────────────
	// bAutoDetectSimulation = true이면 HMD가 미연결 시 자동으로 시뮬레이션 모드.
	// VR Preview 실행 시에는 HMD가 감지되어 자동으로 VR 모드로 유지된다.
	if (bAutoDetectSimulation && !DetectHMDPresence())
	{
		bSimulationMode = true;
		UE_LOG(LogTemp, Log, TEXT("[VTC] HMD not detected -> Simulation mode ON (Desktop Test)"));
	}

	// ── 시뮬레이션 이동 컴포넌트 속도 설정 ────────────────────────────────
	if (SimMovement)
	{
		SimMovement->MaxSpeed    = SimMoveSpeed;
		SimMovement->Acceleration = 2000.f;
		SimMovement->Deceleration = 4000.f;
	}

	// ── Enhanced Input: 시뮬레이션 매핑 컨텍스트 등록 ──────────────────────
	// BeginPlay 시점에 Controller가 Possess된 이후라야 Subsystem에 접근 가능
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (SimInputMappingContext)
			{
				Subsystem->AddMappingContext(SimInputMappingContext, 0);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[VTC] SimInputMappingContext가 설정되지 않았습니다. "
					"BP_VTC_TrackerPawn Details > VTC|Simulation|Input에서 IMC 에셋을 연결하세요."));
			}
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[VTC] TrackerPawn initialized. MotionSources: %s / %s / %s / %s / %s | SimMode: %s"),
		*MotionSource_Waist.ToString(),
		*MotionSource_LeftKnee.ToString(),
		*MotionSource_RightKnee.ToString(),
		*MotionSource_LeftFoot.ToString(),
		*MotionSource_RightFoot.ToString(),
		bSimulationMode ? TEXT("ON") : TEXT("OFF"));
}

void AVTC_TrackerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bSimulationMode)
	{
		// ── 마우스 Yaw → Actor 전체 회전 (전방 방향 변경) ──────────────────
		if (!FMath::IsNearlyZero(SimYawInput))
		{
			AddActorLocalRotation(FRotator(0.f, SimYawInput, 0.f));
			SimYawInput = 0.f;
		}

		// ── 마우스 Pitch → Camera만 회전 (-80~80도 클램프) ─────────────────
		// Actor 전체를 회전시키면 착석 자세 오프셋이 틀어지므로 카메라만 회전
		if (!FMath::IsNearlyZero(SimPitchInput) && Camera)
		{
			const float CurPitch = Camera->GetRelativeRotation().Pitch;
			const float NewPitch = FMath::Clamp(CurPitch + SimPitchInput, -80.f, 80.f);
			Camera->SetRelativeRotation(FRotator(NewPitch, 0.f, 0.f));
			SimPitchInput = 0.f;
		}

		UpdateSimulatedTrackers();
	}
	else
	{
		// VR 모드: 실제 Tracker에서 데이터 읽기
		UpdateAllTrackers();
	}

	OnAllTrackersUpdated.Broadcast();
}

void AVTC_TrackerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VTC] Enhanced Input Component를 찾지 못했습니다. "
			     "Project Settings > Engine > Input > Default Input Component Class = "
			     "EnhancedInputComponent 로 설정되어 있는지 확인하세요."));
		return;
	}

	// Triggered: 키를 누르는 동안 매 프레임 호출 (연속 입력)
	if (IA_Move)            EIC->BindAction(IA_Move,            ETriggerEvent::Triggered, this, &AVTC_TrackerPawn::SimMove);
	if (IA_Look)            EIC->BindAction(IA_Look,            ETriggerEvent::Triggered, this, &AVTC_TrackerPawn::SimLook);
	if (IA_AdjustLeftKnee)  EIC->BindAction(IA_AdjustLeftKnee,  ETriggerEvent::Triggered, this, &AVTC_TrackerPawn::SimAdjustLeftKnee);
	if (IA_AdjustRightKnee) EIC->BindAction(IA_AdjustRightKnee, ETriggerEvent::Triggered, this, &AVTC_TrackerPawn::SimAdjustRightKnee);

	// Started: 키를 처음 눌렀을 때 1회만 호출 (토글/리셋)
	if (IA_ToggleSim)       EIC->BindAction(IA_ToggleSim,       ETriggerEvent::Started,   this, &AVTC_TrackerPawn::ToggleSimulationMode);
	if (IA_ResetKnees)      EIC->BindAction(IA_ResetKnees,      ETriggerEvent::Started,   this, &AVTC_TrackerPawn::ResetKneeAdjustments);
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

	if (Data.bIsTracked)
	{
		Data.WorldLocation = MC->GetComponentLocation();
		Data.WorldRotation = MC->GetComponentRotation();
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
		DrawDebugSphere(GetWorld(), Data.WorldLocation, DebugSphereRadius, 8, Color, false, -1.0f, 0, 1.0f);
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

// ─── 시뮬레이션 모드 — 제어 함수 ───────────────────────────────────────────

void AVTC_TrackerPawn::ToggleSimulationMode()
{
	bSimulationMode = !bSimulationMode;
	UE_LOG(LogTemp, Log, TEXT("[VTC] Simulation mode: %s"),
		bSimulationMode ? TEXT("ON") : TEXT("OFF"));
}

void AVTC_TrackerPawn::ResetKneeAdjustments()
{
	SimKneeAdjust_Left  = FVector::ZeroVector;
	SimKneeAdjust_Right = FVector::ZeroVector;
	UE_LOG(LogTemp, Log, TEXT("[VTC] Knee adjustments reset."));
}

// ─── 시뮬레이션 모드 — Tracker 위치 계산 ────────────────────────────────────

void AVTC_TrackerPawn::UpdateSimulatedTrackers()
{
	// 카메라(머리) 위치를 기준으로 각 신체 부위 오프셋을 월드 좌표로 변환.
	// SimOffset_* 는 착석 자세를 가정한 카메라 로컬 오프셋 (cm, 전방X/우Y/위Z).
	// SimKneeAdjust_* 는 런타임에 NumPad/Arrow 키로 추가 조절하는 값.

	UpdateSimulatedTracker(EVTCTrackerRole::Waist,
		SimOffsetToWorld(SimOffset_Waist));

	UpdateSimulatedTracker(EVTCTrackerRole::LeftKnee,
		SimOffsetToWorld(SimOffset_LeftKnee + SimKneeAdjust_Left));

	UpdateSimulatedTracker(EVTCTrackerRole::RightKnee,
		SimOffsetToWorld(SimOffset_RightKnee + SimKneeAdjust_Right));

	UpdateSimulatedTracker(EVTCTrackerRole::LeftFoot,
		SimOffsetToWorld(SimOffset_LeftFoot));

	UpdateSimulatedTracker(EVTCTrackerRole::RightFoot,
		SimOffsetToWorld(SimOffset_RightFoot));
}

void AVTC_TrackerPawn::UpdateSimulatedTracker(EVTCTrackerRole TrackerRole,
	const FVector& WorldLocation)
{
	FVTCTrackerData& Data = TrackerDataMap.FindOrAdd(TrackerRole);
	Data.Role          = TrackerRole;
	Data.bIsTracked    = true;          // 시뮬레이션에서는 항상 추적 중
	Data.WorldLocation = WorldLocation;
	Data.WorldRotation = FRotator::ZeroRotator;

	if (bShowDebugSpheres)
	{
		FColor Color = FColor::White;
		switch (TrackerRole)
		{
		case EVTCTrackerRole::Waist:      Color = FColor::Blue;   break;
		case EVTCTrackerRole::LeftKnee:   Color = FColor::Yellow; break;
		case EVTCTrackerRole::RightKnee:  Color = FColor::Orange; break;
		case EVTCTrackerRole::LeftFoot:   Color = FColor::Cyan;   break;
		case EVTCTrackerRole::RightFoot:  Color = FColor::Purple; break;
		}
		// 시뮬레이션 구는 1.5배 크게 그려서 VR 모드와 시각적으로 구분
		DrawDebugSphere(GetWorld(), WorldLocation, DebugSphereRadius * 1.5f,
			8, Color, false, -1.0f, 0, 1.5f);
	}

	OnTrackerUpdated.Broadcast(TrackerRole, Data);
}

FVector AVTC_TrackerPawn::SimOffsetToWorld(const FVector& LocalOffset) const
{
	if (!Camera) return GetActorLocation();

	// 카메라 월드 위치 기준 + Actor의 Yaw만 적용.
	// Pitch/Roll을 제외해야 착석 자세에서 무릎 오프셋이 바닥 기준으로 고정된다.
	// (카메라를 위아래로 봐도 무릎은 바닥에 있어야 한다)
	const FVector CamWorldPos = Camera->GetComponentLocation();
	const FQuat   YawOnlyRot  = FQuat(FRotator(0.f, GetActorRotation().Yaw, 0.f));

	return CamWorldPos + YawOnlyRot.RotateVector(LocalOffset);
}

// ─── 시뮬레이션 모드 — Enhanced Input 핸들러 ────────────────────────────────

void AVTC_TrackerPawn::SimMove(const FInputActionValue& Value)
{
	if (!bSimulationMode) return;
	const FVector2D MoveVec = Value.Get<FVector2D>();
	if (!FMath::IsNearlyZero(MoveVec.X)) AddMovementInput(GetActorForwardVector(), MoveVec.X);
	if (!FMath::IsNearlyZero(MoveVec.Y)) AddMovementInput(GetActorRightVector(),   MoveVec.Y);
}

void AVTC_TrackerPawn::SimLook(const FInputActionValue& Value)
{
	// SimYawInput / SimPitchInput에 누적 → Tick에서 실제 회전 적용
	if (!bSimulationMode) return;
	const FVector2D LookVec = Value.Get<FVector2D>();
	SimYawInput   = LookVec.X * SimMouseSensitivity;
	SimPitchInput = LookVec.Y * SimMouseSensitivity;
}

void AVTC_TrackerPawn::SimAdjustLeftKnee(const FInputActionValue& Value)
{
	if (!bSimulationMode) return;
	const FVector2D Adj = Value.Get<FVector2D>();
	const float DT = GetWorld()->GetDeltaSeconds();
	SimKneeAdjust_Left.X += Adj.X * SimKneeAdjustSpeed * DT;
	SimKneeAdjust_Left.Y += Adj.Y * SimKneeAdjustSpeed * DT;
}

void AVTC_TrackerPawn::SimAdjustRightKnee(const FInputActionValue& Value)
{
	if (!bSimulationMode) return;
	const FVector2D Adj = Value.Get<FVector2D>();
	const float DT = GetWorld()->GetDeltaSeconds();
	SimKneeAdjust_Right.X += Adj.X * SimKneeAdjustSpeed * DT;
	SimKneeAdjust_Right.Y += Adj.Y * SimKneeAdjustSpeed * DT;
}

// ─── HMD 감지 ────────────────────────────────────────────────────────────────

bool AVTC_TrackerPawn::DetectHMDPresence() const
{
	// UHeadMountedDisplayFunctionLibrary는 UE5.1에서 deprecated되어 링크 오류 유발.
	// GEngine->XRSystem을 직접 참조해 HMD 연결 여부를 확인한다.
	return GEngine && GEngine->XRSystem.IsValid()
		&& GEngine->XRSystem->IsHeadTrackingAllowed();
}
