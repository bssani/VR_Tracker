// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_BodyActor.h — 가상 신체 전체 (세그먼트 4개 + 충돌 Sphere 5개)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Body/VTC_BodySegmentComponent.h"
#include "Body/VTC_CalibrationComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
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

	// TrackerManager 연결 (에디터에서 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body")
	TObjectPtr<AVTC_TrackerManager> TrackerManager;

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

	// 캘리브레이션 시작 (BP에서 호출)
	UFUNCTION(BlueprintCallable, Category = "VKC|Body")
	void StartCalibration();

	// 마지막 신체 측정값 반환
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Body")
	FVTCBodyMeasurements GetBodyMeasurements() const;

	// 특정 신체 부위의 현재 위치 반환
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Body")
	FVector GetBodyPartLocation(EVTCTrackerRole TrackerRole) const;

	// Sphere 반경 일괄 업데이트
	UFUNCTION(BlueprintCallable, Category = "VKC|Body")
	void UpdateSphereRadii();

	// ─── Delegates ──────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "VKC|Body|Events")
	FOnVKCBodyOverlapBegin OnBodyOverlapBegin;

	UPROPERTY(BlueprintAssignable, Category = "VKC|Body|Events")
	FOnVKCBodyOverlapEnd OnBodyOverlapEnd;

private:
	// 매 Tick: Sphere 위치를 Tracker 위치와 동기화
	void SyncSpherePositions();

	// Overlap 이벤트 핸들러
	UFUNCTION()
	void OnSphereOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Sphere → TrackerRole 매핑 (Overlap 시 어느 신체 부위인지 판단)
	EVTCTrackerRole GetRoleFromSphere(UPrimitiveComponent* Sphere) const;

	// 캘리브레이션 완료 시 콜백
	UFUNCTION()
	void OnCalibrationComplete(const FVTCBodyMeasurements& Measurements);

	// TrackerManager 자동 탐색
	void FindTrackerManager();
};
