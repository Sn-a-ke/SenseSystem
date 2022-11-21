//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sensors/Tests/SensorLocationTestBase.h"
#include "Curves/CurveFloat.h"

#include "SensorDistanceTest.generated.h"

/**
 * DistanceTest
 */
UCLASS(BlueprintType, EditInlineNew)
class SENSESYSTEM_API USensorDistanceTest : public USensorLocationTestBase
{
	GENERATED_BODY()

public:
	USensorDistanceTest(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

#if WITH_EDITORONLY_DATA
	virtual void DrawTest(const FSceneView* View, FPrimitiveDrawInterface* PDI) const override;
	virtual void DrawDebug(float Duration) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest")
	float MaxDistance = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest")
	float MaxDistanceLost = 15000;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest")
	void SetDistanceParam(const float InMaxDistance, const float InMaxDistanceLost);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "SensorTest")
	FRuntimeFloatCurve ScoreModifyCurve;

	float ModifyScoreByCurve(float Value) const;

	virtual EUpdateReady GetReadyToTest() override;
	virtual bool PreTest() override;

	virtual FBox GetSensorTestBoundBox() const override { return AABB_Box; }
	virtual float GetSensorTestRadius() const override { return AABB_Box.GetExtent().X; }

protected:
	virtual ESenseTestResult RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const override;

	virtual void InitializeCacheTest() override;

	FBox AABB_Box = FBox(FVector::ZeroVector, FVector::ZeroVector);
	FVector::FReal MaxDistanceSquared;
	FVector::FReal MaxDistanceLostSquared;
};

FORCEINLINE float USensorDistanceTest::ModifyScoreByCurve(const float Value) const
{
	return ScoreModifyCurve.EditorCurveData.Eval(FMath::Clamp(Value, 0.f, 1.f));
}

FORCEINLINE void USensorDistanceTest::SetDistanceParam(const float InMaxDistance, const float InMaxDistanceLost)
{
	MaxDistance = InMaxDistance;
	MaxDistanceLost = InMaxDistanceLost;
	InitializeCacheTest();
}
