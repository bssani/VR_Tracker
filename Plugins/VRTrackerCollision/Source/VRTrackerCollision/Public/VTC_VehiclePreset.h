// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_VehiclePreset.h — 차종별 ReferencePoint 프리셋 JSON 저장/불러오기
//
// [사용법]
//   Level 1 SetupWidget에서 현재 ReferencePoint 목록을 JSON으로 저장하고,
//   차종 프리셋을 선택하면 JSON에서 불러와 SessionConfig에 저장.
//   Level 2 OperatorController에서 프리셋 JSON → ReferencePoint 동적 스폰.

#pragma once

#include "CoreMinimal.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "VTC_VehiclePreset.generated.h"

// 단일 ReferencePoint 데이터
USTRUCT(BlueprintType)
struct FVTCPresetRefPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Preset")
	FString PartName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Preset")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Preset")
	TArray<EVTCTrackerRole> RelevantBodyParts;
};

// 차종 프리셋 전체 데이터
USTRUCT(BlueprintType)
struct FVTCVehiclePreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Preset")
	FString PresetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Preset")
	TArray<FVTCPresetRefPoint> ReferencePoints;
};

// 프리셋 관리 유틸리티 (Static 함수 모음)
UCLASS(BlueprintType)
class VRTRACKERCOLLISION_API UVTC_VehiclePresetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 프리셋 저장 디렉토리 반환 (Saved/VTCPresets/)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Preset")
	static FString GetPresetDirectory();

	// 프리셋을 JSON 파일로 저장
	UFUNCTION(BlueprintCallable, Category = "VTC|Preset")
	static bool SavePreset(const FVTCVehiclePreset& Preset);

	// JSON 파일에서 프리셋 불러오기
	UFUNCTION(BlueprintCallable, Category = "VTC|Preset")
	static bool LoadPreset(const FString& PresetName, FVTCVehiclePreset& OutPreset);

	// 저장된 모든 프리셋 이름 목록 반환
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Preset")
	static TArray<FString> GetAvailablePresetNames();

	// 프리셋 삭제
	UFUNCTION(BlueprintCallable, Category = "VTC|Preset")
	static bool DeletePreset(const FString& PresetName);

	// FVTCVehiclePreset ↔ JSON String 변환
	UFUNCTION(BlueprintCallable, Category = "VTC|Preset")
	static FString PresetToJsonString(const FVTCVehiclePreset& Preset);

	UFUNCTION(BlueprintCallable, Category = "VTC|Preset")
	static bool JsonStringToPreset(const FString& JsonString, FVTCVehiclePreset& OutPreset);
};
