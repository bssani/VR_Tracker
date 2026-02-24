// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Body/VTC_BodyActor.h"
#include "Tracker/VTC_TrackerManager.h"
#include "Kismet/GameplayStatics.h"

AVTC_BodyActor::AVTC_BodyActor()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// ── 세그먼트 4개 생성 ──
	Seg_Hip_LeftKnee = CreateDefaultSubobject<UVTC_BodySegmentComponent>(TEXT("Seg_Hip_LKnee"));
	Seg_Hip_LeftKnee->SetupAttachment(Root);
	Seg_Hip_LeftKnee->RoleStart = EVTCTrackerRole::Waist;
	Seg_Hip_LeftKnee->RoleEnd   = EVTCTrackerRole::LeftKnee;

	Seg_Hip_RightKnee = CreateDefaultSubobject<UVTC_BodySegmentComponent>(TEXT("Seg_Hip_RKnee"));
	Seg_Hip_RightKnee->SetupAttachment(Root);
	Seg_Hip_RightKnee->RoleStart = EVTCTrackerRole::Waist;
	Seg_Hip_RightKnee->RoleEnd   = EVTCTrackerRole::RightKnee;

	Seg_LeftKnee_LeftFoot = CreateDefaultSubobject<UVTC_BodySegmentComponent>(TEXT("Seg_LKnee_LFoot"));
	Seg_LeftKnee_LeftFoot->SetupAttachment(Root);
	Seg_LeftKnee_LeftFoot->RoleStart = EVTCTrackerRole::LeftKnee;
	Seg_LeftKnee_LeftFoot->RoleEnd   = EVTCTrackerRole::LeftFoot;

	Seg_RightKnee_RightFoot = CreateDefaultSubobject<UVTC_BodySegmentComponent>(TEXT("Seg_RKnee_RFoot"));
	Seg_RightKnee_RightFoot->SetupAttachment(Root);
	Seg_RightKnee_RightFoot->RoleStart = EVTCTrackerRole::RightKnee;
	Seg_RightKnee_RightFoot->RoleEnd   = EVTCTrackerRole::RightFoot;

	// ── 충돌 Sphere 5개 생성 ──
	auto MakeSphere = [&](const TCHAR* Name) -> USphereComponent*
	{
		USphereComponent* S = CreateDefaultSubobject<USphereComponent>(Name);
		S->SetupAttachment(Root);
		S->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		S->SetCollisionProfileName(TEXT("OverlapAll"));
		S->SetGenerateOverlapEvents(true);
		return S;
	};

	Sphere_Hip        = MakeSphere(TEXT("Sphere_Hip"));
	Sphere_LeftKnee   = MakeSphere(TEXT("Sphere_LKnee"));
	Sphere_RightKnee  = MakeSphere(TEXT("Sphere_RKnee"));
	Sphere_LeftFoot   = MakeSphere(TEXT("Sphere_LFoot"));
	Sphere_RightFoot  = MakeSphere(TEXT("Sphere_RFoot"));

	// ── 캘리브레이션 컴포넌트 ──
	CalibrationComp = CreateDefaultSubobject<UVTC_CalibrationComponent>(TEXT("CalibrationComp"));
}

void AVTC_BodyActor::BeginPlay()
{
	Super::BeginPlay();

	// TrackerManager 자동 탐색 (에디터에서 설정하지 않은 경우)
	if (!TrackerManager)
	{
		FindTrackerManager();
	}

	// 모든 세그먼트와 캘리브레이션에 TrackerManager 연결
	if (TrackerManager)
	{
		Seg_Hip_LeftKnee->TrackerManager        = TrackerManager;
		Seg_Hip_RightKnee->TrackerManager       = TrackerManager;
		Seg_LeftKnee_LeftFoot->TrackerManager   = TrackerManager;
		Seg_RightKnee_RightFoot->TrackerManager = TrackerManager;
		CalibrationComp->TrackerManager         = TrackerManager;
	}

	// Sphere 반경 초기화
	UpdateSphereRadii();

	// Overlap 이벤트 바인딩
	auto BindOverlap = [&](USphereComponent* S)
	{
		S->OnComponentBeginOverlap.AddDynamic(this, &AVTC_BodyActor::OnSphereOverlapBegin);
		S->OnComponentEndOverlap.AddDynamic(this, &AVTC_BodyActor::OnSphereOverlapEnd);
	};
	BindOverlap(Sphere_Hip);
	BindOverlap(Sphere_LeftKnee);
	BindOverlap(Sphere_RightKnee);
	BindOverlap(Sphere_LeftFoot);
	BindOverlap(Sphere_RightFoot);

	// 캘리브레이션 완료 이벤트 바인딩
	CalibrationComp->OnCalibrationComplete.AddDynamic(this, &AVTC_BodyActor::OnCalibrationComplete);
}

void AVTC_BodyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SyncSpherePositions();
}

void AVTC_BodyActor::SyncSpherePositions()
{
	if (!TrackerManager) return;

	auto SyncSphere = [&](USphereComponent* S, EVTCTrackerRole TrackerRole)
	{
		if (!S) return;
		const FVTCTrackerData Data = TrackerManager->GetTrackerData(TrackerRole);
		S->SetWorldLocation(Data.WorldLocation);
		S->SetVisibility(Data.bIsTracked);
	};

	SyncSphere(Sphere_Hip,       EVTCTrackerRole::Waist);
	SyncSphere(Sphere_LeftKnee,  EVTCTrackerRole::LeftKnee);
	SyncSphere(Sphere_RightKnee, EVTCTrackerRole::RightKnee);
	SyncSphere(Sphere_LeftFoot,  EVTCTrackerRole::LeftFoot);
	SyncSphere(Sphere_RightFoot, EVTCTrackerRole::RightFoot);
}

void AVTC_BodyActor::OnSphereOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this) return;
	EVTCTrackerRole TrackerRole = GetRoleFromSphere(OverlappedComp);
	OnBodyOverlapBegin.Broadcast(TrackerRole, OtherActor);
}

void AVTC_BodyActor::OnSphereOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor || OtherActor == this) return;
	EVTCTrackerRole TrackerRole = GetRoleFromSphere(OverlappedComp);
	OnBodyOverlapEnd.Broadcast(TrackerRole, OtherActor);
}

EVTCTrackerRole AVTC_BodyActor::GetRoleFromSphere(UPrimitiveComponent* Sphere) const
{
	if (Sphere == Sphere_Hip)       return EVTCTrackerRole::Waist;
	if (Sphere == Sphere_LeftKnee)  return EVTCTrackerRole::LeftKnee;
	if (Sphere == Sphere_RightKnee) return EVTCTrackerRole::RightKnee;
	if (Sphere == Sphere_LeftFoot)  return EVTCTrackerRole::LeftFoot;
	if (Sphere == Sphere_RightFoot) return EVTCTrackerRole::RightFoot;
	return EVTCTrackerRole::Waist;
}

void AVTC_BodyActor::OnCalibrationComplete(const FVTCBodyMeasurements& Measurements)
{
	UE_LOG(LogTemp, Log, TEXT("[VTC] BodyActor received calibration data. Height: %.1f cm"),
		Measurements.EstimatedHeight);
}

void AVTC_BodyActor::StartCalibration()
{
	if (CalibrationComp) CalibrationComp->StartCalibration();
}

FVTCBodyMeasurements AVTC_BodyActor::GetBodyMeasurements() const
{
	if (CalibrationComp) return CalibrationComp->GetBodyMeasurements();
	return FVTCBodyMeasurements();
}

FVector AVTC_BodyActor::GetBodyPartLocation(EVTCTrackerRole TrackerRole) const
{
	if (!TrackerManager) return FVector::ZeroVector;
	return TrackerManager->GetTrackerLocation(TrackerRole);
}

void AVTC_BodyActor::UpdateSphereRadii()
{
	if (Sphere_Hip)       Sphere_Hip->SetSphereRadius(HipSphereRadius);
	if (Sphere_LeftKnee)  Sphere_LeftKnee->SetSphereRadius(KneeSphereRadius);
	if (Sphere_RightKnee) Sphere_RightKnee->SetSphereRadius(KneeSphereRadius);
	if (Sphere_LeftFoot)  Sphere_LeftFoot->SetSphereRadius(FootSphereRadius);
	if (Sphere_RightFoot) Sphere_RightFoot->SetSphereRadius(FootSphereRadius);
}

void AVTC_BodyActor::FindTrackerManager()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVTC_TrackerManager::StaticClass(), Found);
	if (Found.Num() > 0)
	{
		TrackerManager = Cast<AVTC_TrackerManager>(Found[0]);
		UE_LOG(LogTemp, Log, TEXT("[VTC] BodyActor found TrackerManager automatically."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[VTC] BodyActor: TrackerManager not found in level!"));
	}
}
