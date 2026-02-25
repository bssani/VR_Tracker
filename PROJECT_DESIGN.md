# VR Knee Collision Test System — Project Design Document

> **목표:** 다음 주 내 동작 가능한 프로토타입 완성
> **Plugin:** `VRTrackerCollision` (클래스 접두사: `VTC_`)

---

## Project Overview

운전자가 차량에 탑승/하차할 때 무릎, 발, 엉덩이가 차량 내부 부품(에어컨, 대시보드, 센터콘솔 등)에 닿는지 VR 환경에서 테스트하는 시스템.

**Engine:** Unreal Engine 5.5
**VR HMD:** HTC Vive Pro 2 (OpenXR)
**Trackers:** HTC Vive Tracker 3.0 × 5개 (Waist, L/R Knee, L/R Foot)
**Input System:** SteamVR → OpenXR Plugin (OpenXR이 SteamVR 런타임 래핑)

---

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   VRTrackerCollision Plugin              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────────┐    ┌──────────────────┐           │
│  │  VTC_TrackerPawn │───▶│  VTC_BodyActor   │           │
│  │  (Input Layer)   │    │  (Body Model)    │           │
│  │  IVTC_Tracker    │    │  Segments×4      │           │
│  │  Interface       │    │  Spheres×5       │           │
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
│          [WBP_VTC_HUD — Blueprint으로 구현 예정]          │
└─────────────────────────────────────────────────────────┘
```

---

## Core Systems (실제 구현 기준)

### 1. Tracker Input Layer — `VTC_TrackerPawn` ✅ 구현 완료

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

**SteamVR Tracker Role 설정:**
1. SteamVR → Settings → Controllers → Manage Trackers
2. 각 Tracker에 역할 할당 (Waist, Left Knee, Right Knee, Left Foot, Right Foot)
3. UE5에서 MotionSource = Special_1 ~ Special_5로 매핑

---

### 2. Body Model System — `VTC_BodyActor` ✅ 구현 완료

**`VTC_BodySegmentComponent` — Dynamic Cylinder:**
- 두 Tracker 사이를 실시간으로 Cylinder로 연결
- 매 Tick: MidPoint 계산 → SetWorldLocation, Direction → SetWorldRotation(Z축 -90도 보정), Length/100cm → SetWorldScale3D(Z축)
- Tracker가 미추적 상태이면 Cylinder 숨김

**`VTC_CalibrationComponent` — T-Pose 캘리브레이션:**
- `StartCalibration()`: 3초 카운트다운 후 Tracker 간 거리를 `FVTCBodyMeasurements`에 저장
- `SnapCalibrate()`: 즉시 캘리브레이션 (카운트다운 없이)
- `SetManualMeasurements()`: 수동 입력 지원
- HMD 높이 × 0.92 = 추정 신장 (HeightCorrectionFactor)
- 유효성 검사: 모든 세그먼트 > 10cm

**Sphere Collision (충돌 감지용):**
| 신체 부위 | 기본 반경 |
|---------|---------|
| Hip (Waist) | 12 cm |
| Left/Right Knee | 8 cm |
| Left/Right Foot | 10 cm |

---

### 3. Vehicle Reference Point — `VTC_ReferencePoint` ✅ 구현 완료

- 차량 Interior 돌출 부위에 에디터에서 수동 배치
- `PartName`: 데이터 로그와 HUD에 표시 ("AC Unit", "Dashboard" 등)
- `RelevantBodyParts`: 어느 신체 부위(LeftKnee, RightKnee 등)와 측정할지 지정
- 마커 색상이 경고 단계에 따라 변경됨 (Safe=오렌지, Warning=노랑, Collision=빨강)

---

### 4. Collision & Distance — `VTC_CollisionDetector` ✅ 구현 완료

거리 측정과 충돌 감지를 하나의 컴포넌트에 통합.

**거리 계산:** 30Hz로 제한 (성능 최적화)
```
직선 거리 = FVector::Dist(BodyPartLocation, ReferencePointLocation)
```

**경고 단계:**
```
거리 > 10cm:        Safe      (초록)
3cm < 거리 ≤ 10cm: Warning   (노랑)
거리 ≤ 3cm:        Collision  (빨강)
```

| 단계 | 거리 기준 | 시각 피드백 | 추가 피드백 |
|------|---------|-----------|-----------|
| **SAFE** | > 10 cm | — | — |
| **WARNING** | 3 ~ 10 cm | Vignette 0.5 | WarningSFX |
| **COLLISION** | ≤ 3 cm or Overlap | Vignette 1.0 | CollisionSFX + Niagara |

**주요 Delegate:**
- `OnWarningLevelChanged(BodyPart, PartName, NewLevel)` → WarningFeedback 연결
- `OnDistanceUpdated(DistanceResult)` → HUD 업데이트

---

### 5. Warning Feedback — `VTC_WarningFeedback` ✅ 구현 완료

- **PostProcess Vignette**: 화면 테두리 색상 변화 (Warning: 0.5, Collision: 1.0)
- **Sound**: `WarningSFX` / `CollisionSFX` (쿨다운 0.5초로 연속 재생 방지)
- **Niagara FX**: 충돌 위치에 `CollisionImpactFX` 스폰

---

### 6. Session Management — `VTC_SessionManager` ✅ 구현 완료

레벨의 모든 시스템을 조율하는 중앙 Actor.

**상태 머신:**
```
IDLE → CALIBRATING → TESTING → REVIEWING → IDLE
                   ↕ (RequestReCalibration)
```

**주요 함수:**
- `StartSession(SubjectID)`: CALIBRATING 시작
- `StartTestingDirectly()`: 캘리브레이션 스킵하고 TESTING 진입
- `StopSession()`: REVIEWING으로 이동
- `ExportAndEnd()`: CSV 저장 후 IDLE로 복귀

---

### 7. Data Logging — `VTC_DataLogger` ✅ 구현 완료

**기록 항목:**
- Timestamp, SubjectID, 신장, 다리 세그먼트 길이
- 5개 신체 부위 위치 (X, Y, Z)
- 각 ReferencePoint까지의 거리 + WarningLevel
- 충돌 발생 여부 및 부품명

**Export Format:** CSV (10Hz 샘플링)
```csv
Timestamp, SubjectID, Height, UpperLeftLeg, UpperRightLeg, LowerLeftLeg, LowerRightLeg,
HipX, HipY, HipZ, LKneeX, ..., Dist_AC, Dist_Dash, CollisionPart, WarningLevel
```

저장 경로: `[Project]/Saved/VKCLogs/` (기본값)

---

## Folder Structure (Content Browser) — 구성 예정

```
Plugins/VRTrackerCollision/Content/
├── Blueprints/
│   ├── BP_VTC_TrackerPawn.uasset       ← MotionSource 설정 필수
│   ├── BP_VTC_BodyActor.uasset         ← Material 연결
│   ├── BP_VTC_ReferencePoint.uasset
│   └── BP_VTC_SessionManager.uasset    ← 레퍼런스 연결
├── Materials/
│   ├── M_VTC_BodySegment.uasset
│   ├── MI_VTC_Safe.uasset              ← 초록
│   ├── MI_VTC_Warning.uasset           ← 노랑
│   └── MI_VTC_Collision.uasset         ← 빨강
├── FX/
│   ├── NS_VTC_CollisionImpact.uasset
│   └── NS_VTC_WarningPulse.uasset
├── Sounds/
│   ├── SC_VTC_Warning.uasset
│   └── SC_VTC_Collision.uasset
└── UI/
    └── WBP_VTC_HUD.uasset              ← 거리/상태 실시간 표시
```

---

## C++ Source Structure (실제)

```
Plugins/VRTrackerCollision/Source/VRTrackerCollision/
├── Public/
│   ├── VRTrackerCollisionModule.h
│   ├── VTC_GameMode.h                  ← DefaultPawn = VTC_TrackerPawn
│   ├── Tracker/
│   │   ├── VTC_TrackerTypes.h          ← EVTCTrackerRole, FVTCTrackerData 등
│   │   └── VTC_TrackerInterface.h      ← IVTC_TrackerInterface
│   ├── Pawn/
│   │   └── VTC_TrackerPawn.h
│   ├── Body/
│   │   ├── VTC_BodyActor.h
│   │   ├── VTC_BodySegmentComponent.h
│   │   └── VTC_CalibrationComponent.h
│   ├── Vehicle/
│   │   └── VTC_ReferencePoint.h
│   ├── Collision/
│   │   ├── VTC_CollisionDetector.h
│   │   └── VTC_WarningFeedback.h
│   └── Data/
│       ├── VTC_DataLogger.h
│       └── VTC_SessionManager.h
└── Private/
    └── (각 .cpp 파일, Public과 동일 구조)
```

---

## 남은 작업 — 다음 주까지

### 🔴 이번 주 필수 (동작을 위한 최소 요건)

1. **BP_VTC_TrackerPawn** — MotionSource 이름 검증, SteamVR에서 연결 테스트
2. **BP_VTC_BodyActor** — Body Segment에 Material 연결, Sphere Radius 튜닝
3. **BP_VTC_ReferencePoint** — 테스트 차량 Interior에 배치
4. **BP_VTC_SessionManager** — 각 시스템 레퍼런스 연결, Delegate 바인딩
5. **테스트 레벨** — 차량 Mesh + ReferencePoint + SessionManager 배치, GameMode 설정

### 🟡 있으면 좋음

6. **WBP_VTC_HUD** — 거리, 경고 상태, 세그먼트 길이 실시간 표시
7. **Body Segment Material** — Safe/Warning/Collision 색상 변화
8. **Niagara FX + Sound** — 충돌 피드백 이펙트

---

## Key Technical Considerations

**SteamVR Tracker MotionSource 매핑**
- UE5 OpenXR에서는 `Special_1` ~ `Special_5` 로 최대 5개 Tracker를 구분
- `VTC_TrackerPawn`의 MotionSource 이름은 에디터에서 변경 가능 (BlueprintReadWrite)
- SteamVR에서 Tracker Role 할당 순서와 MotionSource 번호를 일치시켜야 함

**좌표계**
- UE5: Z-Up, cm 단위
- SteamVR: Y-Up, m 단위
- OpenXR Plugin이 자동 변환하지만, 초기 설정 시 실제 위치 확인 필요 (bShowDebugSpheres = true 활용)

**성능**
- Tracker 갱신: 90fps (매 Tick)
- 거리 계산: 30Hz (MeasurementHz로 조절 가능)
- 데이터 로깅: 10Hz (LogHz로 조절 가능)
- HUD 업데이트: Delegate 수신 시 (30Hz 연동)

**Collision 정밀도**
- Vive Tracker 자체 오차: ~1~2mm
- Sphere Radius를 실제 신체 크기에 맞게 튜닝 (에디터에서 조절 가능)
- CollisionThreshold = 3cm (Sphere Overlap 이전에 경고 제공)

**의존 플러그인**
- OpenXR (필수 — Vive Pro 2 + Tracker 입력)
- Niagara (필수 — 충돌 FX)
- SteamVR은 런타임 수준에서만 필요 (uplugin에 명시 불필요)
