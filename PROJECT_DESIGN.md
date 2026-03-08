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

## 레벨 아키텍처 (VRTestLevel 단일 진입점)

> **v3.0: SetupLevel 및 Simulation 코드 전면 제거.**
> VRTestLevel이 유일한 진입점. 설정은 `WBP_VTC_ProfileManager` (Editor Utility Widget)에서 사전 저장.

```
[VRTestLevel]
  VR 테스트 환경 (VR Only)
  ├─ GameMode: VTC_VRGameMode
  ├─ PlayerController: VTC_OperatorController
  │    ├─ 1 캘리브레이션 / 2 테스트 / 3 CSV 내보내기
  │    └─ P키: 마지막 적용 프로파일 재적용
  ├─ 3D WorldSpace 위젯 (VTC_StatusActor → VTC_StatusWidget)
  └─ 세션 상태머신: IDLE → CALIBRATING → TESTING → REVIEWING
```

### 데이터 흐름

- **VTC_GameInstance** (UGameInstance 상속): `FVTCSessionConfig` 보관
- **VTC_SessionConfig.h**: 설정 구조체 (SubjectID, Height, MountOffsets × 5, VehicleHipPosition, 가시성 등)
  - `WarningThreshold_cm`, `CollisionThreshold_cm` — 거리 임계값
  - `bUseVehiclePreset`, `SelectedPresetName`, `LoadedPresetJson` — 프리셋 데이터
  - `ProfileName` — 현재 적용된 프로파일 이름
- **JSON 프로파일**: `Saved/VTCProfiles/<Name>.json` — 피실험자+차량 조합 영속 저장
- **VTC_VehiclePreset**: `Saved/VTCPresets/*.json` — 차종별 ReferencePoint 배치 영속 저장

---

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   VRTrackerCollision Plugin              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  [VRTestLevel]                                          │
│  VTC_VRGameMode → VTC_OperatorController                │
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

### 2. Controller — `VTC_OperatorController`

```
VTC_OperatorController → APlayerController
```

**VTC_OperatorController:**
- **단축키:** 1(캘리브레이션) / 2(테스트) / 3(CSV저장+게임종료) / P(프로파일 재적용) / G(JSON저장)
- **NumPad 마운트 오프셋 실시간 조절:**
  - NumPad 1 = Waist 그룹 선택, NumPad 2 = 양쪽 무릎 선택, NumPad 3 = 양쪽 발 선택
  - NumPad 7/8/9 = 선택 그룹 X+1 / Y+1 / Z+1 (cm), NumPad 4/5/6 = X-1 / Y-1 / Z-1 (cm)
  - G키 = 현재 SessionConfig(MountOffset 포함)를 JSON에 저장
- **VR Map 시작 시 아무것도 하지 않음** — Pawn은 PlayerStart에서 대기
- P키 누를 때만: JSON 재로드 → Pawn을 VehicleHipPosition으로 이동 → offset 적용 → widget 업데이트
- VehicleHipPosition → ReferencePoint 런타임 스폰 + CollisionDetector 등록 (`bCollisionDisabled=true`)
- VehicleHipPosition ↔ Waist 실시간 거리 → StatusWidget에 표시
- StatusActor (3D 월드 위젯) 갱신: 상태, 트래커, 경과시간, 최소거리, 거리Row, 프리셋 정보, Hip↔Waist 거리
- EndPlay에서 SpawnedHipRefPoint 명시적 정리

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

- 차량 Interior 돌출 부위에 에디터에서 수동 배치 또는 런타임 동적 스폰
- `PartName`: 데이터 로그에 기록 ("AC Unit", "Dashboard" 등)
- `RelevantBodyParts`: 어느 신체 부위와 측정할지 지정
- `bActive`: false이면 CollisionDetector가 완전히 무시 (측정 없음)
- `bCollisionDisabled`: true이면 거리 시각화(시안색 라인 + 수치)만 하고 Warning/Collision 판정·색상변경·이벤트 없음
- 마커 색상이 경고 단계에 따라 변경됨 (`bCollisionDisabled=false`인 경우)

**VehicleHipPosition (동적 스폰):**
- WBP_VTC_ProfileManager에서 설정한 차량 설계 기준 Hip 좌표
- OperatorController::ApplyGameInstanceConfig()에서 ReferencePoint를 런타임 스폰
- `bCollisionDisabled = true` 자동 설정 → 시안색 라인 + 거리만 표시, 경고 없음
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
Escape → 세션 중단 / IDLE 복귀
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

#### VTC_ProfileManagerWidget (Editor Utility Widget — 사전 설정)

- **레벨과 무관**하게 에디터에서 더블클릭으로 직접 실행 (별도 맵 불필요)
- `Saved/VTCProfiles/<Name>.json` 형식으로 피실험자+차량 조합 프로파일 저장/불러오기/삭제
- VRTestLevel 실행 전 프로파일을 미리 준비해두는 오프라인 설정 도구

#### VTC_StatusWidget (VRTestLevel — WorldSpace 3D)

- `VTC_StatusActor`의 WidgetComponent에 WorldSpace로 렌더링
- **필수 BindWidget** 4개: Txt_State, Txt_Prompt, Txt_SubjectInfo, Txt_TrackerStatus
- **선택 BindWidgetOptional** 6개: Txt_PresetInfo, Txt_CalibResult, Txt_ElapsedTime, Txt_MinDistance, VB_DistanceList, **Txt_HipWaistDistance** (Feature K)
- OperatorController가 세션 상태 변경 시 자동 갱신
- 캘리브레이션 결과: Calibrating→Testing 시 "Cal: OK ✓" / Calibrating→Idle 시 "Cal: FAILED"
- 키 안내: 1(캘리브레이션) / 2(테스트) / 3(CSV저장+게임종료)
- **Txt_HipWaistDistance**: VehicleHip ↔ Waist 실시간 거리 표시 ("Hip↔Waist: 4.2 cm"), 경고/충돌 이벤트 없음

---

## C++ Source Structure

> **삭제된 파일 (v3.0):** `VTC_SetupGameMode`, `VTC_SetupWidget`, `VTC_GameMode`, `VTC_SimPlayerController`, `VTC_OperatorMonitorWidget`, `VTC_OperatorViewActor`

```
Plugins/VRTrackerCollision/Source/VRTrackerCollision/
├── Public/
│   ├── VRTrackerCollisionModule.h
│   ├── VTC_VRGameMode.h             ← VRTestLevel 전용 GameMode (VR Only)
│   ├── VTC_GameInstance.h           ← 설정 저장/불러오기 (JSON)
│   ├── VTC_SessionConfig.h          ← FVTCSessionConfig
│   ├── VTC_VehiclePreset.h          ← JSON 차종 프리셋 구조체 + Manager
│   ├── VTC_ProfileLibrary.h         ← 피실험자+차량 프로파일 CRUD
│   ├── Tracker/
│   │   ├── VTC_TrackerTypes.h
│   │   └── VTC_TrackerInterface.h
│   ├── Pawn/
│   │   └── VTC_TrackerPawn.h
│   ├── Controller/
│   │   └── VTC_OperatorController.h ← 세션 제어 + 설정 적용
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
│   │   ├── VTC_StatusWidget.h
│   │   ├── VTC_SubjectInfoWidget.h
│   │   └── VTC_ProfileManagerWidget.h  ← 프로파일 Editor Utility Widget
│   └── World/
│       └── VTC_StatusActor.h
└── Private/
    └── (각 .cpp 파일, Public과 동일 구조)
```

---

## 남은 작업 — Blueprint / Asset

### 필수 (동작을 위한 최소 요건)

1. **BP_VTC_GameInstance** — Game Instance Class 설정
2. **BP_VTC_TrackerPawn** — MotionSource 검증
3. **BP_VTC_BodyActor** — Material 연결, Sphere Radius 튜닝
4. **BP_VTC_ReferencePoint** — 차량 측정 지점 배치
5. **BP_VTC_SessionManager** — 자동 탐색 (프로퍼티 비워두기)
6. **BP_VTC_StatusActor + WBP_VTC_StatusWidget** — 3D 월드 위젯
7. **VRTestLevel** — 맵 파일 생성 + GameMode Override = BP_VTC_VRGameMode
8. **PostProcessVolume** — Infinite Extent, WarningFeedback에 연결
9. **WBP_VTC_ProfileManager** (Editor Utility Widget) — 피실험자+차량 프로파일 사전 저장

### 있으면 좋음

13. **WBP_VTC_HUD** — 거리, 경고 상태, 세그먼트 길이 실시간 표시
14. **Body Segment Material** — Safe/Warning/Collision 색상 변화
15. **Niagara FX + Sound** — 충돌 피드백 이펙트 + 음성 카운트다운 (CountdownSFX[0~3])

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

**VehicleHipPosition (참조용 마커 — bCollisionDisabled=true)**
- 프로파일 JSON의 VehicleHipPosition 좌표에 시안색 ReferencePoint가 런타임 스폰됨
- [Set Hip Here] 버튼으로 인-VR 캡처 가능 (착석 상태에서 Waist 트래커 위치 → VehicleHipPosition 저장)
- `bCollisionDisabled = true`로 설정되어 항상 시안색 라인 + 거리 수치만 표시
- Warning/Collision 판정, 마커 색상 변경, OnWarningLevelChanged 이벤트 없음
- P키 → ApplyGameInstanceConfig() → `SnapWaistToWithRetry()` 호출로 Waist를 Hip 위치로 이동
- `OnDisabledRefPointDistance` 델리게이트로 실시간 거리를 StatusWidget에 전달 (Feature K)

**TrackerPawn Hip Snap (SnapWaistTo)**
- `SnapWaistToWithRetry()`는 타이머 없이 1회만 스냅 시도 (VR flickering 방지)
- Waist tracker 위치가 유효하면 delta 방식으로 Pawn 전체 이동 (정확한 위치 정렬)
- Waist tracker 위치가 0,0,0(미초기화)이면 XY만 이동하고 Z는 유지 (안전 폴백)
- VR Map 시작 시 자동 스냅 없음 — P키 누를 때만 실행됨

**Feature J — 실시간 마운트 오프셋 NumPad 조절 (VR 전용)**
- NumPad 1/2/3으로 Waist/Knees/Feet 그룹 선택 → NumPad 7-9/4-6으로 X/Y/Z 1cm 단위 조절
- 조절 값은 GameInstance.SessionConfig + BodyActor 멤버에 즉시 반영 (다음 Tick에 적용)
- G키로 JSON 저장 → 다음 INI 로드(P키) 시 복원
- StatusWidget의 Txt_Prompt에 현재 선택 그룹 + offset 값 표시

**Feature K — Hip↔Waist 실시간 거리 위젯**
- CollisionDetector의 `OnDisabledRefPointDistance` 델리게이트: bCollisionDisabled=true 포인트(Vehicle_Hip)의 거리만 별도 브로드캐스트
- OperatorController의 `OnHipRefPointDistance()`: PartName=="Vehicle_Hip" && BodyRole==Waist 필터링
- StatusWidget의 `Txt_HipWaistDistance`에 "Hip↔Waist: X.X cm" 형식으로 표시
- 충돌 감지와 완전히 분리 — Warning/Collision 이벤트 발생하지 않음

**SteamVR 룸 세팅 가이드**
- "앉아서 하기(Seated)" 또는 "서서 하기(Standing Only)" 모드 사용
- 세팅 시 HMD를 차량 시트 위(착석 눈높이)에 놓고 진행
- SteamVR 바닥 높이 = UE5의 VROrigin Z=0
- PlayerStart는 차량 운전석 시트 바닥 위치에 배치
- 대안: TrackerPawn의 `bAutoSnapOnBeginPlay = true` + `SeatHipWorldPosition` 설정으로 자동 보정

**의존 플러그인**
- OpenXR (필수 — Vive Pro 2 + Tracker 입력)
- Niagara (필수 — 충돌 FX)
- SteamVR은 런타임 수준에서만 필요 (uplugin에 명시 불필요)
