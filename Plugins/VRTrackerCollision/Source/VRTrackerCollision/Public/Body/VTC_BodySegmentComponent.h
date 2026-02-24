// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_BodySegmentComponent.h — 두 Tracker 사이를 Cylinder로 동적 연결

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "VTC_BodySegmentComponent.generated.h"

class AVTC_TrackerManager;

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

	// TrackerManager 참조 (레벨에 배치된 TrackerManager Actor 연결)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body Segment")
	TObjectPtr<AVTC_TrackerManager> TrackerManager;

	// ─── 읽기 전용 정보 ───────────────────────────────────────────────────────

	// 현재 프레임의 세그먼트 길이 (cm)
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VKC|Body Segment")
	float CurrentLength = 0.0f;

	// 시각화용 Cylinder Mesh (엔진 기본 Cylinder 사용, 높이 기준 100cm)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body Segment")
	TObjectPtr<UStaticMeshComponent> SegmentMesh;

	// ─── 함수 ────────────────────────────────────────────────────────────────

	// 현재 세그먼트가 유효하게 추적 중인지
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Body Segment")
	bool IsSegmentActive() const;

	// 시작/끝 Tracker 위치 반환
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Body Segment")
	FVector GetStartLocation() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Body Segment")
	FVector GetEndLocation() const;

	// 세그먼트 색상 변경 (경고 단계에 따라)
	UFUNCTION(BlueprintCallable, Category = "VKC|Body Segment")
	void SetSegmentColor(FLinearColor Color);

private:
	// 기본 Cylinder 높이 (UE5 기본 Cylinder Mesh 높이 = 100cm)
	static constexpr float BaseCylinderHeight = 100.0f;

	// 매 Tick: Tracker 위치 기반으로 Cylinder 위치/방향/스케일 갱신
	void UpdateSegmentTransform();

	// Dynamic Material Instance (색상 변경용)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
};
