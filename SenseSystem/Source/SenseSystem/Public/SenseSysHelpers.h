//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "HAL/Platform.h"
#include "UObject/ObjectMacros.h"

#include "SenseSysHelpers.generated.h"


/**
*	FPSTime_SenseSys SenseSys
*/
namespace EFPSTime_SenseSys
{
	constexpr float fps120 = 1.0 / 120;
	constexpr float fps90 = 1.0 / 90;
	constexpr float fps60 = 1.0 / 60;
	constexpr float fps45 = 1.0 / 45;
	constexpr float fps30 = 1.0 / 30;
	constexpr float fps24 = 1.0 / 24;
	constexpr float fps15 = 1.0 / 15;
	constexpr float fps10 = 0.1;

}; // namespace EFPSTime_SenseSys

// clang-format off

/** SensorType */
UENUM(BlueprintType)
enum class ESensorType : uint8
{	
	None = 0,
	Active, 
	Passive,
	Manual,  
};

/** SuccessState SenseSys */
UENUM(BlueprintType)
enum class ESuccessState : uint8
{
	Success = 0,
	Failed
};

/** DrawDepth SenseSys */
UENUM(BlueprintType)
enum class EDrawDepthSenseSys : uint8
{
	/** World scene DPG. */
	World,	
	/** Foreground scene DPG. */
	Foreground
};

/** OnSenseEvent SenseSys */
UENUM(BlueprintType)
enum class EOnSenseEvent : uint8
{
	SenseCurrent = 0 UMETA(DisplayName = "OnSenseCurrent"),
	SenseNew         UMETA(DisplayName = "OnSensedNew"),
	SenseLost        UMETA(DisplayName = "OnSenseLost"),
	SenseForget      UMETA(DisplayName = "OnSenseForget"),
};

/** SenseTestResult SenseSys */
UENUM(BlueprintType)
enum class ESenseTestResult : uint8
{
	None    = 0,
	Sensed  = 1,
	NotLost = 2, 
	Lost    = 3
};

/** SensorArrayByType SenseSys */
UENUM(BlueprintType)
enum class ESensorArrayByType : uint8
{
	SenseLost        = 0,
	SensedNew        = 1,
	SenseCurrent     = 2,
	SenseCurrentLost = 3,
	SenseForget      = 4
};

/** SensorThreadType SenseSys */
UENUM(BlueprintType)
enum class ESensorThreadType : uint8
{
	Main_Thread               = 0,
	Sense_Thread              = 1,
	Sense_Thread_HighPriority = 2,
	Async_Task                = 3
};

// clang-format on

/** DrawElementSetup SenseSys */
USTRUCT(BlueprintType)
struct SENSESYSTEM_API FDrawElementSetup //40 bytes array pointer inside
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawElementSetup")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawElementSetup")
	float Thickness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawElementSetup")
	EDrawDepthSenseSys DrawDepth = EDrawDepthSenseSys::World;

	static ESceneDepthPriorityGroup ToSceneDepthPriorityGroup(const EDrawDepthSenseSys DrawDepth)
	{
		switch (DrawDepth)
		{
			case EDrawDepthSenseSys::World: return ESceneDepthPriorityGroup::SDPG_World;
			case EDrawDepthSenseSys::Foreground: return ESceneDepthPriorityGroup::SDPG_Foreground;
			default: return ESceneDepthPriorityGroup::SDPG_World; ;
		}
	}
};


/** DebugDrawFlag SenseSys */
USTRUCT(BlueprintType)
struct SENSESYSTEM_API FSenseSysDebugDraw //40 bytes array pointer inside
{
	GENERATED_USTRUCT_BODY()

	FSenseSysDebugDraw(const bool bAllVal)
		: Stimulus_DebugSensedPoints(bAllVal)
		, Stimulus_DebugCurrentSensed(bAllVal)
		, Stimulus_DebugLostSensed(bAllVal)
		, Sensor_DebugTest(bAllVal)
		, Sensor_DebugCurrentSensed(bAllVal)
		, Sensor_DebugLostSensed(bAllVal)
		, Sensor_DebugBestSensed(bAllVal)
		, SenseSys_DebugAge(bAllVal)
		, SenseSys_DebugScore(bAllVal)
	{}
	FSenseSysDebugDraw() : FSenseSysDebugDraw(true) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseSysDebugDraw")
	bool Stimulus_DebugSensedPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseSysDebugDraw")
	bool Stimulus_DebugCurrentSensed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseSysDebugDraw")
	bool Stimulus_DebugLostSensed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseSysDebugDraw")
	bool Sensor_DebugTest;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseSysDebugDraw")
	bool Sensor_DebugCurrentSensed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseSysDebugDraw")
	bool Sensor_DebugLostSensed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseSysDebugDraw")
	bool Sensor_DebugBestSensed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseSysDebugDraw")
	bool SenseSys_DebugAge;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SenseSysDebugDraw")
	bool SenseSys_DebugScore;
};


/** BitFlag64 SenseSys */
USTRUCT(BlueprintType)
struct SENSESYSTEM_API FBitFlag64_SenseSys
{
	GENERATED_USTRUCT_BODY()

	FBitFlag64_SenseSys() {}
	FBitFlag64_SenseSys(const uint64 InValue) : Value(InValue) {}

	UPROPERTY()
	uint64 Value = 0;

	FORCEINLINE operator uint64&() { return Value; }
	FORCEINLINE operator const uint64&() const { return Value; }

	FBitFlag64_SenseSys& operator=(const uint64 Other)
	{
		Value = Other;
		return *this;
	}
	FBitFlag64_SenseSys& operator=(const FBitFlag64_SenseSys Other)
	{
		Value = Other.Value;
		return *this;
	}

	friend FArchive& operator<<(FArchive& Ar, FBitFlag64_SenseSys& Val)
	{
		Ar << Val.Value;
		return Ar;
	}

	static uint64 To_64BitFlagChannel(const TArray<uint8>& InChan)
	{
		uint64 Out = 0;
		for (const uint8 Channel : InChan)
		{
			check(Channel < 64);
			Out |= 1llu << Channel;
		}
		return Out;
	}

	static TArray<uint8> To_ArrayBitFlagChannel(uint64 InChan)
	{
		TArray<uint8> Out;
		{
			int32 i = 0;
			while (InChan)
			{
				if (InChan & 1llu) Out.Add(static_cast<uint8>(i));
				i++;
				InChan = InChan >> 1;
			}
		}
		return Out;
	}
};


/** DebugSenseSysHelpers SenseSys */
namespace EDebugSenseSysHelpers
{
	constexpr float DebugSmallSize = 4.f;
	constexpr float DebugNormalSize = 8.f;
	constexpr float DebugBigSize = 16.f;
	constexpr float DebugLargeSize = 32.f;
} // namespace EDebugSenseSysHelpers

#if WITH_EDITOR | WITH_EDITORONLY_DATA

enum class EOwnerBlueprintClassType : uint8
{
	None = 0,
	ActorBP,
	SenseReceiverComponentBP,
	SensorBaseBP,
};

struct FSenseSysRestoreObject
{
	FSenseSysRestoreObject( //
		class UObject* InDefaultObject = nullptr,
		class UClass* InClass = nullptr,
		const FName& InName = NAME_None,
		const int32 InIdx = -1,
		const ESensorType InSensorType = ESensorType::None)
		: DefaultObject(InDefaultObject)
		, Class(InClass)
		, ObjectName(InName)
		, Idx(InIdx)
		, SensorType(InSensorType)
	{}

	UObject* DefaultObject /*= nullptr*/;
	UClass* Class /*= nullptr*/;
	FName ObjectName /*= NAME_None*/;
	int32 Idx /*= -1*/;
	ESensorType SensorType;
};

#endif //WITH_EDITOR