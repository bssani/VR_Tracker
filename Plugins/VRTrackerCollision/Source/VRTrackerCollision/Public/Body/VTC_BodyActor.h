// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_BodyActor.h — 가상 신체 전체 (세그먼트 4개 + 충돌 Sphere 5개)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Body/VTC_BodySegmentComponent.h"
#include "Body/VTC_CalibrationComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "Tracker/VTC_TrackerInterface.h"
#include "VTC_BodyActor.generated.h"

// Sphere가 차량 메시와 Overlap 시작 시 브로드캐스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVKCBodyOverlapBegin,
	EVTCTrackerRole, BodyPart, AActor*, VehicleMesh);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVKCBodyOverlapEnd,
	EVTCTrackerRole, BodyPart, AActor*, VehicleMesh);

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VKC Body Actor"))
class VRTRACKERCOLLISION_API AVTC_BodyActor : public AActor
{
	GENERATED_BODY()

public:
	AVTC_BodyActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ─── 세그먼트 (시각화 + 길이 측정) ──────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body|Segments")
	TObjectPtr<UVTC_BodySegmentComponent> Seg_Hip_LeftKnee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body|Segments")
	TObjectPtr<UVTC_BodySegmentComponent> Seg_Hip_RightKnee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body|Segments")
	TObjectPtr<UVTC_BodySegmentComponent> Seg_LeftKnee_LeftFoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body|Segments")
	TObjectPtr<UVTC_BodySegmentComponent> Seg_RightKnee_RightFoot;

	// ─── 충돌 감지용 Sphere (5개) ─────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body|Collision")
	TObjectPtr<USphereComponent> Sphere_Hip;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body|Collision")
	TObjectPtr<USphereComponent> Sphere_LeftKnee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body|Collision")
	TObjectPtr<USphereComponent> Sphere_RightKnee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body|Collision")
	TObjectPtr<USphereComponent> Sphere_LeftFoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body|Collision")
	TObjectPtr<USphereComponent> Sphere_RightFoot;

	// ─── 캘리브레이션 ─────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Body|Calibration")
	TObjectPtr<UVTC_CalibrationComponent> CalibrationComp;

	// ─── 설정 ────────────────────────────────────────────────────────────────

	// Tracker 공급자 — TrackerPawn 또는 TrackerManager 모두 할당 가능
	// 비워두면 BeginPlay에서 레벨 내 구현체를 자동 탐색
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body")
	TScriptInterface<IVTC_TrackerInterface> TrackerManager;

	// 신체 부위별 Sphere 반경 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body|Collision Radius",
		meta=(ClampMin=3.0f, ClampMax=20.0f))
	float HipSphereRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body|Collision Radius",
		meta=(ClampMin=3.0f, ClampMax=20.0f))
	float KneeSphereRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body|Collision Radius",
		meta=(ClampMin=3.0f, ClampMax=20.0f))
	float FootSphereRadius = 10.0f;

	// ─── 함수 ────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "VKC|Body")
	void StartCalibration();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Body")
	FVTCBodyMeasurements GetBodyMeasurements() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Body")
	FVector GetBodyPartLocation(EVTCTrackerRole TrackerRole) const;

	UFUNCTION(BlueprintCallable, Category = "VKC|Body")
	void UpdateSphereRadii();

	// ─── Delegates ──────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "VKC|Body|Events")
	FOnVKCBodyOverlapBegin OnBodyOverlapBegin;

	UPROPERTY(BlueprintAssignable, Category = "VKC|Body|Events")
	FOnVKCBodyOverlapEnd OnBodyOverlapEnd;

private:
	void SyncSpherePositions();

	UFUNCTION()
	void OnSphereOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	EVTCTrackerRole GetRoleFromSphere(UPrimitiveComponent* Sphere) const;

	UFUNCTION()
	void OnCalibrationComplete(const FVTCBodyMeasurements& Measurements);

	// 인터페이스 구현체(TrackerPawn 또는 TrackerManager)를 레벨에서 자동 탐색
	void FindTrackerManager();
};
