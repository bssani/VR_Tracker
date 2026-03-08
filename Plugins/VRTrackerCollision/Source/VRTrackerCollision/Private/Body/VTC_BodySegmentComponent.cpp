// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Body/VTC_BodySegmentComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/Package.h"

UVTC_BodySegmentComponent::UVTC_BodySegmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// SegmentMesh는 OnRegister()에서 생성한다.
	// 생성자에서 CreateDefaultSubobject<UStaticMeshComponent>("SegmentMesh")를 쓰면
	// AVTC_BodyActor가 이 컴포넌트를 4개 포함할 때 모두 같은 이름 "SegmentMesh"를
	// 동일한 Actor CDO에 등록해 UE 오브젝트 인스턴싱 과정에서 ensure 실패가 발생한다.
}

void UVTC_BodySegmentComponent::OnRegister()
{
	Super::OnRegister();

	// CDO 또는 아직 Owner(Actor)가 없는 경우 생성을 건너뜀.
	// IsTemplate(): CDO / 아키타입 컨텍스트이면 true → 런타임 인스턴스에서만 생성.
	if (IsTemplate() || !GetOwner() || SegmentMesh)
		return;

	// 컴포넌트 자신의 이름을 접두사로 사용해 고유 이름 생성.
	// 예) "Seg_RKnee_RFoot" → "Seg_RKnee_RFoot_SegMesh"
	// Actor 안에서 이름 충돌이 없으므로 CDO 인스턴싱 오류가 발생하지 않는다.
	const FName MeshName(*FString::Printf(TEXT("%s_SegMesh"), *GetName()));
	SegmentMesh = NewObject<UStaticMeshComponent>(GetOwner(), MeshName);
	SegmentMesh->SetupAttachment(this);
	SegmentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SegmentMesh->SetCastShadow(false);

	// ConstructorHelpers는 생성자 외부에서 사용할 수 없으므로 StaticLoadObject 사용
	UStaticMesh* CylinderMesh = Cast<UStaticMesh>(StaticLoadObject(
		UStaticMesh::StaticClass(), nullptr,
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
	if (CylinderMesh)
		SegmentMesh->SetStaticMesh(CylinderMesh);

	SegmentMesh->RegisterComponent();
}

void UVTC_BodySegmentComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateSegmentTransform();
}

void UVTC_BodySegmentComponent::UpdateSegmentTransform()
{
	if (!TrackerSource || !SegmentMesh) {
		// TrackerSource 없으면 숨기기 (tracker 미연결 시 segment 노출 방지)
		if (SegmentMesh) SegmentMesh->SetVisibility(false);
		return;
	}

	// 두 Tracker 위치 가져오기
	const FVTCTrackerData StartData = TrackerSource->GetTrackerData(RoleStart);
	const FVTCTrackerData EndData   = TrackerSource->GetTrackerData(RoleEnd);

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
	if (!TrackerSource) return false;
	return TrackerSource->IsTrackerActive(RoleStart)
		&& TrackerSource->IsTrackerActive(RoleEnd);
}

FVector UVTC_BodySegmentComponent::GetStartLocation() const
{
	if (!TrackerSource) return FVector::ZeroVector;
	return TrackerSource->GetTrackerLocation(RoleStart);
}

FVector UVTC_BodySegmentComponent::GetEndLocation() const
{
	if (!TrackerSource) return FVector::ZeroVector;
	return TrackerSource->GetTrackerLocation(RoleEnd);
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
