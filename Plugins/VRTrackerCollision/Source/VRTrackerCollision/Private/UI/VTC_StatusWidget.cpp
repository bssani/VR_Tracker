// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "UI/VTC_StatusWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

// ─────────────────────────────────────────────────────────────────────────────
//  세션 상태 + 키 안내
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_StatusWidget::UpdateState(EVTCSessionState NewState) {
  if (Txt_State)
    Txt_State->SetText(FText::FromString(GetStateLabel(NewState)));
  if (Txt_Prompt)
    Txt_Prompt->SetText(FText::FromString(GetPromptForState(NewState)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  피실험자 정보
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_StatusWidget::UpdateSubjectInfo(const FString &SubjectID,
                                          float Height_cm) {
  if (Txt_SubjectInfo) {
    const FString Info = FString::Printf(
        TEXT("Subject: %s  |  Height: %.0f cm"), *SubjectID, Height_cm);
    Txt_SubjectInfo->SetText(FText::FromString(Info));
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  트래커 연결 수
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_StatusWidget::UpdateTrackerStatus(int32 ConnectedCount,
                                            int32 TotalCount) {
  if (Txt_TrackerStatus) {
    const FString Status = FString::Printf(TEXT("Trackers: %d / %d Connected"),
                                           ConnectedCount, TotalCount);
    Txt_TrackerStatus->SetText(FText::FromString(Status));
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  프리셋 정보 — 선택 위젯
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_StatusWidget::UpdatePresetInfo(bool bUsePreset, const FString& PresetName) {
  if (!Txt_PresetInfo) return;
  const FString Text = bUsePreset && !PresetName.IsEmpty()
      ? FString::Printf(TEXT("Preset: %s"), *PresetName)
      : TEXT("Preset: None");
  Txt_PresetInfo->SetText(FText::FromString(Text));
}

// ─────────────────────────────────────────────────────────────────────────────
//  캘리브레이션 결과 — 선택 위젯
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_StatusWidget::UpdateCalibrationResult(bool bSuccess, const FString& Reason) {
  if (!Txt_CalibResult) return;
  if (Reason.IsEmpty()) {
    Txt_CalibResult->SetText(FText::GetEmpty());
    return;
  }
  const FString Text = bSuccess
      ? TEXT("Cal: OK ✓")
      : FString::Printf(TEXT("Cal: FAILED — %s"), *Reason);
  const FLinearColor Color = bSuccess ? FLinearColor::Green : FLinearColor::Red;
  Txt_CalibResult->SetText(FText::FromString(Text));
  Txt_CalibResult->SetColorAndOpacity(FSlateColor(Color));
}

// ─────────────────────────────────────────────────────────────────────────────
//  경과 시간 (MM:SS) — 선택 위젯
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_StatusWidget::UpdateElapsedTime(float ElapsedSeconds) {
  if (!Txt_ElapsedTime) return;
  const int32 Minutes = FMath::FloorToInt(ElapsedSeconds / 60.f);
  const int32 Seconds = FMath::FloorToInt(FMath::Fmod(ElapsedSeconds, 60.f));
  Txt_ElapsedTime->SetText(FText::FromString(
      FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  세션 최소 거리 — 선택 위젯
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_StatusWidget::UpdateMinDistance(float MinDist_cm) {
  if (Txt_MinDistance)
    Txt_MinDistance->SetText(FText::FromString(
        FString::Printf(TEXT("Min: %.1f cm"), MinDist_cm)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Hip ReferencePoint ↔ Waist 실시간 거리 (경고 이벤트 없음)
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_StatusWidget::UpdateHipWaistDistance(float Distance_cm) {
  if (Txt_HipWaistDistance)
    Txt_HipWaistDistance->SetText(FText::FromString(
        FString::Printf(TEXT("Hip\u2194Waist: %.1f cm"), Distance_cm)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  거리 Row 갱신 — OperatorMonitorWidget과 동일한 Map 재사용 방식 (30Hz)
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_StatusWidget::UpdateDistanceRow(const FVTCDistanceResult &Result) {
  if (!VB_DistanceList) return;

  FLinearColor Color = FLinearColor::Green;
  if (Result.WarningLevel == EVTCWarningLevel::Warning)
    Color = FLinearColor(1.f, 0.9f, 0.f, 1.f);
  else if (Result.WarningLevel == EVTCWarningLevel::Collision)
    Color = FLinearColor::Red;

  const FString RowText = FString::Printf(TEXT("%-12s  %-18s  %5.1f cm"),
      *GetBodyPartLabel(Result.BodyPart), *Result.VehiclePartName, Result.Distance);

  const FString Key = MakeRowKey(Result.BodyPart, Result.VehiclePartName);

  if (TObjectPtr<UTextBlock>* Existing = DistanceRowMap.Find(Key)) {
    (*Existing)->SetText(FText::FromString(RowText));
    (*Existing)->SetColorAndOpacity(FSlateColor(Color));
  } else {
    UTextBlock* TB = NewObject<UTextBlock>(this);
    TB->SetText(FText::FromString(RowText));
    TB->SetColorAndOpacity(FSlateColor(Color));
    VB_DistanceList->AddChildToVerticalBox(TB);
    DistanceRowMap.Add(Key, TB);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  거리 목록 초기화
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_StatusWidget::ClearDistanceList() {
  if (VB_DistanceList) VB_DistanceList->ClearChildren();
  DistanceRowMap.Empty();
}

// ─────────────────────────────────────────────────────────────────────────────
//  유틸리티
// ─────────────────────────────────────────────────────────────────────────────
FString UVTC_StatusWidget::MakeRowKey(EVTCTrackerRole BodyPart,
                                      const FString &VehiclePartName) {
  return FString::Printf(TEXT("%d_%s"), static_cast<int32>(BodyPart),
                         *VehiclePartName);
}

FString UVTC_StatusWidget::GetBodyPartLabel(EVTCTrackerRole Role) {
  switch (Role) {
  case EVTCTrackerRole::Waist:     return TEXT("Waist");
  case EVTCTrackerRole::LeftKnee:  return TEXT("Left Knee");
  case EVTCTrackerRole::RightKnee: return TEXT("Right Knee");
  case EVTCTrackerRole::LeftFoot:  return TEXT("Left Foot");
  case EVTCTrackerRole::RightFoot: return TEXT("Right Foot");
  default:                         return TEXT("Unknown");
  }
}

FString UVTC_StatusWidget::GetStateLabel(EVTCSessionState State) {
  switch (State) {
  case EVTCSessionState::Idle:        return TEXT("● IDLE");
  case EVTCSessionState::Calibrating: return TEXT("● CALIBRATING");
  case EVTCSessionState::Testing:     return TEXT("● TESTING");
  case EVTCSessionState::Reviewing:   return TEXT("● REVIEWING");
  default:                            return TEXT("● UNKNOWN");
  }
}

FString UVTC_StatusWidget::GetPromptForState(EVTCSessionState State) {
  switch (State) {
  case EVTCSessionState::Idle:
    return TEXT("1  —  Start Calibration\n2  —  Start Test (skip calibration)");
  case EVTCSessionState::Calibrating:
    return TEXT("Sit in correct position and hold still.\nCalibration will complete automatically.");
  case EVTCSessionState::Testing:
    return TEXT("3  —  Save CSV & Quit\nTesting in progress...");
  case EVTCSessionState::Reviewing:
    return TEXT("3  —  Save CSV & Quit");
  default:
    return TEXT("");
  }
}
