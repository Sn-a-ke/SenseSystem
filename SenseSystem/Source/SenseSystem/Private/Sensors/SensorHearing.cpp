//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/SensorHearing.h"
#include "Sensors/Tests/SensorDistanceTest.h"
#include "Sensors/Tests/SensorTraceTest.h"


USensorHearing::USensorHearing(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DistanceCurve.GetRichCurve()->AddKey(0.f, 1.f, false);
	DistanceCurve.GetRichCurve()->AddKey(1.f, 0.f, false);
	SensorTests.SetNum(1);
	SensorTests[0] = CreateDefaultSubobject<USensorDistanceTest>(TEXT("DistanceTest"));
	SetValuesToSensorTests();
}

USensorHearing::~USensorHearing()
{
}

#if WITH_EDITOR
void USensorHearing::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
	SetValuesToSensorTests();
}
#endif

void USensorHearing::SetDistanceParam(const float InMaxHearingDistance, const float InMinScore)
{
	MinScore = InMinScore;
	MaxHearingDistance = InMaxHearingDistance;
	FScopeLock Lock_CriticalSection(&SensorCriticalSection);
	SetValuesToSensorTests();
}

void USensorHearing::SetValuesToSensorTests()
{
	if (SensorTests.IsValidIndex(0))
	{
		if (const auto D = Cast<USensorDistanceTest>(SensorTests[0]))
		{
			D->ScoreModifyCurve.EditorCurveData = *DistanceCurve.GetRichCurveConst();
			D->ScoreModifyCurve.ExternalCurve = nullptr;

			D->SetDistanceParam(MaxHearingDistance, MaxHearingDistance);
			D->bTestBySingleLocation = true;
			D->MinScore = MinScore;
			return;
		}
	}

	UE_LOG(LogSenseSys, Error, TEXT("DistanceTEst invalid cast from - SensorTests[0] "));
}

/*
void USensorHearing::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (SensorTests.Num() > 0)
	{
		// This API returns the "correct" subobject by name, instanced or non-instanced.
		const auto Dso = Cast<USensorDistanceTest>(GetDefaultSubobjectByName(TEXT("DistanceTest")));
		if (SensorTests[0])
		{
			if (SensorTests[0] != Dso)  // ...if they don't match, the saved state needs to be fixed up.
			{
				// We have to dump this guy so he doesn't get serialized out with us anymore.
				SensorTests[0]->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
				// Redirect to the "correct" subobject instance.
				SensorTests[0] = Dso;
			}
		}
		else
		{
			SensorTests[0] = Dso;
		}
	}
}
*/
