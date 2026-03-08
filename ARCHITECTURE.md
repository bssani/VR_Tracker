# VR Knee Collision Test — Architecture & System Flow

> **Plugin Name:** `VRTrackerCollision`
> **Class Prefix:** `VTC_`
> **Engine:** Unreal Engine 5.5

---

## 1. Tracker 배치 (5개)

```
        [HMD - Vive Pro 2]
               │
        ┌──────▼──────┐
        │  Head (HMD) │  ← Camera (VTC_TrackerPawn 내부)
        └─────────────┘
               │
        ┌──────▼──────┐
        │  [Tracker 0]│  ← Waist / Hip (골반 중앙) — MotionSource: Special_1
        └──────┬──────┘
       ┌───────┴───────┐
       ▼               ▼
┌──────────┐     ┌──────────┐
│[Tracker1]│     │[Tracker2]│  ← Left Knee / Right Knee (슬개골 위)
│Special_2 │     │Special_3 │
└────┬─────┘     └─────┬────┘
     ▼                 ▼
┌──────────┐     ┌──────────┐
│[Tracker3]│     │[Tracker4]│  ← Left Foot / Right Foot (발목 또는 발등)
│Special_4 │     │Special_5 │
└──────────┘     └──────────┘
```

| Index | MotionSource | 부착 위치 | 비고 |
|-------|-------------|-----------|------|
| 0 | `Special_1` | 골반 중앙 (벨트 버클 위치) | Waist |
| 1 | `Special_2` | 왼쪽 슬개골 위 | LeftKnee |
| 2 | `Special_3` | 오른쪽 슬개골 위 | RightKnee |
| 3 | `Special_4` | 왼쪽 발목 바깥쪽 | LeftFoot |
| 4 | `Special_5` | 오른쪽 발목 바깥쪽 | RightFoot |

> SteamVR → Settings → Controllers → Manage Trackers 에서 각 Tracker에 위 Role 할당 필요

---

## 2. 레벨 아키텍처 (VRTestLevel 단일 진입점)

> **v3.0부터 Level 1 (SetupLevel) 및 Simulation 코드 전면 제거.**
> VRTestLevel이 유일한 진입점. 설정은 `WBP_VTC_ProfileManager` (Utility Editor 위젯)에서 사전 저장.

```
┌─ VRTestLevel ──────────────────────────────────────────────────────────┐
│                                                                         │
│  GameMode: VTC_VRGameMode                                              │
│    ├─ DefaultPawn: VTC_TrackerPawn                                     │
│    └─ PlayerController: VTC_OperatorController                         │
│         ├─ 1 캘리브레이션 / 2 테스트 / 3 CSV 내보내기                  │
│         ├─ P키 → 마지막 적용 프로파일 재적용                            │
│         └─ GameInstance 설정 → 각 Actor에 자동 적용                    │
│                                                                         │
│  레벨 배치 Actor:                                                       │
│    ├─ VTC_BodyActor (가상 신체)                                         │
│    ├─ VTC_SessionManager (중앙 오케스트레이터)                           │
│    │    ├─ VTC_CollisionDetector (컴포넌트)                              │
│    │    ├─ VTC_WarningFeedback (컴포넌트)                                │
│    │    └─ VTC_DataLogger (컴포넌트)                                     │
│    ├─ VTC_StatusActor (3D 월드 위젯 — 상태/키 안내)                     │
│    ├─ VTC_ReferencePoint × N (차량 기준점)                              │
│    ├─ VTC_OperatorViewActor (SceneCapture → Spectator Screen)          │
│    └─ PostProcessVolume (Vignette 피드백용)                             │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 프로파일 워크플로우

```
[사전 설정] WBP_VTC_ProfileManager (Utility Editor 위젯)
    → SubjectID, Height, MountOffset × 5, VehicleHipPosition 등 입력
    → Saved/VTCProfiles/<ProfileName>.json 저장

[VRTestLevel 실행]
    OperatorController::BeginPlay → GameInstance 로드

[P키를 누를 때] ApplyGameInstanceConfig()
         ├─ TrackerPawn.SetTrackerMeshVisible(bShowTrackerMesh)
         ├─ BodyActor.ApplySessionConfig(Config)
         │    ├─ MountOffset 5개 적용
         │    └─ bShowCollisionSpheres → 멤버 변수 저장 (Tick 덮어쓰기 방지)
         ├─ CollisionDetector.WarningThreshold/CollisionThreshold 적용
         └─ VehicleHipPosition → AVTC_ReferencePoint 런타임 스폰 (순수 위치 마커)
              └─ RelevantBodyParts 비움 → 충돌 감지 없음, 시안색 마커만 표시
```

### 삭제된 파일 (v3.0)

- `VTC_SetupGameMode.h/.cpp` — Level 1 GameMode 제거됨
- `VTC_SetupWidget.h/.cpp` — Level 1 설정 UI 제거됨
- `VTC_GameMode.h/.cpp` — 시뮬레이션용 GameMode 제거됨
- `VTC_SimPlayerController.h/.cpp` — WASD 데스크탑 시뮬레이션 제거됨

---

## 3. 전체 시스템 흐름

```
┌─────────────────────────────────────────────────────────────────────┐
│                      VRTrackerCollision Plugin                      │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │  PHASE 1: INPUT                                             │   │
│  │                                                             │   │
│  │   SteamVR ── OpenXR Plugin ── VTC_TrackerPawn (APawn)      │   │
│  │                               ├─ Camera (HMD)              │   │
│  │                               ├─ MC_Waist    (Special_1)   │   │
│  │                               ├─ MC_LeftKnee (Special_2)   │   │
│  │                               ├─ MC_RightKnee(Special_3)   │   │
│  │                               ├─ MC_LeftFoot (Special_4)   │   │
│  │                               └─ MC_RightFoot(Special_5)   │   │
│  │                               → IVTC_TrackerInterface 구현  │   │
│  └──────────────────────────┬────────────────────────────────-┘   │
│                              │ Tick (every frame)                  │
│  ┌───────────────────────────▼────────────────────────────────┐    │
│  │  PHASE 2: BODY MODEL                                       │    │
│  │                                                            │    │
│  │   VTC_BodyActor                                            │    │
│  │    ├─ VTC_BodySegmentComponent × 4 (Dynamic Cylinder)      │    │
│  │    │   Hip→LKnee / Hip→RKnee / LKnee→LFoot / RKnee→RFoot  │    │
│  │    ├─ USphereComponent × 5 (Collision Volume)              │    │
│  │    │   └─ bShowCollisionSpheres 멤버로 Tick 가시성 제어    │    │
│  │    ├─ UStaticMeshComponent × 5 (VisualSphere)              │    │
│  │    └─ VTC_CalibrationComponent (T-Pose 3초 자동 측정)       │    │
│  └──────────────────────────┬──────────────────────────────────┘   │
│                              │                                      │
│              ┌───────────────┴───────────────┐                     │
│              ▼                               ▼                     │
│  ┌──────────────────────────┐   ┌────────────────────────────┐    │
│  │  PHASE 3: COLLISION &    │   │  PHASE 3b: WARNING         │    │
│  │  DISTANCE DETECTION      │   │  FEEDBACK                  │    │
│  │                          │   │                            │    │
│  │  VTC_CollisionDetector   │   │  VTC_WarningFeedback       │    │
│  │  ├─ Sphere Overlap 감지  │──▶│  ├─ PostProcess Vignette   │    │
│  │  ├─ Knee↔ReferencePoint  │   │  ├─ Sound (Warning/Coll.)  │    │
│  │  │  거리 계산 (30Hz)      │   │  └─ Niagara Impact FX      │    │
│  │  └─ WarningLevel 결정    │   │                            │    │
│  │    >10cm: Safe           │   │  Safe    → PostProcess OFF │    │
│  │    3~10cm: Warning       │   │  Warning → Vignette 0.5    │    │
│  │    ≤3cm:  Collision      │   │  Collision→ Vignette 1.0   │    │
│  └──────────────────────────┘   └────────────────────────────┘    │
│                              │                                      │
│  ┌───────────────────────────▼────────────────────────────────┐    │
│  │  PHASE 4: SESSION MANAGEMENT & DATA                        │    │
│  │                                                            │    │
│  │  VTC_SessionManager (Actor)                                │    │
│  │  State: IDLE → CALIBRATING → TESTING → REVIEWING          │    │
│  │  ├─ VTC_DataLogger (CSV, 10Hz)                             │    │
│  │  │    ├─ *_summary.csv (세션당 1행, HF 분석용)              │    │
│  │  │    └─ *_frames.csv  (10Hz 원시 데이터, 연구자용)          │    │
│  │  └─ VTC_ReferencePoint × N (차량 기준점)                   │    │
│  │       + VehicleHipPosition (런타임 동적 스폰)               │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                     │
│  [UI: VTC_StatusWidget (WorldSpace 3D — VR 내 상태 표시)]           │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 4. Plugin 구조 (실제)

```
Plugins/
└── VRTrackerCollision/
    ├── VRTrackerCollision.uplugin
    │
    ├── Source/
    │   └── VRTrackerCollision/
    │       ├── VRTrackerCollision.Build.cs
    │       │
    │       ├── Public/
    │       │   ├── VRTrackerCollisionModule.h
    │       │   │
    │       │   ├── VTC_VRGameMode.h             ← VRTestLevel 전용 GameMode (VR Only)
    │       │   ├── VTC_GameInstance.h           ← 설정 저장/불러오기 (JSON)
    │       │   ├── VTC_SessionConfig.h          ← FVTCSessionConfig 구조체
    │       │   ├── VTC_VehiclePreset.h          ← JSON 차종 프리셋 구조체 + Manager
    │       │   ├── VTC_ProfileLibrary.h         ← 피실험자+차량 프로파일 CRUD
    │       │   │
    │       │   ├── Tracker/
    │       │   │   ├── VTC_TrackerTypes.h       ← 공통 Enum/Struct
    │       │   │   └── VTC_TrackerInterface.h   ← IVTC_TrackerInterface
    │       │   │
    │       │   ├── Pawn/
    │       │   │   └── VTC_TrackerPawn.h        ← HMD + 5 MotionController 통합
    │       │   │
    │       │   ├── Controller/
    │       │   │   └── VTC_OperatorController.h ← 세션 제어 + 설정 적용
    │       │   │
    │       │   ├── Body/
    │       │   │   ├── VTC_BodyActor.h          ← 가상 신체 Actor
    │       │   │   ├── VTC_BodySegmentComponent.h ← Dynamic Cylinder
    │       │   │   └── VTC_CalibrationComponent.h ← T-Pose 캘리브레이션
    │       │   │
    │       │   ├── Vehicle/
    │       │   │   └── VTC_ReferencePoint.h     ← 차량 기준점 Actor
    │       │   │
    │       │   ├── Collision/
    │       │   │   ├── VTC_CollisionDetector.h  ← 거리 측정 + Overlap 감지
    │       │   │   └── VTC_WarningFeedback.h    ← 시각/청각 피드백
    │       │   │
    │       │   ├── Data/
    │       │   │   ├── VTC_DataLogger.h         ← CSV 로깅 (summary + frames)
    │       │   │   └── VTC_SessionManager.h     ← 세션 상태머신
    │       │   │
    │       │   ├── UI/
    │       │   │   ├── VTC_StatusWidget.h       ← VR 3D WorldSpace 상태 표시 위젯
    │       │   │   ├── VTC_SubjectInfoWidget.h  ← 피실험자 입력 위젯
    │       │   │   └── VTC_ProfileManagerWidget.h  ← 프로파일 Utility Editor
    │       │   │
    │       │   └── World/
    │       │       ├── VTC_StatusActor.h        ← 3D 월드 위젯 Actor
    │       │       └── VTC_OperatorViewActor.h  ← SceneCapture → Spectator Screen
    │       │
    │       └── Private/
    │           ├── VRTrackerCollisionModule.cpp
    │           ├── VTC_GameInstance.cpp
    │           ├── VTC_VehiclePreset.cpp
    │           ├── VTC_ProfileLibrary.cpp
    │           │
    │           ├── Pawn/VTC_TrackerPawn.cpp
    │           ├── Controller/VTC_OperatorController.cpp
    │           ├── Body/
    │           │   ├── VTC_BodyActor.cpp
    │           │   ├── VTC_BodySegmentComponent.cpp
    │           │   └── VTC_CalibrationComponent.cpp
    │           ├── Vehicle/VTC_ReferencePoint.cpp
    │           ├── Collision/
    │           │   ├── VTC_CollisionDetector.cpp
    │           │   └── VTC_WarningFeedback.cpp
    │           ├── Data/
    │           │   ├── VTC_DataLogger.cpp
    │           │   └── VTC_SessionManager.cpp
    │           ├── UI/
    │           │   ├── VTC_StatusWidget.cpp
    │           │   ├── VTC_SubjectInfoWidget.cpp
    │           │   └── VTC_ProfileManagerWidget.cpp
    │           └── World/
    │               ├── VTC_StatusActor.cpp
    │               └── VTC_OperatorViewActor.cpp
    │
    └── Content/                          ← Blueprint/Asset은 여기서 제작 예정
        ├── Blueprints/
        │   ├── BP_VTC_TrackerPawn.uasset
        │   ├── BP_VTC_BodyActor.uasset
        │   ├── BP_VTC_ReferencePoint.uasset
        │   └── BP_VTC_SessionManager.uasset
        ├── Materials/
        │   ├── M_VTC_BodySegment.uasset
        │   ├── MI_VTC_Safe.uasset
        │   ├── MI_VTC_Warning.uasset
        │   └── MI_VTC_Collision.uasset
        ├── FX/
        │   ├── NS_VTC_CollisionImpact.uasset
        │   └── NS_VTC_WarningPulse.uasset
        └── UI/
            └── WBP_VTC_HUD.uasset
```

> **삭제된 파일 (v3.0):** `VTC_SetupGameMode`, `VTC_SetupWidget`, `VTC_GameMode`, `VTC_SimPlayerController`, `VTC_OperatorMonitorWidget`

---

## 5. 핵심 클래스 관계

```
[공통]
  VTC_GameInstance (UGameInstance)
    ├─ FVTCSessionConfig 보관
    ├─ JSON 저장/불러오기 (Saved/VTCProfiles/<Name>.json)
    ├─ ApplyProfileByName(ProfileName) → SessionConfig 갱신
    └─ VTC_SessionConfig.h → FVTCSessionConfig

[VRTestLevel]
  VTC_VRGameMode (AGameModeBase)
    └─ DefaultPawnClass = VTC_TrackerPawn
       PlayerControllerClass = VTC_OperatorController

  VTC_OperatorController → APlayerController
    ├─ 단축키: 1(캘리브레이션) / 2(테스트) / 3(CSV+종료) / P(프로파일 재적용) / G(JSON저장)
    ├─ ApplyGameInstanceConfig() → TrackerPawn, BodyActor, CollisionDetector
    ├─ VehicleHipPosition → ReferencePoint 런타임 스폰
    ├─ StatusActor/StatusWidget 갱신 (Tick 1초마다)
    └─ EndPlay → SpawnedHipRefPoint 정리

  VTC_TrackerPawn (APawn, IVTC_TrackerInterface)
    ├─ Camera (HMD)
    └─ MotionControllerComponent × 5

  VTC_BodyActor (AActor)
    ├─ VTC_BodySegmentComponent × 4 (Dynamic Cylinder)
    ├─ USphereComponent × 5 (Collision)
    ├─ UStaticMeshComponent × 5 (VisualSphere)
    ├─ VTC_CalibrationComponent (T-Pose)
    └─ ApplySessionConfig() → MountOffset + bShowCollisionSpheres

  VTC_SessionManager (AActor) ← 중앙 오케스트레이터
    ├─ VTC_CollisionDetector (UActorComponent)
    │    └─ VTC_ReferencePoint × N (레벨 배치 + 런타임 스폰)
    ├─ VTC_WarningFeedback (UActorComponent)
    └─ VTC_DataLogger (UActorComponent)
         ├─ *_summary.csv (세션 1행, HF용)
         └─ *_frames.csv (10Hz 원시 데이터)

  VTC_StatusActor (AActor)
    └─ WidgetComponent (WorldSpace)
         └─ VTC_StatusWidget (UUserWidget)
              ├─ 세션 상태 표시 (IDLE / CALIBRATING / TESTING / REVIEWING)
              ├─ 키 안내 (F1/F2/F3/Escape)
              ├─ 피실험자 정보
              └─ 트래커 연결 수

```

### 주요 타입 (VTC_TrackerTypes.h)

```cpp
// Tracker 역할
enum class EVTCTrackerRole : uint8
{ Waist, LeftKnee, RightKnee, LeftFoot, RightFoot }

// 경고 단계
enum class EVTCWarningLevel : uint8
{ Safe, Warning, Collision }
// Safe: > 10cm | Warning: 3~10cm | Collision: ≤ 3cm 또는 Overlap
// 임계값은 FVTCSessionConfig.WarningThreshold_cm / CollisionThreshold_cm에 저장

// 세션 상태
enum class EVTCSessionState : uint8
{ Idle, Calibrating, Testing, Reviewing }

// 실행 모드 (VTC_SessionConfig.h)
enum class EVTCRunMode : uint8
{ VR, Simulation }

// [NEW] 이동 단계 (VTC_TrackerTypes.h)
enum class EVTCMovementPhase : uint8
{ Unknown, Stationary, Entering, Seated, Exiting }
// Hip Z 속도로 자동 감지. DataLogger의 Phase별 최소 클리어런스 추적에 사용.

// 주요 Struct
FVTCSessionConfig     — Level 1↔Level 2 설정 전달 (SubjectID, Height, Offsets, 모드 등)
                        WarningThreshold_cm, CollisionThreshold_cm
                        bUseVehiclePreset, SelectedPresetName, LoadedPresetJson
                        bShowCollisionSpheres, bShowTrackerMesh
                        [NEW] ProfileName — Saved/VTCProfiles/<ProfileName>.json 파일명
FVTCTrackerData       — 단일 Tracker 위치/회전/추적여부
                        bIsInterpolated (dropout 보간 중 여부)
FVTCBodyMeasurements  — 캘리브레이션 결과 (세그먼트 길이, 키)
FVTCDistanceResult    — 신체부위 ↔ 기준점 거리 측정 결과
FVTCCollisionEvent    — 충돌 이벤트 기록 (시간, 부위, 부품명, 거리, 밀리초 정밀도)
FVTCVehiclePreset     — 차종 프리셋 (PresetName + ReferencePoint 배열)
FVTCPresetRefPoint    — 프리셋 내 단일 ReferencePoint 데이터
```

---

## 6. 프레임별 실행 흐름 (Tick)

```
매 프레임 (약 90fps, Vive Pro 2 기준)
│
├─ [1] VTC_TrackerPawn::Tick()
│       └─ UpdateAllTrackers()
│           ├─ 5개 MotionControllerComponent.IsTracked() 확인
│           ├─ [NEW] Dropout 보간: 추적 실패 시 최근 2프레임 선형 외삽 (MaxDropoutFrames=5)
│           ├─ TrackerDataMap 갱신 (WorldLocation, WorldRotation, bIsInterpolated)
│           ├─ Debug Sphere 표시 (bShowDebugSpheres = true, 보간 중은 얇게)
│           ├─ [NEW] DetectMovementPhase(): Hip Z 속도 → EVTCMovementPhase 업데이트
│           └─ OnTrackerUpdated / OnAllTrackersUpdated / OnPhaseChanged Delegate 브로드캐스트
│
├─ [2] VTC_BodySegmentComponent::TickComponent() × 4개
│       └─ UpdateSegmentTransform()
│           ├─ RoleStart, RoleEnd 위치 가져오기 (TrackerInterface)
│           ├─ MidPoint 계산 → SetWorldLocation
│           ├─ Direction → SetWorldRotation (Z축 -90도 보정)
│           └─ Length / BaseCylinderHeight → SetWorldScale3D(Z축)
│
├─ [3] VTC_BodyActor::Tick()
│       └─ SyncSpherePositions()
│           └─ 5개 Sphere를 각 Tracker 위치로 이동
│               bShowCollisionSpheres 멤버로 가시성 제어
│               (ApplySessionConfig에서 설정된 값을 Tick이 덮어쓰지 않음)
│
├─ [4] VTC_CollisionDetector::TickComponent() (30Hz 제한)
│       └─ PerformDistanceMeasurements()
│           ├─ FlushPersistentDebugLines + FlushDebugStrings (이전 사이클 라인 제거)
│           ├─ 각 ReferencePoint ↔ 관련 신체부위 거리 계산
│           ├─ WarningLevel 결정 (Safe/Warning/Collision)
│           ├─ OverallWarningLevel 갱신
│           ├─ Persistent DrawDebugLine + DrawDebugString (다음 사이클에서 Flush)
│           └─ OnWarningLevelChanged / OnDistanceUpdated Delegate 브로드캐스트
│
├─ [5] VTC_WarningFeedback (CollisionDetector Delegate 수신)
│       └─ Safe:      → PostProcess OFF, 사운드 OFF
│          Warning:   → Vignette 0.5, WarningSFX (500ms 쿨다운 — 다중 재생 방지)
│          Collision: → Vignette 1.0, CollisionSFX (500ms 쿨다운), Niagara FX 스폰
│
├─ [6] VTC_DataLogger (10Hz, Testing 상태일 때만)
│       └─ LogFrame() — 위치 + 거리 + 경고레벨 CSV 버퍼에 추가
│            + WarningDuration/CollisionDuration 누적
│            + MinClearance 갱신 시 Timestamp 기록
│
└─ [7] VTC_OperatorController::Tick() (1초마다)
        └─ StatusWidget 갱신 (TrackerStatus)
```

---

## 7. 세션 상태 머신 (VTC_SessionManager)

```
  [IDLE]
    │
    │  F1 = StartSession(SubjectID, Height)
    ▼
  [CALIBRATING]
    │  T-Pose 유지 → CalibrationComponent 3초 카운트다운
    │  완료: OnCalibrationComplete → DataLogger.StartLogging()
    │  실패: OnCalibrationFailed → 다시 IDLE
    │
    │  F2 = StartTestingDirectly() 로 바로 건너뛰기 가능
    ▼
  [TESTING]  ◀──── RequestReCalibration() ─────┐
    │                                           │
    │  실시간: Tracker 추적 + 거리 측정 + 피드백  │
    │  DataLogger 10Hz 기록 중                  │
    │                                           │
    │  StopSession()                            │
    ▼                                           │
  [REVIEWING]                                   │
    │  결과 요약 표시                             │
    │  F3 = ExportAndEnd() → CSV 저장            │
    │                                           │
    │  "New Test" ──────────────────────────────┘
    │  "Export & End"
    ▼
  [IDLE]

  ※ Escape → 세션 중단 / IDLE 복귀
```

---

## 8. 데이터 출력 (VTC_DataLogger)

### Summary CSV (`*_summary.csv`) — 세션당 1행, Human Factors 분석용

```csv
SubjectID, Date,
Height_cm,
WaistToKnee_L_cm, WaistToKnee_R_cm,
KneeToFoot_L_cm, KneeToFoot_R_cm,
WaistToFoot_L_cm, WaistToFoot_R_cm,
HipPos_avg_X, HipPos_avg_Y, HipPos_avg_Z,
HipDist_to_Ref_min_cm,
LKnee_to_Ref_min_cm, RKnee_to_Ref_min_cm,
MinClearance_cm, NearestBodyPart, NearestRefPoint,
MinClearance_Timestamp,
HipX_atMinClearance, HipY_atMinClearance, HipZ_atMinClearance,
OverallStatus, CollisionCount, WarningFrames, TotalFrames,
TestingStartTime, TestingEndTime, TestingDuration_sec,
WarningDuration_sec, CollisionDuration_sec
```

| 컬럼 | 설명 |
|------|------|
| SubjectID, Date | 피실험자 ID, 세션 날짜 |
| Height_cm | 키 (수동 입력 우선, 없으면 HMD 추정) |
| WaistToKnee/KneeToFoot/WaistToFoot | 캘리브레이션 측정 세그먼트 길이 (cm) |
| HipPos_avg_X/Y/Z | 세션 전체 Hip 평균 위치 (UE 좌표, cm) |
| HipDist_to_Ref_min_cm | Hip↔기준점 최소 거리 |
| LKnee/RKnee_to_Ref_min_cm | 무릎별 최소 거리 |
| MinClearance_cm | 전체 최소 클리어런스 |
| NearestBodyPart / NearestRefPoint | 최악 순간의 신체 부위 / 기준점 이름 |
| MinClearance_Timestamp | 최악 클리어런스 발생 시점 (밀리초) |
| HipX/Y/Z_atMinClearance | 최악 순간의 Hip 위치 |
| OverallStatus | GREEN / YELLOW / RED |
| CollisionCount | 충돌 이벤트 횟수 |
| WarningFrames / TotalFrames | 경고 프레임 수 / 전체 프레임 수 |
| TestingStartTime / TestingEndTime | 테스트 시작/종료 시각 |
| TestingDuration_sec | 테스트 지속 시간 (초) |
| WarningDuration_sec | Warning 이상 누적 시간 (초) |
| CollisionDuration_sec | Collision 누적 시간 (초) |

### Frame CSV (`*_frames.csv`) — 10Hz 원시 프레임 데이터

```csv
Timestamp, SubjectID, Height,
UpperLeftLeg, UpperRightLeg, LowerLeftLeg, LowerRightLeg,
HipX, HipY, HipZ, LKneeX, LKneeY, LKneeZ, RKneeX, RKneeY, RKneeZ,
LFootX, LFootY, LFootZ, RFootX, RFootY, RFootZ,
Dist_[RefPoint1], Dist_[RefPoint2], ...,
CollisionOccurred, CollisionPartName
```

저장 경로: `[Project]/Saved/VTCLogs/`

### Collision Event

- 충돌 발생 시 즉시 기록 (OnWarningLevelChanged → Collision)
- 타임스탬프: 밀리초 정밀도 (`%Y-%m-%d %H:%M:%S.%s`)
- BodyPart, VehiclePartName, CollisionLocation 포함

---

## 9. uplugin 실제 설정

```json
{
    "FriendlyName": "Vehicle Knee Collision Test",
    "Category": "GMTCK|PQDQ",
    "Modules": [
        {
            "Name": "VRTrackerCollision",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        }
    ],
    "Plugins": [
        { "Name": "OpenXR",  "Enabled": true },
        { "Name": "Niagara", "Enabled": true }
    ]
}
```

> `SteamVR` 플러그인은 `.uplugin`에 명시되지 않음.
> OpenXR이 SteamVR 런타임을 감싸므로, UE5 프로젝트 설정에서 OpenXR을 기본 HMD Plugin으로 설정해야 함.

---

## 10. 레벨 셋업 방법 (다른 프로젝트/차종 적용 시)

```
1. Plugins/VRTrackerCollision/ 폴더를 복사
2. Project Settings > Game Instance Class = BP_VTC_GameInstance
3. VRTestLevel:
   a. GameMode Override = BP_VTC_VRGameMode
   b. BP_VTC_SessionManager, BP_VTC_BodyActor, BP_VTC_StatusActor 배치
   c. 차량 Interior Mesh 위에 BP_VTC_ReferencePoint 배치 (PartName 설정)
   d. PostProcessVolume (Infinite Extent) 배치
4. WBP_VTC_ProfileManager (Utility Editor)에서 피실험자+차량 프로파일 사전 저장
5. 차량 모델만 교체 + ReferencePoint 재배치하면 다른 차종에 바로 적용 가능
```

---

## 11. 구현 현황

### C++ 구현 완료

| 파일 | 카테고리 | 내용 |
|------|---------|------|
| VTC_TrackerTypes.h | Tracker | 공통 Enum/Struct |
| VTC_TrackerInterface.h | Tracker | TrackerPawn 접근 인터페이스 |
| VTC_TrackerPawn | Pawn | HMD+5 Tracker 통합 Pawn |
| VTC_VRGameMode | Core | VRTestLevel 전용 GameMode (VR Only) |
| VTC_GameInstance | Core | 설정 저장/불러오기 (JSON) + ApplyProfileByName |
| VTC_SessionConfig | Core | FVTCSessionConfig 구조체 (WarningThreshold, CollisionThreshold, ProfileName 포함) |
| VTC_ProfileLibrary | Core | 피실험자+차량 조합 프로파일 CRUD (Saved/VTCProfiles/) |
| VTC_OperatorController | Controller | 세션 제어 + 설정 적용 + 동적 스폰 |
| VTC_BodyActor | Body | 가상 신체 (세그먼트+Sphere+VisualSphere) |
| VTC_BodySegmentComponent | Body | Dynamic Cylinder |
| VTC_CalibrationComponent | Body | T-Pose 캘리브레이션 |
| VTC_ReferencePoint | Vehicle | 차량 기준점 Actor + SetActive(가시성 연동) |
| VTC_CollisionDetector | Collision | 거리 측정 + 충돌 감지 (자동 스크린샷 / VR 거리 라벨) |
| VTC_WarningFeedback | Collision | 시각/청각 피드백 (Warning+Collision 500ms 쿨다운) |
| VTC_DataLogger | Data | CSV 로깅 (summary + frames) |
| VTC_SessionManager | Data | 세션 상태머신 |
| VTC_StatusWidget | UI | VR 3D WorldSpace 상태 표시 위젯 |
| VTC_SubjectInfoWidget | UI | 피실험자 입력 위젯 |
| VTC_ProfileManagerWidget | UI | 프로파일 Utility Editor 위젯 (사전 설정 저장/불러오기/삭제) |
| VTC_StatusActor | World | 3D 월드 위젯 Actor |
| VTC_VehiclePreset | Core | JSON 차종 프리셋 구조체 + 파일 I/O Manager |
| VTC_OperatorViewActor | World | SceneCapture2D → TextureRenderTarget → Spectator Screen |

### Blueprint / Asset 작업 필요

| 작업 | 우선순위 |
|------|---------|
| BP_VTC_GameInstance 생성 | 높음 |
| BP_VTC_TrackerPawn (MotionSource 설정) | 높음 |
| BP_VTC_OperatorController (VRTestLevel GameMode에 지정) | 높음 |
| BP_VTC_BodyActor (Material 연결) | 높음 |
| BP_VTC_ReferencePoint (차량 위 배치) | 높음 |
| BP_VTC_SessionManager (시스템 자동 탐색) | 높음 |
| BP_VTC_StatusActor + WBP_VTC_StatusWidget | 높음 |
| WBP_VTC_ProfileManager (Utility Editor 사전 설정) | 높음 |
| VRTestLevel 맵 파일 GameMode Override = VTC_VRGameMode | 높음 |
| Material (Body Segment Safe/Warning/Collision) | 중간 |
| Niagara FX 설정 (CollisionImpact, WarningPulse) | 낮음 |
| Sound Cue 설정 + CountdownSFX 배열 4개 연결 | 낮음 |
| BP_VTC_OperatorViewActor (SceneCapture 설정) | 중간 |

---

## 12. 기능 요약

| Feature | 구현 위치 | 설명 |
|---------|----------|------|
| **프로파일 시스템** | VTC_ProfileLibrary + ProfileManagerWidget | WBP_VTC_ProfileManager에서 사전 저장, VRTestLevel에서 P키로 적용 |
| **Dropout 보간** | VTC_TrackerPawn | 추적 실패 시 최근 2프레임 선형 외삽으로 최대 5프레임 유지 |
| **캘리브레이션 검증 강화** | VTC_CalibrationComponent | 좌우 비대칭 25% 초과, 다리 길이 범위, 대퇴/하퇴 비율 검증 |
| **이동 단계 자동 감지** | VTC_TrackerPawn + DataLogger | Hip Z 속도 기반 Entering/Seated/Exiting 전환 + 단계별 MinClearance |
| **자동 스크린샷** | VTC_CollisionDetector | 세션 최악 클리어런스 갱신 시 PNG 자동 저장 |
| **VR 거리 라인 라벨** | VTC_CollisionDetector | DrawDebugLine + DrawDebugString으로 거리(cm) 실시간 표시 |
| **음성 카운트다운** | VTC_CalibrationComponent | USoundBase 배열로 3초 카운트다운 + 완료 음성 재생 |
| **Operator View** | VTC_OperatorViewActor | SceneCapture2D(탑다운) → TextureRenderTarget2D → Spectator Screen |

### Operator View 연결 구조

```
VTC_OperatorViewActor
  │
  ├─ SceneCaptureComponent2D (탑다운 직교, -90° Pitch)
  │   └─ TextureRenderTarget2D (1280×720 기본)
  │
  └─ BeginPlay → SetupSpectatorScreen()
       └─ ISpectatorScreenController::SetSpectatorScreenTexture(RenderTarget)
            └─ 운영자 모니터(Companion Screen)에 실시간 전시 장면 표시
```

### CSV 출력 변경사항 (v2.0 추가 컬럼)

| 추가 컬럼 | 출력 위치 | 내용 |
|-----------|----------|------|
| `Phase_Entering_MinClearance` | _summary.csv | Entering 단계 최소 클리어런스 |
| `Phase_Seated_MinClearance` | _summary.csv | Seated 단계 최소 클리어런스 |
| `Phase_Exiting_MinClearance` | _summary.csv | Exiting 단계 최소 클리어런스 |
| `WorstClearanceScreenshot` | _summary.csv | 최악 순간 스크린샷 파일 경로 |
