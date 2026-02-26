// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_BodyActor.h — 가상 신체 전체 (세그먼트 4개 + 충돌 Sphere 5개)

#pragma once

#include "Body/VTC_BodySegmentComponent.h"
#include "Body/VTC_CalibrationComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tracker/VTC_TrackerInterface.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "VTC_BodyActor.generated.h"


// Sphere가 차량 메시와 Overlap 시작 시 브로드캐스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVTCBodyOverlapBegin,
                                             EVTCTrackerRole, BodyPart,
                                             AActor *, VehicleMesh);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVTCBodyOverlapEnd,
                                             EVTCTrackerRole, BodyPart,
                                             AActor *, VehicleMesh);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Body Actor"))
class VRTRACKERCOLLISION_API AVTC_BodyActor : public AActor {
  GENERATED_BODY()

public:
  AVTC_BodyActor();

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaTime) override;

public:
  // ─── 세그먼트 (시각화 + 길이 측정) ──────────────────────────────────────

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Segments")
  TObjectPtr<UVTC_BodySegmentComponent> Seg_Hip_LeftKnee;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Segments")
  TObjectPtr<UVTC_BodySegmentComponent> Seg_Hip_RightKnee;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Segments")
  TObjectPtr<UVTC_BodySegmentComponent> Seg_LeftKnee_LeftFoot;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Segments")
  TObjectPtr<UVTC_BodySegmentComponent> Seg_RightKnee_RightFoot;

  // ─── 충돌 감지용 Sphere (5개) ─────────────────────────────────────────────

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Collision")
  TObjectPtr<USphereComponent> Sphere_Hip;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Collision")
  TObjectPtr<USphereComponent> Sphere_LeftKnee;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Collision")
  TObjectPtr<USphereComponent> Sphere_RightKnee;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Collision")
  TObjectPtr<USphereComponent> Sphere_LeftFoot;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Collision")
  TObjectPtr<USphereComponent> Sphere_RightFoot;

  // ─── 시각화용 Sphere 메시 (VR 렌더링, 5개) ───────────────────────────────
  // USphereComponent는 메시가 없어 VR HMD에서 보이지 않으므로
  // UStaticMeshComponent(Engine Sphere)를 별도로 추가해 관절 위치를 표시한다.

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Visual")
  TObjectPtr<UStaticMeshComponent> VisualSphere_Hip;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Visual")
  TObjectPtr<UStaticMeshComponent> VisualSphere_LeftKnee;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Visual")
  TObjectPtr<UStaticMeshComponent> VisualSphere_RightKnee;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Visual")
  TObjectPtr<UStaticMeshComponent> VisualSphere_LeftFoot;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Body|Visual")
  TObjectPtr<UStaticMeshComponent> VisualSphere_RightFoot;

  // ─── 캘리브레이션 ─────────────────────────────────────────────────────────

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
            Category = "VTC|Body|Calibration")
  TObjectPtr<UVTC_CalibrationComponent> CalibrationComp;

  // ─── 설정 ────────────────────────────────────────────────────────────────

  // Tracker 공급자 — 비워두면 BeginPlay에서 TrackerPawn을 자동 탐색
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Body")
  TScriptInterface<IVTC_TrackerInterface> TrackerSource;

  // 신체 부위별 Sphere 반경 (cm)
  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "VTC|Body|Collision Radius",
            meta = (ClampMin = 3.0f, ClampMax = 20.0f))
  float HipSphereRadius = 12.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "VTC|Body|Collision Radius",
            meta = (ClampMin = 3.0f, ClampMax = 20.0f))
  float KneeSphereRadius = 8.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "VTC|Body|Collision Radius",
            meta = (ClampMin = 3.0f, ClampMax = 20.0f))
  float FootSphereRadius = 10.0f;

  // ── 마운트 오프셋 (트래커 → 실제 신체 접촉점, 트래커 로컬 공간 기준) ──────
  // Vive Tracker가 신체 표면보다 약간 돌출되어 있을 때 그 오프셋을
  // 트래커 로컬 좌표(cm)로 입력하면 실제 접촉점 위치로 보정됩니다.
  // 기본값 ZeroVector = 오프셋 없음 (하위 호환 유지)

  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "VTC|Body|Mount Offset",
            meta = (DisplayName = "Waist Mount Offset (Tracker Local, cm)"))
  FVector MountOffset_Waist = FVector::ZeroVector;

  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "VTC|Body|Mount Offset",
            meta = (DisplayName = "Left Knee Mount Offset (Tracker Local, cm)"))
  FVector MountOffset_LeftKnee = FVector::ZeroVector;

  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "VTC|Body|Mount Offset",
            meta = (DisplayName = "Right Knee Mount Offset (Tracker Local, cm)"))
  FVector MountOffset_RightKnee = FVector::ZeroVector;

  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "VTC|Body|Mount Offset",
            meta = (DisplayName = "Left Foot Mount Offset (Tracker Local, cm)"))
  FVector MountOffset_LeftFoot = FVector::ZeroVector;

  UPROPERTY(EditAnywhere, BlueprintReadWrite,
            Category = "VTC|Body|Mount Offset",
            meta = (DisplayName = "Right Foot Mount Offset (Tracker Local, cm)"))
  FVector MountOffset_RightFoot = FVector::ZeroVector;

  // ─── 함수 ────────────────────────────────────────────────────────────────

  UFUNCTION(BlueprintCallable, Category = "VTC|Body")
  void StartCalibration();

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Body")
  FVTCBodyMeasurements GetBodyMeasurements() const;

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Body")
  FVector GetBodyPartLocation(EVTCTrackerRole TrackerRole) const;

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Body")
  float GetSphereRadiusForRole(EVTCTrackerRole TrackerRole) const;

  UFUNCTION(BlueprintCallable, Category = "VTC|Body")
  void UpdateSphereRadii();

  // ─── Delegates ──────────────────────────────────────────────────────────

  UPROPERTY(BlueprintAssignable, Category = "VTC|Body|Events")
  FOnVTCBodyOverlapBegin OnBodyOverlapBegin;

  UPROPERTY(BlueprintAssignable, Category = "VTC|Body|Events")
  FOnVTCBodyOverlapEnd OnBodyOverlapEnd;

private:
  void SyncSpherePositions();

  // 역할에 해당하는 마운트 오프셋 반환 (트래커 로컬 공간)
  FVector GetMountOffsetForRole(EVTCTrackerRole TrackerRole) const;

  UFUNCTION()
  void OnSphereOverlapBegin(UPrimitiveComponent *OverlappedComp,
                            AActor *OtherActor, UPrimitiveComponent *OtherComp,
                            int32 OtherBodyIndex, bool bFromSweep,
                            const FHitResult &SweepResult);

  UFUNCTION()
  void OnSphereOverlapEnd(UPrimitiveComponent *OverlappedComp,
                          AActor *OtherActor, UPrimitiveComponent *OtherComp,
                          int32 OtherBodyIndex);

  EVTCTrackerRole GetRoleFromSphere(UPrimitiveComponent *Sphere) const;

  UFUNCTION()
  void OnCalibrationComplete(const FVTCBodyMeasurements &Measurements);

  // 레벨에서 TrackerPawn을 자동 탐색
  void FindTrackerSource();
};
