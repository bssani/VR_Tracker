// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Controller/VTC_OperatorController.h"
#include "UI/VTC_SubjectInfoWidget.h"
#include "UI/VTC_StatusWidget.h"
#include "World/VTC_StatusActor.h"
#include "Data/VTC_SessionManager.h"
#include "Body/VTC_BodyActor.h"
#include "Pawn/VTC_TrackerPawn.h"
#include "VTC_GameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"   // TActorIterator

// ─────────────────────────────────────────────────────────────────────────────
//  BeginPlay — GameInstance 설정 적용 → SessionManager/StatusActor 탐색
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::BeginPlay()
{
  Super::BeginPlay();

  // Level 2에서는 마우스 커서 불필요 (키만 사용).
  // 단, 예외 상황을 위해 GameAndUI 모드는 유지.
  bShowMouseCursor = false;
  FInputModeGameOnly InputMode;
  SetInputMode(InputMode);

  // ── GameInstance에서 설정 읽어 각 Actor에 적용 ────────────────────────────
  ApplyGameInstanceConfig();

  // ── 레벨에서 필요한 Actor 자동 탐색 ──────────────────────────────────────
  if (!SessionManager) AutoFindSessionManager();
  AutoFindStatusActor();

  // ── SessionManager 상태 변경 시 StatusWidget 갱신 ─────────────────────────
  if (SessionManager)
  {
    SessionManager->OnSessionStateChanged.AddDynamic(
        this, &AVTC_OperatorController::OnSessionStateChanged);
  }

  // 초기 상태 표시
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
//  단축키 바인딩 — F1 / F2 / F3
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::SetupInputComponent()
{
  Super::SetupInputComponent();
  if (!InputComponent) return;

  InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AVTC_OperatorController::Input_F1);
  InputComponent->BindKey(EKeys::F2, IE_Pressed, this, &AVTC_OperatorController::Input_F2);
  InputComponent->BindKey(EKeys::F3, IE_Pressed, this, &AVTC_OperatorController::Input_F3);
}

// ─────────────────────────────────────────────────────────────────────────────
//  위젯 표시 / 숨기기 (Level 2에서는 거의 사용 안 함)
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
  const FString SubjectID  = GI ? GI->SessionConfig.SubjectID  : TEXT("Unknown");
  const float   Height_cm  = GI ? GI->SessionConfig.Height_cm  : 170.0f;
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

// ─────────────────────────────────────────────────────────────────────────────
//  단축키 핸들러
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::Input_F1() { StartCalibration(); }
void AVTC_OperatorController::Input_F2() { StartTest(); }
void AVTC_OperatorController::Input_F3() { StopAndExport(); }

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

    // 트래커 연결 상태도 함께 갱신
    if (APawn* P = GetPawn())
      if (AVTC_TrackerPawn* TP = Cast<AVTC_TrackerPawn>(P))
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

  // TrackerPawn: 시뮬레이션 모드 + 트래커 메시 가시성
  if (APawn* P = GetPawn())
  {
    if (AVTC_TrackerPawn* TP = Cast<AVTC_TrackerPawn>(P))
    {
      TP->bSimulationMode = (C.RunMode == EVTCRunMode::Simulation);
      TP->SetTrackerMeshVisible(C.bShowTrackerMesh);
    }
  }

  // BodyActor: Mount Offset + Hip Reference + Sphere 가시성
  for (TActorIterator<AVTC_BodyActor> It(GetWorld()); It; ++It)
  {
    (*It)->ApplySessionConfig(C);
    break;
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
