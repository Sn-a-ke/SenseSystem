//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sensors/Tests/SensorTestBase.h"

#include "SensorBoolTest.generated.h"


/**
 * BoolTest
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class SENSESYSTEM_API USensorBoolTest : public USensorTestBase
{
	GENERATED_BODY()
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SensorTest")
	ESenseTestResult TestResult = ESenseTestResult::Sensed;

	/** Prepare for the test, called before RunTest */
	virtual EUpdateReady GetReadyToTest() override;

	/** PreTest */
	virtual bool PreTest() override;

	/** Test Body , if return true - remove elem */
	virtual ESenseTestResult RunTest(FSensedStimulus& SensedStimulus) const override;
};
