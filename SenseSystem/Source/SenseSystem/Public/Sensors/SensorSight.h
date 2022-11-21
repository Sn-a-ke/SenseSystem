//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Sensors/ActiveSensor.h"
#include "Sensors/Tests/SensorTraceTest.h"

#include "SensorSight.generated.h"


/**
 * Sensor Sight
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, HideCategories = (SensorTests))
class SENSESYSTEM_API USensorSight : public UActiveSensor
{
	GENERATED_BODY()

public:
	USensorSight(const FObjectInitializer& ObjectInitializer);
	virtual ~USensorSight() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

	/**Skip test if input score < MinScore*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorSight")
	float MinScore = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorSight", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorSight", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDistance = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorSight", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDistanceLost = 6000;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorSight", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
	float MaxAngle = 45;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorSight", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
	float MaxAngleLost = 60;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorSight")
	ETraceTestParam TraceTestParam = ETraceTestParam::BoolTraceTest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorSight")
	TEnumAsByte<ECollisionChannel> CollisionChannel = ECollisionChannel::ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorSight")
	bool bTraceComplex = false;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "SensorSight")
	FRuntimeFloatCurve DistanceCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "SensorSight")
	FRuntimeFloatCurve AngleCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "SensorSight")
	bool bDistanceAngleBySingleLocation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "SensorSight")
	bool bTraceBySingleLocation = false;


	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor|SensorSight")
	void SetTraceParam(ETraceTestParam InTraceParam, ECollisionChannel InCollision, bool TraceComplex, bool InTestBySingleLocation, float InMinScore);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Sensor|SensorSight")
	void SetDistanceAngleParam(
		float InMaxDistance,
		float InMaxDistanceLost,
		float InMaxAngle,
		float InMaxAngleLost,
		bool InTestBySingleLocation,
		float InMinScore,
		float InMinDistance = 0.f);

	//virtual void Serialize(FArchive& Ar) override;

protected:
	void SetValuesToSensorTests();
};
