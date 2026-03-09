// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "UI/VTC_ProfileManagerWidget.h"
#include "VTC_ProfileLibrary.h"
#include "VTC_VehiclePreset.h"
#include "Components/EditableTextBox.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

// ─────────────────────────────────────────────────────────────────────────────
//  초기화
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_ProfileManagerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// [탭 1] Vehicle Preset 버튼
	if (Btn_SaveVehiclePreset)
		Btn_SaveVehiclePreset->OnClicked.AddDynamic(this, &UVTC_ProfileManagerWidget::OnSaveVehiclePresetClicked);
	if (Btn_DeleteVehiclePreset)
		Btn_DeleteVehiclePreset->OnClicked.AddDynamic(this, &UVTC_ProfileManagerWidget::OnDeleteVehiclePresetClicked);

	// [탭 2] 프로파일 관리 버튼
	if (Btn_NewProfile)
		Btn_NewProfile->OnClicked.AddDynamic(this, &UVTC_ProfileManagerWidget::OnNewProfileClicked);
	if (Btn_LoadProfile)
		Btn_LoadProfile->OnClicked.AddDynamic(this, &UVTC_ProfileManagerWidget::OnLoadProfileClicked);
	if (Btn_SaveProfile)
		Btn_SaveProfile->OnClicked.AddDynamic(this, &UVTC_ProfileManagerWidget::OnSaveProfileClicked);
	if (Btn_DeleteProfile)
		Btn_DeleteProfile->OnClicked.AddDynamic(this, &UVTC_ProfileManagerWidget::OnDeleteProfileClicked);

	// Active Profile 지정 버튼
	if (Btn_SetAsActive)
		Btn_SetAsActive->OnClicked.AddDynamic(this, &UVTC_ProfileManagerWidget::OnSetAsActiveClicked);

	// 슬라이더
	if (Slider_Warning)
		Slider_Warning->OnValueChanged.AddDynamic(this, &UVTC_ProfileManagerWidget::OnWarningSliderChanged);
	if (Slider_Collision)
		Slider_Collision->OnValueChanged.AddDynamic(this, &UVTC_ProfileManagerWidget::OnCollisionSliderChanged);

	// 차종 프리셋 선택 콤보 (Profile 탭)
	if (Combo_VehiclePresetSelect)
		Combo_VehiclePresetSelect->OnSelectionChanged.AddDynamic(
			this, &UVTC_ProfileManagerWidget::OnPresetSelectionChanged);

	// 초기 기본값
	if (Slider_Warning)  { Slider_Warning->SetValue(10.f);  OnWarningSliderChanged(10.f); }
	if (Slider_Collision) { Slider_Collision->SetValue(3.f); OnCollisionSliderChanged(3.f); }
	if (CB_ShowCollisionSpheres) CB_ShowCollisionSpheres->SetIsChecked(true);
	if (CB_ShowTrackerMesh)      CB_ShowTrackerMesh->SetIsChecked(false);

	// ActiveProfile.txt 읽어서 Txt_ActiveProfile 초기화
	const FString ActiveProfilePath = FPaths::ProjectSavedDir() / TEXT("VTCConfig/ActiveProfile.txt");
	FString ActiveName;
	if (FFileHelper::LoadFileToString(ActiveName, *ActiveProfilePath))
	{
		ActiveName = ActiveName.TrimStartAndEnd();
		if (Txt_ActiveProfile && !ActiveName.IsEmpty())
			Txt_ActiveProfile->SetText(FText::FromString(ActiveName));
	}

	// 목록 갱신
	RefreshProfileComboBox();
	RefreshVehiclePresetComboBox();
	RefreshVehiclePresetListComboBox();
}

// ─────────────────────────────────────────────────────────────────────────────
//  위젯 → Config
// ─────────────────────────────────────────────────────────────────────────────
FVTCSessionConfig UVTC_ProfileManagerWidget::BuildConfigFromInputs() const
{
	FVTCSessionConfig C;

	// 프로파일 이름
	if (TB_ProfileName)
		C.ProfileName = TB_ProfileName->GetText().ToString().TrimStartAndEnd();

	// 피실험자
	if (TB_SubjectID)
		C.SubjectID = TB_SubjectID->GetText().ToString().TrimStartAndEnd();
	C.Height_cm = ParseFloat(TB_Height, 170.0f);

	// Mount Offsets
	C.MountOffset_Waist     = ParseVector(TB_Offset_Waist_X, TB_Offset_Waist_Y, TB_Offset_Waist_Z);
	C.MountOffset_LeftKnee  = ParseVector(TB_Offset_LKnee_X, TB_Offset_LKnee_Y, TB_Offset_LKnee_Z);
	C.MountOffset_RightKnee = ParseVector(TB_Offset_RKnee_X, TB_Offset_RKnee_Y, TB_Offset_RKnee_Z);
	C.MountOffset_LeftFoot  = ParseVector(TB_Offset_LFoot_X, TB_Offset_LFoot_Y, TB_Offset_LFoot_Z);
	C.MountOffset_RightFoot = ParseVector(TB_Offset_RFoot_X, TB_Offset_RFoot_Y, TB_Offset_RFoot_Z);

	// Vehicle Hip Reference (자동입력된 값 또는 수동 입력)
	C.VehicleHipPosition = ParseVector(TB_HipRef_X, TB_HipRef_Y, TB_HipRef_Z);

	// 임계값
	if (Slider_Warning)   C.WarningThreshold_cm   = Slider_Warning->GetValue();
	if (Slider_Collision) C.CollisionThreshold_cm = Slider_Collision->GetValue();

	// 차종 프리셋
	if (Combo_VehiclePresetSelect)
	{
		const FString Selected = Combo_VehiclePresetSelect->GetSelectedOption();
		if (!Selected.IsEmpty() && Selected != TEXT("(None)"))
		{
			C.bUseVehiclePreset  = true;
			C.SelectedPresetName = Selected;
			FVTCVehiclePreset Preset;
			if (UVTC_VehiclePresetLibrary::LoadPreset(Selected, Preset))
				C.LoadedPresetJson = UVTC_VehiclePresetLibrary::PresetToJsonString(Preset);
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
void UVTC_ProfileManagerWidget::PopulateFromConfig(const FVTCSessionConfig& C)
{
	if (TB_ProfileName) TB_ProfileName->SetText(FText::FromString(C.ProfileName));
	if (TB_SubjectID)   TB_SubjectID->SetText(FText::FromString(C.SubjectID));
	SetFloat(TB_Height, C.Height_cm);

	SetVector(TB_Offset_Waist_X, TB_Offset_Waist_Y, TB_Offset_Waist_Z, C.MountOffset_Waist);
	SetVector(TB_Offset_LKnee_X, TB_Offset_LKnee_Y, TB_Offset_LKnee_Z, C.MountOffset_LeftKnee);
	SetVector(TB_Offset_RKnee_X, TB_Offset_RKnee_Y, TB_Offset_RKnee_Z, C.MountOffset_RightKnee);
	SetVector(TB_Offset_LFoot_X, TB_Offset_LFoot_Y, TB_Offset_LFoot_Z, C.MountOffset_LeftFoot);
	SetVector(TB_Offset_RFoot_X, TB_Offset_RFoot_Y, TB_Offset_RFoot_Z, C.MountOffset_RightFoot);
	SetVector(TB_HipRef_X, TB_HipRef_Y, TB_HipRef_Z, C.VehicleHipPosition);

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

	if (Combo_VehiclePresetSelect && !C.SelectedPresetName.IsEmpty())
		Combo_VehiclePresetSelect->SetSelectedOption(C.SelectedPresetName);

	if (CB_ShowCollisionSpheres) CB_ShowCollisionSpheres->SetIsChecked(C.bShowCollisionSpheres);
	if (CB_ShowTrackerMesh)      CB_ShowTrackerMesh->SetIsChecked(C.bShowTrackerMesh);
}

// ─────────────────────────────────────────────────────────────────────────────
//  [탭 1] Vehicle Preset 버튼 핸들러
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_ProfileManagerWidget::OnSaveVehiclePresetClicked()
{
	const FString VehicleName = TB_VehicleName
		? TB_VehicleName->GetText().ToString().TrimStartAndEnd()
		: TEXT("");

	if (VehicleName.IsEmpty())
	{
		ShowVehicleStatus(TEXT("차량 이름을 입력하세요."), false);
		return;
	}

	// FVTCVehiclePreset 구성: Vehicle_Hip ReferencePoint만 저장
	FVTCVehiclePreset Preset;
	Preset.PresetName = VehicleName;

	FVTCPresetRefPoint HipPoint;
	HipPoint.PartName = TEXT("Vehicle_Hip");
	HipPoint.Location = ParseVector(TB_HipPos_X, TB_HipPos_Y, TB_HipPos_Z);
	Preset.ReferencePoints.Add(HipPoint);

	if (UVTC_VehiclePresetLibrary::SavePreset(Preset))
	{
		RefreshVehiclePresetListComboBox();
		RefreshVehiclePresetComboBox();
		ShowVehicleStatus(FString::Printf(TEXT("저장됨: %s"), *VehicleName));
	}
	else
	{
		ShowVehicleStatus(FString::Printf(TEXT("저장 실패: %s"), *VehicleName), false);
	}
}

void UVTC_ProfileManagerWidget::OnDeleteVehiclePresetClicked()
{
	if (!Combo_VehiclePresetList) return;

	const FString VehicleName = Combo_VehiclePresetList->GetSelectedOption();
	if (VehicleName.IsEmpty() || VehicleName == TEXT("(None)"))
	{
		ShowVehicleStatus(TEXT("삭제할 차량을 목록에서 선택하세요."), false);
		return;
	}

	if (UVTC_VehiclePresetLibrary::DeletePreset(VehicleName))
	{
		RefreshVehiclePresetListComboBox();
		RefreshVehiclePresetComboBox();
		ShowVehicleStatus(FString::Printf(TEXT("삭제됨: %s"), *VehicleName));
	}
	else
	{
		ShowVehicleStatus(FString::Printf(TEXT("삭제 실패: %s"), *VehicleName), false);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  [탭 2] 프로파일 관리 버튼 핸들러
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_ProfileManagerWidget::OnNewProfileClicked()
{
	FVTCSessionConfig Empty;
	PopulateFromConfig(Empty);
	if (TB_ProfileName) TB_ProfileName->SetText(FText::GetEmpty());
	ShowStatus(TEXT("입력란 초기화됨 — 새 프로파일을 입력 후 [저장]"));
}

void UVTC_ProfileManagerWidget::OnLoadProfileClicked()
{
	if (!Combo_ProfileSelect) return;

	const FString ProfileName = Combo_ProfileSelect->GetSelectedOption();
	if (ProfileName.IsEmpty() || ProfileName == TEXT("(None)"))
	{
		ShowStatus(TEXT("불러올 프로파일을 선택하세요."), false);
		return;
	}

	FVTCSessionConfig Config;
	if (UVTC_ProfileLibrary::LoadProfile(ProfileName, Config))
	{
		PopulateFromConfig(Config);
		ShowStatus(FString::Printf(TEXT("불러옴: %s"), *ProfileName));
	}
	else
	{
		ShowStatus(FString::Printf(TEXT("불러오기 실패: %s"), *ProfileName), false);
	}
}

void UVTC_ProfileManagerWidget::OnSaveProfileClicked()
{
	const FString ProfileName = TB_ProfileName
		? TB_ProfileName->GetText().ToString().TrimStartAndEnd()
		: TEXT("");

	if (ProfileName.IsEmpty())
	{
		ShowStatus(TEXT("프로파일 이름을 입력하세요."), false);
		return;
	}

	const FVTCSessionConfig Config = BuildConfigFromInputs();
	if (UVTC_ProfileLibrary::SaveProfile(ProfileName, Config))
	{
		RefreshProfileComboBox();
		if (Combo_ProfileSelect) Combo_ProfileSelect->SetSelectedOption(ProfileName);
		ShowStatus(FString::Printf(TEXT("저장됨: %s"), *ProfileName));
	}
	else
	{
		ShowStatus(FString::Printf(TEXT("저장 실패: %s"), *ProfileName), false);
	}
}

void UVTC_ProfileManagerWidget::OnDeleteProfileClicked()
{
	if (!Combo_ProfileSelect) return;

	const FString ProfileName = Combo_ProfileSelect->GetSelectedOption();
	if (ProfileName.IsEmpty() || ProfileName == TEXT("(None)"))
	{
		ShowStatus(TEXT("삭제할 프로파일을 선택하세요."), false);
		return;
	}

	if (UVTC_ProfileLibrary::DeleteProfile(ProfileName))
	{
		RefreshProfileComboBox();
		ShowStatus(FString::Printf(TEXT("삭제됨: %s"), *ProfileName));
	}
	else
	{
		ShowStatus(FString::Printf(TEXT("삭제 실패: %s"), *ProfileName), false);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Active Profile 지정
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_ProfileManagerWidget::OnSetAsActiveClicked()
{
	if (!Combo_ProfileSelect) return;

	const FString ProfileName = Combo_ProfileSelect->GetSelectedOption();
	if (ProfileName.IsEmpty() || ProfileName == TEXT("(None)"))
	{
		ShowStatus(TEXT("Active로 지정할 프로파일을 선택하세요."), false);
		return;
	}

	// Saved/VTCConfig/ 디렉터리 생성
	const FString ConfigDir = FPaths::ProjectSavedDir() / TEXT("VTCConfig");
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	if (!PF.DirectoryExists(*ConfigDir))
		PF.CreateDirectoryTree(*ConfigDir);

	// ActiveProfile.txt 기록
	const FString FilePath = ConfigDir / TEXT("ActiveProfile.txt");
	if (FFileHelper::SaveStringToFile(ProfileName, *FilePath))
	{
		if (Txt_ActiveProfile)
			Txt_ActiveProfile->SetText(FText::FromString(ProfileName));
		ShowStatus(FString::Printf(TEXT("Active 지정됨: %s"), *ProfileName));
	}
	else
	{
		ShowStatus(TEXT("ActiveProfile.txt 쓰기 실패"), false);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  목록 갱신
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_ProfileManagerWidget::RefreshProfileComboBox()
{
	if (!Combo_ProfileSelect) return;

	const FString Current = Combo_ProfileSelect->GetSelectedOption();
	Combo_ProfileSelect->ClearOptions();
	Combo_ProfileSelect->AddOption(TEXT("(None)"));

	for (const FString& Name : UVTC_ProfileLibrary::GetAvailableProfileNames())
		Combo_ProfileSelect->AddOption(Name);

	if (!Current.IsEmpty() && Current != TEXT("(None)"))
		Combo_ProfileSelect->SetSelectedOption(Current);
}

void UVTC_ProfileManagerWidget::RefreshVehiclePresetComboBox()
{
	// Profile 탭의 Combo_VehiclePresetSelect (선택용)
	if (!Combo_VehiclePresetSelect) return;

	Combo_VehiclePresetSelect->ClearOptions();
	Combo_VehiclePresetSelect->AddOption(TEXT("(None)"));

	for (const FString& Name : UVTC_VehiclePresetLibrary::GetAvailablePresetNames())
		Combo_VehiclePresetSelect->AddOption(Name);
}

void UVTC_ProfileManagerWidget::RefreshVehiclePresetListComboBox()
{
	// 탭 1의 Combo_VehiclePresetList (삭제 대상 선택용)
	if (!Combo_VehiclePresetList) return;

	Combo_VehiclePresetList->ClearOptions();
	Combo_VehiclePresetList->AddOption(TEXT("(None)"));

	for (const FString& Name : UVTC_VehiclePresetLibrary::GetAvailablePresetNames())
		Combo_VehiclePresetList->AddOption(Name);
}

// ─────────────────────────────────────────────────────────────────────────────
//  슬라이더 핸들러
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_ProfileManagerWidget::OnWarningSliderChanged(float Value)
{
	if (Txt_WarningVal)
		Txt_WarningVal->SetText(FText::FromString(FString::Printf(TEXT("%.1f cm"), Value)));
}

void UVTC_ProfileManagerWidget::OnCollisionSliderChanged(float Value)
{
	if (Txt_CollisionVal)
		Txt_CollisionVal->SetText(FText::FromString(FString::Printf(TEXT("%.1f cm"), Value)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  차종 프리셋 선택 핸들러 (Profile 탭 Combo_VehiclePresetSelect)
//  → Preset JSON 로드 → "Vehicle_Hip" PartName 검색 → TB_HipRef_X/Y/Z 자동입력
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_ProfileManagerWidget::OnPresetSelectionChanged(FString SelectedItem,
                                                          ESelectInfo::Type SelectionType)
{
	if (SelectedItem.IsEmpty() || SelectedItem == TEXT("(None)")) return;

	FVTCVehiclePreset Preset;
	if (UVTC_VehiclePresetLibrary::LoadPreset(SelectedItem, Preset))
	{
		for (const FVTCPresetRefPoint& Ref : Preset.ReferencePoints)
		{
			if (Ref.PartName == TEXT("Vehicle_Hip"))
			{
				SetVector(TB_HipRef_X, TB_HipRef_Y, TB_HipRef_Z, Ref.Location);
				break;
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  상태 메시지
// ─────────────────────────────────────────────────────────────────────────────
void UVTC_ProfileManagerWidget::ShowStatus(const FString& Message, bool bSuccess)
{
	if (!Txt_Status) return;
	Txt_Status->SetText(FText::FromString(Message));
	Txt_Status->SetColorAndOpacity(FSlateColor(bSuccess ? FLinearColor::White : FLinearColor::Red));
}

void UVTC_ProfileManagerWidget::ShowVehicleStatus(const FString& Message, bool bSuccess)
{
	if (!Txt_VehicleStatus) return;
	Txt_VehicleStatus->SetText(FText::FromString(Message));
	Txt_VehicleStatus->SetColorAndOpacity(FSlateColor(bSuccess ? FLinearColor::White : FLinearColor::Red));
}

// ─────────────────────────────────────────────────────────────────────────────
//  파싱 / 설정 헬퍼
// ─────────────────────────────────────────────────────────────────────────────
float UVTC_ProfileManagerWidget::ParseFloat(const UEditableTextBox* TB, float Default)
{
	if (!TB) return Default;
	float V = Default;
	return LexTryParseString(V, *TB->GetText().ToString().TrimStartAndEnd()) ? V : Default;
}

void UVTC_ProfileManagerWidget::SetFloat(UEditableTextBox* TB, float Value)
{
	if (TB) TB->SetText(FText::FromString(FString::SanitizeFloat(Value)));
}

FVector UVTC_ProfileManagerWidget::ParseVector(const UEditableTextBox* TX,
                                                 const UEditableTextBox* TY,
                                                 const UEditableTextBox* TZ)
{
	return FVector(ParseFloat(TX), ParseFloat(TY), ParseFloat(TZ));
}

void UVTC_ProfileManagerWidget::SetVector(UEditableTextBox* TX, UEditableTextBox* TY,
                                           UEditableTextBox* TZ, const FVector& V)
{
	SetFloat(TX, V.X);
	SetFloat(TY, V.Y);
	SetFloat(TZ, V.Z);
}
