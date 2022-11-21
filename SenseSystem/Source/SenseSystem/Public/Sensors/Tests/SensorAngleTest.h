//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Sensors/Tests/SensorLocationTestBase.h"

#include "SensorAngleTest.generated.h"

/**
 * AngleTest
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class SENSESYSTEM_API USensorAngleTest : public USensorLocationTestBase
{
	GENERATED_BODY()

public:
	USensorAngleTest(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

#if WITH_EDITORONLY_DATA
	virtual void DrawTest(const FSceneView* View, FPrimitiveDrawInterface* PDI) const override;
	virtual void DrawDebug(float Duration) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
	float MaxAngle = 62;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
	float MaxAngleLost = 120;

#if WITH_EDITORONLY_DATA
	/** EDITORONLY */
	UPROPERTY(EditAnywhere, Category = "SensorTest")
	float DrawDistance = 1000.f;
#endif

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest")
	void SetAngleParam(float Max_Angle, float Max_AngleLost);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "SensorTest")
	FRuntimeFloatCurve ScoreModifyCurve;

	float ModifyScoreByCurve(float Value) const;

	virtual EUpdateReady GetReadyToTest() override;
	virtual bool PreTest() override;

protected:
	virtual ESenseTestResult RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const override;

	virtual void InitializeCacheTest() override;

	FVector TmpSelfForward;

	float MaxAngleCos;
	float MaxAngleLostCos;
};

FORCEINLINE float USensorAngleTest::ModifyScoreByCurve(const float Value) const
{
	return ScoreModifyCurve.EditorCurveData.Eval(FMath::Clamp(Value, 0.f, 1.f));
}