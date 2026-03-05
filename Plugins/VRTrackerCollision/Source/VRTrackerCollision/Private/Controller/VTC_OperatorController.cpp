// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Controller/VTC_OperatorController.h"
#include "Blueprint/UserWidget.h"
#include "Body/VTC_BodyActor.h"
#include "Collision/VTC_CollisionDetector.h"
#include "Components/InputComponent.h"
#include "Data/VTC_SessionManager.h"
#include "EngineUtils.h" // TActorIterator
#include "Kismet/KismetSystemLibrary.h"
#include "Pawn/VTC_TrackerPawn.h"
#include "UI/VTC_OperatorMonitorWidget.h"
#include "UI/VTC_StatusWidget.h"
#include "VTC_GameInstance.h"
#include "VTC_VehiclePreset.h"
#include "Vehicle/VTC_ReferencePoint.h"
#include "World/VTC_OperatorViewActor.h"
#include "World/VTC_StatusActor.h"

AVTC_OperatorController::AVTC_OperatorController() {
  // Tick 활성화 — TrackerStatus 주기적 갱신에 필요
  PrimaryActorTick.bCanEverTick = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  BeginPlay — 탐색 + Delegate 바인딩 + 초기 상태 표시
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::BeginPlay() {
  Super::BeginPlay();

  // Level 2에서는 마우스 커서 불필요 (키만 사용).
  bShowMouseCursor = false;
  FInputModeGameOnly InputMode;
  SetInputMode(InputMode);

  // ── 레벨에서 필요한 Actor 자동 탐색 ──────────────────────────────────────
  if (!SessionManager)
    AutoFindSessionManager();
  AutoFindStatusActor();
  AutoFindOperatorViewActor();

  // ── SessionManager 상태 변경 시 StatusWidget + OperatorMonitorWidget 갱신 ─
  if (SessionManager) {
    SessionManager->OnSessionStateChanged.AddDynamic(
        this, &AVTC_OperatorController::OnSessionStateChanged);

    // CollisionDetector 거리 갱신 → OperatorMonitorWidget Row 갱신
    if (SessionManager->CollisionDetector) {
      SessionManager->CollisionDetector->OnDistanceUpdated.AddDynamic(
          this, &AVTC_OperatorController::OnDistanceUpdated);
    }
  }

  // ── 운영자 데스크탑 모니터링 위젯 생성 (OperatorMonitorWidgetClass 할당 시)
  // 시뮬레이션(데스크탑) 모드에서만 화면에 추가. VR 모드에서는 HMD 뷰를 가리지 않도록 숨김.
  if (OperatorMonitorWidgetClass) {
    OperatorMonitorWidget = CreateWidget<UVTC_OperatorMonitorWidget>(
        this, OperatorMonitorWidgetClass);
    if (OperatorMonitorWidget) {
      const UVTC_GameInstance* GI = GetGameInstance<UVTC_GameInstance>();
      const bool bIsSimMode = GI && (GI->SessionConfig.RunMode == EVTCRunMode::Simulation);
      if (bIsSimMode) {
        OperatorMonitorWidget->AddToViewport(1); // ZOrder 1: StatusWidget 위
        UE_LOG(LogTemp, Log, TEXT("[VTC] OperatorMonitorWidget 생성 완료 (Simulation 모드)."));
      } else {
        UE_LOG(LogTemp, Log, TEXT("[VTC] OperatorMonitorWidget 생성됨 (VR 모드 — 뷰포트 비표시)."));
      }
    }
  }

  // ── GameInstance → 각 Actor에 설정 적용 ──────────────────────────────────
  // BeginPlay 시점에 Pawn이 이미 possess되어 있으면 즉시 적용.
  // Pawn이 없으면 OnPossess()에서 처리된다.
  if (GetPawn()) {
    ApplyGameInstanceConfig();
    bConfigApplied = true;
  }

  // ── 초기 상태 표시 (StatusWidget 3D + OperatorMonitorWidget 데스크탑) ──────
  UVTC_GameInstance *GI = GetGameInstance<UVTC_GameInstance>();
  const FString SubjectID = GI ? GI->SessionConfig.SubjectID : TEXT("");
  const float Height_cm   = GI ? GI->SessionConfig.Height_cm : 170.0f;
  const bool  bUsePreset  = GI && GI->SessionConfig.bUseVehiclePreset;
  const FString PresetName = GI ? GI->SessionConfig.SelectedPresetName : TEXT("");

  if (StatusActor) {
    if (UVTC_StatusWidget *W = StatusActor->GetStatusWidget()) {
      W->UpdateState(EVTCSessionState::Idle);
      W->UpdateSubjectInfo(SubjectID, Height_cm);
      W->UpdateElapsedTime(0.f);
      W->UpdatePresetInfo(bUsePreset, PresetName);
    }
  }

  if (OperatorMonitorWidget) {
    OperatorMonitorWidget->UpdateState(EVTCSessionState::Idle);
    OperatorMonitorWidget->UpdateSubjectInfo(SubjectID, Height_cm);
    OperatorMonitorWidget->UpdateElapsedTime(0.f);
    OperatorMonitorWidget->UpdatePresetInfo(bUsePreset, PresetName);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  OnPossess — Pawn이 BeginPlay 이후에 possess될 때 설정 적용
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::OnPossess(APawn *InPawn) {
  Super::OnPossess(InPawn);

  if (!bConfigApplied) {
    ApplyGameInstanceConfig();
    bConfigApplied = true;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tick — TrackerStatus 1초마다 갱신
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  TrackerStatusTimer += DeltaTime;
  if (TrackerStatusTimer < TrackerStatusInterval)
    return;
  TrackerStatusTimer = 0.0f;

  int32 ConnectedCount = 0;
  if (AVTC_TrackerPawn *TP = Cast<AVTC_TrackerPawn>(GetPawn()))
    ConnectedCount = TP->GetActiveTrackerCount();

  // 3D StatusWidget 갱신 (1초마다)
  if (StatusActor) {
    if (UVTC_StatusWidget *W = StatusActor->GetStatusWidget()) {
      W->UpdateTrackerStatus(ConnectedCount, 5);
      if (SessionManager) {
        if (SessionManager->IsTesting())
          W->UpdateElapsedTime(SessionManager->SessionElapsedTime);
        if (SessionManager->CollisionDetector) {
          const float MinDist =
              SessionManager->CollisionDetector->SessionMinDistance;
          if (MinDist < TNumericLimits<float>::Max())
            W->UpdateMinDistance(MinDist);
        }
      }
    }
  }

  // 운영자 모니터 위젯 갱신 (1초마다: TrackerStatus + 경과 시간 + 최소 거리)
  if (OperatorMonitorWidget) {
    OperatorMonitorWidget->UpdateTrackerStatus(ConnectedCount, 5);

    if (SessionManager) {
      if (SessionManager->IsTesting())
        OperatorMonitorWidget->UpdateElapsedTime(
            SessionManager->SessionElapsedTime);

      if (SessionManager->CollisionDetector) {
        const float MinDist =
            SessionManager->CollisionDetector->SessionMinDistance;
        if (MinDist < TNumericLimits<float>::Max())
          OperatorMonitorWidget->UpdateMinDistance(MinDist);
      }
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  단축키 바인딩 — 1 / 2 / 3 / 4
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::SetupInputComponent() {
  Super::SetupInputComponent();
  if (!InputComponent)
    return;

  InputComponent->BindKey(EKeys::One,   IE_Pressed, this, &AVTC_OperatorController::Input_One);
  InputComponent->BindKey(EKeys::Two,   IE_Pressed, this, &AVTC_OperatorController::Input_Two);
  InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AVTC_OperatorController::Input_Three);
  InputComponent->BindKey(EKeys::P,     IE_Pressed, this, &AVTC_OperatorController::Input_P);
}

// ─────────────────────────────────────────────────────────────────────────────
//  세션 제어 — BlueprintNativeEvent
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::StartCalibration_Implementation() {
  if (!SessionManager)
    return;
  UVTC_GameInstance *GI = GetGameInstance<UVTC_GameInstance>();
  const FString SubjectID = GI ? GI->SessionConfig.SubjectID : TEXT("Unknown");
  const float Height_cm = GI ? GI->SessionConfig.Height_cm : 170.0f;
  SessionManager->StartSessionWithHeight(SubjectID, Height_cm);
}

void AVTC_OperatorController::StartTest_Implementation() {
  if (SessionManager)
    SessionManager->StartTestingDirectly();
}

void AVTC_OperatorController::StopAndExport_Implementation() {
  if (SessionManager)
    SessionManager->ExportAndEnd();
}

void AVTC_OperatorController::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
  // 운영자 모니터 위젯 뷰포트에서 제거
  if (OperatorMonitorWidget && OperatorMonitorWidget->IsInViewport())
    OperatorMonitorWidget->RemoveFromParent();

  // Delegate 명시적 해제 — Controller 소멸 후 SessionManager/CollisionDetector가
  // 이벤트를 브로드캐스트할 경우 dangling 호출 방지.
  // (AddDynamic은 약참조이지만 GC 전 소멸 순서에 따라 크래시 가능)
  if (SessionManager) {
    SessionManager->OnSessionStateChanged.RemoveDynamic(
        this, &AVTC_OperatorController::OnSessionStateChanged);
    if (SessionManager->CollisionDetector) {
      SessionManager->CollisionDetector->OnDistanceUpdated.RemoveDynamic(
          this, &AVTC_OperatorController::OnDistanceUpdated);
    }
  }

  // 동적으로 스폰한 ReferencePoint를 명시적으로 제거.
  if (SpawnedHipRefPoint) {
    SpawnedHipRefPoint->Destroy();
    SpawnedHipRefPoint = nullptr;
  }

  // 프리셋 ReferencePoint 일괄 제거 (Feature B)
  for (TObjectPtr<AVTC_ReferencePoint> &Ref : SpawnedPresetRefPoints) {
    if (Ref)
      Ref->Destroy();
  }
  SpawnedPresetRefPoints.Empty();

  Super::EndPlay(EndPlayReason);
}

// ─────────────────────────────────────────────────────────────────────────────
//  단축키 핸들러
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::Input_One()   { StartCalibration(); }
void AVTC_OperatorController::Input_Two()   { StartTest(); }
void AVTC_OperatorController::Input_Three() {
  StopAndExport();  // CSV 저장 + 상태 Idle로 전환
  UKismetSystemLibrary::QuitGame(GetWorld(), this, EQuitPreference::Quit, false);
}

void AVTC_OperatorController::Input_P() {
  UVTC_GameInstance *GI = GetGameInstance<UVTC_GameInstance>();
  if (!GI)
    return;

  // 1) INI에서 최신 설정 강제 재로드 (디스크에서 읽기)
  GI->LoadConfigFromINI();

  // 2) 기존 preset ref points 정리 (중복 스폰 방지)
  for (auto &Ref : SpawnedPresetRefPoints) {
    if (Ref)
      Ref->Destroy();
  }
  SpawnedPresetRefPoints.Empty();

  // 3) 모든 Actor에 설정 재적용
  //    TrackerPawn: RunMode, TrackerMesh, SnapWaistTo(VehicleHipPosition)
  //    BodyActor: MountOffset, Sphere 가시성, VehicleHipMarker
  //    CollisionDetector: 임계값
  //    HipRefPoint: 스폰 또는 위치 업데이트
  //    Preset RefPoints: 재스폰
  ApplyGameInstanceConfig();

  // 4) StatusWidget + OperatorMonitor에 피실험자 정보 갱신
  const FVTCSessionConfig &C = GI->SessionConfig;
  if (StatusActor) {
    if (UVTC_StatusWidget *W = StatusActor->GetStatusWidget()) {
      W->UpdateSubjectInfo(C.SubjectID, C.Height_cm);
      W->UpdatePresetInfo(C.bUseVehiclePreset, C.SelectedPresetName);
    }
  }
  if (OperatorMonitorWidget) {
    OperatorMonitorWidget->UpdateSubjectInfo(C.SubjectID, C.Height_cm);
    OperatorMonitorWidget->UpdatePresetInfo(C.bUseVehiclePreset,
                                            C.SelectedPresetName);
  }

  UE_LOG(LogTemp, Log,
         TEXT("[VTC] P key: Config reloaded from INI and applied. Subject=%s "
              "Hip=%s"),
         *C.SubjectID, *C.VehicleHipPosition.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
//  세션 상태 변경 → StatusWidget + OperatorMonitorWidget 갱신
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::OnSessionStateChanged(EVTCSessionState OldState,
                                                    EVTCSessionState NewState) {
  int32 ConnectedCount = 0;
  if (AVTC_TrackerPawn *TP = Cast<AVTC_TrackerPawn>(GetPawn()))
    ConnectedCount = TP->GetActiveTrackerCount();

  // 캘리브레이션 결과: Calibrating → Testing = 성공 / Calibrating → Idle = 실패
  const bool bCalibSuccess = (OldState == EVTCSessionState::Calibrating &&
                               NewState == EVTCSessionState::Testing);
  const bool bCalibFailed  = (OldState == EVTCSessionState::Calibrating &&
                               NewState == EVTCSessionState::Idle);

  // 3D StatusWidget 갱신 (한 번만 GetStatusWidget 호출)
  if (StatusActor) {
    if (UVTC_StatusWidget *W = StatusActor->GetStatusWidget()) {
      W->UpdateState(NewState);
      W->UpdateTrackerStatus(ConnectedCount, 5);
      if (bCalibSuccess) W->UpdateCalibrationResult(true,  TEXT("OK"));
      if (bCalibFailed)  W->UpdateCalibrationResult(false, TEXT("Check trackers"));
      if (NewState == EVTCSessionState::Testing) {
        W->ClearDistanceList();
        W->UpdateElapsedTime(0.f);
      }
    }
  }

  // 운영자 모니터 위젯 갱신
  if (OperatorMonitorWidget) {
    OperatorMonitorWidget->UpdateState(NewState);
    OperatorMonitorWidget->UpdateTrackerStatus(ConnectedCount, 5);
    if (NewState == EVTCSessionState::Testing) {
      OperatorMonitorWidget->ClearDistanceList();
      OperatorMonitorWidget->UpdateElapsedTime(0.f);
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  거리 측정 결과 → OperatorMonitorWidget Row 갱신 (30Hz)
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::OnDistanceUpdated(
    const FVTCDistanceResult &Result) {
  if (OperatorMonitorWidget)
    OperatorMonitorWidget->UpdateDistanceRow(Result);

  // 3D StatusWidget에도 동일한 거리 Row 갱신 (30Hz)
  if (StatusActor) {
    if (UVTC_StatusWidget *W = StatusActor->GetStatusWidget())
      W->UpdateDistanceRow(Result);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  GameInstance → 각 Actor에 설정 일괄 적용
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::ApplyGameInstanceConfig() {
  UVTC_GameInstance *GI = GetGameInstance<UVTC_GameInstance>();
  if (!GI)
    return;
  const FVTCSessionConfig &C = GI->SessionConfig;

  // ── TrackerPawn: 시뮬레이션 모드 + 트래커 메시 가시성 ───────────────────
  if (AVTC_TrackerPawn *TP = Cast<AVTC_TrackerPawn>(GetPawn())) {
    TP->bSimulationMode = (C.RunMode == EVTCRunMode::Simulation);
    TP->SetTrackerMeshVisible(C.bShowTrackerMesh);

    // VehicleHipPosition이 설정된 경우 Hip 위치로 Waist 스냅.
    // VR 모드에서는 트래커가 처음 프레임에 비활성 상태일 수 있으므로
    // SnapWaistToWithRetry로 최대 10초(0.5s × 20회) 재시도한다.
    if (!C.VehicleHipPosition.IsNearlyZero())
      TP->SnapWaistToWithRetry(C.VehicleHipPosition);
  }

  // ── BodyActor: Mount Offset + Sphere 가시성 적용 ───────────────────────
  for (TActorIterator<AVTC_BodyActor> It(GetWorld()); It; ++It) {
    (*It)->ApplySessionConfig(C);
    break;
  }

  // ── CollisionDetector: 임계값 적용 (Feature A) ──────────────────────────
  if (SessionManager && SessionManager->CollisionDetector) {
    SessionManager->CollisionDetector->WarningThreshold = C.WarningThreshold_cm;
    SessionManager->CollisionDetector->CollisionThreshold =
        C.CollisionThreshold_cm;
    UE_LOG(
        LogTemp, Log,
        TEXT("[VTC] Thresholds applied — Warning: %.1f cm, Collision: %.1f cm"),
        C.WarningThreshold_cm, C.CollisionThreshold_cm);
  }

  // ── VehicleHipPosition → 참조용 마커 스폰 (bCollisionDisabled = true) ──
  // bCollisionDisabled = true이므로 거리 라인/수치만 표시되고
  // Warning/Collision 판정 및 OnWarningLevelChanged는 발생하지 않는다.
  if (!C.VehicleHipPosition.IsNearlyZero()) {
    const bool bFirstSpawn = !SpawnedHipRefPoint;

    if (bFirstSpawn) {
      FActorSpawnParameters Params;
      Params.Name = TEXT("VTC_HipRefPoint_Dynamic");
      Params.SpawnCollisionHandlingOverride =
          ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

      SpawnedHipRefPoint = GetWorld()->SpawnActor<AVTC_ReferencePoint>(
          AVTC_ReferencePoint::StaticClass(), C.VehicleHipPosition,
          FRotator::ZeroRotator, Params);
    }

    if (SpawnedHipRefPoint) {
      SpawnedHipRefPoint->SetActorLocation(C.VehicleHipPosition);
      SpawnedHipRefPoint->PartName = TEXT("Vehicle_Hip");
      SpawnedHipRefPoint->bCollisionDisabled = true;  // 라인만 표시, 경고 없음
      SpawnedHipRefPoint->RelevantBodyParts.Empty();
      SpawnedHipRefPoint->RelevantBodyParts.Add(
          EVTCTrackerRole::Waist); // Waist ↔ Hip 라인 측정
      SpawnedHipRefPoint->MarkerColor =
          FLinearColor(0.0f, 0.7f, 1.0f, 1.0f); // 시안색으로 구분

      // 처음 스폰될 때만 CollisionDetector에 등록.
      // 이미 존재하는 경우는 위치 업데이트만으로 충분하다.
      if (bFirstSpawn) {
        if (SessionManager && SessionManager->CollisionDetector) {
          SessionManager->CollisionDetector->ReferencePoints.AddUnique(
              SpawnedHipRefPoint);
          UE_LOG(LogTemp, Log,
                 TEXT("[VTC] VehicleHipPosition ReferencePoint 등록: %s"),
                 *C.VehicleHipPosition.ToString());
        } else {
          UE_LOG(LogTemp, Warning,
                 TEXT("[VTC] VehicleHipPosition: CollisionDetector 없음 — 등록 "
                      "실패."
                      " SessionManager를 먼저 탐색했는지 확인하세요."));
        }
      }
    }
  }

  // ── 차종 프리셋 → ReferencePoint 추가 스폰 (Feature B) ──────────────────
  if (C.bUseVehiclePreset && !C.LoadedPresetJson.IsEmpty() && SessionManager &&
      SessionManager->CollisionDetector) {
    FVTCVehiclePreset Preset;
    if (UVTC_VehiclePresetLibrary::JsonStringToPreset(C.LoadedPresetJson,
                                                      Preset)) {
      for (const FVTCPresetRefPoint &PRef : Preset.ReferencePoints) {
        // Vehicle_Hip는 위에서 이미 처리했으므로 중복 방지
        if (PRef.PartName == TEXT("Vehicle_Hip"))
          continue;

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        if (AVTC_ReferencePoint *NewRef =
                GetWorld()->SpawnActor<AVTC_ReferencePoint>(
                    AVTC_ReferencePoint::StaticClass(), PRef.Location,
                    FRotator::ZeroRotator, Params)) {
          NewRef->PartName = PRef.PartName;
          NewRef->RelevantBodyParts = PRef.RelevantBodyParts;
          SessionManager->CollisionDetector->ReferencePoints.AddUnique(NewRef);
          SpawnedPresetRefPoints.Add(NewRef);
          UE_LOG(LogTemp, Log,
                 TEXT("[VTC] Preset ReferencePoint 스폰: %s @ %s"),
                 *PRef.PartName, *PRef.Location.ToString());
        }
      }
      UE_LOG(LogTemp, Log, TEXT("[VTC] Preset '%s' 적용 완료 (%d ref points)"),
             *Preset.PresetName, Preset.ReferencePoints.Num());
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  자동 탐색
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::AutoFindSessionManager() {
  for (TActorIterator<AVTC_SessionManager> It(GetWorld()); It; ++It) {
    SessionManager = *It;
    UE_LOG(LogTemp, Log, TEXT("[VTC] OperatorController: SessionManager → %s"),
           *SessionManager->GetName());
    return;
  }
  UE_LOG(LogTemp, Warning,
         TEXT("[VTC] OperatorController: SessionManager 없음."));
}

void AVTC_OperatorController::AutoFindStatusActor() {
  for (TActorIterator<AVTC_StatusActor> It(GetWorld()); It; ++It) {
    StatusActor = *It;
    UE_LOG(LogTemp, Log, TEXT("[VTC] OperatorController: StatusActor → %s"),
           *StatusActor->GetName());
    return;
  }
  UE_LOG(LogTemp, Warning, TEXT("[VTC] OperatorController: StatusActor 없음."));
}

void AVTC_OperatorController::AutoFindOperatorViewActor() {
  if (OperatorViewActor)
    return; // 이미 수동으로 연결된 경우

  for (TActorIterator<AVTC_OperatorViewActor> It(GetWorld()); It; ++It) {
    OperatorViewActor = *It;
    UE_LOG(LogTemp, Log,
           TEXT("[VTC] OperatorController: OperatorViewActor → %s"),
           *OperatorViewActor->GetName());
    return;
  }
  // 없으면 경고만 — 필수 컴포넌트가 아님
  UE_LOG(LogTemp, Log,
         TEXT("[VTC] OperatorController: OperatorViewActor 없음 (Spectator "
              "Screen 비활성)."));
}
