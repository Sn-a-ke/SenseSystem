//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sensors/PassiveSensor.h"
#include "Curves/CurveFloat.h"
#include "Sensors/Tests/SensorTraceTestBase.h"

#include "SensorHearing.generated.h"


/**
 * SensorHearing
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, HideCategories = (SensorTests))
class SENSESYSTEM_API USensorHearing : public UPassiveSensor
{
	GENERATED_BODY()
public:
	USensorHearing(const FObjectInitializer& ObjectInitializer);
	virtual ~USensorHearing() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorHearing")
	float MinScore = 0.f;

	/**Max Hearing Distance*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SensorHearing")
	float MaxHearingDistance = 10000;

	/**Score Modify by Curve*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "SensorHearing")
	FRuntimeFloatCurve DistanceCurve;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor|SensorHearing")
	void SetDistanceParam(float InMaxHearingDistance, float InMinScore);


	//virtual void Serialize(FArchive& Ar) override;

	//todo: noise trace ScoreFromTransparency, need an overridden method of scoring points for test tests when Overlapping without interfaces.

	// /** whether Noise will propagate through objects */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorHearing")
	//	ETraceTestParam NoiseCollisionCheck = ETraceTestParam::MultiTraceTest;

	// /**
	// * NoiseOverlapChannel - add  NoiseChannel in project settings trace channels
	// * and set overlap if need reduce noise score
	// * NoiseCollisionCheck set to MultiTraceTest
	// */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorHearing")
	//	TEnumAsByte<ECollisionChannel>  NoiseOverlapChannel = ECollisionChannel::ECC_Visibility;


protected:
	void SetValuesToSensorTests();
};
