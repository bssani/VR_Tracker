// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Body/VTC_BodyActor.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AVTC_BodyActor::AVTC_BodyActor() {
  PrimaryActorTick.bCanEverTick = true;

  USceneComponent *Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
  SetRootComponent(Root);

  // ── 세그먼트 4개 생성 ──
  Seg_Hip_LeftKnee =
      CreateDefaultSubobject<UVTC_BodySegmentComponent>(TEXT("Seg_Hip_LKnee"));
  Seg_Hip_LeftKnee->SetupAttachment(Root);
  Seg_Hip_LeftKnee->RoleStart = EVTCTrackerRole::Waist;
  Seg_Hip_LeftKnee->RoleEnd = EVTCTrackerRole::LeftKnee;

  Seg_Hip_RightKnee =
      CreateDefaultSubobject<UVTC_BodySegmentComponent>(TEXT("Seg_Hip_RKnee"));
  Seg_Hip_RightKnee->SetupAttachment(Root);
  Seg_Hip_RightKnee->RoleStart = EVTCTrackerRole::Waist;
  Seg_Hip_RightKnee->RoleEnd = EVTCTrackerRole::RightKnee;

  Seg_LeftKnee_LeftFoot = CreateDefaultSubobject<UVTC_BodySegmentComponent>(
      TEXT("Seg_LKnee_LFoot"));
  Seg_LeftKnee_LeftFoot->SetupAttachment(Root);
  Seg_LeftKnee_LeftFoot->RoleStart = EVTCTrackerRole::LeftKnee;
  Seg_LeftKnee_LeftFoot->RoleEnd = EVTCTrackerRole::LeftFoot;

  Seg_RightKnee_RightFoot = CreateDefaultSubobject<UVTC_BodySegmentComponent>(
      TEXT("Seg_RKnee_RFoot"));
  Seg_RightKnee_RightFoot->SetupAttachment(Root);
  Seg_RightKnee_RightFoot->RoleStart = EVTCTrackerRole::RightKnee;
  Seg_RightKnee_RightFoot->RoleEnd = EVTCTrackerRole::RightFoot;

  // ── 충돌 Sphere 5개 생성 ──
  auto MakeSphere = [&](const TCHAR *Name) -> USphereComponent * {
    USphereComponent *S = CreateDefaultSubobject<USphereComponent>(Name);
    S->SetupAttachment(Root);
    S->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    S->SetCollisionProfileName(TEXT("OverlapAll"));
    S->SetGenerateOverlapEvents(true);
    return S;
  };

  Sphere_Hip = MakeSphere(TEXT("Sphere_Hip"));
  Sphere_LeftKnee = MakeSphere(TEXT("Sphere_LKnee"));
  Sphere_RightKnee = MakeSphere(TEXT("Sphere_RKnee"));
  Sphere_LeftFoot = MakeSphere(TEXT("Sphere_LFoot"));
  Sphere_RightFoot = MakeSphere(TEXT("Sphere_RFoot"));

  // ── 시각화용 Sphere 메시 5개 생성 (VR에서 관절 위치 표시) ────────────────
  // USphereComponent는 메시가 없으므로 별도 StaticMesh로 렌더링.
  // 엔진 기본 Sphere(반경 50cm)를 사용하며 스케일로 실제 반경에 맞춤.
  static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
      TEXT("/Engine/BasicShapes/Sphere.Sphere"));

  auto MakeVisualSphere = [&](const TCHAR* Name) -> UStaticMeshComponent*
  {
    UStaticMeshComponent* M = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    M->SetupAttachment(Root);
    M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    M->SetCastShadow(false);
    if (SphereMeshAsset.Succeeded())
      M->SetStaticMesh(SphereMeshAsset.Object);
    return M;
  };

  VisualSphere_Hip       = MakeVisualSphere(TEXT("VisualSphere_Hip"));
  VisualSphere_LeftKnee  = MakeVisualSphere(TEXT("VisualSphere_LKnee"));
  VisualSphere_RightKnee = MakeVisualSphere(TEXT("VisualSphere_RKnee"));
  VisualSphere_LeftFoot  = MakeVisualSphere(TEXT("VisualSphere_LFoot"));
  VisualSphere_RightFoot = MakeVisualSphere(TEXT("VisualSphere_RFoot"));

  // ── 캘리브레이션 컴포넌트 ──
  CalibrationComp = CreateDefaultSubobject<UVTC_CalibrationComponent>(
      TEXT("CalibrationComp"));
}

void AVTC_BodyActor::BeginPlay() {
  Super::BeginPlay();

  // TrackerSource 자동 탐색 (에디터에서 설정하지 않은 경우)
  if (!TrackerSource) {
    FindTrackerSource();
  }

  // 모든 세그먼트와 캘리브레이션에 TrackerSource 연결
  if (TrackerSource) {
    Seg_Hip_LeftKnee->TrackerSource = TrackerSource;
    Seg_Hip_RightKnee->TrackerSource = TrackerSource;
    Seg_LeftKnee_LeftFoot->TrackerSource = TrackerSource;
    Seg_RightKnee_RightFoot->TrackerSource = TrackerSource;
    CalibrationComp->TrackerSource = TrackerSource;
  }

  // Sphere 반경 초기화
  UpdateSphereRadii();

  // Overlap 이벤트 바인딩
  auto BindOverlap = [&](USphereComponent *S) {
    S->OnComponentBeginOverlap.AddDynamic(
        this, &AVTC_BodyActor::OnSphereOverlapBegin);
    S->OnComponentEndOverlap.AddDynamic(this,
                                        &AVTC_BodyActor::OnSphereOverlapEnd);
  };
  BindOverlap(Sphere_Hip);
  BindOverlap(Sphere_LeftKnee);
  BindOverlap(Sphere_RightKnee);
  BindOverlap(Sphere_LeftFoot);
  BindOverlap(Sphere_RightFoot);

  // 캘리브레이션 완료 이벤트 바인딩
  CalibrationComp->OnCalibrationComplete.AddDynamic(
      this, &AVTC_BodyActor::OnCalibrationComplete);
}

void AVTC_BodyActor::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
  SyncSpherePositions();
}

void AVTC_BodyActor::SyncSpherePositions() {
  if (!TrackerSource)
    return;

  auto SyncSphere = [&](USphereComponent* S, UStaticMeshComponent* Visual,
                        EVTCTrackerRole TrackerRole)
  {
    if (!S) return;
    const FVTCTrackerData Data = TrackerSource->GetTrackerData(TrackerRole);

    // 마운트 오프셋 보정: 트래커 로컬 오프셋을 트래커 회전에 맞춰 월드로 변환
    const FVector Offset = GetMountOffsetForRole(TrackerRole);
    const FVector EffectiveLocation = Offset.IsNearlyZero()
        ? Data.WorldLocation
        : Data.WorldLocation + FQuat(Data.WorldRotation).RotateVector(Offset);

    S->SetWorldLocation(EffectiveLocation);
    S->SetVisibility(Data.bIsTracked);
    // 미연결 트래커는 충돌도 비활성화.
    // SetVisibility(false)는 렌더링만 끄고 충돌 판정은 유지하므로
    // 위치가 (0,0,0)에 고정된 sphere가 phantom overlap을 일으키는 것을 방지.
    S->SetCollisionEnabled(Data.bIsTracked
        ? ECollisionEnabled::QueryAndPhysics
        : ECollisionEnabled::NoCollision);

    // 시각화 메시도 동일 위치/가시성으로 동기화 (VR 렌더링용)
    if (Visual)
    {
      Visual->SetWorldLocation(EffectiveLocation);
      Visual->SetVisibility(Data.bIsTracked);
    }
  };

  SyncSphere(Sphere_Hip,       VisualSphere_Hip,       EVTCTrackerRole::Waist);
  SyncSphere(Sphere_LeftKnee,  VisualSphere_LeftKnee,  EVTCTrackerRole::LeftKnee);
  SyncSphere(Sphere_RightKnee, VisualSphere_RightKnee, EVTCTrackerRole::RightKnee);
  SyncSphere(Sphere_LeftFoot,  VisualSphere_LeftFoot,  EVTCTrackerRole::LeftFoot);
  SyncSphere(Sphere_RightFoot, VisualSphere_RightFoot, EVTCTrackerRole::RightFoot);
}

void AVTC_BodyActor::OnSphereOverlapBegin(UPrimitiveComponent *OverlappedComp,
                                          AActor *OtherActor,
                                          UPrimitiveComponent *OtherComp,
                                          int32 OtherBodyIndex, bool bFromSweep,
                                          const FHitResult &SweepResult) {
  if (!OtherActor || OtherActor == this)
    return;
  EVTCTrackerRole TrackerRole = GetRoleFromSphere(OverlappedComp);
  OnBodyOverlapBegin.Broadcast(TrackerRole, OtherActor);
}

void AVTC_BodyActor::OnSphereOverlapEnd(UPrimitiveComponent *OverlappedComp,
                                        AActor *OtherActor,
                                        UPrimitiveComponent *OtherComp,
                                        int32 OtherBodyIndex) {
  if (!OtherActor || OtherActor == this)
    return;
  EVTCTrackerRole TrackerRole = GetRoleFromSphere(OverlappedComp);
  OnBodyOverlapEnd.Broadcast(TrackerRole, OtherActor);
}

EVTCTrackerRole
AVTC_BodyActor::GetRoleFromSphere(UPrimitiveComponent *Sphere) const {
  if (Sphere == Sphere_Hip)
    return EVTCTrackerRole::Waist;
  if (Sphere == Sphere_LeftKnee)
    return EVTCTrackerRole::LeftKnee;
  if (Sphere == Sphere_RightKnee)
    return EVTCTrackerRole::RightKnee;
  if (Sphere == Sphere_LeftFoot)
    return EVTCTrackerRole::LeftFoot;
  if (Sphere == Sphere_RightFoot)
    return EVTCTrackerRole::RightFoot;
  return EVTCTrackerRole::Waist;
}

float AVTC_BodyActor::GetSphereRadiusForRole(
    EVTCTrackerRole TrackerRole) const {
  switch (TrackerRole) {
  case EVTCTrackerRole::Waist:
    return HipSphereRadius;
  case EVTCTrackerRole::LeftKnee:
    return KneeSphereRadius;
  case EVTCTrackerRole::RightKnee:
    return KneeSphereRadius;
  case EVTCTrackerRole::LeftFoot:
    return FootSphereRadius;
  case EVTCTrackerRole::RightFoot:
    return FootSphereRadius;
  }
  return 10.0f; // Default fallback
}

FVector AVTC_BodyActor::GetMountOffsetForRole(EVTCTrackerRole TrackerRole) const {
  switch (TrackerRole) {
  case EVTCTrackerRole::Waist:     return MountOffset_Waist;
  case EVTCTrackerRole::LeftKnee:  return MountOffset_LeftKnee;
  case EVTCTrackerRole::RightKnee: return MountOffset_RightKnee;
  case EVTCTrackerRole::LeftFoot:  return MountOffset_LeftFoot;
  case EVTCTrackerRole::RightFoot: return MountOffset_RightFoot;
  }
  return FVector::ZeroVector;
}

void AVTC_BodyActor::OnCalibrationComplete(
    const FVTCBodyMeasurements &Measurements) {
  UE_LOG(LogTemp, Log,
         TEXT("[VTC] BodyActor received calibration data. Height: %.1f cm"),
         Measurements.EstimatedHeight);
}

void AVTC_BodyActor::StartCalibration() {
  if (CalibrationComp)
    CalibrationComp->StartCalibration();
}

FVTCBodyMeasurements AVTC_BodyActor::GetBodyMeasurements() const {
  if (CalibrationComp)
    return CalibrationComp->GetBodyMeasurements();
  return FVTCBodyMeasurements();
}

FVector AVTC_BodyActor::GetBodyPartLocation(EVTCTrackerRole TrackerRole) const {
  if (!TrackerSource)
    return FVector::ZeroVector;

  const FVector Offset = GetMountOffsetForRole(TrackerRole);
  if (Offset.IsNearlyZero())
    return TrackerSource->GetTrackerLocation(TrackerRole);

  // 오프셋이 있으면 트래커 회전을 포함해 월드 위치 보정
  const FVTCTrackerData Data = TrackerSource->GetTrackerData(TrackerRole);
  return Data.WorldLocation + FQuat(Data.WorldRotation).RotateVector(Offset);
}

void AVTC_BodyActor::UpdateSphereRadii() {
  if (Sphere_Hip)
    Sphere_Hip->SetSphereRadius(HipSphereRadius);
  if (Sphere_LeftKnee)
    Sphere_LeftKnee->SetSphereRadius(KneeSphereRadius);
  if (Sphere_RightKnee)
    Sphere_RightKnee->SetSphereRadius(KneeSphereRadius);
  if (Sphere_LeftFoot)
    Sphere_LeftFoot->SetSphereRadius(FootSphereRadius);
  if (Sphere_RightFoot)
    Sphere_RightFoot->SetSphereRadius(FootSphereRadius);

  // 엔진 기본 Sphere는 반경 50cm → 실제 반경에 맞게 균등 스케일 적용
  auto SetVisualScale = [](UStaticMeshComponent* M, float Radius)
  {
    if (M)
    {
      const float S = Radius / 50.0f;
      M->SetRelativeScale3D(FVector(S));
    }
  };
  SetVisualScale(VisualSphere_Hip,       HipSphereRadius);
  SetVisualScale(VisualSphere_LeftKnee,  KneeSphereRadius);
  SetVisualScale(VisualSphere_RightKnee, KneeSphereRadius);
  SetVisualScale(VisualSphere_LeftFoot,  FootSphereRadius);
  SetVisualScale(VisualSphere_RightFoot, FootSphereRadius);
}

void AVTC_BodyActor::FindTrackerSource() {
  // 레벨에서 IVTC_TrackerInterface 구현체(TrackerPawn) 탐색
  TArray<AActor *> Found;
  UGameplayStatics::GetAllActorsWithInterface(
      GetWorld(), UVTC_TrackerInterface::StaticClass(), Found);
  if (Found.Num() > 0) {
    TrackerSource = TScriptInterface<IVTC_TrackerInterface>(Found[0]);
    UE_LOG(LogTemp, Log, TEXT("[VTC] BodyActor found tracker source: %s"),
           *Found[0]->GetName());
  } else {
    UE_LOG(LogTemp, Warning,
           TEXT("[VTC] BodyActor: No TrackerPawn found in level!"));
  }
}
