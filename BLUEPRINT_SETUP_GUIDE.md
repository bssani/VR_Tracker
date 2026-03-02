# VRTrackerCollision 플러그인 — Blueprint 생성 및 연결 가이드

> **Plugin:** VRTrackerCollision
> **Engine:** Unreal Engine 5.5
> **작성일:** 2026-02-25

---

## 전체 구조 요약

C++ 클래스들은 이미 완성되어 있고, 이제 **Blueprint로 래핑**하고 **레벨에 배치 및 연결**하면 동작합니다.

### Level 1 / Level 2 두 레벨 구조

```
[Level 1 — VTC_SetupLevel]
  GameMode: BP_VTC_SetupGameMode
    └─ BeginPlay → WBP_SetupWidget AddToViewport + 마우스 커서 ON
  WBP_SetupWidget (화면에 표시)
    ├─ SubjectID, Height 입력
    ├─ VR / Simulation 모드 선택 (Toggle_VRMode CheckBox)
    ├─ Mount Offset 5개 (Waist/LKnee/RKnee/LFoot/RFoot) X/Y/Z 입력
    ├─ Vehicle Hip Position X/Y/Z 입력
    ├─ Slider_Warning (3~50cm) + Txt_WarningVal    ← Warning 임계값 (Feature A ✅)
    ├─ Slider_Collision (1~20cm) + Txt_CollisionVal ← Collision 임계값 (Feature A ✅)
    ├─ Combo_VehiclePreset + Btn_SavePreset        ← 차종 프리셋 (Feature B ✅)
    ├─ Collision Sphere 표시 여부 체크박스
    ├─ Tracker Mesh 표시 여부 체크박스
    ├─ [Save Config] / [Load Config] 버튼
    └─ [Start Session] 버튼
          → GameInstance.SessionConfig 저장 + INI 저장 + OpenLevel("VTC_TestLevel")

[Level 2 — VTC_TestLevel]
  GameMode: BP_VTC_GameMode
    └─ DefaultPawn: BP_VTC_TrackerPawn (자동 스폰)
       PlayerController: BP_VTC_SimPlayerController
  BP_VTC_SimPlayerController
    ├─ BeginPlay → GameInstance 설정 읽어서 TrackerPawn/BodyActor에 자동 적용
    ├─ BeginPlay → WBP_VTC_OperatorMonitor 생성 → AddToViewport (클래스 할당 시)
    ├─ F1     → 캘리브레이션 시작 (GameInstance의 SubjectID/Height 사용)
    ├─ F2     → 테스트 직접 시작 (캘리브레이션 건너뜀)
    ├─ F3     → 세션 종료 + CSV 내보내기
    └─ Escape → Level 1(Setup)으로 복귀
  BP_VTC_StatusActor (레벨에 3D 월드 배치 — VR 운전석 앞 대시보드 권장)
    └─ WBP_StatusWidget (WorldSpace 3D 위젯 — VR 피실험자용)
         ├─ 현재 세션 상태 표시 ("● IDLE" / "● TESTING" 등)
         ├─ 키 안내 메시지 (상태마다 자동 변경)
         ├─ 피실험자 정보 (Subject ID + Height)
         └─ 트래커 연결 수 (매 1초 갱신)
  WBP_VTC_OperatorMonitor (Screen Space — 운영자 데스크탑 모니터용)
    ├─ 세션 상태 + 피실험자 정보 + 트래커 수
    ├─ 실시간 거리 목록 (30Hz, Map 재사용)
    ├─ 세션 최솟값 거리 (1초마다 갱신)
    └─ 경과 시간 (Testing 중 1초마다 갱신)
  BP_VTC_OperatorViewActor (레벨에 배치, 차량 위 상공에 위치) (Feature I)
    └─ SceneCaptureComponent2D → RenderTarget → Spectator Screen (운영자 모니터 탑다운 뷰)
```

### 데이터 흐름

```
Level 1 위젯 입력
    ↓ [Start Session] 클릭
GameInstance.SessionConfig (레벨 전환 간 유지)
    ↓ OpenLevel("VTC_TestLevel")
Level 2 로드 → OperatorController::BeginPlay → ApplyGameInstanceConfig()
    ├─ TrackerPawn.bSimulationMode = (RunMode == Simulation)
    ├─ TrackerPawn.SetTrackerMeshVisible(bShowTrackerMesh)
    ├─ BodyActor.ApplySessionConfig(Config)
    │    ├─ MountOffset 5개 적용
    │    └─ bShowCollisionSpheres 적용 (Tick이 덮어쓰지 않도록 멤버 변수 저장)
    └─ VehicleHipPosition → AVTC_ReferencePoint 런타임 스폰
         └─ CollisionDetector.ReferencePoints.AddUnique(SpawnedHipRefPoint)
              (AutoFindReferencePoints는 BeginPlay에서 이미 실행됐으므로 수동 등록)
```

### INI 설정 파일

- 경로: `[프로젝트]/Config/VTCSettings.ini`
- [Save Config] 또는 Start Session 클릭 시 자동 저장
- Level 1 시작 시 자동 로드 (NativeConstruct에서 LoadConfigFromINI 호출)
- **SubjectID / Height는 저장하지 않음** (매 세션마다 새로 입력)

---

### 만들어야 할 Blueprint 전체 목록

```
[Level 1 전용]
  BP_VTC_SetupGameMode    (VTC_SetupGameMode 기반)   ← C++ 신규 추가됨
  WBP_SetupWidget         (VTC_SetupWidget 기반)     ← C++ 신규 추가됨

[Level 2 전용]
  BP_VTC_GameMode         (VTC_GameMode 기반)
  BP_VTC_SimPlayerController (VTC_SimPlayerController 기반)
  BP_VTC_TrackerPawn      (VTC_TrackerPawn 기반)
  BP_VTC_BodyActor        (VTC_BodyActor 기반)
  BP_VTC_ReferencePoint   (VTC_ReferencePoint 기반)
  BP_VTC_SessionManager   (VTC_SessionManager 기반)
  BP_VTC_StatusActor      (VTC_StatusActor 기반)
  WBP_StatusWidget        (VTC_StatusWidget 기반)
  WBP_VTC_OperatorMonitor (VTC_OperatorMonitorWidget 기반) ← 운영자 데스크탑 모니터링 UI
  BP_VTC_OperatorViewActor (VTC_OperatorViewActor 기반) ← Feature I

[공통]
  BP_VTC_GameInstance     (VTC_GameInstance 기반)
  PP_VTC_Warning          (PostProcessVolume — 레벨 배치)

[에셋]
  Material: M_VTC_BodySegment (+ MI_Safe, MI_Warning, MI_Collision)
  Niagara: NS_VTC_CollisionImpact, NS_VTC_WarningPulse
  Sound: SC_VTC_Warning, SC_VTC_Collision
```

---

## [Level 1 설정] BP_VTC_SetupGameMode + WBP_SetupWidget

### Step 1: BP_VTC_GameInstance 생성 (레벨 이름 지정)

1. Content Browser → Blueprint Class → **All Classes** → `VTC_GameInstance` 검색
2. 이름: `BP_VTC_GameInstance`
3. Details 패널:

| 프로퍼티 | 값 |
|---------|-----|
| Test Level Name | `VTC_TestLevel` ← Level 2 레벨 파일 이름과 일치시킬 것 |
| Setup Level Name | `VTC_SetupLevel` ← Level 1 레벨 파일 이름과 일치시킬 것 |

4. **Project Settings → Maps & Modes → Game Instance Class = `BP_VTC_GameInstance`**

---

### Step 2: WBP_SetupWidget 생성 (Level 1 UI)

1. Content Browser → 우클릭 → **User Interface → Widget Blueprint**
2. Parent Class: **All Classes** → `VTC_SetupWidget` 검색 후 선택
3. 이름: `WBP_SetupWidget`

#### 필수 BindWidget 목록

아래 이름을 **정확히** 맞춰야 합니다 (대소문자 포함). 이름이 틀리면 컴파일 오류 발생.

| 위젯 타입 | 이름 | 내용 |
|---------|------|------|
| EditableTextBox | `TB_SubjectID` | 피실험자 ID |
| EditableTextBox | `TB_Height` | 키(cm) |
| CheckBox | `Toggle_VRMode` | VR/Simulation 토글 (Checked=VR, Unchecked=Simulation, 기본값 Checked) |
| EditableTextBox | `TB_Offset_Waist_X` | Waist 트래커 오프셋 X |
| EditableTextBox | `TB_Offset_Waist_Y` | Waist 트래커 오프셋 Y |
| EditableTextBox | `TB_Offset_Waist_Z` | Waist 트래커 오프셋 Z |
| EditableTextBox | `TB_Offset_LKnee_X` | 왼무릎 트래커 오프셋 X |
| EditableTextBox | `TB_Offset_LKnee_Y` | 왼무릎 트래커 오프셋 Y |
| EditableTextBox | `TB_Offset_LKnee_Z` | 왼무릎 트래커 오프셋 Z |
| EditableTextBox | `TB_Offset_RKnee_X` | 오른무릎 트래커 오프셋 X |
| EditableTextBox | `TB_Offset_RKnee_Y` | 오른무릎 트래커 오프셋 Y |
| EditableTextBox | `TB_Offset_RKnee_Z` | 오른무릎 트래커 오프셋 Z |
| EditableTextBox | `TB_Offset_LFoot_X` | 왼발 트래커 오프셋 X |
| EditableTextBox | `TB_Offset_LFoot_Y` | 왼발 트래커 오프셋 Y |
| EditableTextBox | `TB_Offset_LFoot_Z` | 왼발 트래커 오프셋 Z |
| EditableTextBox | `TB_Offset_RFoot_X` | 오른발 트래커 오프셋 X |
| EditableTextBox | `TB_Offset_RFoot_Y` | 오른발 트래커 오프셋 Y |
| EditableTextBox | `TB_Offset_RFoot_Z` | 오른발 트래커 오프셋 Z |
| EditableTextBox | `TB_HipRef_X` | 차량 Hip 기준점 X |
| EditableTextBox | `TB_HipRef_Y` | 차량 Hip 기준점 Y |
| EditableTextBox | `TB_HipRef_Z` | 차량 Hip 기준점 Z |
| CheckBox | `CB_ShowCollisionSpheres` | 충돌 구 표시 여부 |
| CheckBox | `CB_ShowTrackerMesh` | Tracker 하드웨어 메시 표시 여부 |
| Slider | `Slider_Warning` | Warning 임계값 슬라이더 (범위 3~50 cm, 기본 10) |
| Slider | `Slider_Collision` | Collision 임계값 슬라이더 (범위 1~20 cm, 기본 3) |
| TextBlock | `Txt_WarningVal` | Warning 임계값 현재값 표시 ("10 cm") |
| TextBlock | `Txt_CollisionVal` | Collision 임계값 현재값 표시 ("3 cm") |
| ComboBoxString | `Combo_VehiclePreset` | 차종 프리셋 선택 드롭다운 |
| Button | `Btn_SavePreset` | 현재 ReferencePoint 설정을 프리셋으로 저장 |
| Button | `Btn_SaveConfig` | 설정 저장 버튼 |
| Button | `Btn_LoadConfig` | 설정 불러오기 버튼 |
| Button | `Btn_StartSession` | 세션 시작 버튼 |

#### 권장 Designer 레이아웃

```
[ScrollBox] (전체 감싸기 — 항목이 많아서 스크롤 필요)
  └─ [Vertical Box]
       │
       ├─ [Section] 피실험자 정보
       │    ├─ TextBlock "Subject ID"
       │    ├─ EditableTextBox TB_SubjectID    (힌트: "P001")
       │    ├─ TextBlock "Height (cm)"
       │    └─ EditableTextBox TB_Height       (힌트: "175")
       │
       ├─ [Section] 실행 모드
       │    └─ CheckBox Toggle_VRMode  "VR 모드 (Checked=VR / Unchecked=Simulation)"
       │
       ├─ [Section] Mount Offsets (트래커 → 실제 신체 접촉점 보정)
       │    ├─ TextBlock "Waist Offset (X / Y / Z cm)"
       │    ├─ HorizontalBox
       │    │    ├─ EditableTextBox TB_Offset_Waist_X  (힌트: "0")
       │    │    ├─ EditableTextBox TB_Offset_Waist_Y  (힌트: "0")
       │    │    └─ EditableTextBox TB_Offset_Waist_Z  (힌트: "0")
       │    ├─ (Left Knee / Right Knee / Left Foot / Right Foot 동일 패턴)
       │    └─ ...
       │
       ├─ [Section] Vehicle Hip Reference Position
       │    ├─ TextBlock "차량 Hip 기준점 (월드 좌표, cm)"
       │    └─ HorizontalBox
       │         ├─ EditableTextBox TB_HipRef_X
       │         ├─ EditableTextBox TB_HipRef_Y
       │         └─ EditableTextBox TB_HipRef_Z
       │
       ├─ [Section] 가시성
       │    ├─ CheckBox CB_ShowCollisionSpheres "충돌 구 표시"
       │    └─ CheckBox CB_ShowTrackerMesh      "Tracker 하드웨어 메시 표시"
       │
       ├─ [Section] 거리 임계값 설정 (Feature A ✅)
       │    ├─ HorizontalBox
       │    │    ├─ TextBlock "Warning 임계값"
       │    │    ├─ Slider Slider_Warning   (Min=3, Max=50, Step=1, Default=10)
       │    │    └─ TextBlock Txt_WarningVal "10 cm"
       │    └─ HorizontalBox
       │         ├─ TextBlock "Collision 임계값"
       │         ├─ Slider Slider_Collision (Min=1, Max=20, Step=1, Default=3)
       │         └─ TextBlock Txt_CollisionVal "3 cm"
       │    > Collision 임계값은 항상 Warning 임계값보다 작게 자동 클램프됨
       │
       ├─ [Section] 차종 프리셋 (Feature B ✅)
       │    ├─ ComboBoxString Combo_VehiclePreset  "프리셋 선택..."
       │    │    (NativeConstruct에서 Saved/VTCPresets/*.json 목록 자동 채움)
       │    └─ Button Btn_SavePreset  "현재 설정 저장"
       │         (프리셋 이름은 VehicleHipPosition + Offset 입력값으로 현재 프리셋명 사용)
       │
       └─ [Section] 버튼
            ├─ Button Btn_LoadConfig   "Load Config"
            ├─ Button Btn_SaveConfig   "Save Config"
            └─ Button Btn_StartSession "▶ Start Session"
```

> **동작 원리 (C++에서 자동 처리):**
> - `NativeConstruct()`: 시작 시 INI 자동 로드 → 화면에 반영, Toggle_VRMode 기본값 Checked(VR)
> - `[Save Config]`: 입력값 → INI 파일 저장
> - `[Load Config]`: INI 파일 → 화면에 반영
> - `[Start Session]`: SubjectID·Height 유효성 검사 → GameInstance에 저장 → INI 저장 → Level 2 로드
> - `Toggle_VRMode`: Checked=VR, Unchecked=Simulation (단일 CheckBox로 모드 전환)

---

### Step 3: BP_VTC_SetupGameMode 생성

1. Content Browser → Blueprint Class → **All Classes** → `VTC_SetupGameMode` 검색
2. 이름: `BP_VTC_SetupGameMode`
3. Details 패널:

| 프로퍼티 | 값 |
|---------|-----|
| Setup Widget Class | `WBP_SetupWidget` |

---

### Step 4: Level 1 맵 파일 생성

1. Content Browser → 우클릭 → **Level**
2. 이름: `VTC_SetupLevel` ← BP_VTC_GameInstance의 SetupLevelName과 일치
3. `VTC_SetupLevel` 열기 → **World Settings → GameMode Override = `BP_VTC_SetupGameMode`**
4. 플레이하면 SetupWidget이 자동으로 화면에 표시되고 마우스 커서가 켜집니다.

> **기본 맵 설정 (선택):** Project Settings → Maps & Modes → Editor Startup Map = `VTC_SetupLevel`

---

## [Level 2 설정] WBP_StatusWidget + BP_VTC_StatusActor

### Step 1: WBP_StatusWidget 생성 (3D 월드 배치 위젯)

1. Content Browser → **User Interface → Widget Blueprint**
2. Parent Class: `VTC_StatusWidget`
3. 이름: `WBP_StatusWidget`

#### 필수 BindWidget 목록

| 위젯 타입 | 이름 | 표시 내용 |
|---------|------|---------|
| TextBlock | `Txt_State` | 현재 상태 ("● IDLE" 등) |
| TextBlock | `Txt_Prompt` | 키 안내 ("F1 — Start Calibration" 등) |
| TextBlock | `Txt_SubjectInfo` | "Subject: P001 \| Height: 175 cm" |
| TextBlock | `Txt_TrackerStatus` | "Trackers: 5 / 5 Connected" |

#### 권장 Designer 레이아웃 (3D 월드 배치 기준 — 800×400)

```
[Canvas Panel]  크기: 800 × 400
  └─ [Border] 반투명 검은 배경 (Opacity 0.7)
       └─ [Vertical Box]
            ├─ TextBlock Txt_State        폰트 크기 36, Bold, 가운데 정렬
            │                              예) "● TESTING"
            ├─ Separator (Spacer)
            ├─ TextBlock Txt_Prompt       폰트 크기 24, 줄간격 넉넉히
            │                              예) "F3  —  Stop & Export CSV"
            ├─ Separator (Spacer)
            ├─ TextBlock Txt_SubjectInfo  폰트 크기 18, 회색
            └─ TextBlock Txt_TrackerStatus 폰트 크기 18, 회색
```

> **3D Widget 주의사항:**
> - 배경을 불투명하게 하면 레벨 반사광의 영향을 덜 받음
> - 폰트 크기는 3D 월드에서 볼 때 기준으로 설정 (작으면 VR에서 안 보임)

---

### Step 2: BP_VTC_StatusActor 생성

1. Content Browser → Blueprint Class → `VTC_StatusActor`
2. 이름: `BP_VTC_StatusActor`
3. Details 패널:

| 프로퍼티 | 값 |
|---------|-----|
| Status Widget Class | `WBP_StatusWidget` |

---

### Step 3: BP_VTC_GameMode 설정 (Level 2용)

Level 2 GameMode에 OperatorController를 추가해야 합니다.

1. `BP_VTC_GameMode` 열기 (또는 새로 생성: `VTC_GameMode` 기반)
2. Details 패널:

| 프로퍼티 | 값 |
|---------|-----|
| Default Pawn Class | `BP_VTC_TrackerPawn` |
| Player Controller Class | `BP_VTC_SimPlayerController` |

> **BP_VTC_SimPlayerController가 두 역할을 모두 담당합니다:**
> - `VTC_SimPlayerController`가 `VTC_OperatorController`를 상속하도록 C++에서 변경됨
> - F1/F2/F3/Escape 세션 제어 + GameInstance 설정 적용 → OperatorController(부모)가 처리
> - WASD/마우스 시뮬레이션 이동 + Enhanced Input 등록 → SimPlayerController(자식)가 처리
> - **BP_VTC_SimPlayerController 하나만 Player Controller Class로 지정하면 됩니다**

---

### Step 4: Level 2 맵 파일 생성 및 Actor 배치

1. Content Browser → Level → 이름: `VTC_TestLevel`
2. `VTC_TestLevel` 열기 → **World Settings → GameMode Override = `BP_VTC_GameMode`**
3. 다음 Actor들을 레벨에 배치:

| Actor | 수량 | 위치 |
|-------|------|------|
| `BP_VTC_BodyActor` | 1 | (0, 0, 0) |
| `BP_VTC_SessionManager` | 1 | (0, 0, 0) |
| `BP_VTC_StatusActor` | 1 | 운전석 옆 또는 대시보드 위 |
| `BP_VTC_ReferencePoint` | N개 | 차량 측정 지점마다 |
| PostProcessVolume (Infinite) | 1 | 어디든 |

4. **BP_VTC_StatusActor 배치 팁:**
   - 차량 운전석 정면 (대시보드 위) 권장
   - Transform Rotation으로 운전자를 향하도록 조정
   - Scale은 0.01~0.02 범위 (DrawSize 800×400이 cm 단위로 월드에 배치됨)

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
| VTC\|Simulation | bAutoDetectSimulation | `true` | HMD 미감지 시 자동으로 시뮬레이션 모드 전환 |
| VTC\|Simulation | bSimulationMode | `false` | 강제 시뮬레이션 모드 (HMD 없이 에디터에서 테스트) |
| VTC\|Simulation | SimMoveSpeed | `300.0` | WASD 이동 속도 (cm/s) |
| VTC\|Simulation | SimMouseSensitivity | `1.0` | 마우스 룩 감도 |
| VTC\|Simulation | SimOffset_LeftKnee | `(0, -30, -50)` | 착석 시 왼쪽 무릎 오프셋 (cm, 카메라 로컬) |
| VTC\|Simulation | SimOffset_RightKnee | `(0, 30, -50)` | 착석 시 오른쪽 무릎 오프셋 (cm, 카메라 로컬) |

> **시뮬레이션 모드 단축키:**
> - **Backspace** — VR ↔ 시뮬레이션 모드 토글 *(F8은 UE PIE "Eject from Pawn"과 충돌)*
> - **Q / E** — Pawn 위 / 아래 이동
> - **R** — 무릎 오프셋 초기화
> - **NumPad 4/6** — 왼쪽 무릎 좌(4)/우(6), **NumPad 8/2** — 왼쪽 무릎 위(8)/아래(2)
> - **Arrow Left/Right** — 오른쪽 무릎 좌/우, **Arrow Up/Down** — 오른쪽 무릎 위/아래

### Enhanced Input 에셋 생성 및 연결 (필수)

시뮬레이션 모드 키 입력이 동작하려면 **Enhanced Input 에셋을 반드시 생성**하고 BP_VTC_TrackerPawn에 연결해야 합니다.

#### Step A — Input Action 에셋 7개 생성

Content Browser 우클릭 → **Input → Input Action**

| 에셋 이름 | Value Type | 설명 |
|----------|-----------|------|
| `IA_VTC_Move` | **Axis2D (Vector2D)** | 수평 이동 (W/S = 전후 X, A/D = 좌우 Y) |
| `IA_VTC_MoveUp` | **Axis1D (float)** | 수직 이동 (Q=위, E=아래) |
| `IA_VTC_Look` | **Axis2D (Vector2D)** | 마우스 룩 (X=Yaw, Y=Pitch) |
| `IA_VTC_ToggleSim` | **Digital (bool)** | 시뮬레이션 모드 토글 |
| `IA_VTC_ResetKnees` | **Digital (bool)** | 무릎 오프셋 초기화 |
| `IA_VTC_AdjustLeftKnee` | **Axis2D (Vector2D)** | 왼쪽 무릎 조절 (X입력→Y축 좌우, Y입력→Z축 위아래) |
| `IA_VTC_AdjustRightKnee` | **Axis2D (Vector2D)** | 오른쪽 무릎 조절 (X입력→Y축 좌우, Y입력→Z축 위아래) |

#### Step B — Input Mapping Context 에셋 1개 생성

Content Browser 우클릭 → **Input → Input Mapping Context** → 이름: `IMC_VTC_Simulation`

IMC 에셋을 열어 키 매핑 추가:

| Action | Key | Modifier | 설명 |
|--------|-----|----------|------|
| `IA_VTC_Move` | W | *(없음)* | X=+1 (전진) |
| `IA_VTC_Move` | S | **Negate** | X=-1 (후진) |
| `IA_VTC_Move` | D | **Swizzle YXZ** | Y=+1 (우) |
| `IA_VTC_Move` | A | **Swizzle YXZ + Negate** | Y=-1 (좌) |
| `IA_VTC_MoveUp` | Q | *(없음)* | +1 (위) |
| `IA_VTC_MoveUp` | E | **Negate** | -1 (아래) |
| `IA_VTC_Look` | Mouse X | *(없음)* | X=Yaw |
| `IA_VTC_Look` | Mouse Y | **Negate** | Y=Pitch (반전) |
| `IA_VTC_ToggleSim` | Backspace | *(없음)* | 모드 토글 |
| `IA_VTC_ResetKnees` | R | *(없음)* | 무릎 초기화 |
| `IA_VTC_AdjustLeftKnee` | NumPad6 | *(없음)* | 왼무릎 좌우 X=+1 (오른쪽) |
| `IA_VTC_AdjustLeftKnee` | NumPad4 | **Negate** | 왼무릎 좌우 X=-1 (왼쪽) |
| `IA_VTC_AdjustLeftKnee` | NumPad8 | **Swizzle YXZ** | 왼무릎 위아래 Y=+1 (위) |
| `IA_VTC_AdjustLeftKnee` | NumPad2 | **Swizzle YXZ + Negate** | 왼무릎 위아래 Y=-1 (아래) |
| `IA_VTC_AdjustRightKnee` | Right | *(없음)* | 오른무릎 좌우 X=+1 (오른쪽) |
| `IA_VTC_AdjustRightKnee` | Left | **Negate** | 오른무릎 좌우 X=-1 (왼쪽) |
| `IA_VTC_AdjustRightKnee` | Up | **Swizzle YXZ** | 오른무릎 위아래 Y=+1 (위) |
| `IA_VTC_AdjustRightKnee` | Down | **Swizzle YXZ + Negate** | 오른무릎 위아래 Y=-1 (아래) |

> **Swizzle YXZ Modifier**: X 값을 Y 채널로 보내는 역할. Axis2D에서 두 번째 키를 Y에 매핑할 때 사용.

#### Step C — BP_VTC_SimPlayerController에 에셋 연결

> **주의:** 입력 에셋은 **BP_VTC_TrackerPawn이 아니라 BP_VTC_SimPlayerController**에 연결합니다.
> C++ 아키텍처상 Enhanced Input 등록은 PlayerController::BeginPlay()에서 처리됩니다.

1. Content Browser → Blueprint Class → `VTC_SimPlayerController` 기반
2. 이름: `BP_VTC_SimPlayerController`
3. BP_VTC_GameMode → **Player Controller Class** = `BP_VTC_SimPlayerController`

BP_VTC_SimPlayerController 열기 → Details 패널 → **VTC|Input**:

| 프로퍼티 | 연결할 에셋 |
|---------|-----------|
| Sim Input Mapping Context | `IMC_VTC_Simulation` |
| IA Move | `IA_VTC_Move` |
| IA Move Up | `IA_VTC_MoveUp` |
| IA Look | `IA_VTC_Look` |
| IA Toggle Sim | `IA_VTC_ToggleSim` |
| IA Reset Knees | `IA_VTC_ResetKnees` |
| IA Adjust Left Knee | `IA_VTC_AdjustLeftKnee` |
| IA Adjust Right Knee | `IA_VTC_AdjustRightKnee` |

> **Project Settings 확인**: Engine → Input → Default Input Component Class = `EnhancedInputComponent`
> Default Player Input Class = `EnhancedPlayerInput`

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

## 5. BP_VTC_GameMode

### 생성 방법
1. Blueprint Class → `VTC_GameMode` 기반으로 생성
2. 이름: `BP_VTC_GameMode`

### Details 패널 설정

| 프로퍼티 | 값 |
|---------|-----|
| Default Pawn Class | `BP_VTC_TrackerPawn` |
| HUD Class | `None` (또는 커스텀 HUD) |
| Player Controller Class | `BP_VTC_SimPlayerController` |

> **중요:** C++ `VTC_GameMode`는 이미 DefaultPawnClass를 `AVTC_TrackerPawn`으로 설정합니다. 하지만 Blueprint 버전을 사용하려면 **두 항목 모두 반드시 오버라이드**해야 합니다.
> - Default Pawn Class → `BP_VTC_TrackerPawn`
> - Player Controller Class → `BP_VTC_SimPlayerController` (Enhanced Input 등록이 이 컨트롤러에서 처리됨)

---

## 6. WBP_VTC_OperatorMonitor (운영자 데스크탑 모니터링 위젯)

VR 피실험자 옆에 앉은 운영자가 **데스크탑 모니터**에서 실시간 거리 데이터를 확인하는 Screen Space UI입니다.

> **VR 피실험자용 UI ≠ 운영자 모니터링 UI**
> - **VR 피실험자**: WBP_StatusWidget (3D WorldSpace, StatusActor에 배치) — 상태/키 안내
> - **운영자 데스크탑**: WBP_VTC_OperatorMonitor (Screen Space) — 세션 상태 + 거리 수치

### 생성 방법

1. Content Browser → 우클릭 → **User Interface → Widget Blueprint**
2. **Parent Class**: `VTC_OperatorMonitorWidget` *(검색 후 선택)*
3. 이름: `WBP_VTC_OperatorMonitor`

### 필수: BindWidget 위젯 배치

| 위젯 타입 | 이름 (정확히) | 표시 내용 |
|----------|------------|---------|
| TextBlock | `Txt_State` | 세션 상태 ("● TESTING") |
| TextBlock | `Txt_SubjectInfo` | 피실험자 ID + 키 ("Subject: P001 \| Height: 175 cm") |
| TextBlock | `Txt_TrackerStatus` | 트래커 연결 수 ("Trackers: 5 / 5") |
| TextBlock | `Txt_ElapsedTime` | 경과 시간 ("02:34") |
| TextBlock | `Txt_MinDistance` | 세션 최솟값 ("Min: 4.2 cm") |
| VerticalBox | `VB_DistanceList` | 거리 Row 자동 생성 영역 |

**Designer 탭 예시 레이아웃:**
```
[Canvas Panel]
  └─ [Vertical Box] — 전체 레이아웃, 왼쪽 상단 고정
       ├─ Txt_State        (폰트 굵게, 크게)
       ├─ Txt_SubjectInfo
       ├─ Txt_TrackerStatus
       ├─ Txt_ElapsedTime
       ├─ Txt_MinDistance  (강조 색상 권장)
       └─ VB_DistanceList  (빈 VerticalBox — C++가 TextBlock Row 자동 추가)
```

> `VB_DistanceList`는 비워두세요. C++(`VTC_OperatorController`)가 30Hz로 Row를 자동으로 생성/갱신합니다.
> Row TextBlock 포맷: `"Left Knee        Dashboard          8.2 cm"` (색상: 초록/노랑/빨강)

### BP_VTC_SimPlayerController에 연결

1. `BP_VTC_SimPlayerController` 열기
2. **Class Defaults**에서 **"VTC|Operator|Monitor"** 카테고리 확인
3. `Operator Monitor Widget Class` → `WBP_VTC_OperatorMonitor` 선택

이것만 하면 BeginPlay에서 자동으로 생성되어 뷰포트에 표시됩니다.

> **클래스 할당하지 않으면** 모니터링 위젯 없이 진행됩니다 (StatusActor 3D 위젯만 표시됨). 선택 사항입니다.

---

---

## 7. PostProcessVolume (PP_VTC_Warning)

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
2. **GameMode Override** → `BP_VTC_GameMode`
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
1. **VR Preview** 버튼 클릭 (또는 Alt+P)
2. Vive Pro 2 HMD 착용
3. 5개 트래커가 모두 감지되는지 확인 (디버그 구 5개 표시)
4. HUD에서 Start Session → T-Pose 3초 → Testing 시작

---

## 연결 관계 다이어그램

```
레벨 배치 구조:

  ┌─────────────────────────────────────────────────┐
  │                    Level                         │
  │                                                  │
  │   BP_VTC_GameMode (World Settings)               │
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
| OperatorController.OperatorViewActor | `GetAllActorsOfClass()` 자동 탐색 | No — 자동 탐색됨 (Feature I) |

---

## 8. BP_VTC_OperatorViewActor (운영자 탑다운 뷰 — Feature I)

운영자 모니터(Companion Screen / Spectator Screen)에 탑다운 뷰를 실시간으로 전송하는 Actor입니다.
VR 사용자는 HMD를 쓰고, 운영자는 외부 모니터에서 세션 장면을 탑뷰로 확인합니다.

### 생성 방법
1. Blueprint Class → **All Classes** → `VTC_OperatorViewActor` 검색
2. 이름: `BP_VTC_OperatorViewActor`

### Level 2에 배치
- 차량 위 상공에 배치 (Z = 차량 루프 높이 + 200~300 cm 권장)
- `BP_VTC_OperatorViewActor`를 드래그 앤 드롭
- 모든 프로퍼티를 비워두면 자동 동작 (RenderTarget은 BeginPlay에서 자동 생성)

### Details 패널 설정

| 카테고리 | 프로퍼티 | 기본값 | 설명 |
|---------|---------|-------|------|
| VTC\|OperatorView | CaptureWidth | `1280` | RenderTarget 가로 해상도 |
| VTC\|OperatorView | CaptureHeight | `720` | RenderTarget 세로 해상도 |
| VTC\|OperatorView | CaptureOrthoWidth | `500` | 직교 투영 캡처 너비 (cm, 클수록 넓은 영역) |
| VTC\|OperatorView | bOrthographic | `true` | true=직교 투영(탑다운), false=원근 투영 |
| VTC\|OperatorView | RenderTarget | *비워두기* | BeginPlay에서 자동 생성 |

### 자동 동작 원리
- **BeginPlay**: RenderTarget 자동 생성 → SceneCaptureComponent2D에 연결 → `SetupSpectatorScreen()` 호출
- **Spectator Screen 연결**: `GEngine->XRSystem->GetHMDDevice()->GetSpectatorScreenController()` 경로로 HMD의 Companion Screen에 RenderTarget 출력
- **VR 장비 없음**: XRSystem 없으면 자동으로 RenderTarget만 활성화 (데스크탑에서 UMG Image로 연결 가능)
- **OperatorController**: BeginPlay에서 `GetAllActorsOfClass(VTC_OperatorViewActor)`로 자동 탐색

### 데스크탑 모드 대체 (VR 장비 없이 테스트 시)
RenderTarget을 UMG Image 위젯에 직접 연결하면 화면에 탑다운 뷰를 표시할 수 있습니다:

```
WBP_OperatorMonitor (새 위젯 생성)
  └─ Image 위젯
       └─ Brush → Texture = BP_VTC_OperatorViewActor.RenderTarget
```

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
| VTC\|Screenshot | bAutoScreenshotOnWorstClearance | `true` | 최악 클리어런스 갱신 시 자동 스크린샷 저장 (Feature F ✅) |
| VTC\|Screenshot | ScreenshotDirectory | `""` | 스크린샷 저장 경로 (빈값 = `Saved/VTCLogs/Screenshots/`) |
| VTC\|Screenshot | LastScreenshotPath | *(ReadOnly)* | 마지막 저장된 스크린샷 경로 |

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

---

## 문제 해결 FAQ

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
- Level 1 SetupWidget의 `TB_Height`에 키를 입력하고 Start Session 버튼을 눌렀는지 확인
- `StartSessionWithHeight(SubjectID, Height_cm)` 호출 여부 확인 (F1/F2 키 → OperatorController → SessionManager)
- HMD만으로 세션을 시작하면 `EstimatedHeight`(자동 추정, ±5cm 오차)가 사용됨
