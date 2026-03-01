// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "UI/VTC_StatusWidget.h"
#include "Components/TextBlock.h"

void UVTC_StatusWidget::UpdateState(EVTCSessionState NewState)
{
  if (Txt_State)
    Txt_State->SetText(FText::FromString(GetStateLabel(NewState)));
  if (Txt_Prompt)
    Txt_Prompt->SetText(FText::FromString(GetPromptForState(NewState)));
}

void UVTC_StatusWidget::UpdateSubjectInfo(const FString& SubjectID, float Height_cm)
{
  if (Txt_SubjectInfo)
  {
    const FString Info = FString::Printf(TEXT("Subject: %s  |  Height: %.0f cm"),
                                         *SubjectID, Height_cm);
    Txt_SubjectInfo->SetText(FText::FromString(Info));
  }
}

void UVTC_StatusWidget::UpdateTrackerStatus(int32 ConnectedCount, int32 TotalCount)
{
  if (Txt_TrackerStatus)
  {
    const FString Status = FString::Printf(TEXT("Trackers: %d / %d Connected"),
                                            ConnectedCount, TotalCount);
    Txt_TrackerStatus->SetText(FText::FromString(Status));
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  세션 상태별 텍스트
// ─────────────────────────────────────────────────────────────────────────────
FString UVTC_StatusWidget::GetStateLabel(EVTCSessionState State)
{
  switch (State)
  {
    case EVTCSessionState::Idle:        return TEXT("● IDLE");
    case EVTCSessionState::Calibrating: return TEXT("● CALIBRATING");
    case EVTCSessionState::Testing:     return TEXT("● TESTING");
    case EVTCSessionState::Reviewing:   return TEXT("● REVIEWING");
    default:                            return TEXT("● UNKNOWN");
  }
}

FString UVTC_StatusWidget::GetPromptForState(EVTCSessionState State)
{
  switch (State)
  {
    case EVTCSessionState::Idle:
      return TEXT("F1  —  Start Calibration\nF2  —  Start Test (skip calibration)");
    case EVTCSessionState::Calibrating:
      return TEXT("Stand in T-Pose and hold still.\nCalibration will complete automatically.");
    case EVTCSessionState::Testing:
      return TEXT("F3  —  Stop & Export CSV\nTesting in progress...");
    case EVTCSessionState::Reviewing:
      return TEXT("F3  —  Export CSV & Return to Idle");
    default:
      return TEXT("");
  }
}
