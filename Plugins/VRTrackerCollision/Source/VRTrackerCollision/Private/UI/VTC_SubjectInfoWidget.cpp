// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "UI/VTC_SubjectInfoWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"

void UVTC_SubjectInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_StartSession)
	{
		Btn_StartSession->OnClicked.AddDynamic(this, &UVTC_SubjectInfoWidget::OnStartSessionClicked);
	}
}

void UVTC_SubjectInfoWidget::OnStartSessionClicked()
{
	if (!IsInputValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[VTC] SubjectInfoWidget: SubjectID가 비어있거나 키(Height)가 0 이하입니다."));
		return;
	}

	OnSessionStartRequested.Broadcast(GetEnteredSubjectID(), GetEnteredHeight());
}

float UVTC_SubjectInfoWidget::GetEnteredHeight() const
{
	if (!TB_Height) return 0.0f;
	const FString Text = TB_Height->GetText().ToString().TrimStartAndEnd();
	float Value = 0.0f;
	return LexTryParseString(Value, *Text) ? Value : 0.0f;
}

FString UVTC_SubjectInfoWidget::GetEnteredSubjectID() const
{
	if (!TB_SubjectID) return TEXT("");
	return TB_SubjectID->GetText().ToString().TrimStartAndEnd();
}

bool UVTC_SubjectInfoWidget::IsInputValid() const
{
	return !GetEnteredSubjectID().IsEmpty() && GetEnteredHeight() > 0.0f;
}

void UVTC_SubjectInfoWidget::ClearInputs()
{
	if (TB_SubjectID) TB_SubjectID->SetText(FText::GetEmpty());
	if (TB_Height)    TB_Height->SetText(FText::GetEmpty());
}
