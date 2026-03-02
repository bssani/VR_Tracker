# VRTrackerCollision — 신규 기능 구현 계획

> **작성일:** 2026-03-01
> **대상 기능:** A~I (총 9가지)
> **방침:** 기존 구조를 최대한 유지하면서 점진적으로 추가

---

## 전체 요약

| ID | 기능 | 난이도 | 신규 파일 | 수정 파일 | C++ 상태 |
|----|------|--------|----------|----------|---------|
| A | Level 1 임계값 슬라이더 | Low | - | SessionConfig, SetupWidget | ✅ 완료 |
| B | JSON 차종별 프리셋 | Mid | VTC_VehiclePreset.h/cpp | SessionConfig, SetupWidget, OperatorController | ✅ 완료 (SetupWidget/VehiclePreset) / OperatorController 미적용 |
| C | Tracker 드롭아웃 보간 | Low | - | TrackerTypes, TrackerPawn | 미완료 |
| D | 캘리브레이션 유효성 강화 | Low | - | CalibrationComponent | 미완료 |
| E | 진입/이탈 단계 분류 | Mid | - | TrackerTypes, TrackerPawn, DataLogger | 미완료 |
| F | 최악 순간 자동 스크린샷 | Low | - | CollisionDetector, DataLogger | ✅ 완료 (CollisionDetector) |
| G | VR 거리 라인 + 수치 | Low | - | CollisionDetector | ✅ 완료 |
| H | 음성 카운트다운 | Low | - | CalibrationComponent | 미완료 |
| I | Operator View (Spectator) | Mid | VTC_OperatorViewActor.h/cpp | OperatorController | 미완료 |

**신규 파일 2개 / 수정 파일 10개**

> **실제 구현 차이점 (계획 vs 실제)**
> - **A**: 슬라이더 간 상호 클램프(`CollisionThreshold < WarningThreshold`) 로직은 `OnWarningSliderChanged`/`OnCollisionSliderChanged` 에 미포함 — `VTC_CollisionDetector`의 `PostEditChangeProperty` + 런타임 클램프로 대신 처리
> - **B**: `TB_NewPresetName`, `Btn_DeletePreset` 위젯 미구현 (프리셋 이름은 SubjectID 또는 기존 선택 항목 재사용)
> - **F**: `ScreenshotDelay` 타이머 없이 즉시 `FScreenshotRequest::RequestScreenshot()` 호출

---

## 구현 순서 (의존성 기준)

```
Step 1 — 기반 타입 변경 (다른 모든 기능의 전제)
  └─ VTC_TrackerTypes.h       : EVTCMovementPhase 추가, FVTCTrackerData.bIsInterpolated 추가
  └─ VTC_SessionConfig.h      : WarningThreshold, CollisionThreshold, bUsePreset, LoadedPreset 추가

Step 2 — 독립적으로 구현 가능한 기능 (병렬 가능)
  ├─ G: CollisionDetector.cpp  → DrawDebugString 한 줄 추가
  ├─ D: CalibrationComponent   → ValidateMeasurements() 검사 항목 강화
  └─ H: CalibrationComponent   → USoundBase 배열 + PlaySound

Step 3 — 상호 의존하는 기능
  ├─ C: TrackerPawn            → 히스토리 버퍼 + 보간 로직
  ├─ E: TrackerPawn + DataLogger → 단계 감지 + 단계별 CSV 기록
  └─ F: CollisionDetector + DataLogger → 스크린샷 트리거 + 경로 저장

Step 4 — UI 및 데이터 흐름
  ├─ A: SetupWidget + SessionConfig → 임계값 슬라이더
  └─ B: VTC_VehiclePreset (신규) + SetupWidget + OperatorController → 프리셋 시스템

Step 5 — Operator View
  └─ I: VTC_OperatorViewActor (신규) + OperatorController → Spectator Screen
```

---

## Step 1 — 기반 타입 변경

### `VTC_TrackerTypes.h`

**추가할 Enum:**
```cpp
UENUM(BlueprintType)
enum class EVTCMovementPhase : uint8
{
    Unknown     UMETA(DisplayName = "Unknown"),
    Stationary  UMETA(DisplayName = "Stationary"),   // Hip 거의 안 움직임
    Entering    UMETA(DisplayName = "Entering"),      // Hip Z 빠르게 내려감 (탑승 중)
    Seated      UMETA(DisplayName = "Seated"),        // Hip 안정됨 (착석 완료)
    Exiting     UMETA(DisplayName = "Exiting"),       // Hip Z 빠르게 올라감 (하차 중)
};

// Phase 변경 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVTCPhaseChanged,
    EVTCMovementPhase, OldPhase,
    EVTCMovementPhase, NewPhase);
```

**`FVTCTrackerData`에 추가:**
```cpp
// true = 실제 추적 아님, 드롭아웃 보간값 사용 중
UPROPERTY(BlueprintReadOnly, Category = "VTC|Tracker")
bool bIsInterpolated = false;
```

---

### `VTC_SessionConfig.h`

**`FVTCSessionConfig`에 추가:**
```cpp
// ── 거리 임계값 (Level 1에서 설정, CollisionDetector에 적용) ────────────
UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Thresholds")
float WarningThreshold_cm = 10.0f;      // 이 거리 이하 → Warning

UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Thresholds")
float CollisionThreshold_cm = 3.0f;     // 이 거리 이하 → Collision

// ── 차종 프리셋 ─────────────────────────────────────────────────────────
UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Preset")
bool bUseVehiclePreset = false;         // true면 LoadedPreset으로 RefPoint 스폰

UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Preset")
FString SelectedPresetName = TEXT(""); // 선택된 프리셋 이름 (표시/로그용)

// FVTCVehiclePreset은 VTC_VehiclePreset.h 정의 후 여기에 include 추가
// 여기서는 직렬화된 JSON 문자열로 저장 (GameInstance 직렬화 용이)
UPROPERTY(BlueprintReadWrite, Category = "VTC|Config|Preset")
FString LoadedPresetJson = TEXT("");    // JSON 직렬화된 프리셋 내용
```

**INI 저장 항목 추가:** WarningThreshold_cm, CollisionThreshold_cm (LoadedPresetJson은 INI 저장 안 함, 너무 길어짐)

---

## Step 2A — 기능 G: VR 거리 수치 (DrawDebugString)

### `VTC_CollisionDetector.h` 추가

```cpp
// 거리 수치를 VR에서 라인 중간에 표시할지 여부
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Debug")
bool bShowDistanceLabels = true;

// 라인 두께
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Debug")
float DebugLineThickness = 1.5f;
```

### `VTC_CollisionDetector.cpp` 수정

`PerformDistanceMeasurements()` 내 기존 DrawDebugLine 코드 **교체**:

```cpp
// ─ 기존 코드 (라인만) ─────────────────────────────────────────
DrawDebugLine(GetWorld(), BodyLocation, RefPoint->GetReferenceLocation(),
              LineColor, false, -1.0f, 0, 0.5f);

// ─ 교체 후 (라인 + 중간 수치) ─────────────────────────────────
DrawDebugLine(GetWorld(), BodyLocation, RefPoint->GetReferenceLocation(),
              LineColor, false, -1.0f, 0, DebugLineThickness);

if (bShowDistanceLabels)
{
    const FVector MidPoint = (BodyLocation + RefPoint->GetReferenceLocation()) * 0.5f;
    const FString Label = FString::Printf(TEXT("%.1fcm"), SafeDistanceResult);
    // bDrawShadow=true, FontScale=1.5f — VR에서 가독성 확보
    DrawDebugString(GetWorld(), MidPoint, Label,
                    /*TestBaseActor=*/nullptr, LineColor, /*Duration=*/-1.0f,
                    /*bDrawShadow=*/true, /*FontScale=*/1.5f);
}
```

> **참고:** DrawDebugString은 3D 월드 좌표에 텍스트를 그림. VR에서는 양안 모두 보임.
> FontScale 1.5f → 약 30cm 거리에서 읽힘. 필요하면 Blueprint에서 조절.

---

## Step 2B — 기능 D: 캘리브레이션 유효성 강화

### `VTC_CalibrationComponent.cpp`

기존 `ValidateMeasurements(FVTCBodyMeasurements&, FString&)` 함수에 **검사 항목 추가**:

```
기존: 각 세그먼트 > 10cm 확인

추가 ①: 좌우 비대칭 검사
  - |Hip_L - Hip_R| / max(Hip_L, Hip_R) > 15% → Reason: "좌우 허리-무릎 비대칭 15% 초과"
  - |LKnee_LFoot - RKnee_RFoot| / max(...) > 15% → Reason: "좌우 무릎-발 비대칭 15% 초과"
  → 경고이나 실패는 아님 (bIsCalibrated = true, 단 Reason에 경고 문자열 포함)

추가 ②: 해부학적 범위 검사
  - TotalLeftLeg < 40cm → Reason: "왼쪽 다리 전체 길이 너무 짧음 (40cm 미만)"
  - TotalLeftLeg > 120cm → Reason: "왼쪽 다리 전체 길이 너무 김 (120cm 초과)"
  - 동일하게 RightLeg 검사
  → 이 경우는 실패 (bIsCalibrated = false)

추가 ③: 예상 신장과의 정합성
  - EstimatedHeight > 0이고 TotalLeftLeg > EstimatedHeight * 0.65 → 다리가 키보다 너무 긴 비율
  - 경고만 (실패 아님)
```

**`VTC_CalibrationComponent.h`에 추가:**
```cpp
// 좌우 비대칭 경고 임계값 (비율, 0~1)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration|Validation",
    meta=(ClampMin=0.05f, ClampMax=0.5f))
float AsymmetryWarningThreshold = 0.15f;   // 15%

// 다리 전체 길이 최소/최대 (cm)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration|Validation")
float MinTotalLegLength = 40.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration|Validation")
float MaxTotalLegLength = 120.0f;
```

---

## Step 2C — 기능 H: 음성 카운트다운

### `VTC_CalibrationComponent.h`에 추가

```cpp
// ── 음성 카운트다운 ────────────────────────────────────────────────────
// Blueprint에서 사운드 에셋 연결. 인덱스: [0]=3초, [1]=2초, [2]=1초, [3]=완료
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration|Sound")
TArray<TObjectPtr<USoundBase>> CountdownSFX;

// 음성 볼륨
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Calibration|Sound",
    meta=(ClampMin=0.0f, ClampMax=2.0f))
float VoiceVolume = 1.0f;
```

### `VTC_CalibrationComponent.cpp` 수정

카운트다운 타이머 콜백 (OnCalibCountdown 브로드캐스트 직전):
```cpp
// 인덱스 계산: SecondsRemaining=3→[0], 2→[1], 1→[2]
const int32 SFXIndex = FMath::Clamp(3 - SecondsRemaining, 0, 2);
if (CountdownSFX.IsValidIndex(SFXIndex) && CountdownSFX[SFXIndex])
{
    UGameplayStatics::PlaySound2D(GetWorld(), CountdownSFX[SFXIndex], VoiceVolume);
}
OnCalibrationCountdown.Broadcast(SecondsRemaining);
```

캘리브레이션 완료 시:
```cpp
// [3] = 완료 사운드
if (CountdownSFX.IsValidIndex(3) && CountdownSFX[3])
{
    UGameplayStatics::PlaySound2D(GetWorld(), CountdownSFX[3], VoiceVolume);
}
OnCalibrationComplete.Broadcast(Measurements);
```

> Blueprint에서 BP_VTC_SessionManager → CalibrationComp → CountdownSFX 배열에
> SC_VTC_Cal_3, SC_VTC_Cal_2, SC_VTC_Cal_1, SC_VTC_Cal_Complete 4개 연결.

---

## Step 3A — 기능 C: Tracker 드롭아웃 보간

### `VTC_TrackerPawn.h` 추가

```cpp
// ── 드롭아웃 보간 ──────────────────────────────────────────────────────
// 트래커 신호가 끊겼을 때 최대 N프레임 동안 마지막 위치를 유지
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Dropout")
int32 MaxDropoutFrames = 5;      // 5프레임(90fps 기준 ~55ms) 까지 보간

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Dropout")
bool bEnableDropoutInterpolation = true;

// ─ Private ────────────────────────────────────────────────────────────
// 각 트래커별 최근 2개 위치 히스토리 (선형 보간용)
TMap<EVTCTrackerRole, TArray<FVector>> TrackerLocationHistory;  // 최대 2개 유지
TMap<EVTCTrackerRole, int32> DropoutFrameCount;                 // 현재 드롭아웃 프레임 수
```

### `VTC_TrackerPawn.cpp` — `UpdateTracker()` 수정

```cpp
void AVTC_TrackerPawn::UpdateTracker(EVTCTrackerRole TrackerRole, UMotionControllerComponent* MC)
{
    if (!MC) return;

    FVTCTrackerData& Data = TrackerDataMap.FindOrAdd(TrackerRole);
    Data.Role         = TrackerRole;
    Data.bIsInterpolated = false;

    const bool bCurrentlyTracked = MC->IsTracked();

    if (bCurrentlyTracked)
    {
        // ── 정상 추적 ──────────────────────────────────────────────────
        Data.bIsTracked    = true;
        Data.WorldLocation = MC->GetComponentLocation();
        Data.WorldRotation = MC->GetComponentRotation();

        // 히스토리 업데이트 (최대 2개 유지 → 선형 보간에 충분)
        TArray<FVector>& History = TrackerLocationHistory.FindOrAdd(TrackerRole);
        History.Add(Data.WorldLocation);
        if (History.Num() > 2) History.RemoveAt(0);

        DropoutFrameCount.FindOrAdd(TrackerRole) = 0;
    }
    else if (bEnableDropoutInterpolation)
    {
        // ── 드롭아웃: 보간 ──────────────────────────────────────────────
        int32& DropoutFrames = DropoutFrameCount.FindOrAdd(TrackerRole);
        ++DropoutFrames;

        TArray<FVector>* History = TrackerLocationHistory.Find(TrackerRole);

        if (DropoutFrames <= MaxDropoutFrames && History && History->Num() >= 1)
        {
            // 히스토리 2개 있으면 선형 외삽, 1개면 마지막 위치 유지
            if (History->Num() >= 2)
            {
                const FVector Velocity = (*History)[1] - (*History)[0];
                Data.WorldLocation = (*History)[1] + Velocity * (float)DropoutFrames;
            }
            else
            {
                Data.WorldLocation = (*History)[0];
            }
            Data.bIsTracked      = true;   // 다운스트림 코드가 정상 동작하도록
            Data.bIsInterpolated = true;
        }
        else
        {
            // 최대 프레임 초과 → 진짜 미추적
            Data.bIsTracked      = false;
            Data.bIsInterpolated = false;
        }
    }
    else
    {
        Data.bIsTracked = false;
    }

    // Debug Sphere (보간 중이면 회색)
    if (bShowDebugSpheres && Data.bIsTracked)
    {
        FColor Color = Data.bIsInterpolated ? FColor::Silver : GetTrackerDebugColor(TrackerRole);
        DrawDebugSphere(GetWorld(), Data.WorldLocation, DebugSphereRadius, 8, Color, false, -1.0f, 0, 1.0f);
    }

    OnTrackerUpdated.Broadcast(TrackerRole, Data);
}
```

---

## Step 3B — 기능 E: 진입/이탈 단계 분류

### `VTC_TrackerPawn.h` 추가

```cpp
// ── 이동 단계 감지 ─────────────────────────────────────────────────────
UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Phase")
EVTCMovementPhase CurrentPhase = EVTCMovementPhase::Unknown;

UPROPERTY(BlueprintAssignable, Category = "VTC|Phase|Events")
FOnVTCPhaseChanged OnPhaseChanged;

// Hip Z 속도 임계값 (cm/s): 이 속도 이상으로 내려가면 Entering
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Phase",
    meta=(ClampMin=1.0f, ClampMax=30.0f))
float PhaseEnterVelocityThreshold = 5.0f;

// Hip이 이 높이(cm) 이하가 되면 Seated로 전환
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Phase")
float SeatedHipHeightThreshold = 80.0f;   // 바닥 기준 80cm 이하 = 착석 판단

// ─ Private ────────────────────────────────────────────────────────────
FVector PreviousWaistLocation = FVector::ZeroVector;
float   PhaseStationaryTimer  = 0.0f;    // 정지 상태 지속 시간
static constexpr float StationaryTimeout = 0.5f;  // 0.5초 이상 정지 → Stationary
```

### `VTC_TrackerPawn.cpp` — `Tick()` 끝에 추가

```cpp
// 매 프레임: Hip 속도 계산 → 단계 판단
void AVTC_TrackerPawn::UpdateMovementPhase(float DeltaTime)
{
    const FVector CurrentWaist = GetTrackerLocation(EVTCTrackerRole::Waist);
    if (CurrentWaist.IsZero() || PreviousWaistLocation.IsZero())
    {
        PreviousWaistLocation = CurrentWaist;
        return;
    }

    const float HipZVelocity = (CurrentWaist.Z - PreviousWaistLocation.Z) / DeltaTime; // cm/s
    PreviousWaistLocation = CurrentWaist;

    EVTCMovementPhase NewPhase = CurrentPhase;

    if (FMath::Abs(HipZVelocity) < PhaseEnterVelocityThreshold)
    {
        PhaseStationaryTimer += DeltaTime;
        if (PhaseStationaryTimer >= StationaryTimeout)
        {
            // 착석 여부: Hip 높이 기준
            NewPhase = (CurrentWaist.Z <= SeatedHipHeightThreshold)
                       ? EVTCMovementPhase::Seated
                       : EVTCMovementPhase::Stationary;
        }
    }
    else
    {
        PhaseStationaryTimer = 0.0f;
        NewPhase = (HipZVelocity < 0.0f)  // Z 감소 = 몸이 내려감 = 탑승
                   ? EVTCMovementPhase::Entering
                   : EVTCMovementPhase::Exiting;
    }

    if (NewPhase != CurrentPhase)
    {
        const EVTCMovementPhase OldPhase = CurrentPhase;
        CurrentPhase = NewPhase;
        OnPhaseChanged.Broadcast(OldPhase, NewPhase);
    }
}
```

### `VTC_DataLogger.h` 추가 필드

```cpp
// 단계별 최소 클리어런스 추적
float MinClearance_Entering   = TNumericLimits<float>::Max();
float MinClearance_Seated     = TNumericLimits<float>::Max();
float MinClearance_Exiting    = TNumericLimits<float>::Max();
EVTCMovementPhase CurrentPhase = EVTCMovementPhase::Unknown;

// Phase 설정 (OperatorController → 연결)
UFUNCTION(BlueprintCallable, Category = "VTC|Logger")
void SetCurrentPhase(EVTCMovementPhase NewPhase);
```

### `BuildSummaryHeader()` / `BuildSummaryRow()` 추가

```
추가 컬럼:
,MinClearance_Entering_cm,MinClearance_Seated_cm,MinClearance_Exiting_cm
```

---

## Step 3C — 기능 F: 최악 순간 자동 스크린샷

### `VTC_CollisionDetector.h` 추가

```cpp
// 최악 클리어런스 갱신 시 스크린샷 자동 캡처
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Screenshot")
bool bAutoScreenshotOnWorstClearance = true;

// 스크린샷 저장 경로 (비워두면 VTCLogs/Screenshots/ 자동 사용)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Screenshot")
FString ScreenshotDirectory = TEXT("");

// 가장 최근 스크린샷 경로 (DataLogger가 읽어감)
UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Screenshot")
FString LastScreenshotPath = TEXT("");

// 스크린샷 딜레이 (초): 충돌 후 렌더링이 안정되면 찍음
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Screenshot",
    meta=(ClampMin=0.0f, ClampMax=2.0f))
float ScreenshotDelay = 0.1f;
```

### `VTC_CollisionDetector.cpp` — `PerformDistanceMeasurements()` 내

```cpp
// 최소 거리 갱신 시 스크린샷
if (SafeDistanceResult < SessionMinDistance)
{
    SessionMinDistance = SafeDistanceResult;

    if (bAutoScreenshotOnWorstClearance && GetWorld())
    {
        const FString Dir = ScreenshotDirectory.IsEmpty()
            ? FPaths::ProjectSavedDir() + TEXT("VTCLogs/Screenshots/")
            : ScreenshotDirectory;

        // 파일명: VTC_<SubjectID>_<Timestamp>_worst.png
        const FString Filename = Dir + FString::Printf(
            TEXT("worst_%.1fcm_%s.png"),
            SafeDistanceResult,
            *FDateTime::Now().ToString(TEXT("%H%M%S")));

        // 딜레이 후 촬영 (렌더링 안정화)
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, Filename]()
        {
            FScreenshotRequest::RequestScreenshot(Filename, false, false);
            LastScreenshotPath = Filename;
            UE_LOG(LogTemp, Log, TEXT("[VTC] Screenshot: %s"), *Filename);
        }, ScreenshotDelay, false);
    }
}
```

### `VTC_DataLogger.h` 추가

```cpp
FString WorstClearanceScreenshotPath = TEXT("");
```

`BuildSummaryHeader()`에 `,WorstClearanceScreenshot` 컬럼 추가.

---

## Step 4A — 기능 A: Level 1 임계값 슬라이더

### `VTC_SetupWidget.h` 추가

```cpp
// BindWidget: 임계값 슬라이더
UPROPERTY(meta=(BindWidget))
TObjectPtr<USlider> Slider_Warning;      // 범위: 3~50cm, 기본 10

UPROPERTY(meta=(BindWidget))
TObjectPtr<USlider> Slider_Collision;    // 범위: 1~20cm, 기본 3

// 슬라이더 현재값 표시
UPROPERTY(meta=(BindWidget))
TObjectPtr<UTextBlock> Txt_WarningVal;   // "10.0 cm"

UPROPERTY(meta=(BindWidget))
TObjectPtr<UTextBlock> Txt_CollisionVal; // "3.0 cm"
```

**NativeConstruct()에서:**
```cpp
Slider_Warning->OnValueChanged.AddDynamic(this, &ThisClass::OnWarningSliderChanged);
Slider_Collision->OnValueChanged.AddDynamic(this, &ThisClass::OnCollisionSliderChanged);
```

**콜백:**
```cpp
void UVTC_SetupWidget::OnWarningSliderChanged(float Value)
{
    // Collision 임계값보다 항상 크게 유지
    const float CollisionVal = Slider_Collision->GetValue();
    const float ClampedVal   = FMath::Max(Value, CollisionVal + 1.0f);
    Slider_Warning->SetValue(ClampedVal);
    Txt_WarningVal->SetText(FText::FromString(
        FString::Printf(TEXT("%.0f cm"), ClampedVal)));
}
```

**`BuildConfigFromInputs()`에 추가:**
```cpp
Config.WarningThreshold_cm   = Slider_Warning->GetValue();
Config.CollisionThreshold_cm = Slider_Collision->GetValue();
```

**`VTC_OperatorController.cpp` — `ApplyGameInstanceConfig()`에 추가:**
```cpp
// CollisionDetector에 임계값 적용
if (UVTC_CollisionDetector* Detector = /* SessionManager->CollisionDetector */)
{
    Detector->WarningThreshold   = Config.WarningThreshold_cm;
    Detector->CollisionThreshold = Config.CollisionThreshold_cm;
}
```

**Designer 레이아웃 (Level 1 SetupWidget에 추가):**
```
[Section] 거리 임계값 설정
  ├─ HorizontalBox
  │    ├─ TextBlock "Warning 임계값"
  │    ├─ Slider Slider_Warning     (Min=3, Max=50, Default=10)
  │    └─ TextBlock Txt_WarningVal  "10 cm"
  └─ HorizontalBox
       ├─ TextBlock "Collision 임계값"
       ├─ Slider Slider_Collision   (Min=1, Max=20, Default=3)
       └─ TextBlock Txt_CollisionVal "3 cm"
```

---

## Step 4B — 기능 B: JSON 차종별 프리셋

### 신규 파일: `VTC_VehiclePreset.h`

```cpp
// 개별 기준점 데이터 (JSON 직렬화용)
USTRUCT(BlueprintType)
struct FVTCRefPointPresetData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString PartName = TEXT("");

    UPROPERTY(BlueprintReadWrite)
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    TArray<EVTCTrackerRole> RelevantBodyParts;

    UPROPERTY(BlueprintReadWrite)
    float MarkerRadius = 5.0f;
};

// 차종 프리셋 전체
USTRUCT(BlueprintType)
struct FVTCVehiclePreset
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString PresetName = TEXT("");

    UPROPERTY(BlueprintReadWrite)
    FVector VehicleHipPosition = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite)
    TArray<FVTCRefPointPresetData> ReferencePoints;

    UPROPERTY(BlueprintReadWrite)
    float WarningThreshold_cm = 10.0f;

    UPROPERTY(BlueprintReadWrite)
    float CollisionThreshold_cm = 3.0f;
};

// 프리셋 관리 클래스
UCLASS(BlueprintType)
class UVTC_VehiclePresetManager : public UObject
{
    GENERATED_BODY()
public:
    // 저장 디렉토리: [Project]/Saved/VTCPresets/
    static FString GetPresetDirectory();

    UFUNCTION(BlueprintCallable, Category = "VTC|Preset")
    static bool SavePreset(const FVTCVehiclePreset& Preset);

    UFUNCTION(BlueprintCallable, Category = "VTC|Preset")
    static bool LoadPreset(const FString& PresetName, FVTCVehiclePreset& OutPreset);

    UFUNCTION(BlueprintCallable, Category = "VTC|Preset")
    static TArray<FString> GetAllPresetNames();

    UFUNCTION(BlueprintCallable, Category = "VTC|Preset")
    static bool DeletePreset(const FString& PresetName);

    // FVTCVehiclePreset ↔ JSON 변환
    static FString PresetToJson(const FVTCVehiclePreset& Preset);
    static bool JsonToPreset(const FString& JsonStr, FVTCVehiclePreset& OutPreset);
};
```

### 신규 파일: `VTC_VehiclePreset.cpp`

- `GetPresetDirectory()`: `FPaths::ProjectSavedDir() + TEXT("VTCPresets/")`
- `SavePreset()`: `FJsonObjectConverter::UStructToJsonObjectString()` → `FFileHelper::SaveStringToFile()`
- `LoadPreset()`: `FFileHelper::LoadFileToString()` → `FJsonObjectConverter::JsonObjectStringToUStruct()`
- `GetAllPresetNames()`: `IFileManager::FindFiles()` → `.json` 파일명 목록

> UE5의 `FJsonObjectConverter`가 USTRUCT를 JSON으로 자동 변환해줌 (TArray, FVector 포함).
> `EVTCTrackerRole` enum은 `EnumToString` 헬퍼로 직렬화.

### `VTC_SetupWidget.h` 추가

```cpp
// BindWidget: 프리셋 UI
UPROPERTY(meta=(BindWidget))
TObjectPtr<UComboBoxString> Combo_VehiclePreset;

UPROPERTY(meta=(BindWidget))
TObjectPtr<UEditableTextBox> TB_NewPresetName;

UPROPERTY(meta=(BindWidget))
TObjectPtr<UButton> Btn_SavePreset;

UPROPERTY(meta=(BindWidget))
TObjectPtr<UButton> Btn_DeletePreset;
```

**동작 흐름:**
```
NativeConstruct → UVTC_VehiclePresetManager::GetAllPresetNames() → Combo_VehiclePreset 채우기

[프리셋 선택]
Combo_VehiclePreset.OnSelectionChanged
  → LoadPreset(선택된 이름) → PopulateFromPreset()
     → VehicleHipPosition 자동 입력
     → WarningThreshold, CollisionThreshold 자동 입력
     → Config.bUseVehiclePreset = true

[Start Session]
BuildConfigFromInputs()
  → Config.SelectedPresetName = 선택된 프리셋 이름
  → Config.LoadedPresetJson = PresetToJson(LoadedPreset)
  → Level 2로 OpenLevel

[프리셋 저장] (Level 2에서 돌아온 후, 또는 Level 1에서 수동 입력)
Btn_SavePreset.OnClicked
  → TB_NewPresetName.Text = 프리셋 이름
  → BuildPresetFromInputs() → SavePreset()
  → Combo 갱신

[프리셋 삭제]
Btn_DeletePreset.OnClicked
  → DeletePreset(선택된 이름) → Combo 갱신
```

### `VTC_OperatorController.cpp` — `ApplyGameInstanceConfig()` 추가

```cpp
// 프리셋이 있으면 레퍼런스 포인트 스폰
if (Config.bUseVehiclePreset && !Config.LoadedPresetJson.IsEmpty())
{
    FVTCVehiclePreset Preset;
    if (UVTC_VehiclePresetManager::JsonToPreset(Config.LoadedPresetJson, Preset))
    {
        // 기존 자동탐색 RefPoint를 무시하고 프리셋 데이터로 스폰
        SpawnReferencePointsFromPreset(Preset);
    }
}
```

```cpp
void AVTC_OperatorController::SpawnReferencePointsFromPreset(const FVTCVehiclePreset& Preset)
{
    for (const FVTCRefPointPresetData& Data : Preset.ReferencePoints)
    {
        FActorSpawnParameters Params;
        AVTC_ReferencePoint* RefPoint = GetWorld()->SpawnActor<AVTC_ReferencePoint>(
            AVTC_ReferencePoint::StaticClass(), Data.WorldLocation, FRotator::ZeroRotator, Params);
        if (RefPoint)
        {
            RefPoint->PartName          = Data.PartName;
            RefPoint->RelevantBodyParts = Data.RelevantBodyParts;
            RefPoint->MarkerRadius      = Data.MarkerRadius;
            SessionManager->GetCollisionDetector()->ReferencePoints.AddUnique(RefPoint);
            SpawnedPresetRefPoints.Add(RefPoint);   // EndPlay 정리용
        }
    }
}
```

**Designer 레이아웃 (Level 1 SetupWidget에 추가):**
```
[Section] 차종 프리셋
  ├─ ComboBoxString Combo_VehiclePreset  "프리셋 선택..."
  ├─ HorizontalBox
  │    ├─ EditableTextBox TB_NewPresetName  (힌트: "새 프리셋 이름")
  │    ├─ Button Btn_SavePreset   "저장"
  │    └─ Button Btn_DeletePreset "삭제"
```

---

## Step 5 — 기능 I: Operator View (Spectator Screen)

### 신규 파일: `VTC_OperatorViewActor.h`

```cpp
UCLASS(BlueprintType, Blueprintable)
class VRTRACKERCOLLISION_API AVTC_OperatorViewActor : public AActor
{
    GENERATED_BODY()
public:
    AVTC_OperatorViewActor();
    virtual void BeginPlay() override;

    // 탑뷰 카메라 (SceneCapture)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|OperatorView")
    TObjectPtr<USceneCaptureComponent2D> TopDownCapture;

    // 사이드뷰 카메라 (선택사항)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|OperatorView")
    TObjectPtr<USceneCaptureComponent2D> SideCapture;

    // 렌더 타겟 (Blueprint에서 UMG Image에 연결)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|OperatorView")
    TObjectPtr<UTextureRenderTarget2D> TopDownRenderTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|OperatorView")
    TObjectPtr<UTextureRenderTarget2D> SideRenderTarget;

    // 탑뷰 높이 (cm)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|OperatorView")
    float TopDownHeight = 300.0f;

    // 추적 대상 Actor (TrackerPawn)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|OperatorView")
    TObjectPtr<AActor> FollowTarget;

    // Spectator Screen에 표시할 위젯 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|OperatorView")
    TSubclassOf<UUserWidget> OperatorWidgetClass;

    UPROPERTY(BlueprintReadOnly, Category = "VTC|OperatorView")
    TObjectPtr<UUserWidget> OperatorWidget;

    virtual void Tick(float DeltaTime) override;

private:
    void SetupSpectatorScreen();
};
```

### `VTC_OperatorViewActor.cpp` 핵심 로직

**BeginPlay:**
```cpp
// RenderTarget 동적 생성 (Blueprint에서 미리 설정하지 않은 경우)
if (!TopDownRenderTarget)
{
    TopDownRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    TopDownRenderTarget->InitAutoFormat(1280, 720);
    TopDownRenderTarget->UpdateResourceImmediate(true);
}
TopDownCapture->TextureTarget = TopDownRenderTarget;
TopDownCapture->CaptureSource  = SCS_FinalColorLDR;

// Spectator Screen에 위젯 표시
SetupSpectatorScreen();
```

**Tick: FollowTarget 위치 기반 카메라 갱신**
```cpp
if (FollowTarget)
{
    const FVector TargetLoc = FollowTarget->GetActorLocation();
    TopDownCapture->SetWorldLocation(TargetLoc + FVector(0, 0, TopDownHeight));
    TopDownCapture->SetWorldRotation(FRotator(-90.0f, 0.0f, 0.0f));   // 수직 하향
}
```

**SetupSpectatorScreen:**
```cpp
void AVTC_OperatorViewActor::SetupSpectatorScreen()
{
    if (!OperatorWidgetClass) return;

    OperatorWidget = CreateWidget<UUserWidget>(GetWorld()->GetFirstPlayerController(), OperatorWidgetClass);
    if (!OperatorWidget) return;
    OperatorWidget->AddToViewport();

    // IHeadMountedDisplay API로 Spectator Screen에 위젯 지정
    if (GEngine && GEngine->XRSystem.IsValid())
    {
        // Spectator Screen Mode: SingleEyeCroppedToFill + Custom Widget 오버레이
        UHeadMountedDisplayFunctionLibrary::SetSpectatorScreenMode(
            ESpectatorScreenMode::TexturePlusEye);
        UHeadMountedDisplayFunctionLibrary::SetSpectatorScreenTexture(
            TopDownRenderTarget);
    }
}
```

### `WBP_OperatorView` Widget (Blueprint에서 구현)

```
[Canvas Panel]
  ├─ [Image] (RenderTarget 연결) — 탑뷰 카메라 피드 (좌측 70%)
  └─ [VerticalBox] (우측 30%)
       ├─ TextBlock "● TESTING"         ← 세션 상태
       ├─ TextBlock "Min: 8.2 cm"       ← 현재 최소 거리
       ├─ TextBlock "Trackers: 5/5"     ← 연결 상태
       └─ [ListView] ← 실시간 거리 목록 (OnDistanceUpdated 바인딩)
```

### `VTC_OperatorController.h` 추가

```cpp
// Operator View Actor 참조 (BeginPlay에서 자동 탐색 또는 수동 연결)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Operator")
TObjectPtr<AVTC_OperatorViewActor> OperatorViewActor;

// Spectator Screen 활성화 여부
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Operator")
bool bEnableSpectatorView = true;
```

**BeginPlay에서:**
```cpp
if (bEnableSpectatorView)
{
    // 레벨에서 OperatorViewActor 자동 탐색
    if (!OperatorViewActor)
    {
        TArray<AActor*> Found;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(),
            AVTC_OperatorViewActor::StaticClass(), Found);
        if (Found.Num() > 0)
            OperatorViewActor = Cast<AVTC_OperatorViewActor>(Found[0]);
    }
    // FollowTarget 설정
    if (OperatorViewActor)
        OperatorViewActor->FollowTarget = GetPawn();
}
```

**Level 2 레벨 배치 추가:**
- `BP_VTC_OperatorViewActor` 1개 배치 → OperatorWidgetClass = WBP_OperatorView

---

## 최종 파일 변경 목록

### 신규 파일 (2개)
| 파일 | 목적 |
|------|------|
| `Public/VTC_VehiclePreset.h` | 프리셋 구조체 + Manager 클래스 |
| `Private/VTC_VehiclePreset.cpp` | JSON 저장/불러오기 |
| `Public/World/VTC_OperatorViewActor.h` | Spectator Screen SceneCapture Actor |
| `Private/World/VTC_OperatorViewActor.cpp` | RenderTarget + Spectator Screen 설정 |

### 수정 파일 (10개)
| 파일 | 변경 내용 |
|------|----------|
| `Tracker/VTC_TrackerTypes.h` | EVTCMovementPhase, FVTCTrackerData.bIsInterpolated, FOnVTCPhaseChanged |
| `VTC_SessionConfig.h` | WarningThreshold, CollisionThreshold, bUsePreset, PresetJson |
| `Pawn/VTC_TrackerPawn.h/cpp` | 드롭아웃 보간, 단계 감지 |
| `Body/VTC_CalibrationComponent.h/cpp` | 유효성 강화, 음성 카운트다운 |
| `Collision/VTC_CollisionDetector.h/cpp` | DrawDebugString, 자동 스크린샷 |
| `Data/VTC_DataLogger.h/cpp` | 단계별 MinClearance, 스크린샷 경로 |
| `Controller/VTC_OperatorController.h/cpp` | 임계값 적용, Spectator View, 프리셋 스폰 |
| `UI/VTC_SetupWidget.h/cpp` | 임계값 슬라이더, 프리셋 UI |

---

## Blueprint 작업 (구현 후)

| 작업 | 설명 |
|------|------|
| `WBP_OperatorView` 생성 | RenderTarget Image + 거리 목록 |
| `BP_VTC_OperatorViewActor` 생성 | OperatorWidgetClass 연결 |
| `BP_VTC_SessionManager` CalibrationComp | CountdownSFX 4개 연결 |
| `VTC_SetupLevel` SetupWidget | 슬라이더 + 프리셋 UI BindWidget |
