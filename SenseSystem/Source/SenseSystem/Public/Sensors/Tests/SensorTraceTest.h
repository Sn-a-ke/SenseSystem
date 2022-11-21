//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sensors/Tests/SensorTestBase.h"
#include "SensorTraceTestBase.h"

#include "SensorTraceTest.generated.h"


/**
 *	Sensor Trace Test
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class SENSESYSTEM_API USensorTraceTest : public USensorTraceTestBase
{
	GENERATED_BODY()
public:
	USensorTraceTest(const FObjectInitializer& ObjectInitializer);
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest")
	void SetTraceParam(ETraceTestParam TraceParam, ECollisionChannel Collision, bool bInTestBySingleLocation, bool TraceComplex);

	virtual EUpdateReady GetReadyToTest() override;
	virtual bool PreTest() override;

	virtual ESenseTestResult RunTest(FSensedStimulus& SensedStimulus) const override;

protected:
	virtual void InitializeCacheTest() override;

	bool LineTraceTest(const UWorld* World, const FCollisionQueryParams& CollisionParams, const FVector& StartTrace, FSensedStimulus& SensedStimulus) const;
};
