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
    ├─ VR / Simulation 모드 선택
    ├─ Mount Offset 5개 (Waist/LKnee/RKnee/LFoot/RFoot) X/Y/Z 입력
    ├─ Vehicle Hip Position X/Y/Z 입력
    ├─ Collision Sphere 표시 여부 체크박스
    ├─ Tracker Mesh 표시 여부 체크박스
    ├─ [Save Config] / [Load Config] 버튼
    └─ [Start Session] 버튼
          → GameInstance.SessionConfig 저장 + INI 저장 + OpenLevel("VTC_TestLevel")

[Level 2 — VTC_TestLevel]
  GameMode: BP_VTC_GameMode
    └─ DefaultPawn: BP_VTC_TrackerPawn (자동 스폰)
       PlayerController: BP_VTC_SimPlayerController
  BP_VTC_OperatorController (또는 BP_VTC_GameMode에 설정)
    └─ BeginPlay → GameInstance 설정 읽어서 TrackerPawn/BodyActor에 자동 적용
  BP_VTC_StatusActor (레벨에 3D 월드 배치)
    └─ WBP_StatusWidget (WorldSpace 3D 위젯)
         ├─ 현재 세션 상태 표시
         ├─ 피실험자 정보 표시
         ├─ 트래커 연결 수 표시
         └─ 키 안내 메시지: F1 캘리브레이션 / F2 테스트 시작 / F3 종료+CSV
  레벨 내 키 입력 (BP_VTC_SimPlayerController가 F키 + Escape 모두 처리)
    ├─ F1     → 캘리브레이션 시작
    ├─ F2     → 테스트 직접 시작 (캘리브레이션 건너뜀)
    ├─ F3     → 종료 + CSV 내보내기
    └─ Escape → Level 1(Setup)으로 복귀
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
| CheckBox | `CB_ModeVR` | VR 모드 선택 |
| CheckBox | `CB_ModeSimulation` | 시뮬레이션 모드 선택 |
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
       │    ├─ CheckBox CB_ModeVR          "VR (HMD + Trackers)"
       │    └─ CheckBox CB_ModeSimulation  "Simulation (Desktop Only)"
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
       └─ [Section] 버튼
            ├─ Button Btn_LoadConfig   "Load Config"
            ├─ Button Btn_SaveConfig   "Save Config"
            └─ Button Btn_StartSession "▶ Start Session"
```

> **동작 원리 (C++에서 자동 처리):**
> - `NativeConstruct()`: 시작 시 INI 자동 로드 → 화면에 반영
> - `[Save Config]`: 입력값 → INI 파일 저장
> - `[Load Config]`: INI 파일 → 화면에 반영
> - `[Start Session]`: SubjectID·Height 유효성 검사 → GameInstance에 저장 → INI 저장 → Level 2 로드
> - CB_ModeVR ↔ CB_ModeSimulation은 자동으로 상호 배타 처리됨

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

## 6. WBP_VTC_SubjectInfo (피실험자 정보 입력 위젯)

### 생성 방법
1. Content Browser → 우클릭 → **User Interface → Widget Blueprint**
2. **Parent Class**: `VTC_SubjectInfoWidget` *(검색 후 선택)*
3. 이름: `WBP_VTC_SubjectInfo`

### 필수: BindWidget 위젯 배치

C++ 코드에서 `meta=(BindWidget)` 으로 선언된 위젯 이름을 **정확히** 맞춰야 합니다.
이름이 하나라도 다르면 컴파일 에러가 발생합니다.

| 위젯 타입 | 이름 (대소문자 정확히) | 내용 |
|---------|-------------------|------|
| EditableTextBox | `TB_SubjectID` | 피실험자 ID 입력 |
| EditableTextBox | `TB_Height` | 키(cm) 숫자 입력 (예: `175`) |
| Button | `Btn_StartSession` | 시작 버튼 |

**Designer 탭 예시 레이아웃:**
```
[Vertical Box]
  ├─ TextBlock  "피실험자 ID"
  ├─ EditableTextBox  TB_SubjectID    (힌트: "P001")
  ├─ TextBlock  "키 (cm)"
  ├─ EditableTextBox  TB_Height       (힌트: "175")
  │     → KeyboardType: NumberPad (숫자 입력 강제)
  └─ Button  Btn_StartSession  "시작"
```

> **`TB_Height` 설정 팁:**
> - Input Method Type → `Number` 로 설정하면 숫자만 입력 가능
> - Hint Text → `"키 입력 (cm), 예: 175"` 설정 권장

### 연결 방법 (Level Blueprint 또는 WBP_VTC_HUD에서)

```
Event BeginPlay
  │
  └─ SubjectInfoWidgetRef → Bind Event to OnSessionStartRequested
       └─ Custom Event HandleSessionStart (SubjectID: String, Height_cm: float)
               └─ SessionManagerRef → StartSessionWithHeight (SubjectID, Height_cm)
```

> **참고:** `OnSessionStartRequested`는 버튼 클릭 시 자동으로 발동합니다.
> C++ `NativeConstruct()`에서 버튼 바인딩이 완료되어 있습니다.
> IsInputValid() 검사도 C++ 안에서 자동으로 처리됩니다 (SubjectID 비어있거나 Height ≤ 0이면 브로드캐스트 안 함).

---

## 7. WBP_VTC_HUD (UMG Widget)

### 생성 방법
1. Content Browser → 우클릭 → **User Interface → Widget Blueprint**
2. 이름: `WBP_VTC_HUD`

---

### Designer 탭 — 전체 위젯 트리

HUD는 **항상 표시되는 상단 바** + **세션 상태별로 바뀌는 4개 패널**로 구성됩니다.

```
[Canvas Panel]  (루트)
  │
  ├─ [Vertical Box]  (전체 레이아웃)
  │    │
  │    ├─ ── 상단 상태 바 (항상 표시) ──────────────────────
  │    │   HorizontalBox
  │    │     ├─ TextBlock  TB_SessionState   "IDLE"
  │    │     ├─ TextBlock  TB_ElapsedTime    "00:00:00"
  │    │     └─ TextBlock  TB_SubjectID      ""
  │    │
  │    ├─ ── Panel_Idle (Overlay) ────────────────────────
  │    │   VerticalBox  [이름: Panel_Idle]
  │    │     └─ [WBP_VTC_SubjectInfo]  SubjectInfoWidget
  │    │          (피실험자 ID + 키 입력 + 시작 버튼 — 모두 내장)
  │    │
  │    ├─ ── Panel_Calibrating (Overlay) ─────────────────
  │    │   VerticalBox  [이름: Panel_Calibrating]
  │    │     ├─ TextBlock  "T-Pose를 취하고 있으세요"
  │    │     ├─ TextBlock  TB_CalibCountdown  "3"  (카운트다운)
  │    │     └─ Button  BTN_SkipCalib  "Skip (Direct Test)"
  │    │
  │    ├─ ── Panel_Testing (Overlay) ──────────────────────
  │    │   VerticalBox  [이름: Panel_Testing]
  │    │     ├─ VerticalBox  VB_DistanceList  (동적 행 생성)
  │    │     ├─ TextBlock  TB_MinDistance  "Min: -- cm"
  │    │     └─ HorizontalBox
  │    │          ├─ Button  BTN_Stop         "Stop"
  │    │          └─ Button  BTN_ReCalibrate  "Re-Calibrate"
  │    │
  │    └─ ── Panel_Reviewing (Overlay) ───────────────────
  │        VerticalBox  [이름: Panel_Reviewing]
  │          ├─ TextBlock  "세션 완료"
  │          ├─ TextBlock  TB_FinalMinDist  "최소 거리: -- cm"
  │          ├─ Button  BTN_Export   "Export CSV"
  │          └─ Button  BTN_NewSession  "New Session"
```

> **중요:** Panel_Idle / Panel_Calibrating / Panel_Testing / Panel_Reviewing 는
> UMG에서 **Is Variable = true** 로 체크해야 Event Graph에서 참조할 수 있습니다.

---

### 각 패널이 하는 일 요약

| 패널 이름 | 표시 조건 (SessionState) | 내용 |
|----------|------------------------|------|
| **Panel_Idle** | `Idle` | 피험자 ID 입력 + Start 버튼. 세션 시작 전 대기 화면 |
| **Panel_Calibrating** | `Calibrating` | "T-Pose 취하세요" 안내 + 카운트다운. 캘리브레이션 진행 중 |
| **Panel_Testing** | `Testing` | VB_DistanceList (실시간 거리 목록) + 최소거리 + Stop 버튼 |
| **Panel_Reviewing** | `Reviewing` | 세션 종료 후 최종 결과 + Export + New Session 버튼 |

---

### Event Graph 연결 (Blueprint)

#### [1] BeginPlay — 참조 취득 + 델리게이트 바인딩

```
Event BeginPlay
  │
  ├─ Get All Actors Of Class → BP_VTC_SessionManager
  │    └─ [0] → Set SessionManagerRef (변수)
  │
  ├─ SessionManagerRef → CollisionDetector
  │    └─ Set CollisionDetectorRef (변수)
  │
  ├─ Panel_Idle 안의 WBP_VTC_SubjectInfo → Get (Is Variable = true 로 설정)
  │    └─ Set SubjectInfoWidgetRef (변수)
  │         └─ Bind Event to OnSessionStartRequested
  │              └─ Custom Event HandleSessionStart (SubjectID, Height_cm: float)
  │                       └─ SessionManagerRef → StartSessionWithHeight (SubjectID, Height_cm)
  │
  ├─ Bind Event to OnSessionStateChanged (Target: SessionManagerRef)
  │    └─ Event: Custom Event [HandleStateChanged]
  │
  ├─ Bind Event to OnDistanceUpdated (Target: CollisionDetectorRef)
  │    └─ Event: Custom Event [HandleDistanceUpdated]
  │
  └─ Call HandleStateChanged (OldState: Idle, NewState: Idle)
       ← 시작 시 Idle 패널을 즉시 표시하기 위해 1회 수동 호출
```

---

#### [2] HandleStateChanged — 패널 전환

```
Custom Event HandleStateChanged (OldState, NewState: EVTCSessionState)
  │
  ├─ Panel_Idle        → SetVisibility (Collapsed)
  ├─ Panel_Calibrating → SetVisibility (Collapsed)
  ├─ Panel_Testing     → SetVisibility (Collapsed)
  └─ Panel_Reviewing   → SetVisibility (Collapsed)
         (일단 전부 숨기고)
  │
  └─ Switch on EVTCSessionState (NewState)
       │
       ├─ Idle        → Panel_Idle        SetVisibility (Visible)
       │
       ├─ Calibrating → Panel_Calibrating SetVisibility (Visible)
       │
       ├─ Testing     → Panel_Testing     SetVisibility (Visible)
       │                VB_DistanceList   ClearChildren  ← 이전 행 제거
       │                DistanceWidgetMap Clear          ← Map 초기화
       │
       └─ Reviewing   → Panel_Reviewing   SetVisibility (Visible)
                        TB_FinalMinDist   SetText ( SessionManagerRef → GetSessionMinDistance )
```

> **왜 전부 Collapsed 후 하나만 Visible?**
> 상태가 바뀔 때마다 어떤 패널이 켜져있는지 추적할 필요 없이
> "전부 끄고 해당하는 것만 켠다" 패턴이 가장 안전합니다.

---

#### [3] HandleDistanceUpdated — VB_DistanceList Row 관리 (Map 방식)

`OnDistanceUpdated`는 30Hz로 발동하며 **매번 (BodyPart 1개, VehiclePart 1개) 쌍**을 전달합니다.
ClearChildren + 재생성하면 30Hz × Row 수만큼 Widget이 생성/삭제되어 성능 낭비입니다.
대신 **Map으로 Row를 재사용**합니다.

**변수 추가 (WBP_VTC_HUD Variables):**
```
DistanceWidgetMap : Map <EVTCTrackerRole, WBP_Distance>
  (Variable Type: Map, Key: EVTCTrackerRole Enum, Value: WBP_Distance Object Reference)
```

**HandleDistanceUpdated 흐름:**
```
Custom Event HandleDistanceUpdated (Result: FVTCDistanceResult)
  │
  ├─ Break FVTCDistanceResult → BodyPart, VehiclePartName, Distance, WarningLevel
  │
  └─ Map Contains? DistanceWidgetMap[BodyPart]
       │                          │
      YES                         NO
       │                          │
       ▼                          ▼
  Map Find                   Create Widget (WBP_Distance)
  DistanceWidgetMap[BodyPart]      │
       │                    VB_DistanceList → Add Child
       │                    Map Add (BodyPart → 새 위젯)
       │                          │
       └──────────────────────────┘
                    │
                    ▼
          [WBP_Distance 위젯 ref]
                    │
                    ▼
          Call Function: UpdateRow(Result)
          (WBP_Distance 안에 만드는 함수 — 아래 [4] 참조)
```

> **핵심:** BodyPart 하나당 Row 하나입니다. Waist, LeftKnee, RightKnee, LeftFoot, RightFoot 최대 5개.
> 한번 생성된 Row는 UpdateRow()로 값만 바꾸고 재사용합니다.

---

#### [4] WBP_Distance — Row 위젯 (이미 만드셨죠)

Content Browser → Widget Blueprint → `WBP_Distance`

**Designer 레이아웃:**
```
HorizontalBox
  ├─ TextBlock  TB_BodyPart     Width: 100   예) "Left Knee"
  ├─ TextBlock  TB_VehiclePart  Width: 140   예) "Dashboard"
  ├─ TextBlock  TB_Distance     Width: 80    예) "8.2 cm"
  └─ Border     BDR_Status      Width: 16    (배경색으로 경고 단계 표시)
```

**Function: UpdateRow (Result: FVTCDistanceResult)**
```
Break FVTCDistanceResult (Result)
  │
  ├─ BodyPart → Switch on EVTCTrackerRole
  │               Waist      → TB_BodyPart SetText "Waist"
  │               LeftKnee   → TB_BodyPart SetText "Left Knee"
  │               RightKnee  → TB_BodyPart SetText "Right Knee"
  │               LeftFoot   → TB_BodyPart SetText "Left Foot"
  │               RightFoot  → TB_BodyPart SetText "Right Foot"
  │
  ├─ VehiclePartName → TB_VehiclePart SetText
  │
  ├─ Distance → Float To Text (최대소수점 1자리) → Append " cm" → TB_Distance SetText
  │
  └─ WarningLevel → Switch on EVTCWarningLevel
                      Safe      → BDR_Status SetBrushColor (0, 0.8, 0, 1)  초록
                      Warning   → BDR_Status SetBrushColor (1, 0.9, 0, 1)  노랑
                      Collision → BDR_Status SetBrushColor (1, 0.1, 0, 1)  빨강
```

---

#### [5] Panel_Calibrating — 카운트다운 연결

캘리브레이션 카운트다운은 **CalibrationComponent의 OnCalibrationCountdown** 델리게이트를 이용합니다.
SessionManager → BodyActor → CalibrationComp 경로로 접근합니다.

```
BeginPlay (추가)
  │
  └─ SessionManagerRef → BodyActor → CalibrationComp
       └─ Bind Event to OnCalibrationCountdown
            └─ Custom Event HandleCalibCountdown (SecondsRemaining: int)
                    └─ TB_CalibCountdown SetText (SecondsRemaining → To Text)
```

BTN_SkipCalib.OnClicked → SessionManagerRef → StartTestingDirectly()

---

#### [6] Tick — 경과 시간 + 최소 거리 갱신

```
Event Tick (DeltaTime)
  │
  └─ SessionManagerRef → IsTesting?
       true →
         ├─ SessionManagerRef → SessionElapsedTime
         │    └─ TB_ElapsedTime SetText ( 초 → "MM:SS" 포맷 )
         │
         └─ CollisionDetectorRef → CurrentDistanceResults → Length > 0?
              true → SessionManagerRef → GetSessionMinDistance
                       └─ TB_MinDistance SetText ( Format "Min: {0} cm" )
```

> **MM:SS 포맷 팁:** `Floor(Time / 60)` → 분, `Fmod(Time, 60)` → 초, 각각 두자리로 포맷

---

#### [7] 버튼 클릭

```
[Panel_Idle 시작 버튼]
  WBP_VTC_SubjectInfo.OnSessionStartRequested → HandleSessionStart (BeginPlay에서 바인딩)
    └─ SessionManagerRef → StartSessionWithHeight (SubjectID, Height_cm)
       ← SubjectID 비어있거나 Height ≤ 0이면 C++ 내부에서 자동으로 차단됨

[Testing / Reviewing 패널 버튼]
BTN_Stop.OnClicked          → SessionManagerRef → StopSession()
BTN_ReCalibrate.OnClicked   → SessionManagerRef → RequestReCalibration()
BTN_Export.OnClicked        → SessionManagerRef → ExportAndEnd()
                               (반환: summary CSV 경로 — Print String으로 확인 가능)
BTN_NewSession.OnClicked    → SessionManagerRef → StopSession()
                               (Idle로 돌아가면 HandleStateChanged가 Panel_Idle 표시)
```

---

### HUD를 VR에서 표시하는 방법

VR에서는 Screen Space Widget이 보이지 않으므로 **Widget Component**를 사용합니다:

1. BP_VTC_TrackerPawn에 **WidgetComponent** 추가
2. Widget Class → `WBP_VTC_HUD`
3. Draw Size → `(800, 600)`
4. Space → `World`
5. Camera에 Attach (또는 고정 위치)

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
SessionManager의 WarningFeedback 컴포넌트에서:
- `WarningSFX` → 경고 사운드 에셋
- `CollisionSFX` → 충돌 사운드 에셋
- `CollisionImpactFX` → Niagara 시스템 (있으면)
- `WarningPulseFX` → Niagara 시스템 (있으면)

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

---

## CollisionDetector 임계값 설정

SessionManager 내 CollisionDetector 컴포넌트에서:

| 프로퍼티 | 기본값 | 설명 |
|---------|-------|------|
| WarningThreshold | `10.0` cm | 이 거리 이내 → Warning |
| CollisionThreshold | `3.0` cm | 이 거리 이내 → Collision |
| MeasurementHz | `30.0` Hz | 거리 측정 빈도 |

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
- `WBP_VTC_SubjectInfo`에서 키를 입력하고 시작했는지 확인
- `StartSessionWithHeight(SubjectID, Height_cm)` 호출 여부 확인
- HMD만으로 세션을 시작하면 `EstimatedHeight`(자동 추정, ±5cm 오차)가 사용됨
