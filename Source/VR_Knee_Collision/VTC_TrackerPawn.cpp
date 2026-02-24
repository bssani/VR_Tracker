// Copyright 2025 VR_Tracker Project. All Rights Reserved.

#include "VTC_TrackerPawn.h"
#include "DrawDebugHelpers.h"

// ──────────────────────────────────────────────────────────────────────────────
// 생성자 — 컴포넌트 생성 및 기본 계층 구조 설정
// ──────────────────────────────────────────────────────────────────────────────
AVTC_TrackerPawn::AVTC_TrackerPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    // 루트: VR Origin (SteamVR 의 트래킹 공간 원점)
    VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
    SetRootComponent(VROrigin);

    // HMD 카메라 (VR 시점)
    HMDCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("HMDCamera"));
    HMDCamera->SetupAttachment(VROrigin);
    HMDCamera->bLockToHmd = true; // HMD 움직임에 따라 자동 업데이트

    // ── 트래커 컴포넌트 생성 ──────────────────────────────────────────────────
    // 모두 VROrigin 에 부착 (트래킹 공간 기준으로 위치 읽기)

    TrackerHip = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("TrackerHip"));
    TrackerHip->SetupAttachment(VROrigin);
    TrackerHip->MotionSource = MotionSource_Hip; // 기본값: "Special_1"

    TrackerLeftKnee = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("TrackerLeftKnee"));
    TrackerLeftKnee->SetupAttachment(VROrigin);
    TrackerLeftKnee->MotionSource = MotionSource_LeftKnee;

    TrackerRightKnee = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("TrackerRightKnee"));
    TrackerRightKnee->SetupAttachment(VROrigin);
    TrackerRightKnee->MotionSource = MotionSource_RightKnee;

    TrackerLeftFoot = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("TrackerLeftFoot"));
    TrackerLeftFoot->SetupAttachment(VROrigin);
    TrackerLeftFoot->MotionSource = MotionSource_LeftFoot;

    TrackerRightFoot = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("TrackerRightFoot"));
    TrackerRightFoot->SetupAttachment(VROrigin);
    TrackerRightFoot->MotionSource = MotionSource_RightFoot;

    // ── 오프셋 기본값 (실제 부착 측정 후 Blueprint Details 에서 덮어씀) ──────
    // 무릎 트래커: 무릎 앞면에 부착 시 관절 중심은 약 3cm 뒤
    LeftKneeOffset.LocalSpaceOffset  = FVector(-3.0f, 0.0f, 0.0f);
    RightKneeOffset.LocalSpaceOffset = FVector(-3.0f, 0.0f, 0.0f);
    LeftKneeOffset.DebugColor        = FColor::Cyan;
    RightKneeOffset.DebugColor       = FColor::Yellow;

    // 허리 트래커: 벨트 뒤쪽에 부착 → 골반 중심은 앞쪽, 아래쪽
    HipOffset.LocalSpaceOffset = FVector(10.0f, 0.0f, -5.0f);
    HipOffset.DebugColor       = FColor::Orange;

    // 발목 트래커: 복숭아뼈 위에 부착 → 관절 중심은 약간 아래
    LeftFootOffset.LocalSpaceOffset  = FVector(0.0f, 0.0f, -2.0f);
    RightFootOffset.LocalSpaceOffset = FVector(0.0f, 0.0f, -2.0f);
    LeftFootOffset.DebugColor        = FColor::Blue;
    RightFootOffset.DebugColor       = FColor::Magenta;
}

// ──────────────────────────────────────────────────────────────────────────────
// BeginPlay — MotionSource 이름 적용
// ──────────────────────────────────────────────────────────────────────────────
void AVTC_TrackerPawn::BeginPlay()
{
    Super::BeginPlay();

    // Blueprint Details 패널에서 설정한 MotionSource 이름을
    // 실제 컴포넌트에 적용 (생성자에서 설정된 기본값을 덮어씀)
    ApplyMotionSources();
}

// ──────────────────────────────────────────────────────────────────────────────
// Tick — 디버그 시각화
// ──────────────────────────────────────────────────────────────────────────────
void AVTC_TrackerPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bShowDebugVisuals)
    {
        return;
    }

    DrawDebugForTracker(TrackerHip,        HipOffset,        TEXT("Hip"));
    DrawDebugForTracker(TrackerLeftKnee,   LeftKneeOffset,   TEXT("L.Knee"));
    DrawDebugForTracker(TrackerRightKnee,  RightKneeOffset,  TEXT("R.Knee"));
    DrawDebugForTracker(TrackerLeftFoot,   LeftFootOffset,   TEXT("L.Foot"));
    DrawDebugForTracker(TrackerRightFoot,  RightFootOffset,  TEXT("R.Foot"));
}

// ──────────────────────────────────────────────────────────────────────────────
// 보정된 관절 위치 반환 함수들
// ──────────────────────────────────────────────────────────────────────────────

FVector AVTC_TrackerPawn::GetHipPosition() const
{
    return ApplyOffset(TrackerHip, HipOffset);
}

FVector AVTC_TrackerPawn::GetLeftKneePosition() const
{
    return ApplyOffset(TrackerLeftKnee, LeftKneeOffset);
}

FVector AVTC_TrackerPawn::GetRightKneePosition() const
{
    return ApplyOffset(TrackerRightKnee, RightKneeOffset);
}

FVector AVTC_TrackerPawn::GetLeftFootPosition() const
{
    return ApplyOffset(TrackerLeftFoot, LeftFootOffset);
}

FVector AVTC_TrackerPawn::GetRightFootPosition() const
{
    return ApplyOffset(TrackerRightFoot, RightFootOffset);
}

void AVTC_TrackerPawn::GetAllJointPositions(
    FVector& OutHip,
    FVector& OutLeftKnee,
    FVector& OutRightKnee,
    FVector& OutLeftFoot,
    FVector& OutRightFoot) const
{
    OutHip        = GetHipPosition();
    OutLeftKnee   = GetLeftKneePosition();
    OutRightKnee  = GetRightKneePosition();
    OutLeftFoot   = GetLeftFootPosition();
    OutRightFoot  = GetRightFootPosition();
}

// ──────────────────────────────────────────────────────────────────────────────
// 트래커 상태 조회
// ──────────────────────────────────────────────────────────────────────────────

bool AVTC_TrackerPawn::IsTrackerActive(EVTCTrackerType TrackerType) const
{
    UMotionControllerComponent* Tracker = nullptr;

    switch (TrackerType)
    {
        case EVTCTrackerType::Hip:        Tracker = TrackerHip;        break;
        case EVTCTrackerType::LeftKnee:   Tracker = TrackerLeftKnee;   break;
        case EVTCTrackerType::RightKnee:  Tracker = TrackerRightKnee;  break;
        case EVTCTrackerType::LeftFoot:   Tracker = TrackerLeftFoot;   break;
        case EVTCTrackerType::RightFoot:  Tracker = TrackerRightFoot;  break;
        default: return false;
    }

    return Tracker && Tracker->IsTracked();
}

int32 AVTC_TrackerPawn::GetActiveTrackerCount() const
{
    int32 Count = 0;
    if (TrackerHip       && TrackerHip->IsTracked())       ++Count;
    if (TrackerLeftKnee  && TrackerLeftKnee->IsTracked())  ++Count;
    if (TrackerRightKnee && TrackerRightKnee->IsTracked()) ++Count;
    if (TrackerLeftFoot  && TrackerLeftFoot->IsTracked())  ++Count;
    if (TrackerRightFoot && TrackerRightFoot->IsTracked()) ++Count;
    return Count;
}

// ──────────────────────────────────────────────────────────────────────────────
// Runtime 오프셋 변경
// ──────────────────────────────────────────────────────────────────────────────

void AVTC_TrackerPawn::SetTrackerOffset(EVTCTrackerType TrackerType, const FVTCTrackerOffsetConfig& NewOffset)
{
    switch (TrackerType)
    {
        case EVTCTrackerType::Hip:        HipOffset        = NewOffset; break;
        case EVTCTrackerType::LeftKnee:   LeftKneeOffset   = NewOffset; break;
        case EVTCTrackerType::RightKnee:  RightKneeOffset  = NewOffset; break;
        case EVTCTrackerType::LeftFoot:   LeftFootOffset   = NewOffset; break;
        case EVTCTrackerType::RightFoot:  RightFootOffset  = NewOffset; break;
        default: break;
    }
}

FVTCTrackerOffsetConfig AVTC_TrackerPawn::GetTrackerOffset(EVTCTrackerType TrackerType) const
{
    switch (TrackerType)
    {
        case EVTCTrackerType::Hip:        return HipOffset;
        case EVTCTrackerType::LeftKnee:   return LeftKneeOffset;
        case EVTCTrackerType::RightKnee:  return RightKneeOffset;
        case EVTCTrackerType::LeftFoot:   return LeftFootOffset;
        case EVTCTrackerType::RightFoot:  return RightFootOffset;
        default:                          return FVTCTrackerOffsetConfig();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Private: 오프셋 계산 핵심 로직
// ──────────────────────────────────────────────────────────────────────────────

FVector AVTC_TrackerPawn::ApplyOffset(
    UMotionControllerComponent* Tracker,
    const FVTCTrackerOffsetConfig& Config) const
{
    if (!Tracker)
    {
        return FVector::ZeroVector;
    }

    const FVector TrackerWorldPos = Tracker->GetComponentLocation();

    // 오프셋이 0이면 바로 반환 (불필요한 연산 스킵)
    if (Config.LocalSpaceOffset.IsNearlyZero())
    {
        return TrackerWorldPos;
    }

    if (Config.bUseLocalSpace)
    {
        // 트래커 로컬 공간 오프셋 → 월드 공간으로 변환
        // 트래커가 어떤 방향으로 회전해도 관절 상대 위치가 유지됨
        //
        // 예: 트래커가 앞을 향하든 옆을 향하든
        //     "트래커 앞에서 3cm" 는 항상 트래커 앞 방향 3cm
        const FVector WorldOffset = Tracker->GetComponentRotation().RotateVector(Config.LocalSpaceOffset);
        return TrackerWorldPos + WorldOffset;
    }
    else
    {
        // 월드 공간 오프셋 (단순 덧셈)
        // 예: "항상 위쪽으로 5cm" 같은 수직 보정에 적합
        return TrackerWorldPos + Config.LocalSpaceOffset;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Private: 디버그 시각화
// ──────────────────────────────────────────────────────────────────────────────

void AVTC_TrackerPawn::DrawDebugForTracker(
    UMotionControllerComponent* Tracker,
    const FVTCTrackerOffsetConfig& Config,
    const FString& Label) const
{
    if (!Tracker || !GetWorld())
    {
        return;
    }

    const bool bIsTracked = Tracker->IsTracked();

    // 추적 안 됨 → 빨간 X 표시
    if (!bIsTracked)
    {
        const FVector Pos = Tracker->GetComponentLocation();
        DrawDebugSphere(GetWorld(), Pos, Config.DebugSphereRadius, 8,
                        FColor::Red, false, -1.f, 0, 0.5f);
        DrawDebugString(GetWorld(), Pos + FVector(0, 0, 10),
                        Label + TEXT(" [NO SIGNAL]"), nullptr, FColor::Red, 0.f, true);
        return;
    }

    // 원시 트래커 위치 (흰색 작은 점)
    const FVector RawPos = Tracker->GetComponentLocation();
    DrawDebugSphere(GetWorld(), RawPos, Config.DebugSphereRadius * 0.4f, 8,
                    FColor::White, false, -1.f, 0, 0.3f);

    // 오프셋이 있을 때만 보정 위치 표시
    if (!Config.LocalSpaceOffset.IsNearlyZero() && Config.bShowDebugSphere)
    {
        const FVector CorrectedPos = ApplyOffset(Tracker, Config);

        // 보정된 관절 위치 (설정한 색상)
        DrawDebugSphere(GetWorld(), CorrectedPos, Config.DebugSphereRadius, 12,
                        Config.DebugColor, false, -1.f, 0, 1.f);

        // 트래커 → 관절 연결선
        DrawDebugLine(GetWorld(), RawPos, CorrectedPos,
                      Config.DebugColor, false, -1.f, 0, 0.5f);

        // 라벨 (보정 위치 위에 표시)
        const FString OffsetStr = FString::Printf(
            TEXT("%s\nRaw:(%.1f,%.1f,%.1f)\nJoint:(%.1f,%.1f,%.1f)"),
            *Label,
            RawPos.X, RawPos.Y, RawPos.Z,
            CorrectedPos.X, CorrectedPos.Y, CorrectedPos.Z);

        DrawDebugString(GetWorld(), CorrectedPos + FVector(0, 0, 12),
                        OffsetStr, nullptr, Config.DebugColor, 0.f, true, 0.8f);
    }
    else
    {
        // 오프셋 없을 때 트래커 위치만 라벨
        DrawDebugString(GetWorld(), RawPos + FVector(0, 0, 10),
                        Label, nullptr, FColor::White, 0.f, true, 0.8f);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Private: MotionSource 이름 적용
// ──────────────────────────────────────────────────────────────────────────────

void AVTC_TrackerPawn::ApplyMotionSources()
{
    // Blueprint Details 패널에서 변경된 MotionSource 이름을
    // 각 컴포넌트에 반영 (컴포넌트 생성 시점에는 UPROPERTY 기본값이
    // 아직 적용되지 않았을 수 있으므로 BeginPlay 에서 명시적으로 설정)
    if (TrackerHip)        TrackerHip->MotionSource       = MotionSource_Hip;
    if (TrackerLeftKnee)   TrackerLeftKnee->MotionSource  = MotionSource_LeftKnee;
    if (TrackerRightKnee)  TrackerRightKnee->MotionSource = MotionSource_RightKnee;
    if (TrackerLeftFoot)   TrackerLeftFoot->MotionSource  = MotionSource_LeftFoot;
    if (TrackerRightFoot)  TrackerRightFoot->MotionSource = MotionSource_RightFoot;
}
