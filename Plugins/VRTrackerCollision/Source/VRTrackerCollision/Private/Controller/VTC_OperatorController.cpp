// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Controller/VTC_OperatorController.h"
#include "UI/VTC_SubjectInfoWidget.h"
#include "UI/VTC_StatusWidget.h"
#include "World/VTC_StatusActor.h"
#include "Data/VTC_SessionManager.h"
#include "Body/VTC_BodyActor.h"
#include "Pawn/VTC_TrackerPawn.h"
#include "Collision/VTC_CollisionDetector.h"
#include "Vehicle/VTC_ReferencePoint.h"
#include "VTC_GameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"   // TActorIterator

AVTC_OperatorController::AVTC_OperatorController()
{
  // Tick 활성화 — TrackerStatus 주기적 갱신에 필요
  PrimaryActorTick.bCanEverTick = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  BeginPlay — 탐색 + Delegate 바인딩 + 초기 상태 표시
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::BeginPlay()
{
  Super::BeginPlay();

  // Level 2에서는 마우스 커서 불필요 (키만 사용).
  bShowMouseCursor = false;
  FInputModeGameOnly InputMode;
  SetInputMode(InputMode);

  // ── 레벨에서 필요한 Actor 자동 탐색 ──────────────────────────────────────
  if (!SessionManager) AutoFindSessionManager();
  AutoFindStatusActor();

  // ── SessionManager 상태 변경 시 StatusWidget 갱신 ─────────────────────────
  if (SessionManager)
  {
    SessionManager->OnSessionStateChanged.AddDynamic(
        this, &AVTC_OperatorController::OnSessionStateChanged);
  }

  // ── GameInstance → 각 Actor에 설정 적용 ──────────────────────────────────
  // BeginPlay 시점에 Pawn이 이미 possess되어 있으면 즉시 적용.
  // Pawn이 없으면 OnPossess()에서 처리된다.
  if (GetPawn())
  {
    ApplyGameInstanceConfig();
    bConfigApplied = true;
  }

  // ── 초기 상태 StatusWidget 표시 ───────────────────────────────────────────
  if (StatusActor)
  {
    if (UVTC_StatusWidget* W = StatusActor->GetStatusWidget())
    {
      W->UpdateState(EVTCSessionState::Idle);
      if (UVTC_GameInstance* GI = GetGameInstance<UVTC_GameInstance>())
        W->UpdateSubjectInfo(GI->SessionConfig.SubjectID, GI->SessionConfig.Height_cm);
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  OnPossess — Pawn이 BeginPlay 이후에 possess될 때 설정 적용
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::OnPossess(APawn* InPawn)
{
  Super::OnPossess(InPawn);

  if (!bConfigApplied)
  {
    ApplyGameInstanceConfig();
    bConfigApplied = true;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tick — TrackerStatus 1초마다 갱신
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  TrackerStatusTimer += DeltaTime;
  if (TrackerStatusTimer < TrackerStatusInterval) return;
  TrackerStatusTimer = 0.0f;

  if (!StatusActor) return;
  UVTC_StatusWidget* W = StatusActor->GetStatusWidget();
  if (!W) return;

  if (AVTC_TrackerPawn* TP = Cast<AVTC_TrackerPawn>(GetPawn()))
    W->UpdateTrackerStatus(TP->GetActiveTrackerCount(), 5);
}

// ─────────────────────────────────────────────────────────────────────────────
//  단축키 바인딩 — F1 / F2 / F3 / Escape
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::SetupInputComponent()
{
  Super::SetupInputComponent();
  if (!InputComponent) return;

  InputComponent->BindKey(EKeys::F1,     IE_Pressed, this, &AVTC_OperatorController::Input_F1);
  InputComponent->BindKey(EKeys::F2,     IE_Pressed, this, &AVTC_OperatorController::Input_F2);
  InputComponent->BindKey(EKeys::F3,     IE_Pressed, this, &AVTC_OperatorController::Input_F3);
  InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AVTC_OperatorController::Input_Escape);
}

// ─────────────────────────────────────────────────────────────────────────────
//  위젯 표시 / 숨기기
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::ShowSubjectInfoWidget()
{
  if (SubjectInfoWidget)
    SubjectInfoWidget->SetVisibility(ESlateVisibility::Visible);
}

void AVTC_OperatorController::HideSubjectInfoWidget()
{
  if (SubjectInfoWidget)
    SubjectInfoWidget->SetVisibility(ESlateVisibility::Collapsed);
}

// ─────────────────────────────────────────────────────────────────────────────
//  세션 제어 — BlueprintNativeEvent
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::StartCalibration_Implementation()
{
  if (!SessionManager) return;
  UVTC_GameInstance* GI = GetGameInstance<UVTC_GameInstance>();
  const FString SubjectID = GI ? GI->SessionConfig.SubjectID : TEXT("Unknown");
  const float   Height_cm = GI ? GI->SessionConfig.Height_cm : 170.0f;
  SessionManager->StartSessionWithHeight(SubjectID, Height_cm);
}

void AVTC_OperatorController::StartTest_Implementation()
{
  if (SessionManager) SessionManager->StartTestingDirectly();
}

void AVTC_OperatorController::StopAndExport_Implementation()
{
  if (SessionManager) SessionManager->ExportAndEnd();
}

void AVTC_OperatorController::ReturnToSetupLevel()
{
  if (UVTC_GameInstance* GI = GetGameInstance<UVTC_GameInstance>())
    GI->OpenSetupLevel();
}

// ─────────────────────────────────────────────────────────────────────────────
//  단축키 핸들러
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::Input_F1()     { StartCalibration(); }
void AVTC_OperatorController::Input_F2()     { StartTest(); }
void AVTC_OperatorController::Input_F3()     { StopAndExport(); }
void AVTC_OperatorController::Input_Escape() { ReturnToSetupLevel(); }

// ─────────────────────────────────────────────────────────────────────────────
//  세션 상태 변경 → StatusWidget 갱신
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::OnSessionStateChanged(EVTCSessionState OldState,
                                                     EVTCSessionState NewState)
{
  if (!StatusActor) return;
  if (UVTC_StatusWidget* W = StatusActor->GetStatusWidget())
  {
    W->UpdateState(NewState);

    // 상태 변경 시 TrackerStatus도 즉시 갱신
    if (AVTC_TrackerPawn* TP = Cast<AVTC_TrackerPawn>(GetPawn()))
      W->UpdateTrackerStatus(TP->GetActiveTrackerCount(), 5);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  GameInstance → 각 Actor에 설정 일괄 적용
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::ApplyGameInstanceConfig()
{
  UVTC_GameInstance* GI = GetGameInstance<UVTC_GameInstance>();
  if (!GI) return;
  const FVTCSessionConfig& C = GI->SessionConfig;

  // ── TrackerPawn: 시뮬레이션 모드 + 트래커 메시 가시성 ───────────────────
  if (AVTC_TrackerPawn* TP = Cast<AVTC_TrackerPawn>(GetPawn()))
  {
    TP->bSimulationMode = (C.RunMode == EVTCRunMode::Simulation);
    TP->SetTrackerMeshVisible(C.bShowTrackerMesh);
  }

  // ── BodyActor: Mount Offset + Sphere 가시성 적용 ───────────────────────
  for (TActorIterator<AVTC_BodyActor> It(GetWorld()); It; ++It)
  {
    (*It)->ApplySessionConfig(C);
    break;
  }

  // ── VehicleHipPosition → CollisionDetector용 ReferencePoint 동적 스폰 ──
  // VehicleHipPosition이 설정되어 있으면 레벨에 ReferencePoint를 스폰하고
  // SessionManager의 CollisionDetector에 직접 추가한다.
  // (AutoFindReferencePoints가 BeginPlay에서 이미 실행된 이후이므로 수동 등록 필요)
  if (!C.VehicleHipPosition.IsNearlyZero())
  {
    if (!SpawnedHipRefPoint)
    {
      FActorSpawnParameters Params;
      Params.Name = TEXT("VTC_HipRefPoint_Dynamic");
      Params.SpawnCollisionHandlingOverride =
          ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

      SpawnedHipRefPoint = GetWorld()->SpawnActor<AVTC_ReferencePoint>(
          AVTC_ReferencePoint::StaticClass(),
          C.VehicleHipPosition, FRotator::ZeroRotator, Params);
    }

    if (SpawnedHipRefPoint)
    {
      SpawnedHipRefPoint->SetActorLocation(C.VehicleHipPosition);
      SpawnedHipRefPoint->PartName         = TEXT("Vehicle_Hip");
      SpawnedHipRefPoint->RelevantBodyParts = { EVTCTrackerRole::Waist };

      // CollisionDetector에 직접 추가 (중복 방지: AddUnique)
      for (TActorIterator<AVTC_SessionManager> It(GetWorld()); It; ++It)
      {
        if (UVTC_CollisionDetector* CD = (*It)->CollisionDetector)
          CD->ReferencePoints.AddUnique(SpawnedHipRefPoint);
        break;
      }

      UE_LOG(LogTemp, Log, TEXT("[VTC] VehicleHipPosition ReferencePoint 등록: %s"),
             *C.VehicleHipPosition.ToString());
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  자동 탐색
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::AutoFindSessionManager()
{
  for (TActorIterator<AVTC_SessionManager> It(GetWorld()); It; ++It)
  {
    SessionManager = *It;
    UE_LOG(LogTemp, Log, TEXT("[VTC] OperatorController: SessionManager → %s"),
           *SessionManager->GetName());
    return;
  }
  UE_LOG(LogTemp, Warning, TEXT("[VTC] OperatorController: SessionManager 없음."));
}

void AVTC_OperatorController::AutoFindStatusActor()
{
  for (TActorIterator<AVTC_StatusActor> It(GetWorld()); It; ++It)
  {
    StatusActor = *It;
    UE_LOG(LogTemp, Log, TEXT("[VTC] OperatorController: StatusActor → %s"),
           *StatusActor->GetName());
    return;
  }
  UE_LOG(LogTemp, Warning, TEXT("[VTC] OperatorController: StatusActor 없음."));
}

// ─────────────────────────────────────────────────────────────────────────────
//  (Level 2에서는 사용 안 함 — 위젯 버튼 대신 키 사용)
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::OnSessionStartRequested(const FString& SubjectID,
                                                       float Height_cm)
{
  if (!SessionManager) return;
  SessionManager->StartSessionWithHeight(SubjectID, Height_cm);
  HideSubjectInfoWidget();
}
