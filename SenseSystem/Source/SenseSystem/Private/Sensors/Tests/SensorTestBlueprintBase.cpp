//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/Tests/SensorTestBlueprintBase.h"
#include "Sensors/SensorBase.h"
#include "SenseStimulusBase.h"
#include "UObject/GarbageCollection.h"


#if WITH_EDITOR
void USensorTestBlueprintBase::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
}
#endif

/********************************/

FTransform USensorTestBlueprintBase::GetSensorTransformBP() const
{
	return GetSensorTransform();
}

ESenseTestResult USensorTestBlueprintBase::RunTest(FSensedStimulus& SensedStimulus) const
{
	if (SensedStimulus.TmpHash != MAX_uint32 && SensedStimulus.StimulusComponent.IsValid() && IsValid(GetSensorOwner()))
	{
		FGCScopeGuard GCScope;
		return RunTestBP(SensedStimulus);
	}
	return ESenseTestResult::Lost;
}

ESenseTestResult USensorTestBlueprintBase::RunTestBP_Implementation(FSensedStimulus& SensedStimulus) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SensorTestBlueprintBase_RunTestBPImplementation);

	return USensorLocationTestBase::RunTest(SensedStimulus);
}

/********************************/

ESenseTestResult USensorTestBlueprintBase::RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SensorTestBlueprintBase_RunTestForLocation);

	if (SensedStimulus.TmpHash != MAX_uint32 && SensedStimulus.StimulusComponent.IsValid() && IsValid(GetSensorOwner()))
	{
		FGCScopeGuard GCScope;
		return RunLocationTestBP(SensedStimulus, TestLocation, ScoreResult);
	}
	return ESenseTestResult::Lost;
}

EUpdateReady USensorTestBlueprintBase::GetReadyToTest()
{
	USensorTestBase::GetReadyToTest();
	if (IsValid(GetSensorOwner()))
	{
		return GetReadyToTestBP();
	}
	return EUpdateReady::Fail;
}

bool USensorTestBlueprintBase::PreTest()
{
	if (IsValid(GetSensorOwner()))
	{
		Super::PreTest();
		return PreTest_BP();
	}
	return false;
}

FBox USensorTestBlueprintBase::GetSensorTestBoundBox() const
{
	if (IsValid(GetSensorOwner()))
	{
		return GetSensorTestBoundBoxBP();
	}
	return Super::GetSensorTestBoundBox();
}

float USensorTestBlueprintBase::GetSensorTestRadius() const
{
	if (IsValid(GetSensorOwner()))
	{
		return GetSensorTestRadiusBP();
	}
	return Super::GetSensorTestRadius();
}
