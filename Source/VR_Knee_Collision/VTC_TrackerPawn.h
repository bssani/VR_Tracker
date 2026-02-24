// Copyright 2025 VR_Tracker Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MotionControllerComponent.h"
#include "Camera/CameraComponent.h"
#include "VTC_TrackerPawn.generated.h"

// ──────────────────────────────────────────────────────────────────────────────
// Tracker 종류 열거형
// ──────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EVTCTrackerType : uint8
{
    Hip         UMETA(DisplayName = "Hip / Waist"),
    LeftKnee    UMETA(DisplayName = "Left Knee"),
    RightKnee   UMETA(DisplayName = "Right Knee"),
    LeftFoot    UMETA(DisplayName = "Left Foot / Ankle"),
    RightFoot   UMETA(DisplayName = "Right Foot / Ankle"),
};

// ──────────────────────────────────────────────────────────────────────────────
// 트래커 오프셋 설정 구조체
//
// 왜 오프셋이 필요한가?
//   Vive Tracker 는 스트랩으로 신체에 부착되며, 트래커 중심과
//   실제 관절 중심(예: 무릎 관절) 사이에 물리적 거리가 있습니다.
//   이 오프셋을 보정해야 정확한 충돌 거리를 측정할 수 있습니다.
//
// LocalSpaceOffset 좌표계:
//   X = 트래커 앞(Forward)  → 양수: 트래커 전면 방향
//   Y = 트래커 오른쪽(Right) → 양수: 오른쪽
//   Z = 트래커 위(Up)        → 양수: 위쪽
//
// 사용 예시 (무릎 트래커가 무릎 앞면에 부착된 경우):
//   실제 무릎 관절 중심은 트래커보다 3cm 뒤에 있음
//   → LocalSpaceOffset = (-3, 0, 0)
// ──────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FVTCTrackerOffsetConfig
{
    GENERATED_BODY()

    // ── 오프셋 값 ──────────────────────────────────────────────────────────────

    /**
     * 트래커 로컬 공간 기준 오프셋 (단위: cm)
     * X = 트래커 앞방향 / Y = 트래커 오른쪽 / Z = 트래커 위
     *
     * bUseLocalSpace = true  → 트래커가 회전해도 관절 상대 위치 유지 (권장)
     * bUseLocalSpace = false → 월드 축 기준 고정 오프셋 (단순 수직 보정 등에 사용)
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset",
              meta = (ClampMin = "-50.0", ClampMax = "50.0",
                      ToolTip = "Offset from tracker center to actual joint center (cm).\nX=Forward, Y=Right, Z=Up (in tracker local space)"))
    FVector LocalSpaceOffset;

    /**
     * true  → LocalSpaceOffset 을 트래커 회전에 따라 월드 공간으로 변환 (권장)
     * false → LocalSpaceOffset 을 월드 공간에 그대로 더함 (수직 보정 등 단순 케이스)
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset",
              meta = (ToolTip = "If true, offset is in tracker's local space (rotates with tracker).\nIf false, offset is in world space (global axes)."))
    bool bUseLocalSpace;

    // ── 디버그 시각화 ────────────────────────────────────────────────────────

    /** 보정된 관절 위치에 디버그 구체를 표시 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug",
              meta = (ToolTip = "Draw a debug sphere at the corrected joint position"))
    bool bShowDebugSphere;

    /** 디버그 구체 색상 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug",
              meta = (EditCondition = "bShowDebugSphere"))
    FColor DebugColor;

    /** 디버그 구체 반지름 (cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug",
              meta = (EditCondition = "bShowDebugSphere", ClampMin = "1.0", ClampMax = "20.0"))
    float DebugSphereRadius;

    // ── 기본값 ──────────────────────────────────────────────────────────────
    FVTCTrackerOffsetConfig()
        : LocalSpaceOffset(FVector::ZeroVector)
        , bUseLocalSpace(true)
        , bShowDebugSphere(true)
        , DebugColor(FColor::Green)
        , DebugSphereRadius(4.0f)
    {}
};


// ──────────────────────────────────────────────────────────────────────────────
// VTC_TrackerPawn
//
// SteamVR Vive Tracker 5개 (허리, 양쪽 무릎, 양쪽 발목) 를 읽어
// 각 관절의 보정된 월드 위치를 제공하는 Pawn 클래스.
//
// Blueprint에서 BP_VTC_TrackerPawn 으로 상속 후:
//   1. Details > VTC|Motion Sources 에서 MotionSource 이름 설정
//      (SteamVR Tracker 기본값: Special_1 ~ Special_5)
//   2. Details > VTC|Tracker Offsets 에서 각 트래커별 오프셋 설정
// ──────────────────────────────────────────────────────────────────────────────
UCLASS(Blueprintable, BlueprintType,
       meta = (ShortTooltip = "VR body tracker pawn for vehicle collision testing"))
class VR_KNEE_COLLISION_API AVTC_TrackerPawn : public APawn
{
    GENERATED_BODY()

public:
    AVTC_TrackerPawn();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // ──────────────────────────────────────────────────────────────────────────
    // 컴포넌트
    // ──────────────────────────────────────────────────────────────────────────

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Components")
    TObjectPtr<USceneComponent> VROrigin;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Components")
    TObjectPtr<UCameraComponent> HMDCamera;

    /** 허리 트래커 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Trackers")
    TObjectPtr<UMotionControllerComponent> TrackerHip;

    /** 왼쪽 무릎 트래커 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Trackers")
    TObjectPtr<UMotionControllerComponent> TrackerLeftKnee;

    /** 오른쪽 무릎 트래커 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Trackers")
    TObjectPtr<UMotionControllerComponent> TrackerRightKnee;

    /** 왼쪽 발목 트래커 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Trackers")
    TObjectPtr<UMotionControllerComponent> TrackerLeftFoot;

    /** 오른쪽 발목 트래커 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Trackers")
    TObjectPtr<UMotionControllerComponent> TrackerRightFoot;

    // ──────────────────────────────────────────────────────────────────────────
    // Motion Source 이름 설정
    // SteamVR Vive Tracker: "Special_1" ~ "Special_5"
    // OpenXR 기반일 경우 다를 수 있음
    // ──────────────────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Motion Sources",
              meta = (ToolTip = "SteamVR motion source name for the HIP tracker (e.g. Special_1)"))
    FName MotionSource_Hip = FName("Special_1");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Motion Sources",
              meta = (ToolTip = "SteamVR motion source name for the LEFT KNEE tracker (e.g. Special_2)"))
    FName MotionSource_LeftKnee = FName("Special_2");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Motion Sources",
              meta = (ToolTip = "SteamVR motion source name for the RIGHT KNEE tracker (e.g. Special_3)"))
    FName MotionSource_RightKnee = FName("Special_3");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Motion Sources",
              meta = (ToolTip = "SteamVR motion source name for the LEFT FOOT tracker (e.g. Special_4)"))
    FName MotionSource_LeftFoot = FName("Special_4");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Motion Sources",
              meta = (ToolTip = "SteamVR motion source name for the RIGHT FOOT tracker (e.g. Special_5)"))
    FName MotionSource_RightFoot = FName("Special_5");

    // ──────────────────────────────────────────────────────────────────────────
    // 트래커 오프셋 설정
    // 트래커 부착 후 실제 측정한 관절까지의 거리를 입력합니다.
    // ──────────────────────────────────────────────────────────────────────────

    /** 허리 트래커 오프셋 (트래커 중심 → 실제 골반/허리 중심) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker Offsets",
              meta = (ShowOnlyInnerProperties))
    FVTCTrackerOffsetConfig HipOffset;

    /** 왼쪽 무릎 트래커 오프셋 (트래커 중심 → 실제 왼쪽 무릎 관절 중심) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker Offsets",
              meta = (ShowOnlyInnerProperties))
    FVTCTrackerOffsetConfig LeftKneeOffset;

    /** 오른쪽 무릎 트래커 오프셋 (트래커 중심 → 실제 오른쪽 무릎 관절 중심) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker Offsets",
              meta = (ShowOnlyInnerProperties))
    FVTCTrackerOffsetConfig RightKneeOffset;

    /** 왼쪽 발목 트래커 오프셋 (트래커 중심 → 실제 왼쪽 발목 관절 중심) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker Offsets",
              meta = (ShowOnlyInnerProperties))
    FVTCTrackerOffsetConfig LeftFootOffset;

    /** 오른쪽 발목 트래커 오프셋 (트래커 중심 → 실제 오른쪽 발목 관절 중심) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker Offsets",
              meta = (ShowOnlyInnerProperties))
    FVTCTrackerOffsetConfig RightFootOffset;

    // ──────────────────────────────────────────────────────────────────────────
    // 디버그 전역 설정
    // ──────────────────────────────────────────────────────────────────────────

    /** 모든 트래커 디버그 시각화 활성화 (개발 시에만 사용) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Debug")
    bool bShowDebugVisuals = false;

    // ──────────────────────────────────────────────────────────────────────────
    // 보정된 관절 위치 반환 함수 (Blueprint + C++ 접근 가능)
    // 반환값 = 트래커 월드 위치 + 오프셋 적용 후 실제 관절 중심 위치
    // ──────────────────────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Joint Positions",
              meta = (ToolTip = "Returns the corrected world position of the HIP joint (tracker pos + offset)"))
    FVector GetHipPosition() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Joint Positions",
              meta = (ToolTip = "Returns the corrected world position of the LEFT KNEE joint"))
    FVector GetLeftKneePosition() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Joint Positions",
              meta = (ToolTip = "Returns the corrected world position of the RIGHT KNEE joint"))
    FVector GetRightKneePosition() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Joint Positions",
              meta = (ToolTip = "Returns the corrected world position of the LEFT FOOT / ANKLE joint"))
    FVector GetLeftFootPosition() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Joint Positions",
              meta = (ToolTip = "Returns the corrected world position of the RIGHT FOOT / ANKLE joint"))
    FVector GetRightFootPosition() const;

    /**
     * 5개 관절의 보정된 위치를 한 번에 반환
     * Blueprint 에서 하나의 노드로 모든 위치를 얻을 때 사용
     */
    UFUNCTION(BlueprintCallable, Category = "VTC|Joint Positions")
    void GetAllJointPositions(
        FVector& OutHip,
        FVector& OutLeftKnee,
        FVector& OutRightKnee,
        FVector& OutLeftFoot,
        FVector& OutRightFoot) const;

    /**
     * 특정 트래커가 현재 추적 중인지 여부
     * SteamVR 이 해당 트래커를 인식하고 있으면 true
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Status")
    bool IsTrackerActive(EVTCTrackerType TrackerType) const;

    /**
     * 활성 트래커 개수 반환 (0~5)
     * HUD 에서 "3/5 trackers active" 표시 등에 사용
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Status")
    int32 GetActiveTrackerCount() const;

    /**
     * Runtime 에 오프셋 변경 (피험자 교체 시 재보정 등)
     * 예: 다른 체형의 피험자로 교체할 때 오프셋 값 업데이트
     */
    UFUNCTION(BlueprintCallable, Category = "VTC|Tracker Offsets",
              meta = (ToolTip = "Update offset config for a specific tracker at runtime (e.g. when subject changes)"))
    void SetTrackerOffset(EVTCTrackerType TrackerType, const FVTCTrackerOffsetConfig& NewOffset);

    /** 현재 설정된 오프셋 반환 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Tracker Offsets")
    FVTCTrackerOffsetConfig GetTrackerOffset(EVTCTrackerType TrackerType) const;

private:
    /**
     * 핵심 오프셋 계산 함수
     * 트래커의 현재 월드 위치에 오프셋 설정을 적용하여 실제 관절 위치 반환
     *
     * @param Tracker  오프셋을 적용할 MotionController 컴포넌트
     * @param Config   해당 트래커의 오프셋 설정
     * @return         보정된 관절 월드 위치
     */
    FVector ApplyOffset(UMotionControllerComponent* Tracker, const FVTCTrackerOffsetConfig& Config) const;

    /** 디버그 구체 + 라벨 렌더링 */
    void DrawDebugForTracker(
        UMotionControllerComponent* Tracker,
        const FVTCTrackerOffsetConfig& Config,
        const FString& Label) const;

    /** MotionSource 이름 변수를 각 MotionController 컴포넌트에 반영 */
    void ApplyMotionSources();
};
