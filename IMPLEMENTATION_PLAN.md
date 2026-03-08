# VRTrackerCollision — 신규 기능 구현 계획

> **작성일:** 2026-03-01
> **대상 기능:** A~I (총 9가지)
> **방침:** 기존 구조를 최대한 유지하면서 점진적으로 추가

---

## 전체 요약

| ID | 기능 | 난이도 | 신규 파일 | 수정 파일 | C++ 상태 |
|----|------|--------|----------|----------|---------|
| A | 임계값 슬라이더 (ProfileManager로 이전) | Low | - | SessionConfig | ✅ 완료 (SetupWidget 삭제됨 — ProfileManager에서 설정) |
| B | JSON 차종별 프리셋 | Mid | VTC_VehiclePreset.h/cpp | SessionConfig, OperatorController | ✅ 완료 |
| C | Tracker 드롭아웃 보간 | Low | - | TrackerTypes, TrackerPawn | ✅ 완료 (`UpdateTracker()` 보간 로직 구현) |
| D | 캘리브레이션 유효성 강화 | Low | - | CalibrationComponent | ✅ 완료 (`ValidateMeasurements()` 강화 구현) |
| E | 진입/이탈 단계 분류 | Mid | - | TrackerTypes, TrackerPawn, DataLogger | ✅ 완료 (`DetectMovementPhase()` + DataLogger CSV) |
| F | 최악 순간 자동 스크린샷 | Low | - | CollisionDetector, DataLogger | ✅ 완료 (CollisionDetector) |
| G | VR 거리 라인 + 수치 | Low | - | CollisionDetector | ✅ 완료 |
| H | 음성 카운트다운 | Low | - | CalibrationComponent | ✅ C++ 완료 (Blueprint에서 SFX 에셋 연결 필요) |
**신규 파일 1개 / 수정 파일 10개**

> **전체 완료 현황 (2026-03-08 기준)**
> - **A~H**: C++ 코드 모두 완료. Blueprint 에셋 연결만 남은 항목: H(SFX)
> - **v3.0 추가**: VTC_ProfileLibrary + VTC_ProfileManagerWidget (프로파일 시스템) 신규 구현 완료
> - **v3.0 제거**: VTC_SetupGameMode, VTC_SetupWidget, VTC_GameMode, VTC_SimPlayerController, VTC_OperatorMonitorWidget, VTC_OperatorViewActor 전면 삭제
>
> **실제 구현 차이점 (계획 vs 실제)**
> - **A**: SetupWidget 삭제로 슬라이더 UI 제거됨. 임계값은 프로파일 JSON에서 직접 설정
> - **B**: `TB_NewPresetName`, `Btn_DeletePreset` — WBP_VTC_ProfileManager에서 대체
> - **C**: 히스토리 기반 선형 외삽(2프레임) 구현. 드롭아웃 중 Debug Sphere 색상 Silver로 표시
> - **D**: `AsymmetryWarningThreshold=0.25f` (계획의 0.15f보다 관대하게 설정), 허벅지/종아리 비율 0.7~1.5 추가 검증
> - **E**: `PhaseVelocityThreshold=5.0f cm/s`, `DataLogger.PhaseMinClearance TMap` + CSV 컬럼 자동 포함
> - **F**: `ScreenshotDelay` 타이머 없이 즉시 `FScreenshotRequest::RequestScreenshot()` 호출
> - **H**: `CountdownSFX` 배열 + `VoiceVolume` 구현 완료. SFX 에셋은 Blueprint에서 할당

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
// ── 거리 임계값 (ProfileManagerWidget에서 설정, CollisionDetector에 적용) ─
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

## Step 2B — 기능 D: 캘리브레이션 유효성 강화 ✅ 구현 완료

> 구현 위치: `Private/Body/VTC_CalibrationComponent.cpp` — `ValidateMeasurements()`
> 프로퍼티: `AsymmetryWarningThreshold=0.25f`, `MinTotalLegLength=50.0f`, `MaxTotalLegLength=130.0f`

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

## Step 2C — 기능 H: 음성 카운트다운 ✅ C++ 완료 (SFX 에셋 Blueprint 연결 필요)

> 구현 위치: `Private/Body/VTC_CalibrationComponent.cpp`
> Blueprint 작업: `BP_VTC_SessionManager` → CalibrationComp → `CountdownSFX[0~3]` 에셋 연결

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

## Step 3A — 기능 C: Tracker 드롭아웃 보간 ✅ 구현 완료

> 구현 위치: `Private/Pawn/VTC_TrackerPawn.cpp` — `UpdateTracker()`
> 히스토리 2프레임 선형 외삽, MaxDropoutFrames=5, 드롭아웃 시 Debug Sphere 색상 Silver

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

## Step 3B — 기능 E: 진입/이탈 단계 분류 ✅ 구현 완료

> 구현 위치: `Private/Pawn/VTC_TrackerPawn.cpp` — `DetectMovementPhase(DeltaTime)`
> DataLogger: `PhaseMinClearance TMap<EVTCMovementPhase, float>` + Summary CSV 컬럼 포함
> CSV 컬럼: `Phase_Entering_MinClearance`, `Phase_Seated_MinClearance`, `Phase_Exiting_MinClearance`

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

## Step 4A — 기능 A: 거리 임계값 적용 (ProfileManagerWidget → OperatorController)

> **v3.0:** 임계값 슬라이더는 VRTestLevel이 아닌 `WBP_VTC_ProfileManager` (Editor Utility Widget)에서 사전 설정.
> `VTC_SetupWidget`은 삭제됨. OperatorController가 프로파일 JSON을 읽어 임계값을 CollisionDetector에 적용.

**`VTC_OperatorController.cpp` — `ApplyGameInstanceConfig()`에 추가:**
```cpp
// CollisionDetector에 임계값 적용
if (UVTC_CollisionDetector* Detector = /* SessionManager->CollisionDetector */)
{
    Detector->WarningThreshold   = Config.WarningThreshold_cm;
    Detector->CollisionThreshold = Config.CollisionThreshold_cm;
}
```

**WBP_VTC_ProfileManager BindWidget (거리 임계값 슬라이더 — v3.0 이전 SetupWidget 대체):**
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

**동작 흐름 (WBP_VTC_ProfileManager — Editor Utility Widget):**
```
> v3.0: VTC_SetupWidget 삭제됨. 프리셋 선택은 WBP_VTC_ProfileManager에서 사전 저장.

[에디터에서 사전 설정]
WBP_VTC_ProfileManager → Combo_VehiclePreset 선택 → 프로파일에 프리셋 정보 포함하여 저장

[VRTestLevel 실행 후 P키]
GameInstance.ApplyProfileByName() → SessionConfig 로드
  → Config.SelectedPresetName 포함 → ApplyGameInstanceConfig() 호출

[프리셋 저장/삭제]
WBP_VTC_ProfileManager에서 직접 관리 (에디터에서 실행)
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

**WBP_VTC_ProfileManager BindWidget (차종 프리셋 선택 — v3.0 이전 SetupWidget 대체):**
```
[Section] 차종 프리셋
  ├─ ComboBoxString Combo_VehiclePreset  "프리셋 선택..."
  ├─ HorizontalBox
  │    ├─ EditableTextBox TB_NewPresetName  (힌트: "새 프리셋 이름")
  │    ├─ Button Btn_SavePreset   "저장"
  │    └─ Button Btn_DeletePreset "삭제"
```

---

## 최종 파일 변경 목록

### 신규 파일 (1개)
| 파일 | 목적 |
|------|------|
| `Public/VTC_VehiclePreset.h` | 프리셋 구조체 + Manager 클래스 |
| `Private/VTC_VehiclePreset.cpp` | JSON 저장/불러오기 |

### 수정 파일 (10개)
| 파일 | 변경 내용 |
|------|----------|
| `Tracker/VTC_TrackerTypes.h` | EVTCMovementPhase, FVTCTrackerData.bIsInterpolated, FOnVTCPhaseChanged |
| `VTC_SessionConfig.h` | WarningThreshold, CollisionThreshold, bUsePreset, PresetJson |
| `Pawn/VTC_TrackerPawn.h/cpp` | 드롭아웃 보간, 단계 감지 |
| `Body/VTC_CalibrationComponent.h/cpp` | 유효성 강화, 음성 카운트다운 |
| `Collision/VTC_CollisionDetector.h/cpp` | DrawDebugString, 자동 스크린샷 |
| `Data/VTC_DataLogger.h/cpp` | 단계별 MinClearance, 스크린샷 경로 |
| `Controller/VTC_OperatorController.h/cpp` | 임계값 적용, 프리셋 스폰 |

---

## Blueprint 작업 (구현 후)

| 작업 | 설명 |
|------|------|
| `BP_VTC_SessionManager` CalibrationComp | CountdownSFX 4개 연결 |
