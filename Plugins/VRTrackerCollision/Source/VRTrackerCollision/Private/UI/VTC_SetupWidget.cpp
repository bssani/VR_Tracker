// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "UI/VTC_SetupWidget.h"
#include "VTC_GameInstance.h"
#include "VTC_VehiclePreset.h"
#include "Components/EditableTextBox.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"

void UVTC_SetupWidget::NativeConstruct()
{
  Super::NativeConstruct();

  // 버튼 바인딩
  if (Btn_SaveConfig)
    Btn_SaveConfig->OnClicked.AddDynamic(this, &UVTC_SetupWidget::OnSaveConfigClicked);
  if (Btn_LoadConfig)
    Btn_LoadConfig->OnClicked.AddDynamic(this, &UVTC_SetupWidget::OnLoadConfigClicked);
  if (Btn_StartSession)
    Btn_StartSession->OnClicked.AddDynamic(this, &UVTC_SetupWidget::OnStartSessionClicked);

  // 모드 Toggle 기본값: VR
  if (Toggle_VRMode) Toggle_VRMode->SetIsChecked(true);

  // 슬라이더 바인딩 (Feature A)
  if (Slider_Warning)
    Slider_Warning->OnValueChanged.AddDynamic(this, &UVTC_SetupWidget::OnWarningSliderChanged);
  if (Slider_Collision)
    Slider_Collision->OnValueChanged.AddDynamic(this, &UVTC_SetupWidget::OnCollisionSliderChanged);

  // 프리셋 바인딩 (Feature B)
  if (Btn_SavePreset)
    Btn_SavePreset->OnClicked.AddDynamic(this, &UVTC_SetupWidget::OnSavePresetClicked);
  if (Combo_VehiclePreset)
    Combo_VehiclePreset->OnSelectionChanged.AddDynamic(
        this, &UVTC_SetupWidget::OnPresetSelectionChanged);

  // 시작 시 저장된 Config 자동 로드
  if (UVTC_GameInstance* GI = GetGameInstance<UVTC_GameInstance>())
  {
    GI->LoadConfigFromINI();
    PopulateFromConfig(GI->SessionConfig);
  }

  // 프리셋 목록 초기화
  RefreshPresetComboBox();
}

// ─────────────────────────────────────────────────────────────────────────────
//  위젯 → Config
// ─────────────────────────────────────────────────────────────────────────────
FVTCSessionConfig UVTC_SetupWidget::BuildConfigFromInputs() const
{
  FVTCSessionConfig C;

  // 피실험자
  if (TB_SubjectID) C.SubjectID  = TB_SubjectID->GetText().ToString().TrimStartAndEnd();
  C.Height_cm = ParseFloat(TB_Height, 170.0f);

  // 모드 (Toggle: Checked=VR, Unchecked=Simulation)
  C.RunMode = (Toggle_VRMode && Toggle_VRMode->IsChecked())
              ? EVTCRunMode::VR
              : EVTCRunMode::Simulation;

  // Mount Offsets
  C.MountOffset_Waist     = ParseVector(TB_Offset_Waist_X,  TB_Offset_Waist_Y,  TB_Offset_Waist_Z);
  C.MountOffset_LeftKnee  = ParseVector(TB_Offset_LKnee_X,  TB_Offset_LKnee_Y,  TB_Offset_LKnee_Z);
  C.MountOffset_RightKnee = ParseVector(TB_Offset_RKnee_X,  TB_Offset_RKnee_Y,  TB_Offset_RKnee_Z);
  C.MountOffset_LeftFoot  = ParseVector(TB_Offset_LFoot_X,  TB_Offset_LFoot_Y,  TB_Offset_LFoot_Z);
  C.MountOffset_RightFoot = ParseVector(TB_Offset_RFoot_X,  TB_Offset_RFoot_Y,  TB_Offset_RFoot_Z);

  // Vehicle Hip Reference
  C.VehicleHipPosition = ParseVector(TB_HipRef_X, TB_HipRef_Y, TB_HipRef_Z);

  // 임계값 슬라이더 (Feature A)
  if (Slider_Warning)
    C.WarningThreshold_cm = Slider_Warning->GetValue();
  if (Slider_Collision)
    C.CollisionThreshold_cm = Slider_Collision->GetValue();

  // 차종 프리셋 (Feature B)
  if (Combo_VehiclePreset)
  {
    const FString Selected = Combo_VehiclePreset->GetSelectedOption();
    if (!Selected.IsEmpty() && Selected != TEXT("(None)"))
    {
      C.bUseVehiclePreset = true;
      C.SelectedPresetName = Selected;
      FVTCVehiclePreset Preset;
      if (UVTC_VehiclePresetLibrary::LoadPreset(Selected, Preset))
      {
        C.LoadedPresetJson = UVTC_VehiclePresetLibrary::PresetToJsonString(Preset);
      }
    }
  }

  // 가시성
  C.bShowCollisionSpheres = CB_ShowCollisionSpheres && CB_ShowCollisionSpheres->IsChecked();
  C.bShowTrackerMesh      = CB_ShowTrackerMesh      && CB_ShowTrackerMesh->IsChecked();

  return C;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Config → 위젯
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_SetupWidget::PopulateFromConfig(const FVTCSessionConfig& C)
{
  if (TB_SubjectID) TB_SubjectID->SetText(FText::FromString(C.SubjectID));
  SetFloat(TB_Height, C.Height_cm);

  if (Toggle_VRMode) Toggle_VRMode->SetIsChecked(C.RunMode == EVTCRunMode::VR);

  SetVector(TB_Offset_Waist_X,  TB_Offset_Waist_Y,  TB_Offset_Waist_Z,  C.MountOffset_Waist);
  SetVector(TB_Offset_LKnee_X,  TB_Offset_LKnee_Y,  TB_Offset_LKnee_Z,  C.MountOffset_LeftKnee);
  SetVector(TB_Offset_RKnee_X,  TB_Offset_RKnee_Y,  TB_Offset_RKnee_Z,  C.MountOffset_RightKnee);
  SetVector(TB_Offset_LFoot_X,  TB_Offset_LFoot_Y,  TB_Offset_LFoot_Z,  C.MountOffset_LeftFoot);
  SetVector(TB_Offset_RFoot_X,  TB_Offset_RFoot_Y,  TB_Offset_RFoot_Z,  C.MountOffset_RightFoot);
  SetVector(TB_HipRef_X,        TB_HipRef_Y,        TB_HipRef_Z,        C.VehicleHipPosition);

  // 임계값 슬라이더 (Feature A)
  if (Slider_Warning)
  {
    Slider_Warning->SetValue(C.WarningThreshold_cm);
    OnWarningSliderChanged(C.WarningThreshold_cm);
  }
  if (Slider_Collision)
  {
    Slider_Collision->SetValue(C.CollisionThreshold_cm);
    OnCollisionSliderChanged(C.CollisionThreshold_cm);
  }

  // 차종 프리셋 (Feature B)
  if (Combo_VehiclePreset && !C.SelectedPresetName.IsEmpty())
    Combo_VehiclePreset->SetSelectedOption(C.SelectedPresetName);

  if (CB_ShowCollisionSpheres) CB_ShowCollisionSpheres->SetIsChecked(C.bShowCollisionSpheres);
  if (CB_ShowTrackerMesh)      CB_ShowTrackerMesh->SetIsChecked(C.bShowTrackerMesh);
}

bool UVTC_SetupWidget::IsInputValid() const
{
  const FString ID = TB_SubjectID ? TB_SubjectID->GetText().ToString().TrimStartAndEnd() : TEXT("");
  return !ID.IsEmpty() && ParseFloat(TB_Height, 0.0f) > 0.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
//  버튼 핸들러
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_SetupWidget::OnSaveConfigClicked()
{
  if (UVTC_GameInstance* GI = GetGameInstance<UVTC_GameInstance>())
  {
    GI->SessionConfig = BuildConfigFromInputs();
    GI->SaveConfigToINI();
    UE_LOG(LogTemp, Log, TEXT("[VTC] Setup: Config saved."));
  }
}

void UVTC_SetupWidget::OnLoadConfigClicked()
{
  if (UVTC_GameInstance* GI = GetGameInstance<UVTC_GameInstance>())
  {
    GI->LoadConfigFromINI();
    PopulateFromConfig(GI->SessionConfig);
    UE_LOG(LogTemp, Log, TEXT("[VTC] Setup: Config loaded."));
  }
}

void UVTC_SetupWidget::OnStartSessionClicked()
{
  if (!IsInputValid())
  {
    UE_LOG(LogTemp, Warning, TEXT("[VTC] Setup: SubjectID 또는 Height가 유효하지 않습니다."));
    return;
  }
  UVTC_GameInstance* GI = GetGameInstance<UVTC_GameInstance>();
  if (!GI) return;

  GI->SessionConfig = BuildConfigFromInputs();
  GI->SaveConfigToINI();   // 시작 전에 항상 저장
  GI->OpenTestLevel();
}

// ─────────────────────────────────────────────────────────────────────────────
//  파싱 / 설정 헬퍼
// ─────────────────────────────────────────────────────────────────────────────
float UVTC_SetupWidget::ParseFloat(const UEditableTextBox* TB, float Default)
{
  if (!TB) return Default;
  float V = Default;
  return LexTryParseString(V, *TB->GetText().ToString().TrimStartAndEnd()) ? V : Default;
}

void UVTC_SetupWidget::SetFloat(UEditableTextBox* TB, float Value)
{
  if (TB) TB->SetText(FText::FromString(FString::SanitizeFloat(Value)));
}

FVector UVTC_SetupWidget::ParseVector(const UEditableTextBox* TX,
                                       const UEditableTextBox* TY,
                                       const UEditableTextBox* TZ)
{
  return FVector(ParseFloat(TX), ParseFloat(TY), ParseFloat(TZ));
}

void UVTC_SetupWidget::SetVector(UEditableTextBox* TX, UEditableTextBox* TY,
                                  UEditableTextBox* TZ, const FVector& V)
{
  SetFloat(TX, V.X);
  SetFloat(TY, V.Y);
  SetFloat(TZ, V.Z);
}

// ─────────────────────────────────────────────────────────────────────────────
//  슬라이더 핸들러 (Feature A)
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_SetupWidget::OnWarningSliderChanged(float Value)
{
  if (Txt_WarningVal)
    Txt_WarningVal->SetText(FText::FromString(
        FString::Printf(TEXT("%.1f cm"), Value)));
}

void UVTC_SetupWidget::OnCollisionSliderChanged(float Value)
{
  if (Txt_CollisionVal)
    Txt_CollisionVal->SetText(FText::FromString(
        FString::Printf(TEXT("%.1f cm"), Value)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  프리셋 핸들러 (Feature B)
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_SetupWidget::OnSavePresetClicked()
{
  // 현재 설정에서 프리셋을 생성하여 저장
  FVTCVehiclePreset Preset;
  // 프리셋 이름 = 현재 선택된 항목 또는 SubjectID 기반
  if (Combo_VehiclePreset)
  {
    const FString Selected = Combo_VehiclePreset->GetSelectedOption();
    Preset.PresetName = (!Selected.IsEmpty() && Selected != TEXT("(None)"))
        ? Selected
        : (TB_SubjectID ? TB_SubjectID->GetText().ToString().TrimStartAndEnd()
                        : TEXT("Default"));
  }
  else
  {
    Preset.PresetName = TEXT("Default");
  }

  // VehicleHipPosition을 단일 ReferencePoint로 저장
  FVTCPresetRefPoint HipRef;
  HipRef.PartName = TEXT("Vehicle_Hip");
  HipRef.Location = ParseVector(TB_HipRef_X, TB_HipRef_Y, TB_HipRef_Z);
  HipRef.RelevantBodyParts = { EVTCTrackerRole::Waist };
  Preset.ReferencePoints.Add(HipRef);

  if (UVTC_VehiclePresetLibrary::SavePreset(Preset))
  {
    RefreshPresetComboBox();
    if (Combo_VehiclePreset)
      Combo_VehiclePreset->SetSelectedOption(Preset.PresetName);
    UE_LOG(LogTemp, Log, TEXT("[VTC] Preset saved: %s"), *Preset.PresetName);
  }
}

void UVTC_SetupWidget::OnPresetSelectionChanged(FString SelectedItem,
                                                  ESelectInfo::Type SelectionType)
{
  if (SelectedItem.IsEmpty() || SelectedItem == TEXT("(None)")) return;

  FVTCVehiclePreset Preset;
  if (UVTC_VehiclePresetLibrary::LoadPreset(SelectedItem, Preset))
  {
    // 프리셋의 첫 번째 Vehicle_Hip ReferencePoint → TB_HipRef 에 반영
    for (const FVTCPresetRefPoint& Ref : Preset.ReferencePoints)
    {
      if (Ref.PartName == TEXT("Vehicle_Hip"))
      {
        SetVector(TB_HipRef_X, TB_HipRef_Y, TB_HipRef_Z, Ref.Location);
        break;
      }
    }
    UE_LOG(LogTemp, Log, TEXT("[VTC] Preset loaded: %s (%d ref points)"),
           *SelectedItem, Preset.ReferencePoints.Num());
  }
}

void UVTC_SetupWidget::RefreshPresetComboBox()
{
  if (!Combo_VehiclePreset) return;

  Combo_VehiclePreset->ClearOptions();
  Combo_VehiclePreset->AddOption(TEXT("(None)"));

  const TArray<FString> Names = UVTC_VehiclePresetLibrary::GetAvailablePresetNames();
  for (const FString& Name : Names)
  {
    Combo_VehiclePreset->AddOption(Name);
  }
}
