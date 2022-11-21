//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/Tests/SensorLocationTestBase.h"
#include "SensedStimulStruct.h"


ESenseTestResult USensorLocationTestBase::RunTest(FSensedStimulus& SensedStimulus) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_LocationTestBase_RunTest);

	ESenseTestResult OutResult = ESenseTestResult::Lost;
	if (MinScore < SensedStimulus.Score && SensedStimulus.SensedPoints.Num())
	{
		TArray<FSensedPoint>& Points = SensedStimulus.SensedPoints;
		const FIntPoint TestBound = GetBoundFotSensePoints(Points);
		float TotalScore = 0.f;

		for (int32 i = TestBound.X; i < TestBound.Y; i++)
		{
			float Score = 1.0f;
			Points[i].PointTestResult = RunTestForLocation(SensedStimulus, Points[i].SensedPoint, Score);
			if (Points[i].PointTestResult != ESenseTestResult::Lost)
			{
				Points[i].PointScore *= Score;
				OutResult = FMath::Min(OutResult, Points[i].PointTestResult);
				TotalScore += Score;
			}
		}

		if (OutResult != ESenseTestResult::Lost)
		{
			SensedStimulus.Score *= TotalScore / static_cast<float>(TestBound.Y - TestBound.X);
			if (OutResult == ESenseTestResult::Sensed && MinScore > SensedStimulus.Score)
			{
				OutResult = ESenseTestResult::NotLost;
			}
		}
	}
	return OutResult;
}

ESenseTestResult USensorLocationTestBase::RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const
{
	return ESenseTestResult::Lost;
}

FIntPoint USensorLocationTestBase::GetBoundFotSensePoints(TArray<FSensedPoint>& SensedPoints) const
{
	if (!bTestBySingleLocation && SensedPoints.Num() > 1)
	{
		SensedPoints[0].PointTestResult = ESenseTestResult::None;
		return FIntPoint(1, SensedPoints.Num()); // the location of the actor is not taken into account
	}
	return FIntPoint(0, 1);
}
