# VR Tracker Collision (VTC) — 개발자 인수인계 문서

## 1. Project Overview

**VRTrackerCollision** 플러그인은 VR 환경에서 HTC Vive Tracker를 사용하여
피실험자의 하체 움직임을 추적하고, 차량 내부 구조물과의 충돌을 감지·기록하는
UE5 플러그인이다.

| 항목 | 내용 |
|------|------|
| 엔진 | Unreal Engine 5.5 |
| VR 장비 | HTC Vive Pro 2 (HMD) + Vive Tracker 3.0 × 5 |
| 트래커 부위 | Waist, Left Knee, Right Knee, Left Ankle, Right Ankle |
| 레벨 구조 | **단일 VR 레벨** (Setup 레벨 없음) |
| 설정 방식 | 에디터 Details 패널 + INI 파일 (`Config/VTCSettings.ini`) |
| 클래스 접두사 | `VTC_` |

## 2. System Architecture

```
[INI File]  →  [GameInstance]  →  [OperatorController]
                                         │
                ┌────────────────────────┼────────────────────────┐
                ▼                        ▼                        ▼
         [TrackerPawn]            [BodyActor]             [SessionManager]
         5× MotionController     5× BodySegment                  │
         HMD Camera              MountOffset 적용          ┌─────┼─────┐
                │                       │                  ▼     ▼     ▼
                └───────────────────────┘          [Collision] [Data] [Warning]
                      TrackerData 전달             [Detector]  [Logger][Feedback]
                                                       │              │
                                                  [ReferencePoint]  [PostProcess
                                                   (차량 구조물)     Volume]
```

### 핵심 흐름

1. `GameInstance::Init()` → INI 자동 로드
2. 레벨 시작 → `OperatorController::BeginPlay()` → SessionManager/StatusActor 자동 탐색
3. `ApplyGameInstanceConfig()` → TrackerPawn, BodyActor, CollisionDetector에 설정 적용
4. VehicleHipPosition ReferencePoint 동적 스폰 (bMonitorOnly=true)
5. 숫자 키로 세션 제어 → SessionManager 상태 전환

## 3. Core Systems

### TrackerPawn (`VTC_TrackerPawn`)
- VR HMD 카메라 + 5개 MotionControllerComponent를 하나의 Pawn에 통합
- VROrigin 하위에 Camera와 MC_* 배치 → SteamVR 트래킹 좌표계 공유
- `IVTC_TrackerInterface` 구현: GetTrackerData(), GetTrackerLocation() 등
- Dropout 보간 (Feature C): 추적 실패 시 최대 N 프레임 선형 외삽
- Movement Phase 감지 (Feature E): Hip Z 속도로 Entering/Seated/Exiting 판별
- `SnapWaistTo()`: Waist tracker 기준으로 Pawn 전체 이동

### BodyActor (`VTC_BodyActor`)
- 5개 `VTC_BodySegmentComponent` (Sphere)를 관리
- TrackerPawn에서 받은 raw 위치에 **MountOffset** 회전 보정을 적용
- `GetBodyPartLocation()`: MountOffset 적용된 신체 부위 월드 좌표 반환
- CalibrationComponent를 통한 T-Pose 캘리브레이션

### CollisionDetector (`VTC_CollisionDetector`)
- ReferencePoint 배열과 BodyActor 신체 부위 간 거리를 매 Tick 계산
- WarningThreshold / CollisionThreshold 기준으로 경고 단계 판별 (Safe → Warning → Collision)
- `bMonitorOnly` ReferencePoint: 거리 측정만 수행, Warning/Collision 무시
- `OnDistanceUpdated` 델리게이트로 실시간 거리 브로드캐스트

### WarningFeedback (`VTC_WarningFeedback`)
- PostProcessVolume을 제어하여 시각 경고 (노란색=Warning, 빨간색=Collision)
- BeginPlay에서 PostProcessVolume 자동 탐색 (`GetAllActorsOfClass`)
- 경고 단계에 따라 PostProcess 머티리얼 파라미터 동적 변경

### SessionManager (`VTC_SessionManager`)
- 세션 상태 머신: Idle → Calibrating → Testing → Reviewing
- CollisionDetector, DataLogger, WarningFeedback 컴포넌트 보유
- `OnSessionStateChanged` 델리게이트로 상태 변경 브로드캐스트

### DataLogger (`VTC_DataLogger`)
- 테스트 중 트래커 데이터 + 거리 + 경고 단계를 프레임별 기록
- CSV 내보내기: `Saved/VTC_Logs/` 디렉토리에 타임스탬프 파일명

### OperatorController (`VTC_OperatorController`)
- 숫자 키 입력 처리 (1~4)
- GameInstance 설정 → 각 Actor에 일괄 적용 (`ApplyGameInstanceConfig`)
- StatusActor(3D 위젯) + OperatorMonitorWidget(데스크탑 2D) 갱신
- VehicleHipPosition ReferencePoint 동적 스폰
- `SnapToVehicleHip()`: BodyActor의 보정된 Waist 위치 → VehicleHipPosition으로 스냅

### ReferencePoint (`VTC_ReferencePoint`)
- 차량 내부 구조물의 기준점 (Dashboard, Door, Steering Wheel 등)
- `RelevantBodyParts`: 이 기준점과 거리를 측정할 신체 부위 목록
- `bMonitorOnly`: true면 거리 표시만 (Warning/Collision 트리거 안 함)

## 4. Data Flow

```
INI (VTCSettings.ini)
  → GameInstance::Init() → SessionConfig 채움
    → OperatorController::ApplyGameInstanceConfig()
      → TrackerPawn: 트래커 메시 가시성
      → BodyActor: MountOffset + Sphere 가시성
      → CollisionDetector: 임계값 (Warning/Collision)
      → ReferencePoint: VehicleHipPosition 스폰
```

**Tick 데이터 흐름:**
```
TrackerPawn (VR MotionController)
  → TrackerData (위치/회전/추적여부)
    → BodyActor (MountOffset 보정)
      → CollisionDetector (거리 계산)
        → WarningFeedback (시각 경고)
        → DataLogger (CSV 기록)
        → OperatorMonitorWidget (거리 표시)
```

## 5. Key Bindings

| 키 | 기능 | 함수 |
|----|------|------|
| 1 | 캘리브레이션 시작 | `StartCalibration()` |
| 2 | 테스트 시작 (캘리브레이션 건너뜀) | `StartTest()` |
| 3 | 세션 종료 + CSV 내보내기 | `StopAndExport()` |
| 4 | Hip Position 스냅 | `SnapToVehicleHip()` |

## 6. C++ Source File List

### Public Headers

| 파일 | 설명 |
|------|------|
| `Body/VTC_BodyActor.h` | 5개 신체 부위 관리 Actor |
| `Body/VTC_BodySegmentComponent.h` | 개별 신체 부위 Sphere 컴포넌트 |
| `Body/VTC_CalibrationComponent.h` | T-Pose 캘리브레이션 |
| `Collision/VTC_CollisionDetector.h` | 거리 계산 + 경고 단계 판별 |
| `Collision/VTC_WarningFeedback.h` | PostProcess 시각 경고 |
| `Controller/VTC_OperatorController.h` | 세션 제어 PlayerController |
| `Data/VTC_DataLogger.h` | CSV 데이터 기록 |
| `Data/VTC_SessionManager.h` | 세션 상태 머신 |
| `Pawn/VTC_TrackerPawn.h` | VR HMD + 5 Tracker Pawn |
| `Tracker/VTC_TrackerInterface.h` | 트래커 데이터 인터페이스 |
| `Tracker/VTC_TrackerTypes.h` | 트래커 관련 타입/Enum/구조체 |
| `UI/VTC_OperatorMonitorWidget.h` | 운영자 데스크탑 모니터링 위젯 |
| `UI/VTC_StatusWidget.h` | 3D 상태 표시 위젯 |
| `Vehicle/VTC_ReferencePoint.h` | 차량 기준점 Actor |
| `World/VTC_OperatorViewActor.h` | Spectator Screen 관리 |
| `World/VTC_StatusActor.h` | StatusWidget 호스팅 Actor |
| `VRTrackerCollisionModule.h` | 플러그인 모듈 |
| `VTC_GameInstance.h` | INI 기반 세션 설정 관리 |
| `VTC_GameMode.h` | 기본 Pawn/Controller 설정 |
| `VTC_SessionConfig.h` | 세션 설정 구조체 |
| `VTC_VehiclePreset.h` | 차종 프리셋 JSON 직렬화 |

### Private Implementations

| 파일 | 대응 헤더 |
|------|-----------|
| `Body/VTC_BodyActor.cpp` | VTC_BodyActor.h |
| `Body/VTC_BodySegmentComponent.cpp` | VTC_BodySegmentComponent.h |
| `Body/VTC_CalibrationComponent.cpp` | VTC_CalibrationComponent.h |
| `Collision/VTC_CollisionDetector.cpp` | VTC_CollisionDetector.h |
| `Collision/VTC_WarningFeedback.cpp` | VTC_WarningFeedback.h |
| `Controller/VTC_OperatorController.cpp` | VTC_OperatorController.h |
| `Data/VTC_DataLogger.cpp` | VTC_DataLogger.h |
| `Data/VTC_SessionManager.cpp` | VTC_SessionManager.h |
| `Pawn/VTC_TrackerPawn.cpp` | VTC_TrackerPawn.h |
| `UI/VTC_OperatorMonitorWidget.cpp` | VTC_OperatorMonitorWidget.h |
| `UI/VTC_StatusWidget.cpp` | VTC_StatusWidget.h |
| `Vehicle/VTC_ReferencePoint.cpp` | VTC_ReferencePoint.h |
| `VRTrackerCollisionModule.cpp` | VRTrackerCollisionModule.h |
| `VTC_GameInstance.cpp` | VTC_GameInstance.h |
| `VTC_GameMode.cpp` | VTC_GameMode.h |
| `VTC_VehiclePreset.cpp` | VTC_VehiclePreset.h |
| `World/VTC_OperatorViewActor.cpp` | VTC_OperatorViewActor.h |
| `World/VTC_StatusActor.cpp` | VTC_StatusActor.h |
