// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Controller/VTC_OperatorController.h"
#include "UI/VTC_SubjectInfoWidget.h"
#include "Data/VTC_SessionManager.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"   // TActorIterator

// ─────────────────────────────────────────────────────────────────────────────
//  BeginPlay — 마우스 커서 활성화, 위젯 생성, SessionManager 탐색
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::BeginPlay()
{
  Super::BeginPlay();

  // ── 1. 데스크탑 마우스 입력 활성화 ────────────────────────────────────────
  //   VR HMD가 연결된 상태에서도 데스크탑 창(컴패니언 윈도우)에
  //   마우스 커서가 표시되고 위젯을 클릭할 수 있게 한다.
  //   VR 피실험자의 움직임은 TrackerPawn이 HMD 트래킹으로 처리하므로
  //   여기서는 게임 이동 입력 캡처 없이 UI 조작만 허용한다.
  bShowMouseCursor = true;

  FInputModeGameAndUI InputMode;
  InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
  InputMode.SetHideCursorDuringCapture(false);
  SetInputMode(InputMode);

  // ── 2. SubjectInfo 위젯 생성 및 뷰포트 추가 ───────────────────────────────
  //   AddToViewport()는 데스크탑 뷰포트(컴패니언 윈도우)에 위젯을 추가한다.
  //   VR HMD에는 이 위젯이 표시되지 않으므로 피실험자의 시야를 방해하지 않는다.
  if (SubjectInfoWidgetClass)
  {
    SubjectInfoWidget = CreateWidget<UVTC_SubjectInfoWidget>(this, SubjectInfoWidgetClass);
    if (SubjectInfoWidget)
    {
      SubjectInfoWidget->AddToViewport();
      // 세션 시작 요청을 이 컨트롤러가 수신하여 SessionManager로 전달
      SubjectInfoWidget->OnSessionStartRequested.AddDynamic(
          this, &AVTC_OperatorController::OnSessionStartRequested);
    }
  }

  // ── 3. SessionManager 자동 탐색 ───────────────────────────────────────────
  if (!SessionManager)
    AutoFindSessionManager();
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
//  세션 제어 — BlueprintNativeEvent (BP에서 Override 가능)
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::StartCalibration_Implementation()
{
  if (!SessionManager) return;
  // SubjectInfo 위젯에서 받은 정보로 세션 시작 (캘리브레이션 포함)
  // 위젯이 없거나 입력이 비어있으면 기본값으로 세션 시작
  if (SubjectInfoWidget && SubjectInfoWidget->IsInputValid())
  {
    SessionManager->StartSessionWithHeight(
        SubjectInfoWidget->GetEnteredSubjectID(),
        SubjectInfoWidget->GetEnteredHeight());
  }
  else
  {
    SessionManager->StartSession(TEXT("Operator_Default"));
  }
}

void AVTC_OperatorController::StartTest_Implementation()
{
  if (SessionManager)
    SessionManager->StartTestingDirectly();
}

void AVTC_OperatorController::StopAndExport_Implementation()
{
  if (SessionManager)
    SessionManager->ExportAndEnd();
}

// ─────────────────────────────────────────────────────────────────────────────
//  단축키 핸들러
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::Input_F1() { StartCalibration(); }
void AVTC_OperatorController::Input_F2() { StartTest(); }
void AVTC_OperatorController::Input_F3() { StopAndExport(); }

// ─────────────────────────────────────────────────────────────────────────────
//  SessionManager 자동 탐색
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::AutoFindSessionManager()
{
  for (TActorIterator<AVTC_SessionManager> It(GetWorld()); It; ++It)
  {
    SessionManager = *It;
    UE_LOG(LogTemp, Log, TEXT("[VTC] OperatorController: SessionManager 자동 연결 → %s"),
           *SessionManager->GetName());
    return;
  }
  UE_LOG(LogTemp, Warning,
         TEXT("[VTC] OperatorController: 레벨에서 SessionManager를 찾지 못했습니다."));
}

// ─────────────────────────────────────────────────────────────────────────────
//  위젯 → SessionManager 브릿지
// ─────────────────────────────────────────────────────────────────────────────
void AVTC_OperatorController::OnSessionStartRequested(const FString& SubjectID,
                                                       float Height_cm)
{
  if (!SessionManager)
  {
    UE_LOG(LogTemp, Warning,
           TEXT("[VTC] OperatorController: SessionManager가 없어 세션을 시작할 수 없습니다."));
    return;
  }
  SessionManager->StartSessionWithHeight(SubjectID, Height_cm);
  // 세션 시작 후 위젯 숨기기 (선택사항 — BP에서 OnSessionStateChanged로 제어해도 됨)
  HideSubjectInfoWidget();
}
