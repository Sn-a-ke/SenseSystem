//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/Tests/SensorTraceTest.h"
#include "Sensors/SensorBase.h"
#include "SensedStimulStruct.h"
#include "SenseStimulusBase.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"


USensorTraceTest::USensorTraceTest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{}

#if WITH_EDITOR
void USensorTraceTest::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	USensorTraceTestBase::PostEditChangeProperty(e);
	InitializeCacheTest();
}
#endif

void USensorTraceTest::SetTraceParam(const ETraceTestParam TraceParam, const ECollisionChannel Collision, const bool bInTestBySingleLocation, const bool TraceComplex)
{
	SetBaseTraceParam(TraceParam, Collision, TraceComplex);
	bTestBySingleLocation = bInTestBySingleLocation;
	InitializeCacheTest();
}

EUpdateReady USensorTraceTest::GetReadyToTest()
{
	bool bNeedInitCollision;
	{
		const TArray<AActor*>& IgnActors = GetSensorOwner()->GetIgnoredActors();
		const auto& IgnCollisions = Collision_Params.GetIgnoredActors();

		bNeedInitCollision = IgnCollisions.Num() != IgnActors.Num();
		if (!bNeedInitCollision)
		{
			for (int32 i = 0; i < IgnActors.Num(); i++)
			{
				if (IgnActors[i])
				{
					if (IgnActors[i]->GetUniqueID() != IgnCollisions[i])
					{
						bNeedInitCollision = true;
						break;
					}
				}
			}
		}
	}

	if (bNeedInitCollision)
	{
		InitCollisionParams();
	}

	return USensorTraceTestBase::GetReadyToTest();
}

bool USensorTraceTest::PreTest()
{
	return Super::PreTest();
}

void USensorTraceTest::InitializeCacheTest()
{
	Super::InitializeCacheTest();
}

ESenseTestResult USensorTraceTest::RunTest(FSensedStimulus& SensedStimulus) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SensorTraceTestRunTest);
	if (MinScore < SensedStimulus.Score)
	{
		if (const UWorld* World = GetWorld())
		{
			if (SensedStimulus.StimulusComponent.IsValid())
			{
				check(SensedStimulus.SensedPoints.Num());
				FCollisionQueryParams CollisionParams = Collision_Params;
				CollisionParams.AddIgnoredActors(SensedStimulus.StimulusComponent.Get()->GetIgnoreTraceActors());
				if (LineTraceTest(World, CollisionParams, GetSensorTransform().GetLocation(), SensedStimulus))
				{
					return MinScore > SensedStimulus.Score ? ESenseTestResult::NotLost : ESenseTestResult::Sensed;
				}
			}
		}
	}
	return ESenseTestResult::Lost;
}

bool USensorTraceTest::LineTraceTest(
	const UWorld* World,
	const FCollisionQueryParams& CollisionParams,
	const FVector& StartTrace,
	FSensedStimulus& SensedStimulus) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_TraceTest);

	TArray<FSensedPoint>& Points = SensedStimulus.SensedPoints;
	const FIntPoint TestBound = GetBoundFotSensePoints(Points);
	const int32 CountLoop = TestBound.Y - TestBound.X;

	const float MaxScorePerLoop = 1.f / static_cast<float>(CountLoop);
	const float ScaledMinScore = MinScore / SensedStimulus.Score;

	float TotalScore = 0.f;
	for (int32 i = TestBound.X; i < TestBound.Y; i++)
	{
		float LocScore = 1.f;
		bool bNotHit;
		{
			if (TraceTestParam == ETraceTestParam::BoolTraceTest)
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_LineTraceTestByChannel);
				bNotHit = !World->LineTraceTestByChannel(StartTrace, Points[i].SensedPoint, CollisionChannel, CollisionParams);
			}
			else
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_MultiLineTraceByChannel);
				TArray<struct FHitResult> OutHits;
				bNotHit = !World->LineTraceMultiByChannel(OutHits, StartTrace, Points[i].SensedPoint, CollisionChannel, CollisionParams);
				bNotHit = bNotHit && ScoreFromTransparency(OutHits, LocScore);
			}
		}
		if (bNotHit)
		{
			TotalScore += LocScore;
			Points[i].PointScore *= LocScore;
			if (Points[i].PointTestResult == ESenseTestResult::None)
			{
				Points[i].PointTestResult = ESenseTestResult::Sensed;
			}
		}
		else
		{
			Points[i].PointTestResult = ESenseTestResult::Lost;
			Points[i].PointScore = 0;
			if (ScaledMinScore > (TotalScore + ((CountLoop - i - 1) * MaxScorePerLoop))) //max possible score
			{
				return false;
			}
		}
	}

	if (TotalScore > 0 && TotalScore >= MinScore)
	{
		SensedStimulus.Score *= TotalScore / static_cast<float>(CountLoop);
		return true;
	}
	return false;
}
