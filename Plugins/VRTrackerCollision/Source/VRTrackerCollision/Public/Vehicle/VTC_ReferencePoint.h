// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_ReferencePoint.h — 차량 내 기준점 Actor (에어컨, 대시보드 등 돌출 부위)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BillboardComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "VTC_ReferencePoint.generated.h"

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VTC Reference Point"))
class VRTRACKERCOLLISION_API AVTC_ReferencePoint : public AActor
{
	GENERATED_BODY()

public:
	AVTC_ReferencePoint();

	// ─── 식별 정보 ───────────────────────────────────────────────────────────

	// 차량 부품 이름 (데이터 로그, HUD에 표시됨)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Reference Point")
	FString PartName = TEXT("Vehicle Part");

	// 어느 신체 부위와 측정할지 (LeftKnee, RightKnee 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Reference Point")
	TArray<EVTCTrackerRole> RelevantBodyParts;

	// 이 기준점이 활성화 상태인지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Reference Point")
	bool bActive = true;

	// ─── 시각화 ─────────────────────────────────────────────────────────────

	// 에디터/게임에서 기준점 위치를 보여주는 Marker Mesh (작은 Sphere)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Reference Point")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	// 에디터에서 보이는 Billboard (스프라이트 아이콘)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Reference Point")
	TObjectPtr<UBillboardComponent> EditorBillboard;

	// 마커 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Reference Point",
		meta=(ClampMin=1.0f, ClampMax=20.0f))
	float MarkerRadius = 5.0f;

	// 기준점 색상 (에디터에서 구분하기 쉽게)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Reference Point")
	FLinearColor MarkerColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f); // 오렌지

	// ─── 함수 ────────────────────────────────────────────────────────────────

	// 월드 위치 반환 (ActorLocation과 동일하지만 BP 편의용)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Reference Point")
	FVector GetReferenceLocation() const;

	// 마커 색상 업데이트 (경고 상태에 따라)
	UFUNCTION(BlueprintCallable, Category = "VTC|Reference Point")
	void SetMarkerWarningColor(EVTCWarningLevel Level);

	// 원래 색상으로 복원
	UFUNCTION(BlueprintCallable, Category = "VTC|Reference Point")
	void ResetMarkerColor();

private:
	TObjectPtr<UMaterialInstanceDynamic> MarkerDynMaterial;
	FLinearColor OriginalColor;
};
