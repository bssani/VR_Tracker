# VR Knee Collision Test — Full Architecture & System Flow

---

## 1. Tracker 배치 (5개)

```
        [HMD - Vive Pro 2]
               │
        ┌──────▼──────┐
        │  Head (HMD) │  ← 방향 기준 (전면/후면 판단)
        └─────────────┘
               │
        ┌──────▼──────┐
        │  [Tracker 0]│  ← Waist / Hip (골반 중앙)
        └──────┬──────┘
       ┌───────┴───────┐
       ▼               ▼
┌──────────┐     ┌──────────┐
│[Tracker1]│     │[Tracker2]│  ← Left Knee / Right Knee (슬개골 위)
└────┬─────┘     └─────┬────┘
     ▼                 ▼
┌──────────┐     ┌──────────┐
│[Tracker3]│     │[Tracker4]│  ← Left Foot / Right Foot (발목 또는 발등)
└──────────┘     └──────────┘
```

| Index | SteamVR Role | 부착 위치 | 비고 |
|-------|-------------|-----------|------|
| 0 | `Waist` | 골반 중앙 (벨트 버클 위치) | 1개, 좌우 공용 |
| 1 | `LeftKnee` | 왼쪽 슬개골 위 | |
| 2 | `RightKnee` | 오른쪽 슬개골 위 | |
| 3 | `LeftFoot` | 왼쪽 발목 바깥쪽 | |
| 4 | `RightFoot` | 오른쪽 발목 바깥쪽 | |

---

## 2. 전체 시스템 흐름

```
┌─────────────────────────────────────────────────────────────────────┐
│                         PLUGIN BOUNDARY                             │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │                    VehicleKneeCollision Plugin                 │ │
│  │                                                                │ │
│  │  ┌─────────────────────────────────────────────────────────┐  │ │
│  │  │  PHASE 1: INPUT                                         │  │ │
│  │  │                                                         │  │ │
│  │  │   SteamVR ──── OpenXR Plugin ──── FTrackerInputData    │  │ │
│  │  │                                   (5x Position+Rotation)│  │ │
│  │  └──────────────────────┬──────────────────────────────────┘  │ │
│  │                         │ Tick (every frame)                  │ │
│  │  ┌──────────────────────▼──────────────────────────────────┐  │ │
│  │  │  PHASE 2: BODY MODEL (Dynamic Segment)                  │  │ │
│  │  │                                                         │  │ │
│  │  │   Hip ─────────────────────────────────────────────────│  │ │
│  │  │    ├─[Cylinder: Hip→L.Knee] ─ [Cylinder: L.Knee→L.Foot]│  │ │
│  │  │    └─[Cylinder: Hip→R.Knee] ─ [Cylinder: R.Knee→R.Foot]│  │ │
│  │  │                                                         │  │ │
│  │  │   Tracker 위치 → MidPoint 계산 → Scale/Rotate 적용     │  │ │
│  │  │   (키/다리 길이에 관계없이 완전 자동)                    │  │ │
│  │  └──────────────────────┬──────────────────────────────────┘  │ │
│  │                         │                                      │ │
│  │         ┌───────────────┼───────────────┐                     │ │
│  │         ▼               ▼               ▼                     │ │
│  │  ┌─────────────┐ ┌────────────┐ ┌────────────────┐           │ │
│  │  │  PHASE 3a   │ │  PHASE 3b  │ │   PHASE 3c     │           │ │
│  │  │  Collision  │ │  Distance  │ │  Calibration   │           │ │
│  │  │  Detection  │ │  Measure   │ │  System        │           │ │
│  │  │             │ │            │ │                │           │ │
│  │  │ SphereOverlap│ │ Knee ↔    │ │ T-Pose로       │           │ │
│  │  │ 5 Tracker×  │ │ Ref Point  │ │ 세그먼트 길이  │           │ │
│  │  │ Vehicle Mesh│ │ 거리(cm)   │ │ 자동 측정      │           │ │
│  │  └──────┬──────┘ └─────┬──────┘ └───────┬────────┘           │ │
│  │         │              │                │                     │ │
│  │         └──────────────▼────────────────┘                     │ │
│  │                        │                                      │ │
│  │  ┌─────────────────────▼───────────────────────────────────┐  │ │
│  │  │  PHASE 4: WARNING SYSTEM                                │  │ │
│  │  │                                                         │  │ │
│  │  │   Safe(Green) ──▶ Warning(Yellow) ──▶ Collision(Red)   │  │ │
│  │  │                                                         │  │ │
│  │  │   - 차량 Mesh 충돌 부위 Red Emissive                    │  │ │
│  │  │   - 화면 테두리 PostProcess Effect                      │  │ │
│  │  │   - Niagara Impact Particle                             │  │ │
│  │  └──────────────────────┬──────────────────────────────────┘  │ │
│  │                         │                                      │ │
│  │         ┌───────────────┴───────────────┐                     │ │
│  │         ▼                               ▼                     │ │
│  │  ┌─────────────────┐          ┌──────────────────────┐        │ │
│  │  │   PHASE 5a      │          │   PHASE 5b           │        │ │
│  │  │   VR HUD        │          │   Data Logger        │        │ │
│  │  │   (World Widget)│          │   (CSV Export)       │        │ │
│  │  └─────────────────┘          └──────────────────────┘        │ │
│  └────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 3. Plugin 구조

```
Plugins/
└── VehicleKneeCollision/
    ├── VehicleKneeCollision.uplugin        ← Plugin 메타데이터
    │
    ├── Resources/
    │   └── Icon128.png
    │
    ├── Source/
    │   │
    │   ├── VehicleKneeCollision/           ── [Runtime Module]
    │   │   │                                  게임 실행 중 동작하는 모든 로직
    │   │   ├── VehicleKneeCollision.Build.cs
    │   │   ├── Public/
    │   │   │   ├── VehicleKneeCollisionModule.h
    │   │   │   │
    │   │   │   ├── Tracker/
    │   │   │   │   ├── VKC_TrackerManager.h        ← 5개 Tracker 통합 관리
    │   │   │   │   └── VKC_TrackerTypes.h          ← FTrackerData, ETrackerRole Enum
    │   │   │   │
    │   │   │   ├── Body/
    │   │   │   │   ├── VKC_BodySegmentComponent.h  ← Dynamic Cylinder 연결
    │   │   │   │   ├── VKC_BodyActor.h             ← 전체 가상 신체 Actor
    │   │   │   │   └── VKC_CalibrationComponent.h  ← T-Pose 캘리브레이션
    │   │   │   │
    │   │   │   ├── Vehicle/
    │   │   │   │   ├── VKC_ReferencePoint.h        ← 차량 기준점 Actor
    │   │   │   │   └── VKC_VehicleManager.h        ← Interior Mesh 통합 관리
    │   │   │   │
    │   │   │   ├── Collision/
    │   │   │   │   ├── VKC_CollisionTypes.h        ← EWarningLevel, FCollisionEvent
    │   │   │   │   ├── VKC_CollisionDetector.h     ← Sphere Overlap 감지
    │   │   │   │   └── VKC_WarningFeedback.h       ← 시각/청각 피드백
    │   │   │   │
    │   │   │   ├── Measurement/
    │   │   │   │   ├── VKC_DistanceMeasurer.h      ← 실시간 거리 계산
    │   │   │   │   └── VKC_DistanceVisualizer.h    ← 거리 라인 시각화
    │   │   │   │
    │   │   │   └── Data/
    │   │   │       └── VKC_DataLogger.h            ← CSV 로깅 & 내보내기
    │   │   │
    │   │   └── Private/
    │   │       └── (각 .cpp 파일들)
    │   │
    │   └── VehicleKneeCollisionEditor/     ── [Editor Module]
    │       │                                  에디터 전용 툴 (런타임 불필요)
    │       ├── VehicleKneeCollisionEditor.Build.cs
    │       ├── Public/
    │       │   ├── VKCEditorModule.h
    │       │   ├── VKC_ReferencePointVisualizer.h  ← 기준점 에디터 Gizmo
    │       │   └── VKC_BodyPreviewActor.h          ← 에디터에서 체형 미리보기
    │       └── Private/
    │
    └── Content/                            ── [Plugin Content]
        ├── Blueprints/
        │   ├── BP_VKC_BodyActor.uasset
        │   ├── BP_VKC_TrackerManager.uasset
        │   ├── BP_VKC_ReferencePoint.uasset
        │   └── BP_VKC_SessionManager.uasset
        ├── Materials/
        │   ├── M_VKC_BodySegment.uasset            ← 기본 신체 세그먼트
        │   ├── MI_VKC_Safe.uasset                  ← 초록색
        │   ├── MI_VKC_Warning.uasset               ← 노란색
        │   └── MI_VKC_Collision.uasset             ← 빨간색
        ├── FX/
        │   ├── NS_VKC_CollisionImpact.uasset
        │   └── NS_VKC_WarningPulse.uasset
        └── UI/
            ├── WBP_VKC_HUD.uasset
            └── WBP_VKC_CalibrationPanel.uasset
```

---

## 4. 핵심 클래스 상세

### 4-1. VKC_TrackerTypes.h
```cpp
// Tracker 역할 정의
UENUM(BlueprintType)
enum class EVKCTrackerRole : uint8
{
    Waist       UMETA(DisplayName = "Waist / Hip"),
    LeftKnee    UMETA(DisplayName = "Left Knee"),
    RightKnee   UMETA(DisplayName = "Right Knee"),
    LeftFoot    UMETA(DisplayName = "Left Foot"),
    RightFoot   UMETA(DisplayName = "Right Foot"),
};

// 단일 Tracker 데이터
USTRUCT(BlueprintType)
struct FVKCTrackerData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) EVKCTrackerRole Role;
    UPROPERTY(BlueprintReadOnly) FVector  WorldLocation;
    UPROPERTY(BlueprintReadOnly) FRotator WorldRotation;
    UPROPERTY(BlueprintReadOnly) bool     bIsTracked;     // Tracker 연결 여부
};

// 경고 단계
UENUM(BlueprintType)
enum class EVKCWarningLevel : uint8
{
    Safe        UMETA(DisplayName = "Safe"),       // > 10cm
    Warning     UMETA(DisplayName = "Warning"),    // 5 ~ 10cm
    Collision   UMETA(DisplayName = "Collision"),  // <= 5cm or Overlap
};

// 충돌 이벤트 기록
USTRUCT(BlueprintType)
struct FVKCCollisionEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FDateTime  Timestamp;
    UPROPERTY(BlueprintReadOnly) EVKCTrackerRole  BodyPart;    // 어느 신체 부위
    UPROPERTY(BlueprintReadOnly) FString    VehiclePartName;   // 어느 차량 부품
    UPROPERTY(BlueprintReadOnly) FVector    CollisionLocation;
    UPROPERTY(BlueprintReadOnly) float      Distance;          // 충돌 시 거리(cm)
};
```

### 4-2. VKC_TrackerManager.h
```cpp
UCLASS(BlueprintType, Blueprintable)
class VEHICLEKNEECOLLISION_API AVKC_TrackerManager : public AActor
{
    GENERATED_BODY()

public:
    // 5개 Motion Controller Component
    UPROPERTY(VisibleAnywhere) UMotionControllerComponent* MC_Waist;
    UPROPERTY(VisibleAnywhere) UMotionControllerComponent* MC_LeftKnee;
    UPROPERTY(VisibleAnywhere) UMotionControllerComponent* MC_RightKnee;
    UPROPERTY(VisibleAnywhere) UMotionControllerComponent* MC_LeftFoot;
    UPROPERTY(VisibleAnywhere) UMotionControllerComponent* MC_RightFoot;

    // 현재 Tracker 데이터 조회 (BP에서도 사용 가능)
    UFUNCTION(BlueprintCallable, Category = "VKC|Tracker")
    FVKCTrackerData GetTrackerData(EVKCTrackerRole Role) const;

    UFUNCTION(BlueprintCallable, Category = "VKC|Tracker")
    bool IsTrackerActive(EVKCTrackerRole Role) const;

    // 외부 시스템에 Tracker 업데이트 알림 (Delegate)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
        FOnTrackerUpdated, const FVKCTrackerData&, TrackerData);
    UPROPERTY(BlueprintAssignable) FOnTrackerUpdated OnTrackerUpdated;
};
```

### 4-3. VKC_BodySegmentComponent.h
```cpp
// Hip→Knee, Knee→Foot 등 두 Tracker 사이를 Cylinder로 동적 연결
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class VEHICLEKNEECOLLISION_API UVKC_BodySegmentComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    // 연결할 두 끝점의 Tracker 역할
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body")
    EVKCTrackerRole RoleA;  // 예: Waist

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body")
    EVKCTrackerRole RoleB;  // 예: LeftKnee

    // 시각화 Cylinder Mesh (기본 높이 100cm 기준)
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* SegmentMesh;

    // 반경 조절 (cm) - 피험자에 맞게 조절 가능
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Body", meta=(ClampMin=2.0, ClampMax=20.0))
    float SegmentRadius = 8.0f;

    // 현재 측정된 세그먼트 길이 (캘리브레이션 후 자동 갱신)
    UPROPERTY(BlueprintReadOnly, Category = "VKC|Body")
    float CurrentSegmentLength = 0.0f;

    // Tick에서 두 Tracker 위치 → Cylinder 위치/스케일/방향 자동 갱신
    void TickComponent(float DeltaTime, ELevelTick TickType,
                       FActorComponentTickFunction* ThisTickFunction) override;

private:
    AVKC_TrackerManager* CachedTrackerManager;
};
```

### 4-4. VKC_BodyActor.h
```cpp
// 전체 가상 신체 (Tracker 5개 + Segment 4개 통합)
UCLASS(BlueprintType, Blueprintable)
class VEHICLEKNEECOLLISION_API AVKC_BodyActor : public AActor
{
    GENERATED_BODY()

public:
    // ── 세그먼트 4개 ──────────────────────────────
    UPROPERTY(VisibleAnywhere) UVKC_BodySegmentComponent* Seg_Hip_LKnee;
    UPROPERTY(VisibleAnywhere) UVKC_BodySegmentComponent* Seg_Hip_RKnee;
    UPROPERTY(VisibleAnywhere) UVKC_BodySegmentComponent* Seg_LKnee_LFoot;
    UPROPERTY(VisibleAnywhere) UVKC_BodySegmentComponent* Seg_RKnee_RFoot;

    // ── 충돌 감지용 Sphere 5개 ───────────────────
    UPROPERTY(VisibleAnywhere) USphereComponent* Sphere_Hip;
    UPROPERTY(VisibleAnywhere) USphereComponent* Sphere_LKnee;
    UPROPERTY(VisibleAnywhere) USphereComponent* Sphere_RKnee;
    UPROPERTY(VisibleAnywhere) USphereComponent* Sphere_LFoot;
    UPROPERTY(VisibleAnywhere) USphereComponent* Sphere_RFoot;

    // ── 캘리브레이션 ─────────────────────────────
    UPROPERTY(VisibleAnywhere) UVKC_CalibrationComponent* CalibrationComp;

    // 캘리브레이션 실행 (T-Pose에서 호출)
    UFUNCTION(BlueprintCallable, Category = "VKC|Body")
    void PerformCalibration();

    // 현재 세그먼트 길이들 조회
    UFUNCTION(BlueprintCallable, Category = "VKC|Body")
    FVKCBodyMeasurements GetBodyMeasurements() const;
};

// 신체 측정값 구조체
USTRUCT(BlueprintType)
struct FVKCBodyMeasurements
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) float Hip_LeftKnee;    // cm
    UPROPERTY(BlueprintReadOnly) float Hip_RightKnee;   // cm
    UPROPERTY(BlueprintReadOnly) float LKnee_LFoot;     // cm
    UPROPERTY(BlueprintReadOnly) float RKnee_RFoot;     // cm
    UPROPERTY(BlueprintReadOnly) float TotalLeftLeg;    // cm
    UPROPERTY(BlueprintReadOnly) float TotalRightLeg;   // cm
    UPROPERTY(BlueprintReadOnly) float SubjectHeight;   // HMD → Floor (cm)
};
```

---

## 5. 프레임별 실행 흐름 (Tick)

```
매 프레임 (약 90fps, Vive Pro 2 기준)
│
├─ [1] TrackerManager::TickActor()
│       └─ 5개 MotionControllerComponent 위치 읽기
│           └─ FVKCTrackerData 업데이트
│
├─ [2] BodySegmentComponent::TickComponent() × 4개
│       └─ RoleA, RoleB 위치 가져오기
│           ├─ MidPoint 계산
│           ├─ Direction 계산 → SetWorldRotation
│           ├─ Length 계산 → SetWorldScale3D (Z축만 스케일)
│           └─ CurrentSegmentLength 저장
│
├─ [3] CollisionDetector::TickComponent()
│       └─ 5개 Sphere 위치 → Tracker 위치와 동기화
│           └─ Overlap 이벤트 (이미 등록됨, 자동 처리)
│
├─ [4] DistanceMeasurer::TickComponent()
│       └─ Knee Trackers ↔ ReferencePoints 거리 계산
│           ├─ 좌측: LKnee ↔ 각 ReferencePoint
│           ├─ 우측: RKnee ↔ 각 ReferencePoint
│           └─ MinDistance 갱신
│
├─ [5] WarningFeedback::Update()
│       └─ EVKCWarningLevel 갱신
│           ├─ Safe:      → 라인 Green, Effect OFF
│           ├─ Warning:   → 라인 Yellow, Pulse ON
│           └─ Collision: → 라인 Red, Impact Particle, PostProcess ON
│
├─ [6] HUD Widget 갱신 (30Hz로 제한하여 성능 최적화)
│       └─ 거리값, 세그먼트 길이, 경고 상태 표시
│
└─ [7] DataLogger (선택, 기록 중일 때만)
        └─ 일정 간격(10Hz)으로 CSV에 행 추가
```

---

## 6. 세션 상태 머신

```
  ┌─────────────────────────────────────────────────────┐
  │                  Session State Machine               │
  └─────────────────────────────────────────────────────┘

  [IDLE] ──── "Start Session" 버튼 ────▶ [CALIBRATING]
                                              │
                                    T-Pose 유지 3초
                                    세그먼트 길이 자동 측정
                                              │
                                              ▼
                                        [TESTING]  ◀───┐
                                              │         │
                                    실시간 추적 + 거리 측정
                                    충돌 감지 + 경고 피드백
                                    데이터 로깅 중           │
                                              │         │
                          ┌───────────────────┤         │
                          ▼                   ▼         │
                  "Re-Calibrate"        "Stop Session"  │
                          │                   │         │
                          └──────────────────▶▼         │
                                        [REVIEWING]     │
                                              │         │
                                    결과 요약 표시         │
                                    CSV Export 옵션      │
                                              │         │
                                    "New Test" ──────────┘
                                    "Export & End" ──▶ [IDLE]
```

---

## 7. uplugin 파일

```json
{
    "FileVersion": 3,
    "Version": 1,
    "VersionName": "1.0.0",
    "FriendlyName": "Vehicle Knee Collision Test",
    "Description": "VR-based knee/foot/hip collision testing system for vehicle ingress/egress evaluation using Vive Trackers",
    "Category": "GMTCK | PQDQ",
    "CreatedBy": "GMTCK PQDQ Team",
    "CanContainContent": true,
    "IsBetaVersion": true,
    "Modules": [
        {
            "Name": "VehicleKneeCollision",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        },
        {
            "Name": "VehicleKneeCollisionEditor",
            "Type": "Editor",
            "LoadingPhase": "PostEngineInit"
        }
    ],
    "Plugins": [
        { "Name": "OpenXR",             "Enabled": true },
        { "Name": "SteamVR",            "Enabled": true },
        { "Name": "EnhancedInput",      "Enabled": true },
        { "Name": "Niagara",            "Enabled": true }
    ]
}
```

---

## 8. 다른 프로젝트에서 사용하는 법

이 Plugin을 완성하면, 다른 UE5 프로젝트에서도 쉽게 재사용 가능:

```
다른 UE5 프로젝트/
└── Plugins/
    └── VehicleKneeCollision/  ← 폴더 복사만 하면 됨
        └── VehicleKneeCollision.uplugin
```

1. 레벨에 `BP_VKC_BodyActor` 배치
2. 레벨에 `BP_VKC_TrackerManager` 배치
3. 차량 Interior Mesh 위에 `BP_VKC_ReferencePoint` 배치
4. `BP_VKC_SessionManager`로 세션 시작

→ 설정 완료. 차량 모델만 교체하면 다른 차종에도 바로 적용 가능.

---

## 9. 구현 순서 (Plugin 기준)

```
Week 1  │ Plugin 뼈대 생성 (uplugin, Build.cs, Module 파일)
        │ TrackerTypes.h (Enum, Struct 정의)
        │ TrackerManager 기본 구현 (5 MotionController)
        │ Tracker 위치 Debug Sphere로 시각화

Week 2  │ BodySegmentComponent 구현 (Dynamic Cylinder)
        │ BodyActor 조립 (4 Segment + 5 Sphere)
        │ CalibrationComponent 구현 (T-Pose 3초)

Week 3  │ VehicleReferencePoint 구현
        │ DistanceMeasurer 구현
        │ DistanceVisualizer (색상 변화 라인)

Week 4  │ CollisionDetector (Sphere Overlap)
        │ WarningFeedback (Material, PostProcess, Niagara)
        │ 충돌 이벤트 Delegate 연결

Week 5  │ VR HUD Widget (World Space)
        │ DataLogger (CSV)
        │ SessionManager (State Machine)

Week 6  │ Editor Module (ReferencePoint Gizmo)
        │ 체형 프리셋 (5th~95th Percentile)
        │ 통합 테스트 + 최적화
```
