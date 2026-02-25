// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_BodySegmentComponent.h — 두 Tracker 사이를 Cylinder로 동적 연결

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "Tracker/VTC_TrackerInterface.h"
#include "VTC_BodySegmentComponent.generated.h"

UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="VKC Body Segment"))
class VRTRACKERCOLLISION_API UVTC_BodySegmentComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UVTC_BodySegmentComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// ─── 설정 ────────────────────────────────────────────────────────────────

	// 이 세그먼트의 시작점 Tracker 역할 (예: Waist)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body Segment")
	EVTCTrackerRole RoleStart = EVTCTrackerRole::Waist;

	// 이 세그먼트의 끝점 Tracker 역할 (예: LeftKnee)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body Segment")
	EVTCTrackerRole RoleEnd = EVTCTrackerRole::LeftKnee;

	// Cylinder 반경 (cm) — 체형에 따라 조절 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body Segment",
		meta=(ClampMin=1.0f, ClampMax=30.0f, UIMin=1.0f, UIMax=30.0f))
	float SegmentRadius = 8.0f;

	// Tracker 공급자 (TrackerPawn)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body Segment")
	TScriptInterface<IVTC_TrackerInterface> TrackerSource;

	// ─── 읽기 전용 정보 ───────────────────────────────────────────────────────

	// 현재 프레임의 세그먼트 길이 (cm)
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VKC|Body Segment")
	float CurrentLength = 0.0f;

	// 시각화용 Cylinder Mesh (엔진 기본 Cylinder 사용, 높이 기준 100cm)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body Segment")
	TObjectPtr<UStaticMeshComponent> SegmentMesh;

	// ─── 함수 ────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Body Segment")
	bool IsSegmentActive() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Body Segment")
	FVector GetStartLocation() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Body Segment")
	FVector GetEndLocation() const;

	UFUNCTION(BlueprintCallable, Category = "VKC|Body Segment")
	void SetSegmentColor(FLinearColor Color);

private:
	static constexpr float BaseCylinderHeight = 100.0f;

	void UpdateSegmentTransform();

	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
};
