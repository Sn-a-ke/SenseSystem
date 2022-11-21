//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sensors/Tests/SensorLocationTestBase.h"
#include "Curves/CurveFloat.h"

#include "SensorBoxTest.generated.h"


/**
 * BoolTest
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class SENSESYSTEM_API USensorBoxTest : public USensorLocationTestBase
{
	GENERATED_BODY()
public:
	USensorBoxTest();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SensorTest")
	FVector BoxExtent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SensorTest")
	bool bOrientedBox = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SensorTest", meta = (EditCondition = "bOrientedBox"))
	bool bIgnoreVerticalRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SensorTest")
	bool bScore = false;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "SensorTest")
	//FRuntimeFloatCurve ScoreModifyCurve;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest")
	void SetBoxParam(FVector Extent, bool OrientedBox, bool IgnoreVerticalRotation, bool Score);

	virtual FBox GetSensorTestBoundBox() const override { return AABB_Box; }
	//virtual float GetSensorTestRadius() const override { return 0.f; }

	virtual EUpdateReady GetReadyToTest() override;
	virtual void InitializeCacheTest() override;
	virtual void InitializeFromSensor() override;
	virtual bool PreTest() override;
	virtual ESenseTestResult RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const override;

#if WITH_EDITORONLY_DATA
	virtual void DrawTest(const FSceneView* View, FPrimitiveDrawInterface* PDI) const override;
	virtual void DrawDebug(const float Duration) const override;
#endif

private:
	FBox AABB_Box;
	FVector::FReal MaxManhattanDistance = 0.f;
	void InitAABBBoxAndSensorTransform();
	//float ModifyScoreByCurve(float Value) const;
};

//FORCEINLINE float USensorBoxTest::ModifyScoreByCurve(const float Value) const
//{
//	return ScoreModifyCurve.EditorCurveData.Eval(FMath::Clamp(Value, 0.f, 1.f));
//}