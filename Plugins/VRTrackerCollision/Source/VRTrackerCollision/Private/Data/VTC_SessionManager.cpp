// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Data/VTC_SessionManager.h"
#include "Body/VTC_BodyActor.h"
#include "Body/VTC_CalibrationComponent.h"
#include "Collision/VTC_CollisionDetector.h"
#include "Collision/VTC_WarningFeedback.h"
#include "Data/VTC_DataLogger.h"
#include "Kismet/GameplayStatics.h"

AVTC_SessionManager::AVTC_SessionManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// SessionManager가 가지는 서브시스템 컴포넌트
	CollisionDetector = CreateDefaultSubobject<UVTC_CollisionDetector>(TEXT("CollisionDetector"));
	WarningFeedback   = CreateDefaultSubobject<UVTC_WarningFeedback>(TEXT("WarningFeedback"));
	DataLogger        = CreateDefaultSubobject<UVTC_DataLogger>(TEXT("DataLogger"));
}

void AVTC_SessionManager::BeginPlay()
{
	Super::BeginPlay();
	AutoFindSystems();

	// Delegate 연결
	if (CollisionDetector)
	{
		CollisionDetector->OnWarningLevelChanged.AddDynamic(this,
			&AVTC_SessionManager::OnWarningLevelChanged);
	}

	if (BodyActor && BodyActor->CalibrationComp)
	{
		BodyActor->CalibrationComp->OnCalibrationComplete.AddDynamic(this,
			&AVTC_SessionManager::OnCalibrationComplete);
		BodyActor->CalibrationComp->OnCalibrationFailed.AddDynamic(this,
			&AVTC_SessionManager::OnCalibrationFailed);
	}

	UE_LOG(LogTemp, Log, TEXT("[VTC] SessionManager ready. State: Idle"));
}

void AVTC_SessionManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState != EVTCSessionState::Testing) return;

	SessionElapsedTime += DeltaTime;

	// DataLogger에 프레임 데이터 전송 (LogHz 주기)
	if (DataLogger && DataLogger->bIsLogging)
	{
		LogTimer += DeltaTime;
		const float LogInterval = 1.0f / DataLogger->LogHz;
		if (LogTimer >= LogInterval)
		{
			LogTimer = 0.0f;
			FVTCBodyMeasurements Measurements;
			if (BodyActor) Measurements = BodyActor->GetBodyMeasurements();
			TArray<FVTCDistanceResult> Distances;
			if (CollisionDetector) Distances = CollisionDetector->CurrentDistanceResults;
			DataLogger->LogFrame(Measurements, Distances);
		}
	}
}

void AVTC_SessionManager::StartSession(const FString& SubjectID)
{
	StartSessionWithHeight(SubjectID, 0.0f);
}

void AVTC_SessionManager::StartSessionWithHeight(const FString& SubjectID, float ManualHeight_cm)
{
	if (CurrentState != EVTCSessionState::Idle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VTC] StartSession called but not in Idle state."));
		return;
	}

	CurrentSubjectID       = SubjectID.IsEmpty() ? TEXT("Unknown") : SubjectID;
	PendingManualHeight_cm = ManualHeight_cm;
	SessionElapsedTime     = 0.0f;
	LogTimer               = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("[VTC] StartSession: SubjectID=%s, ManualHeight=%.1f cm"),
		*CurrentSubjectID, PendingManualHeight_cm);

	TransitionToState(EVTCSessionState::Calibrating);

	if (BodyActor)
	{
		BodyActor->StartCalibration();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[VTC] BodyActor not found. Cannot calibrate."));
		TransitionToState(EVTCSessionState::Idle);
	}
}

void AVTC_SessionManager::StartTestingDirectly()
{
	if (CurrentState == EVTCSessionState::Testing) return;
	TransitionToState(EVTCSessionState::Testing);

	if (DataLogger) DataLogger->StartLogging(CurrentSubjectID);
	if (CollisionDetector) CollisionDetector->ResetSessionStats();
}

void AVTC_SessionManager::StopSession()
{
	if (CurrentState == EVTCSessionState::Idle) return;
	if (DataLogger) DataLogger->StopLogging();
	if (WarningFeedback) WarningFeedback->ResetFeedback();
	TransitionToState(EVTCSessionState::Reviewing);
}

void AVTC_SessionManager::RequestReCalibration()
{
	if (DataLogger) DataLogger->StopLogging();
	TransitionToState(EVTCSessionState::Calibrating);
	if (BodyActor) BodyActor->StartCalibration();
}

FString AVTC_SessionManager::ExportAndEnd()
{
	FString FilePath;
	if (DataLogger)
	{
		if (DataLogger->bIsLogging) DataLogger->StopLogging();
		FilePath = DataLogger->ExportToCSV();
	}
	if (WarningFeedback) WarningFeedback->ResetFeedback();
	TransitionToState(EVTCSessionState::Idle);
	OnSessionExported.Broadcast(FilePath);
	return FilePath;
}

bool AVTC_SessionManager::IsTesting() const    { return CurrentState == EVTCSessionState::Testing; }
bool AVTC_SessionManager::IsCalibrating() const { return CurrentState == EVTCSessionState::Calibrating; }

FVTCBodyMeasurements AVTC_SessionManager::GetCurrentBodyMeasurements() const
{
	if (BodyActor) return BodyActor->GetBodyMeasurements();
	return FVTCBodyMeasurements();
}

float AVTC_SessionManager::GetSessionMinDistance() const
{
	if (CollisionDetector) return CollisionDetector->SessionMinDistance;
	return -1.0f;
}

void AVTC_SessionManager::TransitionToState(EVTCSessionState NewState)
{
	if (CurrentState == NewState) return;
	const EVTCSessionState OldState = CurrentState;
	CurrentState = NewState;
	OnSessionStateChanged.Broadcast(OldState, NewState);
	UE_LOG(LogTemp, Log, TEXT("[VTC] State: %s → %s"),
		*StaticEnum<EVTCSessionState>()->GetValueAsString(OldState),
		*StaticEnum<EVTCSessionState>()->GetValueAsString(NewState));
}

void AVTC_SessionManager::OnCalibrationComplete(const FVTCBodyMeasurements& Measurements)
{
	// 위젯에서 직접 입력된 키가 있으면 CalibrationComp.LastMeasurements에 주입
	// → DataLogger의 CachedMeasurements.ManualHeight_cm에 반영됨
	if (PendingManualHeight_cm > 0.0f && BodyActor && BodyActor->CalibrationComp)
	{
		BodyActor->CalibrationComp->LastMeasurements.ManualHeight_cm = PendingManualHeight_cm;
		UE_LOG(LogTemp, Log, TEXT("[VTC] ManualHeight %.1f cm applied to calibration data."),
			PendingManualHeight_cm);
	}

	UE_LOG(LogTemp, Log, TEXT("[VTC] Calibration complete → Starting test."));
	StartTestingDirectly();
}

void AVTC_SessionManager::OnCalibrationFailed(const FString& Reason)
{
	UE_LOG(LogTemp, Warning, TEXT("[VTC] Calibration failed: %s → Back to Idle."), *Reason);
	TransitionToState(EVTCSessionState::Idle);
}

void AVTC_SessionManager::OnWarningLevelChanged(EVTCTrackerRole BodyPart,
	FString PartName, EVTCWarningLevel Level)
{
	if (WarningFeedback)
	{
		WarningFeedback->OnWarningLevelChanged(BodyPart, PartName, Level);
	}

	// 충돌 이벤트 즉시 기록
	if (Level == EVTCWarningLevel::Collision && DataLogger && BodyActor)
	{
		FVTCCollisionEvent Event;
		Event.Timestamp        = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S.%s"));
		Event.BodyPart         = BodyPart;
		Event.VehiclePartName  = PartName;
		Event.CollisionLocation = BodyActor->GetBodyPartLocation(BodyPart);
		Event.Level            = Level;
		DataLogger->LogCollisionEvent(Event);
	}
}

void AVTC_SessionManager::AutoFindSystems()
{
	// TrackerPawn 탐색 (IVTC_TrackerInterface 구현체)
	if (!TrackerSource)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UVTC_TrackerInterface::StaticClass(), Found);
		if (Found.Num() > 0)
		{
			TrackerSource = TScriptInterface<IVTC_TrackerInterface>(Found[0]);
			UE_LOG(LogTemp, Log, TEXT("[VTC] Found tracker source: %s"), *Found[0]->GetName());
		}
	}

	// BodyActor 탐색
	if (!BodyActor)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVTC_BodyActor::StaticClass(), Found);
		if (Found.Num() > 0) BodyActor = Cast<AVTC_BodyActor>(Found[0]);
	}

	if (!TrackerSource) { UE_LOG(LogTemp, Warning, TEXT("[VTC] No TrackerPawn found in level!")); }
	if (BodyActor) { UE_LOG(LogTemp, Log, TEXT("[VTC] Found BodyActor.")); }
	else { UE_LOG(LogTemp, Warning, TEXT("[VTC] BodyActor NOT found in level!")); }

	// DataLogger에 TrackerSource 연결 (신체 위치 로깅에 필요)
	if (DataLogger && TrackerSource)
	{
		DataLogger->TrackerSource = TrackerSource;
	}
}
