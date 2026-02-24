// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Body/VTC_BodySegmentComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

UVTC_BodySegmentComponent::UVTC_BodySegmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Cylinder Static Mesh 생성 (UE5 엔진 기본 Cylinder)
	SegmentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SegmentMesh"));
	SegmentMesh->SetupAttachment(this);
	SegmentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SegmentMesh->SetCastShadow(false);

	// 엔진 기본 Cylinder Mesh 로드
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		SegmentMesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void UVTC_BodySegmentComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateSegmentTransform();
}

void UVTC_BodySegmentComponent::UpdateSegmentTransform()
{
	if (!TrackerManager || !SegmentMesh) return;

	// 두 Tracker 위치 가져오기
	const FVTCTrackerData StartData = TrackerManager->GetTrackerData(RoleStart);
	const FVTCTrackerData EndData   = TrackerManager->GetTrackerData(RoleEnd);

	// 둘 다 추적 중일 때만 갱신
	if (!StartData.bIsTracked || !EndData.bIsTracked)
	{
		SegmentMesh->SetVisibility(false);
		return;
	}
	SegmentMesh->SetVisibility(true);

	const FVector PosA = StartData.WorldLocation;
	const FVector PosB = EndData.WorldLocation;

	// 1. 두 점 사이 중간 지점
	const FVector MidPoint = (PosA + PosB) * 0.5f;

	// 2. A → B 방향 (Cylinder는 Z축이 높이 방향)
	const FVector Direction = (PosB - PosA).GetSafeNormal();
	FRotator Rotation = Direction.ToOrientationRotator();
	// UE5 기본 Cylinder는 Z-Up이므로 90도 보정
	Rotation.Pitch -= 90.0f;

	// 3. 길이 계산 → Z 스케일
	CurrentLength = FVector::Dist(PosA, PosB);
	const float LengthScale  = CurrentLength / BaseCylinderHeight;
	const float RadiusScale  = SegmentRadius / 50.0f;  // 기본 반경 50cm 기준

	// 4. World Transform 적용
	SegmentMesh->SetWorldLocation(MidPoint);
	SegmentMesh->SetWorldRotation(Rotation);
	SegmentMesh->SetWorldScale3D(FVector(RadiusScale, RadiusScale, LengthScale));
}

bool UVTC_BodySegmentComponent::IsSegmentActive() const
{
	if (!TrackerManager) return false;
	return TrackerManager->IsTrackerActive(RoleStart)
		&& TrackerManager->IsTrackerActive(RoleEnd);
}

FVector UVTC_BodySegmentComponent::GetStartLocation() const
{
	if (!TrackerManager) return FVector::ZeroVector;
	return TrackerManager->GetTrackerLocation(RoleStart);
}

FVector UVTC_BodySegmentComponent::GetEndLocation() const
{
	if (!TrackerManager) return FVector::ZeroVector;
	return TrackerManager->GetTrackerLocation(RoleEnd);
}

void UVTC_BodySegmentComponent::SetSegmentColor(FLinearColor Color)
{
	if (!SegmentMesh) return;

	if (!DynamicMaterial)
	{
		DynamicMaterial = SegmentMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(FName("Color"), Color);
		DynamicMaterial->SetVectorParameterValue(FName("EmissiveColor"), Color * 0.5f);
	}
}
