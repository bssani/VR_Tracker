// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_DataLogger.h — CSV 데이터 로깅 및 내보내기

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "VTC_DataLogger.generated.h"

// CSV 행 구조 (메모리 내 버퍼)
USTRUCT(BlueprintType)
struct FVTCLogRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Timestamp;
	UPROPERTY(BlueprintReadOnly) FString SubjectID;
	UPROPERTY(BlueprintReadOnly) float   Height;
	UPROPERTY(BlueprintReadOnly) float   UpperLeftLeg;
	UPROPERTY(BlueprintReadOnly) float   UpperRightLeg;
	UPROPERTY(BlueprintReadOnly) float   LowerLeftLeg;
	UPROPERTY(BlueprintReadOnly) float   LowerRightLeg;

	// 각 신체 부위 위치 (cm, UE 좌표계)
	UPROPERTY(BlueprintReadOnly) FVector HipLocation;
	UPROPERTY(BlueprintReadOnly) FVector LeftKneeLocation;
	UPROPERTY(BlueprintReadOnly) FVector RightKneeLocation;
	UPROPERTY(BlueprintReadOnly) FVector LeftFootLocation;
	UPROPERTY(BlueprintReadOnly) FVector RightFootLocation;

	// 거리 정보 (기준점별)
	UPROPERTY(BlueprintReadOnly) TArray<FVTCDistanceResult> DistanceResults;

	// 충돌 발생 여부
	UPROPERTY(BlueprintReadOnly) bool    bCollisionOccurred;
	UPROPERTY(BlueprintReadOnly) FString CollisionPartName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVKCLogExported, const FString&, FilePath);

UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="VKC Data Logger"))
class VRTRACKERCOLLISION_API UVTC_DataLogger : public UActorComponent
{
	GENERATED_BODY()

public:
	UVTC_DataLogger();

	// ─── 설정 ────────────────────────────────────────────────────────────────

	// 로그 저장 폴더 (기본: 프로젝트 Saved/VKCLogs/)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Logger")
	FString LogDirectory = TEXT("");

	// 로그 샘플링 레이트 (Hz)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Logger",
		meta=(ClampMin=1.0f, ClampMax=30.0f))
	float LogHz = 10.0f;

	// ─── 상태 ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VKC|Logger")
	bool bIsLogging = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VKC|Logger")
	FString CurrentSubjectID;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VKC|Logger")
	int32 LoggedRowCount = 0;

	// ─── 함수 ────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "VKC|Logger")
	void StartLogging(const FString& SubjectID);

	UFUNCTION(BlueprintCallable, Category = "VKC|Logger")
	void StopLogging();

	// CSV 파일로 내보내기 → 파일 경로 반환
	UFUNCTION(BlueprintCallable, Category = "VKC|Logger")
	FString ExportToCSV();

	// 충돌 이벤트 즉시 기록
	UFUNCTION(BlueprintCallable, Category = "VKC|Logger")
	void LogCollisionEvent(const FVTCCollisionEvent& Event);

	// 거리 결과 + 신체 데이터를 한 행으로 기록 (매 LogHz마다 SessionManager가 호출)
	UFUNCTION(BlueprintCallable, Category = "VKC|Logger")
	void LogFrame(const FVTCBodyMeasurements& Measurements,
		const TArray<FVTCDistanceResult>& DistanceResults);

	// 로그 버퍼 초기화
	UFUNCTION(BlueprintCallable, Category = "VKC|Logger")
	void ClearLog();

	UPROPERTY(BlueprintAssignable, Category = "VKC|Logger|Events")
	FOnVKCLogExported OnLogExported;

private:
	TArray<FVTCLogRow> LogBuffer;
	TArray<FVTCCollisionEvent> CollisionEvents;

	float LogTimer = 0.0f;
	FString SessionStartTime;

	// CSV 헤더 문자열 생성
	FString BuildCSVHeader() const;

	// 단일 행을 CSV 문자열로 변환
	FString RowToCSVLine(const FVTCLogRow& Row) const;

	// 타임스탬프 문자열 생성
	static FString GetTimestampString();
};
