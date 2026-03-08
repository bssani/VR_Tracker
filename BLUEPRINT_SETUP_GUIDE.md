# VRTrackerCollision 플러그인 — Blueprint 생성 및 연결 가이드

> **Plugin:** VRTrackerCollision
> **Engine:** Unreal Engine 5.5
> **작성일:** 2026-02-25 / **최종 업데이트:** 2026-03-08

---

## 전체 구조 요약

C++ 클래스들은 이미 완성되어 있고, **Blueprint로 래핑**하고 **레벨에 배치 및 연결**하면 동작합니다.

> **v3.0: SetupLevel 및 Simulation 코드 전면 제거.**
> VRTestLevel이 유일한 진입점. 설정은 `WBP_VTC_ProfileManager` (Editor Utility Widget)에서 사전 저장.

```
[VRTestLevel]  ← VR 전용, 유일한 진입점
  GameMode: BP_VTC_VRGameMode
    └─ DefaultPawn: BP_VTC_TrackerPawn
       PlayerController: BP_VTC_OperatorController
            ├─ BeginPlay → 액터 자동 탐색 + 델리게이트 바인딩
            ├─ P     → 프로파일 재적용 (JSON 재로드 + Pawn Hip 스냅 + 전체 적용)
            ├─ 1     → 캘리브레이션 시작
            ├─ 2     → 테스트 직접 시작 (캘리브레이션 건너뜀)
            ├─ 3     → 세션 종료 + CSV 내보내기 + 게임 종료
            ├─ NumPad 1/2/3 → Waist / 무릎 / 발 오프셋 그룹 선택
            ├─ NumPad 7/8/9 → X+1 / Y+1 / Z+1 cm
            ├─ NumPad 4/5/6 → X-1 / Y-1 / Z-1 cm
            └─ G     → 현재 SessionConfig를 JSON에 저장

  레벨 배치 Actor:
    ├─ BP_VTC_BodyActor
    ├─ BP_VTC_SessionManager
    ├─ BP_VTC_StatusActor (3D 월드 위젯)
    ├─ BP_VTC_ReferencePoint × N (차량 측정 지점)
    └─ PostProcessVolume (Infinite Extent)
```

### 데이터 흐름

```
[사전 설정 — 에디터에서]
WBP_VTC_ProfileManager (Editor Utility Widget)
    ├─ SubjectID, Height, MountOffset × 5, VehicleHipPosition 입력
    ├─ WarningThreshold_cm / CollisionThreshold_cm 설정
    └─ Saved/VTCProfiles/<ProfileName>.json 저장

[VRTestLevel 진입]
OperatorController::BeginPlay() → 자동 탐색 + 델리게이트 바인딩만 수행
    └─ Pawn은 PlayerStart에서 대기

[P키를 누를 때]
ApplyGameInstanceConfig()
    ├─ TrackerPawn.SetTrackerMeshVisible(bShowTrackerMesh)
    ├─ BodyActor.ApplySessionConfig(Config)   (MountOffset, Sphere 가시성)
    ├─ CollisionDetector 임계값 적용
    └─ VehicleHipPosition → HipRefPoint 런타임 스폰 (시안색 마커, bCollisionDisabled)
```

### 만들어야 할 Blueprint 전체 목록

```
[VRTestLevel]
  BP_VTC_VRGameMode           (VTC_VRGameMode 기반)
  BP_VTC_OperatorController   (VTC_OperatorController 기반)
  BP_VTC_TrackerPawn          (VTC_TrackerPawn 기반)
  BP_VTC_BodyActor            (VTC_BodyActor 기반)
  BP_VTC_ReferencePoint       (VTC_ReferencePoint 기반)
  BP_VTC_SessionManager       (VTC_SessionManager 기반)
  BP_VTC_StatusActor          (VTC_StatusActor 기반)
  WBP_VTC_StatusWidget        (VTC_StatusWidget 기반)

[프로파일 관리 — Editor Utility Widget]
  WBP_VTC_ProfileManager      (VTC_ProfileManagerWidget 기반) ← 에디터에서 사전 실행

[공통]
  BP_VTC_GameInstance         (VTC_GameInstance 기반)
  PP_VTC_Warning              (PostProcessVolume — 레벨 배치)

[에셋]
  Material: M_VTC_BodySegment (+ MI_Safe, MI_Warning, MI_Collision)
  Niagara: NS_VTC_CollisionImpact, NS_VTC_WarningPulse
  Sound: SC_VTC_Warning, SC_VTC_Collision
```

---

## BP_VTC_GameInstance 생성

1. Content Browser → Blueprint Class → **All Classes** → `VTC_GameInstance` 검색
2. 이름: `BP_VTC_GameInstance`
3. **Project Settings → Maps & Modes → Game Instance Class = `BP_VTC_GameInstance`**

---

## [VR 레벨 설정] BP_VTC_VRGameMode + BP_VTC_OperatorController

### Step 1: BP_VTC_OperatorController 생성

1. Content Browser → Blueprint Class → **All Classes** → `VTC_OperatorController` 검색
2. 이름: `BP_VTC_OperatorController`

---

### Step 2: BP_VTC_VRGameMode 생성

1. Content Browser → Blueprint Class → **All Classes** → `VTC_VRGameMode` 검색
2. 이름: `BP_VTC_VRGameMode`
3. Details 패널:

| 프로퍼티 | 값 |
|---------|-----|
| Default Pawn Class | `BP_VTC_TrackerPawn` |
| Player Controller Class | `BP_VTC_OperatorController` |

---

### Step 3: VR 테스트 레벨 맵 파일 생성

1. Content Browser → Level → 이름: `VRTestLevel`
2. `VRTestLevel` 열기 → **World Settings → GameMode Override = `BP_VTC_VRGameMode`**
3. 다음 Actor들을 레벨에 배치:

| Actor | 수량 | 위치 |
|-------|------|------|
| `BP_VTC_BodyActor` | 1 | (0, 0, 0) |
| `BP_VTC_SessionManager` | 1 | (0, 0, 0) |
| `BP_VTC_StatusActor` | 1 | 운전석 옆 또는 대시보드 위 |
| `BP_VTC_ReferencePoint` | N개 | 차량 측정 지점마다 |
| PostProcessVolume (Infinite) | 1 | 어디든 |

4. **VR PlayerStart 배치:**
   - PlayerStart를 차량 운전석 시트 **바닥 위치**에 배치 (VROrigin 기준점)
   - SteamVR 룸 세팅은 "앉아서 하기(Seated)" 권장

---

## WBP_VTC_StatusWidget 생성 (3D 월드 배치 위젯)

1. Content Browser → **User Interface → Widget Blueprint**
2. Parent Class: `VTC_StatusWidget`
3. 이름: `WBP_StatusWidget`

#### 필수 BindWidget 목록

| 위젯 타입 | 이름 | 표시 내용 |
|---------|------|---------|
| TextBlock | `Txt_State` | 현재 상태 ("● IDLE" 등) |
| TextBlock | `Txt_Prompt` | 키 안내 ("1 — Start Calibration" 등) |
| TextBlock | `Txt_SubjectInfo` | "Subject: P001 \| Height: 175 cm" |
| TextBlock | `Txt_TrackerStatus` | "Trackers: 5 / 5 Connected" |

#### 선택 BindWidget 목록 (없어도 컴파일 오류 없음 — VR 실시간 정보)

| 위젯 타입 | 이름 | 표시 내용 |
|---------|------|---------|
| TextBlock | `Txt_PresetInfo` | 적용된 차종 프리셋 ("Preset: SantaFe") |
| TextBlock | `Txt_CalibResult` | 캘리브레이션 결과 ("Cal: OK ✓" / "Cal: FAILED — Check trackers") |
| TextBlock | `Txt_ElapsedTime` | 경과 시간 ("02:34") |
| TextBlock | `Txt_MinDistance` | 세션 최솟값 ("Min: 4.2 cm") |
| VerticalBox | `VB_DistanceList` | 기준점별 거리 Row 자동 생성 영역 |
| TextBlock | `Txt_HipWaistDistance` | VehicleHip ↔ Waist 실시간 거리 ("Hip↔Waist: 4.2 cm") — 경고 이벤트 없음 |

#### 권장 Designer 레이아웃 (3D 월드 배치 기준 — 800×600)

```
[Canvas Panel]  크기: 800 × 600
  └─ [Border] 반투명 검은 배경 (Opacity 0.7)
       └─ [Vertical Box]
            ├─ TextBlock Txt_State         폰트 크기 36, Bold, 가운데 정렬
            │                               예) "● TESTING"
            ├─ TextBlock Txt_CalibResult    폰트 크기 20 (OK=초록, FAILED=빨강)
            ├─ Separator (Spacer)
            ├─ TextBlock Txt_Prompt         폰트 크기 24, 줄간격 넉넉히
            │                               예) "3  —  Save CSV & Quit"
            ├─ Separator (Spacer)
            ├─ TextBlock Txt_PresetInfo     폰트 크기 18, 청록색
            ├─ TextBlock Txt_SubjectInfo    폰트 크기 18, 회색
            ├─ TextBlock Txt_TrackerStatus  폰트 크기 18, 회색
            ├─ TextBlock Txt_ElapsedTime    폰트 크기 24, 굵게
            ├─ TextBlock Txt_MinDistance    폰트 크기 20, 강조색
            └─ VerticalBox VB_DistanceList  (비워두기 — C++가 Row 자동 추가)
```

> **3D Widget 주의사항:**
> - 배경을 불투명하게 하면 레벨 반사광의 영향을 덜 받음
> - 폰트 크기는 3D 월드에서 볼 때 기준으로 설정 (작으면 VR에서 안 보임)

---

### BP_VTC_StatusActor 생성

1. Content Browser → Blueprint Class → `VTC_StatusActor`
2. 이름: `BP_VTC_StatusActor`
3. Details 패널:

| 프로퍼티 | 값 |
|---------|-----|
| Status Widget Class | `WBP_VTC_StatusWidget` |

---

## 1. BP_VTC_TrackerPawn (VR 입력 + 트래커 추적)

### 생성 방법
1. Content Browser → 우클릭 → **Blueprint Class**
2. **All Classes** 검색 → `VTC_TrackerPawn` 선택
3. 이름: `BP_VTC_TrackerPawn`
4. 저장 위치: `Plugins/VRTrackerCollision/Content/Blueprints/`

### Details 패널 설정

| 카테고리 | 프로퍼티 | 값 | 설명 |
|---------|---------|-----|------|
| VTC\|Tracker Config | MotionSource_Waist | `Special_1` | Vive Tracker 0 (골반) |
| VTC\|Tracker Config | MotionSource_LeftKnee | `Special_2` | Vive Tracker 1 (왼쪽 무릎) |
| VTC\|Tracker Config | MotionSource_RightKnee | `Special_3` | Vive Tracker 2 (오른쪽 무릎) |
| VTC\|Tracker Config | MotionSource_LeftFoot | `Special_4` | Vive Tracker 3 (왼발) |
| VTC\|Tracker Config | MotionSource_RightFoot | `Special_5` | Vive Tracker 4 (오른발) |
| VTC\|Debug | bShowDebugSpheres | `true` (개발 중) | 디버그 구 표시 |
| VTC\|Debug | DebugSphereRadius | `5.0` | 디버그 구 반지름(cm) |

### MotionSource 매핑 확인
SteamVR → Settings → Controllers → **Manage Trackers**에서:
- 각 물리 트래커를 위 Role(Special_1~5)에 매핑해야 합니다.
- `Special_1` = Waist 트래커, `Special_2` = LeftKnee, ... 순서

### 컴포넌트 구조 (자동 생성됨)
```
BP_VTC_TrackerPawn (Root: DefaultSceneRoot)
  └── VROrigin (SceneComponent)
        ├── Camera (CameraComponent) ← HMD 자동 추적
        ├── MC_Waist (MotionController, Special_1)
        ├── MC_LeftKnee (MotionController, Special_2)
        ├── MC_RightKnee (MotionController, Special_3)
        ├── MC_LeftFoot (MotionController, Special_4)
        └── MC_RightFoot (MotionController, Special_5)
```

### 사용 가능한 이벤트 (Event Graph에서 바인딩 가능)
- **OnTrackerUpdated** (EVTCTrackerRole, FVTCTrackerData) — 개별 트래커 갱신 시
- **OnAllTrackersUpdated** — 전체 트래커 갱신 완료 시 (매 프레임)

### Blueprint 호출 가능 함수
- `BP_GetTrackerData(Role)` → FVTCTrackerData (위치, 회전, 추적 여부)
- `BP_GetTrackerLocation(Role)` → FVector
- `BP_IsTrackerActive(Role)` → bool
- `BP_AreAllTrackersActive()` → bool
- `BP_GetActiveTrackerCount()` → int32

> **참고:** 이 Pawn은 C++ 생성자에서 모든 MotionController를 자동으로 생성합니다. Blueprint에서 별도로 컴포넌트를 추가할 필요 없습니다. Details 패널에서 MotionSource 이름만 확인하세요.

---

## 2. BP_VTC_BodyActor (가상 신체 모델)

### 생성 방법
1. Blueprint Class → `VTC_BodyActor` 기반으로 생성
2. 이름: `BP_VTC_BodyActor`

### Details 패널 설정

| 카테고리 | 프로퍼티 | 기본값 | 설명 |
|---------|---------|-------|------|
| VTC\|Body | TrackerSource | *비워두기* | BeginPlay에서 자동 탐색 |
| VTC\|Body\|Collision Radius | HipSphereRadius | `12.0` | 골반 충돌 구 반지름(cm) |
| VTC\|Body\|Collision Radius | KneeSphereRadius | `8.0` | 무릎 충돌 구 반지름(cm) |
| VTC\|Body\|Collision Radius | FootSphereRadius | `10.0` | 발 충돌 구 반지름(cm) |
| VTC\|Body\|Mount Offset | MountOffset_Waist | `(0,0,0)` | 골반 트래커 마운트 오프셋 (트래커 로컬, cm) |
| VTC\|Body\|Mount Offset | MountOffset_LeftKnee | `(0,0,0)` | 왼쪽 무릎 트래커 마운트 오프셋 |
| VTC\|Body\|Mount Offset | MountOffset_RightKnee | `(0,0,0)` | 오른쪽 무릎 트래커 마운트 오프셋 |
| VTC\|Body\|Mount Offset | MountOffset_LeftFoot | `(0,0,0)` | 왼발 트래커 마운트 오프셋 |
| VTC\|Body\|Mount Offset | MountOffset_RightFoot | `(0,0,0)` | 오른발 트래커 마운트 오프셋 |

> **마운트 오프셋 사용법:**
> Vive Tracker가 무릎 앞으로 2cm 돌출된 경우 (트래커 X축 = 앞 방향):
> `MountOffset_LeftKnee = (2.0, 0.0, 0.0)`
> 트래커 방향이 바뀌어도 자동으로 올바른 월드 위치로 변환됩니다.

### 자동 동작 원리
- **BeginPlay**에서 `TrackerSource`가 비어있으면 `GetAllActorsWithInterface(IVTC_TrackerInterface)`로 **자동 탐색**합니다.
- 즉, BP_VTC_TrackerPawn이 레벨에 스폰되면 자동으로 연결됩니다.
- 매 Tick마다 5개 Sphere를 각 트래커 위치로 동기화합니다.

### 컴포넌트 구조 (자동 생성됨)
```
BP_VTC_BodyActor (Root: DefaultSceneRoot)
  ├── Seg_Hip_LeftKnee (BodySegmentComponent) — Waist↔LeftKnee 실린더
  ├── Seg_Hip_RightKnee (BodySegmentComponent) — Waist↔RightKnee 실린더
  ├── Seg_LeftKnee_LeftFoot (BodySegmentComponent) — LeftKnee↔LeftFoot 실린더
  ├── Seg_RightKnee_RightFoot (BodySegmentComponent) — RightKnee↔RightFoot 실린더
  ├── Sphere_Hip (SphereComponent, 충돌 볼륨)
  ├── Sphere_LeftKnee (SphereComponent)
  ├── Sphere_RightKnee (SphereComponent)
  ├── Sphere_LeftFoot (SphereComponent)
  ├── Sphere_RightFoot (SphereComponent)
  └── CalibrationComp (CalibrationComponent)
```

### Material 연결 (Blueprint에서 설정)
각 BodySegmentComponent의 SegmentMesh에 Material을 적용합니다:

1. **M_VTC_BodySegment** (Material 생성)
   - Translucent 블렌드 모드
   - Base Color: 파라미터 `BodyColor` (기본: 초록)
   - Opacity: 0.5
   - Two Sided: true

2. Event Graph에서 경고 단계에 따라 색상 변경:
```
// Blueprint 이벤트 그래프 (Pseudo)
OnWarningLevelChanged →
  Switch on EVTCWarningLevel:
    Safe      → SetSegmentColor(Green)
    Warning   → SetSegmentColor(Yellow)
    Collision → SetSegmentColor(Red)
```

### 사용 가능한 이벤트
- **OnBodyOverlapBegin** — 신체 Sphere가 다른 오브젝트와 겹칠 때
- **OnBodyOverlapEnd** — 겹침이 끝날 때

---

## 3. BP_VTC_ReferencePoint (차량 기준점)

### 생성 방법
1. Blueprint Class → `VTC_ReferencePoint` 기반으로 생성
2. 이름: `BP_VTC_ReferencePoint`

### Details 패널 설정 (인스턴스마다 다르게)

| 카테고리 | 프로퍼티 | 예시 값 | 설명 |
|---------|---------|--------|------|
| VTC\|Reference Point | PartName | `"Dashboard"` | 차량 부품 이름 |
| VTC\|Reference Point | RelevantBodyParts | `[LeftKnee, RightKnee]` | 거리 측정 대상 신체 부위 |
| VTC\|Reference Point | bActive | `true` | 활성 상태 |
| VTC\|Reference Point | MarkerRadius | `5.0` | 마커 구 크기(cm) |
| VTC\|Reference Point | MarkerColor | `Orange (1,0.5,0,1)` | 마커 기본 색상 |

### 차량별 배치 예시

차량 Interior 메시 위에 다음 위치에 BP_VTC_ReferencePoint를 배치합니다:

| PartName | 위치 | RelevantBodyParts |
|----------|------|-------------------|
| `"Dashboard_Lower"` | 대시보드 하단 | `[LeftKnee, RightKnee]` |
| `"Steering_Column"` | 스티어링 컬럼 | `[LeftKnee, RightKnee]` |
| `"Center_Console"` | 센터 콘솔 측면 | `[RightKnee]` |
| `"Door_Panel_L"` | 왼쪽 도어 내측 | `[LeftKnee, LeftFoot]` |
| `"Glove_Box"` | 글로브박스 | `[RightKnee, RightFoot]` |
| `"AC_Unit"` | 에어컨 유닛 하단 | `[LeftKnee, RightKnee]` |

> **팁:** RelevantBodyParts 배열은 Details 패널에서 `+` 버튼으로 요소를 추가하고 드롭다운에서 선택합니다.

---

## 4. BP_VTC_SessionManager (세션 관리 — 핵심 오케스트레이터)

### 생성 방법
1. Blueprint Class → `VTC_SessionManager` 기반으로 생성
2. 이름: `BP_VTC_SessionManager`

### Details 패널 설정

| 카테고리 | 프로퍼티 | 설정 방법 | 설명 |
|---------|---------|----------|------|
| VTC\|Session\|Systems | TrackerSource | *비워두기* | BeginPlay 자동 탐색 |
| VTC\|Session\|Systems | BodyActor | *비워두기* | BeginPlay 자동 탐색 |
| VTC\|Session\|Systems | CollisionDetector | *자기 자신의 컴포넌트* | 아래 설명 참조 |
| VTC\|Session\|Systems | WarningFeedback | *자기 자신의 컴포넌트* | 아래 설명 참조 |
| VTC\|Session\|Systems | DataLogger | *자기 자신의 컴포넌트* | 아래 설명 참조 |

### 중요: 컴포넌트 연결 방식

SessionManager의 C++ 코드를 보면 `CollisionDetector`, `WarningFeedback`, `DataLogger`는 **UActorComponent** 타입입니다. 하지만 SessionManager 자체에는 이 컴포넌트들이 자동으로 생성되지 않습니다.

**두 가지 접근법:**

#### 방법 A: BeginPlay 자동 탐색 (권장)
SessionManager의 `BeginPlay`에서 레벨 내 액터/컴포넌트를 자동으로 찾습니다:
- `TrackerSource` → TrackerInterface를 구현한 Pawn 탐색
- `BodyActor` → AVTC_BodyActor 타입 탐색
- 나머지 컴포넌트도 자동 초기화

→ **모든 프로퍼티를 비워두면 됩니다!**

#### 방법 B: 수동 레퍼런스 연결 (디버깅용)
레벨에 배치 후 Details 패널에서 드롭다운으로 직접 선택합니다.

### SessionManager에 내장된 컴포넌트

C++ 코드 분석 결과, SessionManager는 자체적으로 `CollisionDetector`, `WarningFeedback`, `DataLogger`를 **소유하는 Actor**입니다. 이 3개는 SessionManager Actor의 컴포넌트로 자동 생성됩니다.

### 사용 가능한 함수 (HUD에서 호출)
- `StartSessionWithHeight("SubjectID", Height_cm)` — **(권장)** 세션 시작, 직접 입력한 키 포함
- `StartSession("SubjectID")` — 키 없이 시작 (HMD 높이에서 자동 추정)
- `StartTestingDirectly()` — 캘리브레이션 건너뛰고 바로 테스트
- `StopSession()` — 테스트 종료 → Reviewing 상태
- `RequestReCalibration()` — 재캘리브레이션 요청
- `ExportAndEnd()` → FString (요약 CSV 경로 반환)
- `IsTesting()`, `IsCalibrating()` → bool
- `GetCurrentBodyMeasurements()` → FVTCBodyMeasurements
- `GetSessionMinDistance()` → float (최소 거리 cm)

### 이벤트
- **OnSessionStateChanged** (EVTCSessionState OldState, EVTCSessionState NewState)
- **OnSessionExported** (FString FilePath)

---

## 5. BP_VTC_VRGameMode

#### 생성 방법
1. Blueprint Class → **All Classes** → `VTC_VRGameMode` 검색 후 선택
2. 이름: `BP_VTC_VRGameMode`

#### Details 패널 설정

| 프로퍼티 | 값 |
|---------|-----|
| Default Pawn Class | `BP_VTC_TrackerPawn` |
| HUD Class | `None` |
| Player Controller Class | `BP_VTC_OperatorController` |

---

## 6. PostProcessVolume (PP_VTC_Warning)

### 레벨에 배치

1. **Place Actors → Volumes → Post Process Volume**
2. Settings:
   - **Infinite Extent (Unbound)**: `true` (전체 레벨에 적용)
   - **Priority**: `1`
   - Vignette Intensity: `0` (초기값, 런타임에 C++가 제어)

3. BP_VTC_SessionManager의 **WarningFeedback 컴포넌트**에서:
   - `PostProcessVolume` → 레벨에 배치한 PostProcessVolume을 드롭다운으로 선택

---

## 레벨 셋업 — 단계별 전체 가이드

### Step 1: GameMode 설정
1. **World Settings** (레벨 열기 → Window → World Settings)
2. **GameMode Override**: `VRTestLevel` → `BP_VTC_VRGameMode`
3. 확인: Default Pawn Class가 `BP_VTC_TrackerPawn`인지

### Step 2: 차량 메시 배치
1. 차량 Interior 3D 메시를 레벨 원점(0,0,0) 근처에 배치
2. 운전석 H-Point(히프 포인트) 위치 확인

### Step 3: BP_VTC_BodyActor 배치
1. `BP_VTC_BodyActor`를 레벨에 **드래그 앤 드롭**
2. **위치: (0, 0, 0)** — 원점에 놓으면 됩니다
3. TrackerSource는 비워두기 (자동 탐색)

### Step 4: BP_VTC_ReferencePoint 배치 (여러 개)
각 측정 지점에 하나씩:
1. `BP_VTC_ReferencePoint`를 레벨에 드래그
2. **차량 부품 표면 근처에 정확히 위치** 시킴
3. Details 패널에서:
   - `PartName` 입력 (예: "Dashboard_Lower")
   - `RelevantBodyParts` 배열에 관련 부위 추가
   - `bActive` — 이 기준점 측정 전체 on/off (false면 거리 계산 자체 안 함)
   - `bCollisionDisabled` — **거리 라인만 표시하고 Warning/Collision 판정 없음**
     - Vehicle_Hip 동적 스폰 마커는 자동으로 `bCollisionDisabled = true` 설정됨
     - 직접 배치하는 참조용 마커에도 활용 가능
4. 필요한 만큼 반복 (보통 6~10개)

### Step 5: BP_VTC_SessionManager 배치
1. `BP_VTC_SessionManager`를 레벨에 드래그
2. **모든 프로퍼티를 비워두기** — BeginPlay에서 자동 탐색
3. 또는 수동 연결이 필요한 경우:
   - `BodyActor` → 레벨의 BP_VTC_BodyActor 선택
   - CollisionDetector의 `ReferencePoints` 배열 → 자동 탐색 (AutoFindReferencePoints)

### Step 6: PostProcessVolume 배치
1. PostProcessVolume 추가 (Infinite Extent = true)
2. SessionManager의 WarningFeedback → PostProcessVolume 연결

### Step 7: 사운드 & FX 에셋 연결

SessionManager의 **WarningFeedback** 컴포넌트에서:
- `WarningSFX` → 경고 사운드 에셋
- `CollisionSFX` → 충돌 사운드 에셋
- `CollisionImpactFX` → Niagara 시스템 (있으면)
- `WarningPulseFX` → Niagara 시스템 (있으면)

SessionManager → BodyActor → **CalibrationComp** 컴포넌트에서 (Feature H):
- `CountdownSFX[0]` → 카운트다운 3초 음성 (예: SC_VTC_Cal_3)
- `CountdownSFX[1]` → 카운트다운 2초 음성 (예: SC_VTC_Cal_2)
- `CountdownSFX[2]` → 카운트다운 1초 음성 (예: SC_VTC_Cal_1)
- `CountdownSFX[3]` → 캘리브레이션 완료 음성 (예: SC_VTC_Cal_Complete)
- 비워두면 무음, 일부만 연결해도 동작함

### Step 8: 플레이 테스트

#### VRTestLevel 실행 워크플로우

1. **에디터에서 사전 설정 (1회):** `WBP_VTC_ProfileManager` (Editor Utility Widget) 실행 → SubjectID, Height, MountOffset, VehicleHipPosition 입력 → 프로파일 JSON 저장
2. **매 세션:** `VRTestLevel` 열기 → **VR Preview(Alt+P)** 실행
3. VR 진입 후: **P키**로 마지막 저장 프로파일 적용

#### 단축키 목록

| 키 | 동작 |
|----|------|
| **P** | 마지막 적용 프로파일 재로드 + Pawn Hip 스냅 |
| **1** | 캘리브레이션 시작 (앉은 자세 유지, 3초 자동 완료) |
| **2** | 캘리브레이션 건너뛰고 테스트 직접 시작 |
| **3** | CSV 저장 + 게임 종료 |
| **G** | 현재 SessionConfig(MountOffset 포함) JSON 저장 |
| NumPad 1/2/3 | Waist / 무릎 / 발 오프셋 그룹 선택 |
| NumPad 7/8/9 | 선택 그룹 X+1 / Y+1 / Z+1 cm |
| NumPad 4/5/6 | 선택 그룹 X-1 / Y-1 / Z-1 cm |

#### SteamVR 룸 세팅
1. SteamVR에서 **"앉아서 하기(Seated)"** 또는 **"서서 하기(Standing Only)"** 모드로 룸 세팅
2. HMD를 **차량 시트 위 (착석 눈높이)**에 놓고 세팅 진행
3. SteamVR 바닥 = UE5의 VROrigin Z=0 → 트래커 높이가 맞는지 확인
4. **PlayerStart**는 차량 운전석 시트 **바닥 위치**에 배치 (VROrigin 기준점)
5. **Hip Position 스냅:** VR 레벨 시작 시 자동 스냅 없음 — **P키를 눌러야** Pawn이 VehicleHipPosition으로 이동합니다.
   - P키 → `ApplyGameInstanceConfig()` → `SnapWaistTo()` 1회 실행 (타이머 없음, VR flickering 방지)
   - Waist tracker 활성화 여부와 관계없이 즉시 스냅 시도 (tracker 미활성이면 XY만 이동)
   - **프리셋에 Vehicle_Hip이 있고 INI에 HipPosition이 없으면** LoadConfigFromINI()에서 자동으로 채워줍니다.
   - 대안: BP_VTC_TrackerPawn에서 `bAutoSnapOnBeginPlay = true` + `SeatHipWorldPosition` 직접 입력 (에디터 고정값)

---

## 연결 관계 다이어그램

```
레벨 배치 구조:

  ┌─────────────────────────────────────────────────┐
  │                VRTestLevel                       │
  │                                                  │
  │   BP_VTC_VRGameMode  ← World Settings Override   │
  │     └─ DefaultPawn: BP_VTC_TrackerPawn           │
  │          (플레이 시 자동 스폰)                      │
  │                                                  │
  │   BP_VTC_BodyActor ◄──── TrackerSource 자동 탐색  │
  │     ├─ 4x BodySegment (실린더)    ◄── TrackerPawn │
  │     ├─ 5x SphereCollision         위치 추적       │
  │     └─ CalibrationComponent                      │
  │                                                  │
  │   BP_VTC_SessionManager ◄── 자동으로 모든것 탐색   │
  │     ├─ CollisionDetector (Component)              │
  │     │    ├─ BodyActor 자동 탐색                    │
  │     │    └─ ReferencePoints 자동 탐색              │
  │     ├─ WarningFeedback (Component)                │
  │     │    └─ PostProcessVolume 참조 ──────┐        │
  │     └─ DataLogger (Component)            │        │
  │                                          │        │
  │   PostProcessVolume (Infinite) ◄─────────┘       │
  │                                                  │
  │   BP_VTC_ReferencePoint × N (차량 위 배치)        │
  │     ├─ "Dashboard_Lower"                         │
  │     ├─ "Steering_Column"                         │
  │     ├─ "Center_Console"                          │
  │     └─ ...                                       │
  │                                                  │
  │   Vehicle Interior Mesh (3D 차량 모델)            │
  └─────────────────────────────────────────────────┘
```

---

## 자동 탐색 vs 수동 연결 정리

| 컴포넌트 | 자동 탐색 방식 | 수동 연결 필요? |
|---------|-------------|--------------|
| TrackerPawn | GameMode가 자동 스폰 | No |
| BodyActor.TrackerSource | `GetAllActorsWithInterface()` | No |
| SessionManager.TrackerSource | `GetAllActorsWithInterface()` | No |
| SessionManager.BodyActor | `GetActorOfClass()` | No |
| CollisionDetector.BodyActor | `GetActorOfClass()` | No |
| CollisionDetector.ReferencePoints | `GetAllActorsOfClass()` | No |
| WarningFeedback.PostProcessVolume | **자동 탐색 없음** | **Yes** — 수동 연결 필요 |
| WarningFeedback.WarningSFX | **자동 탐색 없음** | **Yes** — 에셋 지정 필요 |
| WarningFeedback.CollisionSFX | **자동 탐색 없음** | **Yes** — 에셋 지정 필요 |
| WarningFeedback.CollisionImpactFX | **자동 탐색 없음** | Optional |
| WarningFeedback.WarningPulseFX | **자동 탐색 없음** | Optional |
| CalibrationComp.CountdownSFX | **자동 탐색 없음** | Optional — 4개 음성 에셋 (Feature H) |

---

## CollisionDetector 임계값 설정

SessionManager 내 CollisionDetector 컴포넌트에서:

| 카테고리 | 프로퍼티 | 기본값 | 설명 |
|---------|---------|-------|------|
| VTC\|Collision | WarningThreshold | `10.0` cm | 이 거리 이내 → Warning |
| VTC\|Collision | CollisionThreshold | `3.0` cm | 이 거리 이내 → Collision (≤WarningThreshold 자동 클램프) |
| VTC\|Collision | MeasurementHz | `30.0` Hz | 거리 측정 빈도 |
| VTC\|Debug | bShowDistanceLabels | `true` | 거리 라인 중간에 수치 텍스트 표시 (VR HMD 가시) |
| VTC\|Debug | DebugLineThickness | `1.5` | 디버그 라인 두께 (범위 0.5~5.0, VR에서 가독성 향상) |

> **임계값 자동 클램프 (에디터 + 런타임):**
> - Details 패널에서 WarningThreshold를 낮추면 CollisionThreshold도 자동으로 따라 내려감
> - CollisionThreshold를 올리면 WarningThreshold도 자동으로 따라 올라감
> - 런타임: `PerformDistanceMeasurements()` 호출마다 자동 클램프 적용

---

## Material 생성 가이드

### M_VTC_BodySegment (마스터 머티리얼)
1. Content Browser → Material → `M_VTC_BodySegment`
2. Material Domain: **Surface**
3. Blend Mode: **Translucent**
4. Two Sided: **true**
5. 노드 구성:
   - **Vector Parameter** `BodyColor` (Default: 0, 1, 0, 1 = Green)
   - **Scalar Parameter** `Opacity` (Default: 0.5)
   - BodyColor → Base Color
   - Opacity → Opacity

### Material Instance 3개
- `MI_VTC_Safe`: BodyColor = **(0, 1, 0)** (초록)
- `MI_VTC_Warning`: BodyColor = **(1, 1, 0)** (노랑)
- `MI_VTC_Collision`: BodyColor = **(1, 0, 0)** (빨강)

### M_VTC_ReferenceMarker (ReferencePoint 마커 머티리얼)
1. Content Browser → Material → `M_VTC_ReferenceMarker`
2. Material Domain: **Surface**
3. Blend Mode: **Opaque** (또는 Translucent)
4. 노드 구성:
   - **Vector Parameter** `BaseColor` (Default: 1, 0.5, 0, 1 = Orange)
   - **Scalar Parameter** `EmissiveIntensity` (Default: 1.0)
   - BaseColor → Base Color
   - BaseColor × EmissiveIntensity → Emissive Color
5. BP_VTC_ReferencePoint → MarkerMesh → Material Slot 0에 할당
6. C++이 `CreateAndSetMaterialInstanceDynamic(0)`으로 런타임에 동적 색상 변경 (경고 단계별)

> **Vehicle_Hip 마커**: 시안색 (0, 0.7, 1) — 충돌 감지 없는 순수 위치 마커로 자동 스폰됨. 별도 머티리얼 불필요 (동적 머티리얼이 색상을 직접 설정)

---

## 문제 해결 FAQ

**Q: VR이 작동하지 않아요**
- VR 레벨의 World Settings → GameMode Override가 `BP_VTC_VRGameMode`인지 확인
- SteamVR이 실행 중인지, HMD가 연결되었는지 확인
- **에디터에서 VR 레벨 테스트**: Play 버튼 옆 드롭다운 → **"VR Preview"** 선택

**Q: 트래커가 감지되지 않아요**
- SteamVR에서 트래커가 녹색인지 확인
- Manage Trackers에서 Special_1~5 매핑 확인
- Project Settings → Plugins → OpenXR이 활성화되었는지 확인

**Q: BodyActor가 트래커를 따라가지 않아요**
- BP_VTC_TrackerPawn이 IVTC_TrackerInterface를 구현하는지 확인 (C++에서 이미 구현됨)
- BP_VTC_BodyActor가 레벨에 배치되었는지 확인

**Q: 거리 측정이 안 돼요**
- BP_VTC_ReferencePoint가 레벨에 최소 1개 배치되었는지 확인
- ReferencePoint의 RelevantBodyParts 배열이 비어있지 않은지 확인

**Q: PostProcess 비네팅이 안 나와요**
- PostProcessVolume이 Infinite Extent인지 확인
- SessionManager → WarningFeedback → PostProcessVolume 레퍼런스가 연결되었는지 확인

**Q: CSV가 저장되지 않아요**
- DataLogger의 LogDirectory가 비어있으면 `Saved/VTCLogs/`에 자동 저장
- 파일 쓰기 권한 확인

**Q: Vehicle Hip Position을 설정하면 바로 충돌이 발생해요**
- 이전 버전에서는 Vehicle_Hip이 Waist와 충돌 감지 대상이었으나, 현재 버전에서는 **순수 위치 마커** (시안색)로 변경됨
- `RelevantBodyParts`가 비어있어 CollisionDetector가 건너뜀 → Warning/Collision 미발생
- 실제 충돌 감지는 Dashboard, Door 등 차량 구조물용 ReferencePoint가 담당

---

## [신규] WBP_VTC_ProfileManager — Editor Utility Widget

피실험자·차량 조합별 설정을 사전에 저장하는 **오프라인 설정 도구**입니다.
레벨과 무관하게 **에디터에서 직접 실행**하는 Editor Utility Widget입니다.

### 생성 방법

1. Content Browser → 우클릭 → **Editor Utilities → Editor Utility Widget**
2. Parent Class: **All Classes** → `VTC_ProfileManagerWidget` 검색 후 선택
3. 이름: `WBP_VTC_ProfileManager`

> **실행 방법:** Content Browser에서 `WBP_VTC_ProfileManager`를 **더블클릭** 하면
> 에디터 탭으로 열립니다. 별도의 레벨 없이 에디터 안에서 바로 사용합니다.

### 필수 BindWidget 목록

| 위젯 타입 | 이름 (정확히) | 역할 |
|---------|------------|-----|
| ComboBoxString | `Combo_ProfileSelect` | 저장된 프로파일 목록 |
| EditableTextBox | `TB_ProfileName` | 저장할 프로파일 이름 입력 |
| Button | `Btn_NewProfile` | 입력란 초기화 (새 프로파일 준비) |
| Button | `Btn_LoadProfile` | 선택한 프로파일 불러오기 |
| Button | `Btn_SaveProfile` | 현재 입력값을 프로파일로 저장 |
| Button | `Btn_DeleteProfile` | 선택한 프로파일 삭제 |
| EditableTextBox | `TB_SubjectID` | 피실험자 ID |
| EditableTextBox | `TB_Height` | 키(cm) |
| CheckBox | `Toggle_VRMode` | VR/Simulation 모드 |
| EditableTextBox | `TB_Offset_Waist_X/Y/Z` | Waist 오프셋 |
| EditableTextBox | `TB_Offset_LKnee_X/Y/Z` | 왼무릎 오프셋 |
| EditableTextBox | `TB_Offset_RKnee_X/Y/Z` | 오른무릎 오프셋 |
| EditableTextBox | `TB_Offset_LFoot_X/Y/Z` | 왼발 오프셋 |
| EditableTextBox | `TB_Offset_RFoot_X/Y/Z` | 오른발 오프셋 |
| EditableTextBox | `TB_HipRef_X/Y/Z` | 차량 Hip 기준점 |
| Slider | `Slider_Warning` | Warning 임계값 (3~50cm) |
| Slider | `Slider_Collision` | Collision 임계값 (1~20cm) |
| TextBlock | `Txt_WarningVal` | Warning 슬라이더 현재값 |
| TextBlock | `Txt_CollisionVal` | Collision 슬라이더 현재값 |
| ComboBoxString | `Combo_VehiclePreset` | 차종 프리셋 선택 |
| CheckBox | `CB_ShowCollisionSpheres` | 충돌 구 표시 |
| CheckBox | `CB_ShowTrackerMesh` | Tracker 메시 표시 |
| TextBlock | `Txt_Status` | 저장/불러오기 결과 메시지 (선택) |

### 워크플로우

```
1. WBP_VTC_ProfileManager 열기
2. [New Profile] → 입력란 초기화
3. TB_ProfileName 입력: "Subject01_SantaFe"
4. SubjectID, Height, 각종 오프셋, 차량 프리셋 선택
5. [Save Profile] → Saved/VTCProfiles/Subject01_SantaFe.json 생성

6. VRTestLevel 실행 → P키로 "Subject01_SantaFe" 프로파일 적용
```

### 파일 저장 위치

```
[프로젝트]/Saved/VTCProfiles/
  ├── Subject01_SantaFe.json
  ├── Subject01_GrandStarex.json
  ├── Subject02_SantaFe.json
  └── ...
```


**Q: ReferencePoint를 숨기고 싶어요**
- `SetActive(false)` 호출 → MarkerMesh 숨김 + CollisionDetector가 건너뜀 + 디버그 라인 안 그려짐
- `SetActive(true)` 호출 → 원래대로 복원
- BP에서: Details → `bActive` 체크박스 해제 (또는 Blueprint 이벤트에서 `Set Active` 노드 사용)

**Q: 경고/충돌 사운드가 여러 번 겹쳐서 재생돼요**
- Warning 사운드와 Collision 사운드 모두 **500ms 쿨다운**이 적용되어 있음
- 한 측정 사이클에서 여러 (BodyPart, ReferencePoint) 쌍이 동시 트리거되어도 1회만 재생
- `ResetFeedback()` 호출 시 쿨다운 타이머도 함께 초기화됨

**Q: CSV에 어떤 데이터가 저장되나요?**
- `ExportAndEnd()` / `ExportToCSV()` → `*_summary.csv` (세션당 1행, Human Factors 분석용)
  - 피실험자: SubjectID, Date, Height_cm
  - 신체 측정: WaistToKnee L/R, KneeToFoot L/R, WaistToFoot L/R (cm)
  - Hip 평균 위치: HipPos_avg_X/Y/Z (cm)
  - 최소 클리어런스: HipDist_to_Ref_min, LKnee/RKnee_to_Ref_min (cm)
  - 전체 최악: MinClearance_cm + NearestBodyPart + NearestRefPoint
  - 최악 시점: MinClearance_Timestamp (밀리초 정밀도)
  - 최악 위치: HipX/Y/Z_atMinClearance
  - 상태 요약: OverallStatus (GREEN/YELLOW/RED), CollisionCount, WarningFrames, TotalFrames
  - 시간 분석: TestingStartTime, TestingEndTime, TestingDuration_sec
  - 노출 시간: WarningDuration_sec (Warning 이상 누적), CollisionDuration_sec (Collision만 누적)
- `DataLogger → ExportFrameDataCSV()` → `*_frames.csv` (10Hz 원시 데이터, 연구자용)
  - 5개 신체 부위 위치 (X, Y, Z) + 모든 기준점별 거리 전체 포함
  - 충돌 발생 여부 및 부품명

**Q: 키(Height)가 CSV에 0으로 저장돼요**
- 설정 위젯의 `TB_Height`에 키를 입력하고 **[Save Config]** 버튼을 눌렀는지 확인
- `StartSessionWithHeight(SubjectID, Height_cm)` 호출 여부 확인 (1/2 키 → OperatorController → SessionManager)
- HMD만으로 세션을 시작하면 `EstimatedHeight`(자동 추정, ±5cm 오차)가 사용됨
