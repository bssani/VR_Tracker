# VR Knee Collision Test System - Project Design Document

## Project Overview
운전자가 차량에 탑승/하차할 때 무릎, 발, 엉덩이가 차량 내부 부품(에어컨, 대시보드, 센터콘솔 등)에 닿는지 VR 환경에서 테스트하는 시스템.

**Engine**: Unreal Engine 5.5
**VR HMD**: HTC Vive Pro 2 (OpenXR)
**Trackers**: HTC Vive Tracker 3.0 × 3 (Knee, Foot, Hip)
**Input System**: SteamVR → OpenXR Plugin

---

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    VR Knee Collision System              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────┐    ┌──────────────┐    ┌───────────┐ │
│  │  VR Tracker  │───▶│  Body Model  │───▶│ Collision │ │
│  │  Input Layer │    │  System      │    │ Detector  │ │
│  └──────────────┘    └──────────────┘    └─────┬─────┘ │
│         │                    │                  │       │
│         │                    ▼                  ▼       │
│         │            ┌──────────────┐    ┌───────────┐ │
│         │            │  Distance    │    │ Warning   │ │
│         │            │  Measurement │    │ Feedback  │ │
│         └───────────▶│  System      │    │ System    │ │
│                      └──────────────┘    └───────────┘ │
│                             │                  │       │
│                             ▼                  ▼       │
│                      ┌──────────────────────────────┐  │
│                      │     VR HUD / UI Widget       │  │
│                      └──────────────────────────────┘  │
│                                    │                    │
│                             ┌──────┴──────┐            │
│                             │  Data Log   │            │
│                             │  (CSV Export)│            │
│                             └─────────────┘            │
└─────────────────────────────────────────────────────────┘
```

---

## Core Systems

### 1. VR Tracker Input Layer

Vive Tracker 3.0을 SteamVR에서 역할(Role)을 지정하여 사용.

**Tracker Role 매핑:**
| Tracker | SteamVR Role | 부착 위치 |
|---------|-------------|-----------|
| Tracker 1 | Left Knee / Right Knee | 무릎 위 (슬개골) |
| Tracker 2 | Left Foot / Right Foot | 발등 또는 발목 |
| Tracker 3 | Waist | 골반 (Hip) |

**구현 방식:**
- OpenXR Plugin을 통해 Tracker 위치/회전 데이터를 받음
- `UMotionControllerComponent`를 사용하여 각 Tracker를 Actor에 바인딩
- SteamVR Input Action Manifest에서 Tracker Role을 명시적으로 매핑

**핵심 클래스:**
- `AViveTrackerManager` (Actor): 모든 Tracker 데이터를 관리
  - `MotionController_Knee` (MotionControllerComponent, MotionSource: "Special_1")
  - `MotionController_Foot` (MotionControllerComponent, MotionSource: "Special_2")
  - `MotionController_Hip` (MotionControllerComponent, MotionSource: "Special_3")
  - Tracker가 연결되지 않았을 때의 Fallback 처리

---

### 2. Body Model System (핵심 - 키/다리 길이 대응)

**문제:** 사람마다 키가 다르고, 다리 길이가 달라서 가상 다리 모델이 실제와 맞아야 함.

**해결 방식: Tracker 기반 실시간 Body Calibration**

#### 방식 A: 자동 캘리브레이션 (권장)
피험자가 **T-Pose** 또는 **직립 자세**로 서면, Tracker 간 거리를 자동 계산:

```
Hip Tracker Position ──── (Upper Leg Length) ──── Knee Tracker Position
Knee Tracker Position ─── (Lower Leg Length) ──── Foot Tracker Position
Hip to Head(HMD) ──────── (Torso Length) ────── HMD Position
Total Leg Length = Upper Leg + Lower Leg
```

- 캘리브레이션 버튼을 누르면 현재 Tracker 위치를 기준으로 Segment 길이 저장
- 이후 움직임에서도 이 비율을 유지하여 사실적인 다리 표현

#### 방식 B: 수동 입력
- UI에서 키(cm), 상체/하체 비율을 직접 입력
- 한국인 평균 인체 데이터(Size Korea)를 기본값으로 제공

#### 방식 C: 프리셋 (실무 효율)
- Percentile 기반 프리셋: 5th, 25th, 50th, 75th, 95th
- SAE J826 기반 H-Point 데이터 연동 가능

**핵심 클래스:**
- `UBodyCalibrationComponent` (ActorComponent)
  - `float UpperLegLength` — 엉덩이~무릎 거리
  - `float LowerLegLength` — 무릎~발 거리
  - `float TotalLegLength` — 전체 다리 길이
  - `float SubjectHeight` — 피험자 전체 키
  - `void CalibrateFromTrackers()` — T-Pose에서 자동 캘리브레이션
  - `void SetManualMeasurements(float Height, float UpperLeg, float LowerLeg)` — 수동 입력

- `AVirtualLegActor` (Actor): 시각적 다리 표현
  - Hip → Knee → Foot를 잇는 Skeletal/Procedural Mesh
  - Tracker 위치를 IK Target으로 사용
  - 관절 부위에 Sphere Collision을 부착 (충돌 감지용)

---

### 3. Vehicle Mesh & Reference Point System

**차량 메시 관리:**
- CAD에서 변환된 차량 Interior Mesh를 레벨에 배치
- 부품별로 분리된 Static Mesh (Dashboard, Center Console, A-Pillar, Steering Column 등)

**"제일 많이 나온 부분" 기준점 설정:**
- Blueprint에서 **Reference Point Actor**를 배치
- 차량 Interior에서 가장 돌출된(protruding) 부분에 수동 배치
- 또는 Mesh의 Bounding Box를 분석하여 자동으로 가장 가까운 면(closest surface) 계산

**핵심 클래스:**
- `AVehicleReferencePoint` (Actor)
  - `UStaticMeshComponent` — 기준점 표시용 Marker (작은 Sphere)
  - `FVector ReferenceWorldLocation` — 기준점 월드 위치
  - `FString PartName` — 해당 부품 이름 ("AC Unit", "Dashboard" 등)
  - `bool bAutoDetectClosestSurface` — 자동 감지 모드 플래그

- `AVehicleInteriorManager` (Actor)
  - 차량 전체 Interior Mesh를 관리
  - 부품별 Collision Profile 설정
  - `TArray<AVehicleReferencePoint*> ReferencePoints` — 여러 기준점 관리

---

### 4. Distance Measurement System

무릎 Tracker 위치와 차량 기준점(Reference Point) 사이의 실시간 거리를 계산하고 표시.

**계산 방식:**
```cpp
// 단순 직선 거리
float DirectDistance = FVector::Dist(KneeLocation, ReferencePoint);

// Mesh Surface까지의 최단 거리 (더 정확)
FHitResult Hit;
bool bHit = GetWorld()->LineTraceSingleByChannel(
    Hit, KneeLocation, ReferencePoint,
    ECC_Vehicle_Interior);
float SurfaceDistance = Hit.bBlockingHit ? Hit.Distance : DirectDistance;
```

**표시 정보:**
| 항목 | 설명 |
|------|------|
| Knee ↔ Reference Point | 무릎과 기준점 간 직선 거리 (cm) |
| Knee ↔ Closest Surface | 무릎과 가장 가까운 차량 표면 거리 (cm) |
| Upper Leg Length | 캘리브레이션된 상체 다리 길이 |
| Lower Leg Length | 캘리브레이션된 하체 다리 길이 |
| Total Leg Length | 전체 다리 길이 |
| Collision Status | 충돌 여부 (Safe / Warning / Collision) |

**핵심 클래스:**
- `UDistanceMeasurementComponent` (ActorComponent)
  - `float CurrentDistance` — 현재 거리
  - `float MinDistanceRecorded` — 세션 중 최소 거리
  - `ECollisionZone CurrentZone` — Safe / Warning / Collision
  - `float WarningThreshold` — 경고 거리 (기본값: 5cm)
  - `float CollisionThreshold` — 충돌 거리 (기본값: 0cm, 또는 접촉)

**시각적 거리 표시:**
- 무릎과 기준점 사이에 Debug Line 또는 Beam Particle
- 거리에 따라 색상 변경: 초록(Safe) → 노랑(Warning) → 빨강(Collision)

---

### 5. Collision Detection & Warning System

무릎/발/엉덩이가 차량 부품에 닿았을 때 즉각적인 피드백 제공.

**충돌 감지 방식:**
- 각 Tracker 위치에 `USphereComponent` (Collision Volume) 부착
  - Knee: 반경 ~8cm (무릎 크기 근사)
  - Foot: 반경 ~10cm
  - Hip: 반경 ~12cm
- 차량 Interior Mesh에 Collision 설정 (Complex as Simple 또는 Simplified Collision)
- `OnComponentBeginOverlap` / `OnComponentEndOverlap` 이벤트 활용

**경고 단계:**
```
                Distance
  ────────────────────────────────▶

  ██████████  ▓▓▓▓▓▓▓▓  ░░░░░░░░░░░
  COLLISION   WARNING    SAFE
  (Red)       (Yellow)   (Green)
  0cm         5cm        10cm+
```

| 단계 | 거리 | 시각적 피드백 | 추가 피드백 |
|------|------|--------------|------------|
| **SAFE** | > 10cm | 초록색 거리 라인 | 없음 |
| **WARNING** | 5~10cm | 노란색 라인 + 깜빡임 | 경고음 (옵션) |
| **COLLISION** | ≤ 5cm 또는 Overlap | 빨간색 플래시 + 충돌 지점 하이라이트 | 진동(Controller), 경고 사운드 |

**충돌 시 시각적 효과:**
1. 차량 메시의 충돌 부위가 **빨간색 Emissive**로 변경 (Dynamic Material Instance)
2. 충돌 지점에 **빨간 Sphere/Ring** 이펙트 표시
3. VR HUD에 **"COLLISION DETECTED"** 텍스트 + 빨간 화면 테두리 (Post Process)
4. 충돌 지점의 좌표와 부품 이름을 로그에 기록

**핵심 클래스:**
- `UCollisionWarningComponent` (ActorComponent)
  - `EWarningLevel CurrentWarningLevel` — Safe / Warning / Collision
  - `FOnCollisionDetected OnCollisionDetected` — 충돌 시 Delegate
  - `UMaterialInstanceDynamic* CollisionHighlightMaterial` — 빨간색 하이라이트
  - `void UpdateWarningState(float Distance)` — 거리 기반 상태 갱신

- `ACollisionFeedbackManager` (Actor)
  - Post Process Volume 제어 (화면 빨간색 테두리)
  - Sound Cue 재생
  - Particle/Niagara 이펙트 스폰

---

### 6. VR HUD / UI System

VR 공간에서 실시간 정보를 표시하는 Widget.

**Widget 배치:**
- **World-space Widget**: 차량 옆에 고정된 정보 패널
- **HMD-attached Widget**: 시야 하단에 간략한 상태 표시

**표시 항목:**
```
┌─────────────────────────────────┐
│  VR Knee Collision Test HUD     │
├─────────────────────────────────┤
│  Subject Height:     175.0 cm   │
│  Upper Leg:           45.2 cm   │
│  Lower Leg:           42.8 cm   │
│  Total Leg:           88.0 cm   │
│─────────────────────────────────│
│  Knee → AC Unit:      12.3 cm   │
│  Knee → Dashboard:     8.1 cm   │
│  Status:  ⚠ WARNING             │
│─────────────────────────────────│
│  Min Distance (Session): 3.2 cm │
│  Collisions Count:        2     │
│─────────────────────────────────│
│  [Calibrate]  [Reset]  [Export] │
└─────────────────────────────────┘
```

**핵심 클래스:**
- `UCollisionTestWidget` (UserWidget)
  - 거리, 다리 길이, 충돌 상태 실시간 표시
  - Calibrate / Reset / Export 버튼

---

### 7. Data Logging & Export

테스트 결과를 기록하여 분석에 활용.

**기록 항목:**
- Timestamp
- Subject ID / Height / Leg Measurements
- Tracker Positions (X, Y, Z)
- Distance to each Reference Point
- Collision Events (시간, 위치, 부품명)
- Warning Level 변화 이력

**Export Format:** CSV
```csv
Timestamp, SubjectID, Height, UpperLeg, LowerLeg, KneeX, KneeY, KneeZ, DistToAC, DistToDash, CollisionPart, WarningLevel
2026-02-24 10:30:15.123, S001, 175.0, 45.2, 42.8, 120.5, 35.2, 48.1, 12.3, 8.1, None, Safe
2026-02-24 10:30:15.456, S001, 175.0, 45.2, 42.8, 118.2, 33.1, 46.5, 5.2, 3.1, Dashboard, Collision
```

**핵심 클래스:**
- `UTestDataLogger` (ActorComponent)
  - `void StartLogging(FString SubjectID)`
  - `void StopLogging()`
  - `void ExportToCSV(FString FilePath)`

---

## Folder Structure (Content Browser)

```
Content/
├── VehicleCollisionTest/
│   ├── Blueprints/
│   │   ├── BP_ViveTrackerManager.uasset
│   │   ├── BP_VirtualLeg.uasset
│   │   ├── BP_VehicleReferencePoint.uasset
│   │   ├── BP_VehicleInteriorManager.uasset
│   │   ├── BP_CollisionFeedbackManager.uasset
│   │   └── BP_TestSessionManager.uasset
│   ├── UI/
│   │   ├── WBP_CollisionTestHUD.uasset
│   │   ├── WBP_CalibrationPanel.uasset
│   │   └── WBP_ResultsPanel.uasset
│   ├── Materials/
│   │   ├── M_CollisionHighlight.uasset        (빨간색 충돌 하이라이트)
│   │   ├── M_WarningLine.uasset               (거리 라인 머티리얼)
│   │   ├── M_SafeZone.uasset                  (초록색)
│   │   ├── MI_DistanceLine_Safe.uasset
│   │   ├── MI_DistanceLine_Warning.uasset
│   │   └── MI_DistanceLine_Collision.uasset
│   ├── FX/
│   │   ├── NS_CollisionImpact.uasset          (Niagara - 충돌 이펙트)
│   │   └── NS_WarningPulse.uasset             (Niagara - 경고 펄스)
│   ├── Sounds/
│   │   ├── SC_Warning.uasset
│   │   └── SC_Collision.uasset
│   ├── VehicleMeshes/
│   │   ├── SM_Dashboard.uasset
│   │   ├── SM_CenterConsole.uasset
│   │   ├── SM_ACUnit.uasset
│   │   ├── SM_SteeringColumn.uasset
│   │   └── ...
│   └── Maps/
│       ├── L_CollisionTestMap.umap             (메인 테스트 레벨)
│       └── L_CalibrationRoom.umap             (캘리브레이션 전용)
```

---

## C++ Class Structure

```
Source/VR_Knee_Collision/
├── VR_Knee_Collision.h / .cpp                  (Module)
├── VR_Knee_Collision.Build.cs
│
├── Tracker/
│   ├── ViveTrackerManager.h / .cpp             (Tracker 입력 관리)
│   └── TrackerCalibrationData.h                (캘리브레이션 데이터 구조체)
│
├── Body/
│   ├── BodyCalibrationComponent.h / .cpp       (인체 캘리브레이션)
│   └── VirtualLegActor.h / .cpp                (가상 다리 시각화)
│
├── Vehicle/
│   ├── VehicleReferencePoint.h / .cpp          (차량 기준점)
│   └── VehicleInteriorManager.h / .cpp         (차량 Interior 관리)
│
├── Collision/
│   ├── CollisionWarningComponent.h / .cpp      (충돌 감지 + 경고)
│   ├── CollisionFeedbackManager.h / .cpp       (피드백 이펙트 관리)
│   └── CollisionTypes.h                        (Enum, Struct 정의)
│
├── Measurement/
│   ├── DistanceMeasurementComponent.h / .cpp   (거리 측정)
│   └── MeasurementVisualizer.h / .cpp          (거리 시각화 라인)
│
├── UI/
│   └── CollisionTestWidget.h / .cpp            (VR HUD Widget)
│
└── Data/
    └── TestDataLogger.h / .cpp                 (CSV 로깅)
```

---

## Implementation Priority (Phase Plan)

### Phase 1: 기본 VR + Tracker 입력 (Week 1-2)
- [ ] OpenXR 기반 Vive Pro 2 + Vive Tracker 입력 연결
- [ ] Tracker 위치를 월드에 Sphere로 시각화
- [ ] SteamVR Tracker Role 매핑 확인

### Phase 2: Body Calibration + Virtual Leg (Week 2-3)
- [ ] T-Pose 캘리브레이션 구현
- [ ] 다리 길이 자동 계산
- [ ] Virtual Leg 시각화 (Debug Line 또는 Simple Mesh)
- [ ] 수동 입력 UI

### Phase 3: Vehicle Mesh + Reference Point (Week 3-4)
- [ ] 차량 Interior Mesh Import 및 Collision 설정
- [ ] Reference Point Actor 배치 시스템
- [ ] 부품별 태그/이름 관리

### Phase 4: Distance Measurement + Collision Warning (Week 4-5)
- [ ] 실시간 거리 계산 로직
- [ ] 거리에 따른 색상 변화 라인
- [ ] Overlap 기반 충돌 감지
- [ ] 빨간색 경고 피드백 (Material, Post Process, Sound)

### Phase 5: UI + Data Logging (Week 5-6)
- [ ] VR World Widget HUD 구현
- [ ] CSV 데이터 로깅
- [ ] 세션 관리 (Start/Stop/Export)

### Phase 6: Polish + Testing (Week 6-7)
- [ ] 다양한 체형으로 테스트
- [ ] Percentile 프리셋 추가
- [ ] 성능 최적화
- [ ] 팀 데모 및 피드백 반영

---

## Key Technical Considerations

### SteamVR Tracker Role 설정
Vive Tracker를 UE5에서 인식하려면 SteamVR 설정에서 각 Tracker의 Role을 지정해야 함:
1. SteamVR → Settings → Controllers → Manage Trackers
2. 각 Tracker에 역할 할당 (Left Knee, Right Knee, Left Foot, Right Foot, Waist)
3. UE5 OpenXR에서 해당 Role을 MotionSource로 매핑

### Collision Precision
- VR에서의 Collision은 실제 물리적 정밀도가 아닌 근사치
- Tracker 자체의 오차 (~1-2mm)를 고려해야 함
- Collision Sphere 크기를 조절 가능하게 만들어 정밀도 튜닝

### Performance
- Tick마다 거리 계산 → Timer로 주기 조절 가능 (30Hz 정도면 충분)
- Line Trace는 프레임당 제한적으로 사용
- Niagara 이펙트는 충돌 시에만 활성화

### 좌표계
- UE5: Z-Up, cm 단위
- SteamVR: Y-Up, m 단위
- OpenXR Plugin이 자동 변환하지만 검증 필요

---

## Dependencies

**Required Plugins:**
- OpenXR (이미 활성화됨)
- OpenXR Hand Tracking (이미 활성화됨)
- SteamVR Plugin (Vive Tracker 지원)
- Enhanced Input (UE5 기본)

**Optional:**
- Niagara (이펙트)
- UMG (UI)

---

## Notes for Development
- 기존 `CollisionTest.h/.cpp` 파일은 빈 클래스이므로 위 구조로 리팩토링 예정
- VR Template의 MannequinsXR 에셋을 Virtual Leg 시각화에 활용 가능
- CAD 변환 메시는 Datasmith 또는 직접 FBX Import 사용
