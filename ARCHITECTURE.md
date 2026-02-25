# VR Knee Collision Test — Architecture & System Flow

> **Plugin Name:** `VRTrackerCollision`
> **Class Prefix:** `VTC_`
> **Engine:** Unreal Engine 5.5
> **Target:** 다음 주 내 동작 가능 버전 완성

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

## 2. 전체 시스템 흐름

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
│  │  PHASE 4: SESSION MANAGEMENT                               │    │
│  │                                                            │    │
│  │  VTC_SessionManager (Actor)                                │    │
│  │  State: IDLE → CALIBRATING → TESTING → REVIEWING          │    │
│  │  ├─ VTC_DataLogger (CSV, 10Hz)                             │    │
│  │  └─ VTC_ReferencePoint × N (차량 기준점)                   │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                     │
│  [HUD / UI: Blueprint Widget으로 구현 예정]                          │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 3. Plugin 구조 (실제)

```
Plugins/
└── VRTrackerCollision/
    ├── VRTrackerCollision.uplugin        ← Runtime 모듈 1개만 (Editor 모듈 없음)
    │
    ├── Source/
    │   └── VRTrackerCollision/           ── [Runtime Module]
    │       ├── VRTrackerCollision.Build.cs
    │       ├── Public/
    │       │   ├── VRTrackerCollisionModule.h
    │       │   ├── VTC_GameMode.h              ← DefaultPawn = VTC_TrackerPawn
    │       │   │
    │       │   ├── Tracker/
    │       │   │   ├── VTC_TrackerTypes.h      ← 공통 Enum/Struct (EVTCTrackerRole 등)
    │       │   │   └── VTC_TrackerInterface.h  ← IVTC_TrackerInterface (느슨한 결합)
    │       │   │
    │       │   ├── Pawn/
    │       │   │   └── VTC_TrackerPawn.h       ← HMD Camera + 5 MotionController 통합
    │       │   │
    │       │   ├── Body/
    │       │   │   ├── VTC_BodyActor.h         ← 가상 신체 Actor (세그먼트+Sphere)
    │       │   │   ├── VTC_BodySegmentComponent.h ← Dynamic Cylinder
    │       │   │   └── VTC_CalibrationComponent.h ← T-Pose 캘리브레이션
    │       │   │
    │       │   ├── Vehicle/
    │       │   │   └── VTC_ReferencePoint.h    ← 차량 기준점 Actor (개별 배치)
    │       │   │
    │       │   ├── Collision/
    │       │   │   ├── VTC_CollisionDetector.h ← 거리 측정 + Overlap 감지 통합
    │       │   │   └── VTC_WarningFeedback.h   ← 시각/청각 피드백
    │       │   │
    │       │   └── Data/
    │       │       ├── VTC_DataLogger.h        ← CSV 로깅 (10Hz)
    │       │       └── VTC_SessionManager.h    ← 전체 시스템 조율 + 상태머신
    │       │
    │       └── Private/
    │           ├── VRTrackerCollisionModule.cpp
    │           ├── VTC_GameMode.cpp
    │           ├── Pawn/VTC_TrackerPawn.cpp
    │           ├── Body/
    │           │   ├── VTC_BodyActor.cpp
    │           │   ├── VTC_BodySegmentComponent.cpp
    │           │   └── VTC_CalibrationComponent.cpp
    │           ├── Vehicle/VTC_ReferencePoint.cpp
    │           ├── Collision/
    │           │   ├── VTC_CollisionDetector.cpp
    │           │   └── VTC_WarningFeedback.cpp
    │           └── Data/
    │               ├── VTC_DataLogger.cpp
    │               └── VTC_SessionManager.cpp
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

---

## 4. 핵심 클래스 관계

```
VTC_GameMode
  └─ DefaultPawnClass = VTC_TrackerPawn
       └─ implements IVTC_TrackerInterface
            │
            ├─ VTC_BodyActor  ──── finds TrackerInterface via GetAllActorsWithInterface()
            │   ├─ VTC_BodySegmentComponent × 4
            │   ├─ USphereComponent × 5
            │   └─ VTC_CalibrationComponent
            │
            └─ VTC_SessionManager (Actor)
                ├─ VTC_CollisionDetector (Component)
                │   └─ VTC_ReferencePoint × N (레벨에 배치)
                ├─ VTC_WarningFeedback (Component)
                └─ VTC_DataLogger (Component)
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

// 세션 상태
enum class EVTCSessionState : uint8
{ Idle, Calibrating, Testing, Reviewing }

// 주요 Struct
FVTCTrackerData       — 단일 Tracker 위치/회전/추적여부
FVTCBodyMeasurements  — 캘리브레이션 결과 (세그먼트 길이, 키)
FVTCDistanceResult    — 신체부위 ↔ 기준점 거리 측정 결과
FVTCCollisionEvent    — 충돌 이벤트 기록 (시간, 부위, 부품명, 거리)
```

---

## 5. 프레임별 실행 흐름 (Tick)

```
매 프레임 (약 90fps, Vive Pro 2 기준)
│
├─ [1] VTC_TrackerPawn::Tick()
│       └─ UpdateAllTrackers()
│           ├─ 5개 MotionControllerComponent.IsTracked() 확인
│           ├─ TrackerDataMap 갱신 (WorldLocation, WorldRotation)
│           ├─ Debug Sphere 표시 (bShowDebugSpheres = true)
│           └─ OnTrackerUpdated / OnAllTrackersUpdated Delegate 브로드캐스트
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
│               (Overlap 이벤트는 UE 물리 엔진이 자동 처리)
│
├─ [4] VTC_CollisionDetector::TickComponent() (30Hz 제한)
│       └─ PerformDistanceMeasurements()
│           ├─ 각 ReferencePoint ↔ 관련 신체부위 거리 계산
│           ├─ WarningLevel 결정 (Safe/Warning/Collision)
│           ├─ OverallWarningLevel 갱신
│           └─ OnWarningLevelChanged / OnDistanceUpdated Delegate 브로드캐스트
│
├─ [5] VTC_WarningFeedback (CollisionDetector Delegate 수신)
│       └─ Safe:      → PostProcess OFF, 사운드 OFF
│          Warning:   → Vignette 0.5, WarningSFX
│          Collision: → Vignette 1.0, CollisionSFX, Niagara FX 스폰
│
└─ [6] VTC_DataLogger (10Hz, Testing 상태일 때만)
        └─ LogFrame() — 위치 + 거리 + 경고레벨 CSV 버퍼에 추가
```

---

## 6. 세션 상태 머신 (VTC_SessionManager)

```
  [IDLE]
    │
    │  StartSession(SubjectID)
    ▼
  [CALIBRATING]
    │  T-Pose 유지 → CalibrationComponent 3초 카운트다운
    │  완료: OnCalibrationComplete → DataLogger.StartLogging()
    │  실패: OnCalibrationFailed → 다시 CALIBRATING
    │
    │  SnapCalibrate() 또는 StartTestingDirectly() 로 바로 건너뛰기 가능
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
    │  ExportAndEnd() → CSV 저장                │
    │                                           │
    │  "New Test" ──────────────────────────────┘
    │  "Export & End"
    ▼
  [IDLE]
```

---

## 7. uplugin 실제 설정

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

> ⚠️ `SteamVR` 플러그인은 `.uplugin`에 명시되지 않음.
> OpenXR이 SteamVR 런타임을 감싸므로, UE5 프로젝트 설정에서 OpenXR을 기본 HMD Plugin으로 설정해야 함.

---

## 8. 레벨 셋업 방법 (다른 프로젝트/차종 적용 시)

```
1. Plugins/VRTrackerCollision/ 폴더를 복사
2. 레벨에 BP_VTC_SessionManager 배치
3. 레벨에 BP_VTC_BodyActor 배치
4. GameMode를 VTC_GameMode로 설정 (TrackerPawn 자동 스폰)
5. 차량 Interior Mesh 위에 BP_VTC_ReferencePoint 배치
   (PartName 입력: "AC Unit", "Dashboard" 등)
6. SessionManager의 ReferencePoints 배열에 등록
→ 차량 모델만 교체하면 다른 차종에 바로 적용 가능
```

---

## 9. 구현 현황 및 남은 작업 (다음 주까지)

### ✅ C++ 구현 완료

| 파일 | 상태 | 내용 |
|------|------|------|
| VTC_TrackerTypes.h | ✅ 완료 | 공통 Enum/Struct 정의 |
| VTC_TrackerInterface.h | ✅ 완료 | TrackerPawn 접근 인터페이스 |
| VTC_TrackerPawn | ✅ 완료 | HMD+5 Tracker 통합 Pawn |
| VTC_GameMode | ✅ 완료 | TrackerPawn 자동 스폰 |
| VTC_BodySegmentComponent | ✅ 완료 | Dynamic Cylinder 세그먼트 |
| VTC_BodyActor | ✅ 완료 | 가상 신체 Actor |
| VTC_CalibrationComponent | ✅ 완료 | T-Pose 캘리브레이션 |
| VTC_ReferencePoint | ✅ 완료 | 차량 기준점 Actor |
| VTC_CollisionDetector | ✅ 완료 | 거리 측정 + 충돌 감지 |
| VTC_WarningFeedback | ✅ 완료 | 시각/청각 피드백 |
| VTC_DataLogger | ✅ 완료 | CSV 로깅 |
| VTC_SessionManager | ✅ 완료 | 세션 상태머신 |

### 🔧 Blueprint / Asset 작업 필요

| 작업 | 우선순위 |
|------|---------|
| BP_VTC_TrackerPawn 생성 (MotionSource 설정) | 🔴 높음 |
| BP_VTC_BodyActor 생성 (Body Segment Material 연결) | 🔴 높음 |
| BP_VTC_ReferencePoint 생성 | 🔴 높음 |
| BP_VTC_SessionManager 생성 (시스템 레퍼런스 연결) | 🔴 높음 |
| WBP_VTC_HUD 제작 (거리/상태 표시) | 🟡 중간 |
| Material (Body Segment Safe/Warning/Collision) | 🟡 중간 |
| Niagara FX 설정 (CollisionImpact, WarningPulse) | 🟢 낮음 |
| Sound Cue 설정 | 🟢 낮음 |
| 테스트 레벨 구성 + ReferencePoint 배치 | 🔴 높음 |
