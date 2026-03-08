# VR_Tracker 프로젝트 인수인계 문서

> 새로운 대화 시작 시 이 파일을 먼저 읽어 컨텍스트를 파악할 것.
> 마지막 업데이트: 2026-03-08

---

## 프로젝트 개요

**목적:** 차량 승하차 시 피실험자 신체 부위(Hip/Knee/Foot)와 차량 내장재 간 충돌을 VR 환경에서 실시간 감지하는 연구용 시스템.

**엔진:** Unreal Engine 5 (UE5)
**주요 플러그인:** `VRTrackerCollision` (Source 내 자체 개발)
**하드웨어:** HTC Vive HMD + Vive Tracker 5개 (Waist, LeftKnee, RightKnee, LeftFoot, RightFoot)

---

## Git 정보

- **현재 브랜치:** `claude/fix-build-errors-E0n01`
- **Remote:** origin
- **최근 커밋:** `a3bf467` — TextBlock.h include 누락 빌드 에러 수정

---

## 플러그인 소스 구조

```
Plugins/VRTrackerCollision/Source/VRTrackerCollision/
├── Public/
│   ├── VTC_SessionConfig.h         ← 세션 설정 구조체 (FVTCSessionConfig)
│   ├── VTC_GameInstance.h          ← 설정 저장/불러오기 (INI + JSON)
│   ├── VTC_VRGameMode.h            ← VRTestLevel 전용 GameMode (VR Only)
│   ├── VTC_VehiclePreset.h         ← 차종 프리셋 구조체
│   ├── VTC_ProfileLibrary.h        ← 피실험자+차량 프로파일 CRUD (JSON)
│   ├── Body/
│   │   ├── VTC_BodyActor.h         ← 핵심: 신체 전체 Actor (Sphere 5개)
│   │   ├── VTC_CalibrationComponent.h
│   │   └── VTC_BodySegmentComponent.h
│   ├── Collision/
│   │   ├── VTC_CollisionDetector.h ← 거리 측정 + 경고 로직
│   │   └── VTC_WarningFeedback.h
│   ├── Tracker/
│   │   ├── VTC_TrackerTypes.h      ← 공통 Enum/Struct (EVTCTrackerRole 등)
│   │   └── VTC_TrackerInterface.h  ← 트래커 데이터 접근 인터페이스
│   ├── Pawn/
│   │   └── VTC_TrackerPawn.h       ← VR Pawn (HMD + 트래커 관리)
│   ├── Controller/
│   │   └── VTC_OperatorController.h
│   ├── UI/
│   │   ├── VTC_OperatorMonitorWidget.h  ← VRTestLevel 운영자 모니터 (프로파일 드롭다운 + Hip 캡처)
│   │   ├── VTC_ProfileManagerWidget.h   ← Utility Editor 프로파일 관리 위젯 (신규)
│   │   ├── VTC_StatusWidget.h
│   │   └── VTC_SubjectInfoWidget.h
│   ├── Vehicle/
│   │   └── VTC_ReferencePoint.h    ← 차량 기준점 Actor
│   └── World/
│       ├── VTC_OperatorViewActor.h
│       └── VTC_StatusActor.h
├── Private/
│   ├── VTC_ProfileLibrary.cpp      ← SaveProfile/LoadProfile/GetAvailableProfileNames/DeleteProfile
│   └── UI/
│       ├── VTC_ProfileManagerWidget.cpp
│       └── VTC_OperatorMonitorWidget.cpp
```

**삭제된 파일 (v3.0):**
- `VTC_SetupGameMode.h/.cpp` — Level 1 제거됨
- `VTC_SetupWidget.h/.cpp` — Level 1 제거됨
- `VTC_GameMode.h/.cpp` — 시뮬레이션 GameMode 제거됨
- `VTC_SimPlayerController.h/.cpp` — WASD 데스크탑 시뮬레이션 제거됨

**저장 경로:**
- 프로파일: `Saved/VTCProfiles/<ProfileName>.json`
- 마지막 선택 프로파일: `Saved/VTCConfig/LastProfile.txt`

---

## 핵심 데이터 타입

### EVTCTrackerRole (TrackerTypes.h)
```cpp
Waist / LeftKnee / RightKnee / LeftFoot / RightFoot
```

### EVTCWarningLevel (TrackerTypes.h)
```cpp
Safe (Green) / Warning (Yellow) / Collision (Red)
```

### FVTCSessionConfig (SessionConfig.h)
세션 전체 설정. ProfileManager에서 저장 → GameInstance → VRTestLevel에서 읽어 적용.
`VTCProfiles/<Name>.json`으로 저장/불러오기 지원.
- `ProfileName` — 이 설정을 저장한 프로파일 이름
- `SubjectID` / `Height_cm` — 피실험자 정보
- `MountOffset_Waist/LeftKnee/RightKnee/LeftFoot/RightFoot` — 트래커 로컬 공간 오프셋 (cm)
- `VehicleHipPosition` — 차량 Hip 기준점 (월드 좌표). **[Set Hip Here] 버튼으로 인-VR 캡처 가능**
- `WarningThreshold_cm = 10.0f` / `CollisionThreshold_cm = 3.0f`
- `bUseVehiclePreset` / `LoadedPresetJson`
- `bShowCollisionSpheres` / `bShowTrackerMesh`
- ~~`RunMode`~~ — **삭제됨** (항상 VR 모드)

---

## 핵심 로직: GetBodyPartLocation

```cpp
// VTC_BodyActor.cpp
FVector AVTC_BodyActor::GetBodyPartLocation(EVTCTrackerRole TrackerRole) const
{
    const FVector Offset = GetMountOffsetForRole(TrackerRole);
    if (Offset.IsNearlyZero())
        return TrackerSource->GetTrackerLocation(TrackerRole);

    const FVTCTrackerData Data = TrackerSource->GetTrackerData(TrackerRole);
    return Data.WorldLocation + FQuat(Data.WorldRotation).RotateVector(Offset);
    // → 트래커 로컬 Offset을 회전 변환해서 월드 공간에 더함
}
```

**거리 계산 흐름:**
```
TrackerWorldLocation
  + FQuat(TrackerRotation).RotateVector(MountOffset)   ← 트래커 로컬→월드 변환
  = EffectiveBodyLocation

FVector::Dist(EffectiveBodyLocation, ReferencePoint) - BodyPartRadius = SafeDistance
```

---

## MountOffset 설계 원칙

**트래커 로컬 공간(cm)** 기준으로 입력.
트래커가 실제 신체 표면보다 돌출되어 있거나 오프셋이 있을 때 보정.

```
예시: Waist 트래커가 배쪽에 부착 → 실제 골반 중심은 등 방향으로 약 15cm
MountOffset_Waist = (0, -15, 0)   // 트래커 로컬 Y 음수 = 등 방향 (확인 필요)
```

**FQuat 회전 보정이 자동 적용됨** → 자세가 바뀌어도 오프셋 방향이 정확히 따라감.

---

## Hip 트래커 특이사항

### 문제
- Waist 트래커가 **배(복부) 쪽에 부착**됨
- `GetBodyPartLocation(Waist)`가 배 위치를 반환 → 실제 골반 중심보다 앞에 있음
- `VehicleHipPosition`에 맞추면 차량 기준점이 의미를 잃음

### 올바른 해결 방법
`VehicleHipPosition`을 조정하지 말고, **`MountOffset_Waist`를 이용해 배→골반 중심 오프셋 보정**.

이유: VehicleHipPosition을 조정하면 FQuat 회전 보정 혜택을 못 받아서, 피실험자가 앞으로 숙이거나 기울면 오차가 커짐.

---

## 최근 작업 이력 (커밋 기준)

| 커밋 | 내용 |
|------|------|
| `a3bf467` | TextBlock.h include 누락으로 인한 UTextBlock 빌드 에러 수정 |
| `(이전)` | Level 1 + Simulation 코드 전면 제거 + OperatorMonitor에 [Set Hip Here] 버튼 추가 |
| `7048ab3` | 피실험자+차량 프로파일 시스템 (VTC_ProfileLibrary, VTC_ProfileManagerWidget) + TrackerMesh 가시성 토글 |
| `247a85e` | 트래커 미연결 시 VisualMesh 숨김, Hip↔Waist 거리 위젯, NumPad 오프셋 실시간 조절 |
| `31dca20` | VR에서 P키 이후 깜빡거림 해결 (SnapWaistToWithRetry 타이머 제거) |
| `679ab79` | VR Map 시작 시 자동 설정 적용 제거 — P키 전용으로 변경 |
| `1590ef8` | ConfigFile.h 컴파일 오류 → JSON 저장 방식으로 전환 |
| `8db0668` | TrackerSource retry에서 연결 + 부분 캘리브레이션 지원 |

---

## 미결 논의 사항

1. **Hip MountOffset 값 측정**: 배쪽 트래커→골반 중심까지 실측값이 아직 미결정.
   캘리브레이션 단계에서 자동 측정하는 별도 스텝 추가를 고려 중.

2. **Hip 트래커 위치 보정**: `MountOffset_Waist`로 등 방향 오프셋 적용하는 것이 결정됨.
   `VehicleHipPosition`은 건드리지 않음.

---

## 주의사항

- **Level 1 / Simulation 코드 제거됨**: `VTC_SetupGameMode`, `VTC_SetupWidget`, `VTC_GameMode`, `VTC_SimPlayerController` 삭제.
  VRTestLevel이 유일한 진입점. Blueprint `.uasset` 파일도 정리 필요 (에디터에서 수동 삭제).
- **[Set Hip Here] 버튼**: 피실험자가 차량 시트에 앉은 상태에서 누르면 Waist 트래커 위치를
  `VehicleHipPosition`으로 캡처하고 현재 프로파일 JSON에 저장함 → 수동 좌표 입력 불필요.
- **프로파일 시스템**: `WBP_VTC_ProfileManager` (Utility Editor 위젯)에서 사전 저장.
  VRTestLevel의 `WBP_VTC_OperatorMonitor` 드롭다운에서 선택 후 [Apply] → 즉시 적용.
  저장 경로: `Saved/VTCProfiles/<Name>.json`
- **TrackerMesh 가시성**: `CB_TrackerMeshVisible` 체크박스로 런타임 토글 가능 (OperatorMonitor).
- **P키**: VR 레벨에서 마지막 적용 설정 재적용 (프로파일 드롭다운과 별개)
- Tracker 미연결 시 VisualSphere 자동 숨김 (트래커 연결 상태 = `bIsTracked` 기준)
- `SyncSpherePositions()`는 Tick마다 실행됨 — 무거운 연산 추가 주의
