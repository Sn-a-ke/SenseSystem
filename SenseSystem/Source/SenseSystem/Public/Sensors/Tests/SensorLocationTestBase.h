//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sensors/Tests/SensorTestBase.h"

#include "SensorLocationTestBase.generated.h"


/**
 *	SensorTestBy
 */
UENUM(BlueprintType)
enum class ESensorTestBy : uint8
{
	ActorLocation = 0 UMETA(DisplayName = "ActorLocation"),
	SensePoints       UMETA(DisplayName = "SensePoints"),
};


/**
 *	SensorLocationTestBase
 */
UCLASS(abstract, BlueprintType, EditInlineNew, HideDropdown)
class SENSESYSTEM_API USensorLocationTestBase : public USensorTestBase
{
	GENERATED_BODY()

public:
	/** Test By Single Location */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest")
	bool bTestBySingleLocation = true;

	/** full test implementation */
	virtual ESenseTestResult RunTest(FSensedStimulus& SensedStimulus) const override;

protected:
	/** test for one sensed point */
	virtual ESenseTestResult RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const;

	FIntPoint GetBoundFotSensePoints(TArray<struct FSensedPoint>& SensedPoints) const;
};
