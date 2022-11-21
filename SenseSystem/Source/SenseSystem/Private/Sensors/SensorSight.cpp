//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/SensorSight.h"
#include "Sensors/Tests/SensorDistanceAndAngleTest.h"
#include "Sensors/Tests/SensorTraceTest.h"

USensorSight::USensorSight(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DistanceCurve.GetRichCurve()->AddKey(0.f, 1.f, false);
	DistanceCurve.GetRichCurve()->AddKey(1.f, 0.f, false);
	AngleCurve.GetRichCurve()->AddKey(0.f, 1.f, false);
	AngleCurve.GetRichCurve()->AddKey(1.f, 0.f, false);

	SensorTests.SetNum(2);
	SensorTests[0] = CreateDefaultSubobject<USensorDistanceAndAngleTest>(TEXT("DistanceAndAngleTest"));
	SensorTests[1] = CreateDefaultSubobject<USensorTraceTest>(TEXT("TraceTest"));
	SetValuesToSensorTests();
}

USensorSight::~USensorSight()
{}

#if WITH_EDITOR
void USensorSight::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
	if (MaxDistanceLost < MaxDistance)
	{
		MaxDistanceLost = MaxDistance;
	}
	if (MaxAngleLost < MaxAngle)
	{
		MaxAngleLost = MaxAngle;
	}
	if (MinDistance > MaxDistance)
	{
		MaxDistance = MinDistance;
	}
	SetValuesToSensorTests();
}
#endif

void USensorSight::SetTraceParam(
	const ETraceTestParam InTraceParam,
	const ECollisionChannel InCollision,
	const bool TraceComplex,
	const bool InTestBySingleLocation,
	const float InMinScore)
{
	TraceTestParam = InTraceParam;
	CollisionChannel = InCollision;
	bTraceBySingleLocation = InTestBySingleLocation;
	MinScore = InMinScore;
	bTraceComplex = TraceComplex;
	FScopeLock Lock_CriticalSection(&SensorCriticalSection);
	SetValuesToSensorTests();
}

void USensorSight::SetDistanceAngleParam(
	const float InMaxDistance,
	const float InMaxDistanceLost,
	const float InMaxAngle,
	const float InMaxAngleLost,
	const bool InTestBySingleLocation,
	const float InMinScore,
	const float InMinDistance)
{
	MaxAngle = InMaxAngle;
	MaxAngleLost = InMaxAngleLost;
	MaxDistance = InMaxDistance;
	MaxDistanceLost = InMaxDistanceLost;
	bDistanceAngleBySingleLocation = InTestBySingleLocation;
	MinScore = InMinScore;
	MinDistance = InMinDistance;
	FScopeLock Lock_CriticalSection(&SensorCriticalSection);
	SetValuesToSensorTests();
}

void USensorSight::SetValuesToSensorTests()
{
	check(SensorTests.IsValidIndex(0) && SensorTests.IsValidIndex(1));

	if (const auto D = Cast<USensorDistanceAndAngleTest>(SensorTests[0]))
	{
		D->DistanceCurve.EditorCurveData = *DistanceCurve.GetRichCurveConst();
		D->DistanceCurve.ExternalCurve = nullptr;

		D->AngleCurve.EditorCurveData = *AngleCurve.GetRichCurveConst();
		D->AngleCurve.ExternalCurve = nullptr;

		D->bTestBySingleLocation = bDistanceAngleBySingleLocation;
		D->SetDistanceAndAngleParam(MaxAngle, MaxAngleLost, MaxDistance, MaxDistanceLost, MinDistance);
		D->MinScore = MinScore;
	}
	else
	{
		UE_LOG(LogSenseSys, Error, TEXT("DistanceTEst invalid cast from - SensorTests[0] "));
	}

	if (const auto SensorTraceTest = Cast<USensorTraceTest>(SensorTests[1]))
	{
		SensorTraceTest->SetTraceParam(TraceTestParam, CollisionChannel, bTraceBySingleLocation, bTraceComplex);
		SensorTraceTest->MinScore = MinScore;
	}
	else
	{
		UE_LOG(LogSenseSys, Error, TEXT("TraceTest invalid cast from - SensorTests[1] "));
	}
}


/*
void USensorSight::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (SensorTests.Num() > 1)
	{
		const auto Dso_1 = Cast<USensorDistanceAndAngleTest>(GetDefaultSubobjectByName(TEXT("DistanceAndAngleTest")));
		if (SensorTests[0])
		{
			if (SensorTests[0] != Dso_1)  // ...if they don't match, the saved state needs to be fixed up.
			{
				SensorTests[0]->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
				SensorTests[0] = Dso_1;
			}
		}
		else
		{
			SensorTests[0] = Dso_1;
		}

		const auto Dso_2 = Cast<USensorTraceTest>(GetDefaultSubobjectByName(TEXT("TraceTest")));
		if (SensorTests[1])
		{
			if (SensorTests[1] != Dso_2)  // ...if they don't match, the saved state needs to be fixed up.
			{
				SensorTests[1]->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
				SensorTests[1] = Dso_2;
			}
		}
		else
		{
			SensorTests[1] = Dso_2;
		}
	}

}
*/