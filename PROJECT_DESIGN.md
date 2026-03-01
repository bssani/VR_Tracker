# VR Knee Collision Test System — Project Design Document

> **Plugin:** `VRTrackerCollision` (클래스 접두사: `VTC_`)
> **Engine:** Unreal Engine 5.5

---

## Project Overview

운전자가 차량에 탑승/하차할 때 무릎, 발, 엉덩이가 차량 내부 부품(에어컨, 대시보드, 센터콘솔 등)에 닿는지 VR 환경에서 테스트하는 시스템.

**VR HMD:** HTC Vive Pro 2 (OpenXR)
**Trackers:** HTC Vive Tracker 3.0 × 5개 (Waist, L/R Knee, L/R Foot)
**Input System:** SteamVR → OpenXR Plugin (OpenXR이 SteamVR 런타임 래핑)

---

## Two-Level Architecture

시스템은 **Level 1 (Setup)**과 **Level 2 (Test)** 두 레벨로 분리 운용됩니다.

```
[Level 1 — VTC_SetupLevel]
  데스크탑 UI (마우스 조작)
  ├─ GameMode: VTC_SetupGameMode
  ├─ 피실험자 정보 입력 (SubjectID, Height)
  ├─ 실행 모드 선택 (VR / Simulation)
  ├─ Mount Offset × 5, Vehicle Hip Position 설정
  ├─ [NEW] Warning/Collision 임계값 슬라이더 (Feature A)
  ├─ [NEW] 차종 프리셋 저장/불러오기 ComboBox (Feature B)
  ├─ INI 파일로 설정 저장/불러오기
  └─ [Start Session] → GameInstance에 저장 → Level 2 로드

       ↓  FVTCSessionConfig (GameInstance에 보관, 레벨 전환 간 유지)

[Level 2 — VTC_TestLevel]
  VR/시뮬레이션 테스트 환경
  ├─ GameMode: VTC_GameMode
  ├─ PlayerController: VTC_SimPlayerController (→ VTC_OperatorController 상속)
  │    ├─ F1 캘리브레이션 / F2 테스트 / F3 CSV 내보내기
  │    ├─ Escape → Level 1 복귀
  │    └─ WASD + 마우스 시뮬레이션 이동
  ├─ 3D WorldSpace 위젯 (VTC_StatusActor → VTC_StatusWidget)
  ├─ [NEW] VTC_OperatorViewActor (SceneCapture → Spectator Screen) (Feature I)
  └─ 세션 상태머신: IDLE → CALIBRATING → TESTING → REVIEWING
```

### 레벨 간 데이터 전달

- **VTC_GameInstance** (UGameInstance 상속): `FVTCSessionConfig` 보관
- **VTC_SessionConfig.h**: 설정 구조체 (SubjectID, Height, RunMode, MountOffsets × 5, VehicleHipPosition, 가시성 등)
  - **[NEW]** `WarningThreshold_cm`, `CollisionThreshold_cm` — 임계값 슬라이더 값
  - **[NEW]** `bUseVehiclePreset`, `SelectedPresetName`, `LoadedPresetJson` — 프리셋 데이터
- **INI 파일**: `Config/VTCSettings.ini` (SubjectID/Height 제외, 나머지 설정 영속 저장)
- **[NEW] VTC_VehiclePreset**: `Saved/VTCPresets/*.json` — 차종별 ReferencePoint 배치 영속 저장

---

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   VRTrackerCollision Plugin              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  [Level 1]                                              │
│  VTC_SetupGameMode → VTC_SetupWidget (Desktop UI)      │
│       └─ Start Session → VTC_GameInstance → Level 2     │
│                                                         │
│  [Level 2]                                              │
│  VTC_SimPlayerController → VTC_OperatorController       │
│       └─ ApplyGameInstanceConfig() → 각 Actor에 적용    │
│                                                         │
│  ┌──────────────────┐    ┌──────────────────┐           │
│  │  VTC_TrackerPawn │───▶│  VTC_BodyActor   │           │
│  │  (Input Layer)   │    │  (Body Model)    │           │
│  │  IVTC_Tracker    │    │  Segments×4      │           │
│  │  Interface       │    │  Spheres×5       │           │
│  │                  │    │  VisualSphere×5  │           │
│  └────────┬─────────┘    └────────┬─────────┘           │
│           │ (interface)           │                     │
│           └───────────────────────┤                     │
│                                   ▼                     │
│                      ┌────────────────────────┐         │
│                      │  VTC_CollisionDetector │         │
│                      │  거리 측정 (30Hz)       │         │
│                      │  + Overlap 감지         │         │
│                      │  + WarningLevel 결정   │         │
│                      └───────────┬────────────┘         │
│                                  │                      │
│                    ┌─────────────┴──────────┐           │
│                    ▼                        ▼           │
│          ┌──────────────────┐   ┌────────────────────┐  │
│          │ VTC_WarningFeed  │   │  VTC_SessionManager│  │
│          │ back             │   │  + VTC_DataLogger  │  │
│          │ PostProcess/SFX/ │   │  (CSV, 10Hz)       │  │
│          │ Niagara FX       │   │  상태머신           │  │
│          └──────────────────┘   └────────────────────┘  │
│                                                         │
│  VTC_StatusActor → VTC_StatusWidget (WorldSpace 3D)    │
│  VTC_ReferencePoint × N + VehicleHipPosition (동적)     │
└─────────────────────────────────────────────────────────┘
```

---

## Core Systems

### 1. Tracker Input Layer — `VTC_TrackerPawn`

HMD Camera와 5개 Vive Tracker를 하나의 Pawn에 통합.

**컴포넌트 구조:**
```
Root (SceneComponent)
└─ VROrigin (SceneComponent) ← SteamVR/OpenXR 트래킹 공간 원점
     ├─ Camera (CameraComponent)  ← HMD 자동 추적
     ├─ MC_Waist      (MotionController, MotionSource: Special_1)
     ├─ MC_LeftKnee   (MotionController, MotionSource: Special_2)
     ├─ MC_RightKnee  (MotionController, MotionSource: Special_3)
     ├─ MC_LeftFoot   (MotionController, MotionSource: Special_4)
     └─ MC_RightFoot  (MotionController, MotionSource: Special_5)
```

**핵심 설계 — `IVTC_TrackerInterface`:**
- `VTC_TrackerPawn`이 이 인터페이스를 구현함
- `BodyActor`, `BodySegmentComponent`, `CalibrationComponent` 등이 이 인터페이스로 Tracker 데이터에 접근
- 느슨한 결합(Loose Coupling) → Pawn 구조가 바뀌어도 다른 클래스 수정 불필요

```cpp
// 인터페이스 주요 메서드
FVTCTrackerData GetTrackerData(EVTCTrackerRole Role) const;
FVector         GetTrackerLocation(EVTCTrackerRole Role) const;
bool            IsTrackerActive(EVTCTrackerRole Role) const;
bool            AreAllTrackersActive() const;
int32           GetActiveTrackerCount() const;
```

---

### 2. Controller Hierarchy — `VTC_OperatorController` / `VTC_SimPlayerController`

```
VTC_SimPlayerController → VTC_OperatorController → APlayerController
```

**VTC_OperatorController (부모):**
- F1/F2/F3/Escape 단축키 바인딩
- GameInstance 설정 → TrackerPawn, BodyActor, CollisionDetector에 일괄 적용
- VehicleHipPosition → ReferencePoint 런타임 스폰 + CollisionDetector 등록
- StatusActor (3D 월드 위젯) 갱신 (Tick 1초마다)
- EndPlay에서 SpawnedHipRefPoint 명시적 정리
- BeginPlay / OnPossess 이중 설정 적용 방지 (bConfigApplied 플래그)

**VTC_SimPlayerController (자식):**
- WASD + 마우스 시뮬레이션 이동
- Enhanced Input (IA 7개 + IMC 1개)
- 무릎 오프셋 실시간 조절 (NumPad, 화살표 키)

> **하나의 PlayerController만 지정하면 됨**: BP_VTC_SimPlayerController가 세션 제어 + 시뮬레이션 이동 모두 처리

---

### 3. Body Model System — `VTC_BodyActor`

**`VTC_BodySegmentComponent` — Dynamic Cylinder:**
- 두 Tracker 사이를 실시간으로 Cylinder로 연결
- 매 Tick: MidPoint 계산 → SetWorldLocation, Direction → SetWorldRotation(Z축 -90도 보정)
- Tracker가 미추적 상태이면 Cylinder 숨김

**`VTC_CalibrationComponent` — T-Pose 캘리브레이션:**
- `StartCalibration()`: 3초 카운트다운 후 Tracker 간 거리를 `FVTCBodyMeasurements`에 저장
- `SnapCalibrate()`: 즉시 캘리브레이션 (카운트다운 없이)
- `SetManualMeasurements()`: 수동 입력 지원
- HMD 높이 × 0.92 = 추정 신장 (HeightCorrectionFactor)

**Sphere Collision (충돌 감지용):**
| 신체 부위 | 기본 반경 |
|---------|---------|
| Hip (Waist) | 12 cm |
| Left/Right Knee | 8 cm |
| Left/Right Foot | 10 cm |

**bShowCollisionSpheres 패턴:**
- `ApplySessionConfig()`에서 `bShowCollisionSpheres` 멤버 변수에 저장
- `SyncSpherePositions()` (Tick)에서 이 멤버를 참조하여 가시성 결정
- Tick이 ApplySessionConfig 설정을 덮어쓰지 않도록 설계

---

### 4. Vehicle Reference Point — `VTC_ReferencePoint`

- 차량 Interior 돌출 부위에 에디터에서 수동 배치
- `PartName`: 데이터 로그에 기록 ("AC Unit", "Dashboard" 등)
- `RelevantBodyParts`: 어느 신체 부위와 측정할지 지정
- 마커 색상이 경고 단계에 따라 변경됨

**VehicleHipPosition (동적 스폰):**
- Level 1에서 입력한 차량 설계 기준 Hip 좌표
- OperatorController::ApplyGameInstanceConfig()에서 ReferencePoint를 런타임 스폰
- CollisionDetector.ReferencePoints.AddUnique()로 수동 등록 (AutoFind 이후이므로)
- EndPlay에서 명시적 Destroy + null 처리

---

### 5. Collision & Distance — `VTC_CollisionDetector`

**거리 계산:** 30Hz로 제한 (성능 최적화)

**경고 단계:**
| 단계 | 거리 기준 | 시각 피드백 | 추가 피드백 |
|------|---------|-----------|-----------|
| **SAFE** | > 10 cm | — | — |
| **WARNING** | 3 ~ 10 cm | Vignette 0.5 | WarningSFX |
| **COLLISION** | ≤ 3 cm or Overlap | Vignette 1.0 | CollisionSFX + Niagara |

**주요 Delegate:**
- `OnWarningLevelChanged(BodyPart, PartName, NewLevel)` → WarningFeedback + DataLogger 연결
- `OnDistanceUpdated(DistanceResult)` → HUD 업데이트

---

### 6. Warning Feedback — `VTC_WarningFeedback`

- **PostProcess Vignette**: 화면 테두리 색상 변화 (Warning: 0.5, Collision: 1.0)
- **Sound**: `WarningSFX` / `CollisionSFX` (쿨다운 0.5초로 연속 재생 방지)
- **Niagara FX**: 충돌 위치에 `CollisionImpactFX` 스폰

---

### 7. Session Management — `VTC_SessionManager`

레벨의 모든 시스템을 조율하는 중앙 Actor.

**상태 머신:**
```
IDLE → CALIBRATING → TESTING → REVIEWING → IDLE
                   ↕ (RequestReCalibration)
모든 상태에서 Escape → Level 1 복귀
```

**주요 함수:**
- `StartSessionWithHeight(SubjectID, Height_cm)`: CALIBRATING 시작 (키 포함)
- `StartSession(SubjectID)`: CALIBRATING 시작 (키 자동 추정)
- `StartTestingDirectly()`: 캘리브레이션 스킵하고 TESTING 진입
- `StopSession()`: REVIEWING으로 이동
- `ExportAndEnd()`: Summary CSV 저장 후 IDLE로 복귀

**Collision Event 기록:**
- `OnWarningLevelChanged` → Collision 레벨일 때 즉시 `DataLogger->LogCollisionEvent()`
- 타임스탬프: 밀리초 정밀도 (`%Y-%m-%d %H:%M:%S.%s`)

---

### 8. Data Logging — `VTC_DataLogger`

두 종류의 CSV를 출력합니다.

#### Summary CSV (`*_summary.csv`) — 세션당 1행, Human Factors 분석용

| 컬럼 그룹 | 컬럼 | 설명 |
|-----------|------|------|
| 기본 정보 | SubjectID, Date | 피실험자 ID, 세션 날짜 |
| 신체 측정 | Height_cm | 키 (수동 입력 우선, 없으면 HMD 추정) |
| 세그먼트 길이 | WaistToKnee_L/R_cm, KneeToFoot_L/R_cm, WaistToFoot_L/R_cm | 캘리브레이션 측정 (cm) |
| Hip 위치 | HipPos_avg_X/Y/Z | 세션 전체 평균 Hip 위치 (UE 좌표, cm) |
| 최소 클리어런스 | HipDist_to_Ref_min_cm, LKnee/RKnee_to_Ref_min_cm | 부위별 최소 거리 |
| 전체 최악 | MinClearance_cm, NearestBodyPart, NearestRefPoint | 전체 최소 클리어런스 + 어디서 발생 |
| 최악 시점 | MinClearance_Timestamp | 최악 클리어런스 발생 시점 (밀리초) |
| 최악 위치 | HipX/Y/Z_atMinClearance | 최악 순간의 Hip 위치 |
| 상태 요약 | OverallStatus, CollisionCount, WarningFrames, TotalFrames | GREEN/YELLOW/RED, 횟수, 프레임 |
| 시간 분석 | TestingStartTime, TestingEndTime, TestingDuration_sec | 테스트 시작/종료/지속시간 |
| 노출 시간 | WarningDuration_sec, CollisionDuration_sec | 경고/충돌 누적 노출 시간 (초) |

> 총 **30+ 컬럼** — 자동차 Human Factors 연구에서 요구하는 핵심 지표 포함

#### Frame CSV (`*_frames.csv`) — 10Hz 원시 프레임 데이터

- 모든 프레임의 5개 신체 부위 위치 (X, Y, Z)
- 각 ReferencePoint까지의 거리 + WarningLevel
- 충돌 발생 여부 및 부품명
- 연구자가 직접 시계열 분석 가능

저장 경로: `[Project]/Saved/VTCLogs/` (기본값)

---

### 9. UI System

#### Level 1 — VTC_SetupWidget (Desktop UMG)

- `VTC_SetupGameMode::BeginPlay()`에서 자동 생성 + AddToViewport
- `VTC_SetupGameMode::EndPlay()`에서 RemoveFromParent + null 처리 (레벨 전환 시 정리)
- BindWidget 패턴으로 C++ ↔ Designer 연결
- NativeConstruct에서 INI 자동 로드
- CB_ModeVR ↔ CB_ModeSimulation 상호 배타 자동 처리

#### Level 2 — VTC_StatusWidget (WorldSpace 3D)

- `VTC_StatusActor`의 WidgetComponent에 WorldSpace로 렌더링
- 4개 TextBlock: Txt_State, Txt_Prompt, Txt_SubjectInfo, Txt_TrackerStatus
- OperatorController가 세션 상태 변경 시 자동 갱신
- 모든 상태에서 "ESC — Return to Setup" 안내 표시

---

## C++ Source Structure (24 Headers + 21 Sources = 45 files)

```
Plugins/VRTrackerCollision/Source/VRTrackerCollision/
├── Public/
│   ├── VRTrackerCollisionModule.h
│   ├── VTC_GameMode.h              ← Level 2 GameMode
│   ├── VTC_SetupGameMode.h         ← Level 1 GameMode
│   ├── VTC_GameInstance.h           ← 레벨 간 설정 전달 + INI
│   ├── VTC_SessionConfig.h          ← FVTCSessionConfig, EVTCRunMode
│   ├── VTC_VehiclePreset.h          ← JSON 차종 프리셋 구조체 + Manager (Feature B)
│   ├── Tracker/
│   │   ├── VTC_TrackerTypes.h
│   │   └── VTC_TrackerInterface.h
│   ├── Pawn/
│   │   └── VTC_TrackerPawn.h
│   ├── Controller/
│   │   ├── VTC_OperatorController.h ← F키 + 설정 적용
│   │   └── VTC_SimPlayerController.h← WASD + Enhanced Input
│   ├── Body/
│   │   ├── VTC_BodyActor.h
│   │   ├── VTC_BodySegmentComponent.h
│   │   └── VTC_CalibrationComponent.h
│   ├── Vehicle/
│   │   └── VTC_ReferencePoint.h
│   ├── Collision/
│   │   ├── VTC_CollisionDetector.h
│   │   └── VTC_WarningFeedback.h
│   ├── Data/
│   │   ├── VTC_DataLogger.h
│   │   └── VTC_SessionManager.h
│   ├── UI/
│   │   ├── VTC_SetupWidget.h
│   │   ├── VTC_StatusWidget.h
│   │   └── VTC_SubjectInfoWidget.h
│   └── World/
│       ├── VTC_StatusActor.h
│       └── VTC_OperatorViewActor.h  ← SceneCapture → Spectator Screen (Feature I)
└── Private/
    └── (각 .cpp 파일, Public과 동일 구조)
    └── 신규: VTC_VehiclePreset.cpp, World/VTC_OperatorViewActor.cpp
```

---

## 남은 작업 — Blueprint / Asset

### 필수 (동작을 위한 최소 요건)

1. **BP_VTC_GameInstance** — Game Instance Class 설정, 레벨 이름 지정
2. **BP_VTC_SetupGameMode + WBP_SetupWidget** — Level 1 UI
3. **BP_VTC_TrackerPawn** — MotionSource 검증
4. **BP_VTC_SimPlayerController** — Enhanced Input 에셋 연결 (IA 7개 + IMC 1개)
5. **BP_VTC_BodyActor** — Material 연결, Sphere Radius 튜닝
6. **BP_VTC_ReferencePoint** — 차량 측정 지점 배치
7. **BP_VTC_SessionManager** — 자동 탐색 (프로퍼티 비워두기)
8. **BP_VTC_StatusActor + WBP_StatusWidget** — 3D 월드 위젯
9. **VTC_SetupLevel / VTC_TestLevel** — 맵 파일 생성 + GameMode Override
10. **PostProcessVolume** — Infinite Extent, WarningFeedback에 연결
11. **BP_VTC_OperatorViewActor** — Level 2에 배치, SceneCapture → Spectator Screen (Feature I)
    - `WBP_SetupWidget`에 Slider_Warning/Collision, Combo_VehiclePreset, Btn_SavePreset BindWidget 연결 필수

### 있으면 좋음

12. **WBP_VTC_HUD** — 거리, 경고 상태, 세그먼트 길이 실시간 표시
13. **Body Segment Material** — Safe/Warning/Collision 색상 변화
14. **Niagara FX + Sound** — 충돌 피드백 이펙트 + 음성 카운트다운 (CountdownSFX[0~3])

---

## Key Technical Considerations

**SteamVR Tracker MotionSource 매핑**
- UE5 OpenXR에서는 `Special_1` ~ `Special_5` 로 최대 5개 Tracker를 구분
- SteamVR에서 Tracker Role 할당 순서와 MotionSource 번호를 일치시켜야 함

**좌표계**
- UE5: Z-Up, cm 단위
- SteamVR: Y-Up, m 단위
- OpenXR Plugin이 자동 변환하지만, 초기 설정 시 실제 위치 확인 필요

**성능**
- Tracker 갱신: 90fps (매 Tick)
- 거리 계산: 30Hz (MeasurementHz로 조절 가능)
- 데이터 로깅: 10Hz (LogHz로 조절 가능)
- StatusWidget TrackerStatus 갱신: 1초마다

**Collision 정밀도**
- Vive Tracker 자체 오차: ~1~2mm
- CollisionThreshold = 3cm (Sphere Overlap 이전에 경고 제공)
- 충돌 이벤트 타임스탬프: 밀리초 정밀도

**의존 플러그인**
- OpenXR (필수 — Vive Pro 2 + Tracker 입력)
- Niagara (필수 — 충돌 FX)
- SteamVR은 런타임 수준에서만 필요 (uplugin에 명시 불필요)
