// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "UI/VTC_OperatorMonitorWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

// ─────────────────────────────────────────────────────────────────────────────
//  세션 상태
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_OperatorMonitorWidget::UpdateState(EVTCSessionState NewState)
{
  if (Txt_State)
    Txt_State->SetText(FText::FromString(GetStateLabel(NewState)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  피실험자 정보
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_OperatorMonitorWidget::UpdateSubjectInfo(const FString& SubjectID, float Height_cm)
{
  if (Txt_SubjectInfo)
    Txt_SubjectInfo->SetText(FText::FromString(
        FString::Printf(TEXT("Subject: %s  |  Height: %.0f cm"), *SubjectID, Height_cm)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  트래커 연결 수
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_OperatorMonitorWidget::UpdateTrackerStatus(int32 ConnectedCount, int32 TotalCount)
{
  if (Txt_TrackerStatus)
    Txt_TrackerStatus->SetText(FText::FromString(
        FString::Printf(TEXT("Trackers: %d / %d"), ConnectedCount, TotalCount)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  경과 시간 (MM:SS)
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_OperatorMonitorWidget::UpdateElapsedTime(float ElapsedSeconds)
{
  if (!Txt_ElapsedTime) return;
  const int32 Minutes = FMath::FloorToInt(ElapsedSeconds / 60.f);
  const int32 Seconds = FMath::FloorToInt(FMath::Fmod(ElapsedSeconds, 60.f));
  Txt_ElapsedTime->SetText(FText::FromString(
      FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  거리 Row 갱신 (Map 재사용 — 30Hz에 ClearChildren 없음)
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_OperatorMonitorWidget::UpdateDistanceRow(const FVTCDistanceResult& Result)
{
  if (!VB_DistanceList) return;

  FLinearColor Color = FLinearColor::Green;
  if (Result.WarningLevel == EVTCWarningLevel::Warning)
    Color = FLinearColor(1.f, 0.9f, 0.f, 1.f);
  else if (Result.WarningLevel == EVTCWarningLevel::Collision)
    Color = FLinearColor::Red;

  const FString RowText = FString::Printf(TEXT("%-12s  %-18s  %5.1f cm"),
      *GetBodyPartLabel(Result.BodyPart), *Result.VehiclePartName, Result.Distance);

  const FString Key = MakeRowKey(Result.BodyPart, Result.VehiclePartName);

  if (TObjectPtr<UTextBlock>* Existing = DistanceRowMap.Find(Key))
  {
    // Row 이미 존재 → 텍스트 + 색상만 갱신
    (*Existing)->SetText(FText::FromString(RowText));
    (*Existing)->SetColorAndOpacity(FSlateColor(Color));
  }
  else
  {
    // 처음 등장하는 (BodyPart, VehiclePart) 조합 → TextBlock 생성 + VB에 추가
    UTextBlock* TB = NewObject<UTextBlock>(this);
    TB->SetText(FText::FromString(RowText));
    TB->SetColorAndOpacity(FSlateColor(Color));
    VB_DistanceList->AddChildToVerticalBox(TB);
    DistanceRowMap.Add(Key, TB);
  }

  // 최솟값 갱신
  UpdateMinDistanceFromMap();
}

void UVTC_OperatorMonitorWidget::ClearDistanceList()
{
  if (VB_DistanceList) VB_DistanceList->ClearChildren();
  DistanceRowMap.Empty();
}

void UVTC_OperatorMonitorWidget::UpdateMinDistance(float MinDist_cm)
{
  if (Txt_MinDistance)
    Txt_MinDistance->SetText(FText::FromString(
        FString::Printf(TEXT("Min: %.1f cm"), MinDist_cm)));
}

FString UVTC_OperatorMonitorWidget::MakeRowKey(EVTCTrackerRole BodyPart,
                                                const FString& VehiclePartName)
{
  return FString::Printf(TEXT("%d_%s"), static_cast<int32>(BodyPart), *VehiclePartName);
}

FString UVTC_OperatorMonitorWidget::GetBodyPartLabel(EVTCTrackerRole Role)
{
  switch (Role)
  {
    case EVTCTrackerRole::Waist:     return TEXT("Waist");
    case EVTCTrackerRole::LeftKnee:  return TEXT("Left Knee");
    case EVTCTrackerRole::RightKnee: return TEXT("Right Knee");
    case EVTCTrackerRole::LeftFoot:  return TEXT("Left Foot");
    case EVTCTrackerRole::RightFoot: return TEXT("Right Foot");
    default:                         return TEXT("Unknown");
  }
}

FString UVTC_OperatorMonitorWidget::GetStateLabel(EVTCSessionState State)
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
