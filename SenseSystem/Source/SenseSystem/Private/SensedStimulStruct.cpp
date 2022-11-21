//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "SensedStimulStruct.h"
#include "SenseStimulusBase.h"

FBox FSensedStimulus::Init(
	const FName& SensorTag, USenseStimulusBase* Component, const float InScore, const float InAge, const float CurrentTime, const uint64 InBitChannels)
{
	if (IsValid(Component))
	{
		StimulusComponent = Component;
		TmpHash = GetTypeHash(Component);
		Score = InScore;
		Age = InAge;
		SensedTime = CurrentTime;
		BitChannels = InBitChannels;

		const FSensedPoint P0 = FSensedPoint(StimulusComponent->GetSingleSensePoint(SensorTag), Score);
		FBox NewBox(P0.SensedPoint, P0.SensedPoint);

		const TArray<FVector> Points = StimulusComponent->GetSensePoints(SensorTag);
		SensedPoints.Reserve(Points.Num() + 1);
		SensedPoints.Add(P0);
		for (const auto& V : Points)
		{
			NewBox += V;
			SensedPoints.Add(FSensedPoint(V, Score));
		}
		return NewBox;
	}
	//InValidate();
	return FBox();
}
