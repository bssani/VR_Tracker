// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Vehicle/VTC_ReferencePoint.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

AVTC_ReferencePoint::AVTC_ReferencePoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// 마커 Sphere Mesh
	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(Root);
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		MarkerMesh->SetStaticMesh(SphereMesh.Object);
	}

	// 기본 스케일 (반경 5cm → Sphere 기본 크기 50cm 기준으로 0.1 스케일)
	const float Scale = MarkerRadius / 50.0f;
	MarkerMesh->SetWorldScale3D(FVector(Scale));

	// 기본 기준점 관련 신체 부위: 양쪽 무릎
	RelevantBodyParts.Add(EVTCTrackerRole::LeftKnee);
	RelevantBodyParts.Add(EVTCTrackerRole::RightKnee);

	OriginalColor = MarkerColor;

#if WITH_EDITOR
	EditorBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
	EditorBillboard->SetupAttachment(Root);
#endif
}

FVector AVTC_ReferencePoint::GetReferenceLocation() const
{
	return GetActorLocation();
}

void AVTC_ReferencePoint::SetMarkerWarningColor(EVTCWarningLevel Level)
{
	if (!MarkerMesh) return;

	if (!MarkerDynMaterial)
	{
		MarkerDynMaterial = MarkerMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!MarkerDynMaterial) return;

	FLinearColor Color = FLinearColor::Green;
	switch (Level)
	{
	case EVTCWarningLevel::Safe:
		Color = FLinearColor::Green;
		break;
	case EVTCWarningLevel::Warning:
		Color = FLinearColor::Yellow;
		break;
	case EVTCWarningLevel::Collision:
		Color = FLinearColor::Red;
		break;
	default:
		break;
	}

	MarkerDynMaterial->SetVectorParameterValue(FName("BaseColor"), Color);
	MarkerDynMaterial->SetScalarParameterValue(FName("EmissiveIntensity"),
		Level == EVTCWarningLevel::Collision ? 3.0f : 1.0f);
}

void AVTC_ReferencePoint::ResetMarkerColor()
{
	if (MarkerDynMaterial)
	{
		MarkerDynMaterial->SetVectorParameterValue(FName("BaseColor"), OriginalColor);
		MarkerDynMaterial->SetScalarParameterValue(FName("EmissiveIntensity"), 1.0f);
	}
}
