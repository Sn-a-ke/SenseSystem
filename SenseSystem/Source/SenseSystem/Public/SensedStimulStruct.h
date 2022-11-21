//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectBaseUtility.h"

#include "SenseSystem.h"
#include "SenseSysHelpers.h"

#include "SensedStimulStruct.generated.h"


class AActor;
class USenseStimulusBase;


/** SensedPoint */
USTRUCT(BlueprintType)
struct SENSESYSTEM_API FSensedPoint
{
	GENERATED_USTRUCT_BODY()
public:
	FSensedPoint() {}
	FSensedPoint(const FVector& V, const float Score = 0.f) : SensedPoint(V), PointScore(Score) {}
	~FSensedPoint() {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "SensedStimulus")
	FVector SensedPoint = FVector::ZeroVector; // 12  byte
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "SensedStimulus")
	float PointScore = 0.f; //4
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "SensedStimulus")
	ESenseTestResult PointTestResult = ESenseTestResult::None; //1

	friend FArchive& operator<<(FArchive& Ar, FSensedPoint& Sp)
	{
		Ar << Sp.SensedPoint;
		Ar << Sp.PointScore;
		Ar << Sp.PointTestResult;
		return Ar;
	}

	FORCEINLINE bool operator==(const FSensedPoint& Other) const
	{
		return SensedPoint == Other.SensedPoint && PointScore == Other.PointScore && PointTestResult == Other.PointTestResult;
	}
	FORCEINLINE bool operator!=(const FSensedPoint& Other) const { return !((*this) == Other); }
};


/** SensedStimulus struct */
USTRUCT(BlueprintType)
struct SENSESYSTEM_API FSensedStimulus //56 bytes const int s = sizeof(FSensedStimulus);
{
	GENERATED_USTRUCT_BODY()
public:
	FSensedStimulus() {}
	~FSensedStimulus() {}

	FBox Init(const FName& SensorTag, USenseStimulusBase* Component, float InScore = 1.f, float InAge = 0.f, float CurrentTime = 0.f, uint64 InBitChannels = 0);


	/** StimulusComponent */
	UPROPERTY(BlueprintReadOnly, SkipSerialization, Transient, Category = "SensedStimulus")
	TWeakObjectPtr<USenseStimulusBase> StimulusComponent = nullptr; // 8  byte

	/** if StimulusComponent destroyed filtering will not crash result */
	UPROPERTY(Transient)
	uint32 TmpHash = MAX_uint32; // 4  byte

	/** Sensed Score */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "SensedStimulus")
	float Score = 1.f; // 4  byte

	/** Sensed Age */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "SensedStimulus")
	float Age = 0.f; // 4  byte

	/** SensedTime World->GetTimeSeconds() */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "SensedStimulus")
	float SensedTime = 0.f; // 4  byte

	/** First SensedTime World->GetTimeSeconds() */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "SensedStimulus")
	float FirstSensedTime = -1.f; // 4  byte

	UPROPERTY()
	uint64 BitChannels = 0; // 8 byte //todo replace type to  FBitFlag64_SenseSys

	/** SensedPoints */
	UPROPERTY(BlueprintReadOnly, SkipSerialization, Transient, Category = "SensedStimulus")
	TArray<FSensedPoint> SensedPoints; // 16 byte and dynamic memory allocation

	FORCEINLINE void InValidate()
	{
		StimulusComponent = nullptr;
		TmpHash = MAX_uint32;
		Score = -1.f;
		Age = 0.f;
		SensedPoints.Empty();
		BitChannels = 0;
	}

	FORCEINLINE friend uint32 GetTypeHash(const FSensedStimulus& In) { return In.TmpHash; }
	FORCEINLINE bool operator==(const FSensedStimulus& Other) const { return TmpHash == Other.TmpHash; }
	FORCEINLINE bool operator==(const USenseStimulusBase* Ssc) const { return StimulusComponent.Get() == Ssc; }

	friend FArchive& operator<<(FArchive& Ar, FSensedStimulus& SS)
	{
		Ar << SS.Score;
		Ar << SS.Age;
		Ar << SS.TmpHash;
		Ar << SS.SensedTime;
		Ar << SS.BitChannels;
		Ar << SS.SensedPoints;
		return Ar;
	}

	FORCEINLINE FSensedStimulus& operator=(const FSensedStimulus& Other)
	{
		StimulusComponent = Other.StimulusComponent;
		TmpHash = Other.TmpHash;
		Score = Other.Score;
		Age = Other.Age;
		SensedTime = Other.SensedTime;
		FirstSensedTime = Other.FirstSensedTime;
		BitChannels = Other.BitChannels;
		SensedPoints = Other.SensedPoints;
		return *this;
	}
};


/** TickingTimer struct */
struct SENSESYSTEM_API FTickingTimer
{
	FTickingTimer() {}
	FTickingTimer(const float InUpdateTimeRate) : UpdateTimeRate(FMath::Max(InUpdateTimeRate, EFPSTime_SenseSys::fps120)) {}
	~FTickingTimer() {}

	/** Zero value "0" - disable timer */
	float UpdateTimeRate = EFPSTime_SenseSys::fps30;

	FORCEINLINE bool TickTimer(const float DeltaTime)
	{
		if (IsValidUpdateTimeRate())
		{
			TimeToUpdate += DeltaTime;
			if (TimeToUpdate >= UpdateTimeRate)
			{
				TimeToUpdate = 0.f;
				return true;
			}
		}
		return false;
	}

	FORCEINLINE void SetUpdateTimeRate(const float NewValue) { UpdateTimeRate = FMath::Max(NewValue, EFPSTime_SenseSys::fps120); }
	FORCEINLINE void ContinueTimer() { bStopTimer = false; }
	FORCEINLINE void StopTimer() { bStopTimer = true; }
	FORCEINLINE bool IsStoppedTimer() const { return bStopTimer; }
	FORCEINLINE void ForceNextTickTimer() { TimeToUpdate = UpdateTimeRate; }

private:
	FORCEINLINE bool IsValidUpdateTimeRate() const { return !bStopTimer; }

	float TimeToUpdate = 0.0f;
	bool bStopTimer = false;
};


/** TrackBestSenseScore struct */
struct SENSESYSTEM_API FTrackBestSenseScore
{
	FTrackBestSenseScore()
	{
		if (TrackBestScoreCount < 0)
		{
			TrackBestScoreCount = 0;
		}
	}
	FTrackBestSenseScore(const int32 InTrackBestScoreCount, const float InMinBestScore)
		: TrackBestScoreCount(InTrackBestScoreCount)
		, MinBestScore(InMinBestScore)
	{
		if (TrackBestScoreCount < 0)
		{
			TrackBestScoreCount = 0;
		}
	}
	FTrackBestSenseScore(const FTrackBestSenseScore& In) : TrackBestScoreCount(In.TrackBestScoreCount), MinBestScore(In.MinBestScore)
	{
		if (TrackBestScoreCount < 0)
		{
			TrackBestScoreCount = 0;
		}
	}
	~FTrackBestSenseScore() {}

	int32 TrackBestScoreCount = 1;
	float MinBestScore = 0.f;
	TArray<int32> ScoreIDs;

	friend FArchive& operator<<(FArchive& Ar, FTrackBestSenseScore& Tbs)
	{
		Ar << Tbs.TrackBestScoreCount;
		Ar << Tbs.MinBestScore;
		Ar << Tbs.ScoreIDs;
		return Ar;
	}
};
