//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sensors/Tests/SensorLocationTestBase.h"
#include "Curves/CurveFloat.h"
#include "Containers/StaticArray.h"

#include "SensorDistanceAndAngleTest.generated.h"


/**
 * DistanceAndAngleTest
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class SENSESYSTEM_API USensorDistanceAndAngleTest : public USensorLocationTestBase
{
	GENERATED_BODY()

public:
	USensorDistanceAndAngleTest(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

#if WITH_EDITORONLY_DATA
	virtual void DrawTest(const FSceneView* View, FPrimitiveDrawInterface* PDI) const override;
	virtual void DrawDebug(float Duration) const override;
#endif


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDistance = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDistanceLost = 1500;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
	float MaxAngle = 62.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
	float MaxAngleLost = 120.f;

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest")
	void SetDistanceAndAngleParam(float Max_Angle, float Max_AngleLost, float Max_Distance, float Max_DistanceLost, float Min_Distance = 0.f);


	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "SensorTest")
	FRuntimeFloatCurve DistanceCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "SensorTest")
	FRuntimeFloatCurve AngleCurve;

	float ModifyDistanceScore(float Value) const;
	float ModifyAngleScore(float Value) const;

	virtual bool PreTest() override;

	virtual FBox GetSensorTestBoundBox() const override { return AABB_Box; }
	virtual float GetSensorTestRadius() const override { return MaxDistanceLost; }

protected:
	virtual ESenseTestResult RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const override;

	virtual void InitializeCacheTest() override;

	FBox AABB_Box;
	float MinDistanceSquared;
	float MaxDistanceSquared;
	float MaxDistanceLostSquared;
	float MaxAngleCos;
	float MaxAngleLostCos;

#if SENSESYSTEM_ENABLE_VECTORINTRINSICS

	VectorRegister TmpSelfForward = GlobalVectorConstants::Float1000; // PreTest updt
	TStaticArray<VectorRegister, 4> AABB_Helper;					  // InitializeCacheTest updt

#else

	FVector TmpSelfForward;
	TStaticArray<FVector, 4> AABB_Helper;

#endif
};

FORCEINLINE float USensorDistanceAndAngleTest::ModifyDistanceScore(const float Value) const
{
	return DistanceCurve.EditorCurveData.Eval(FMath::Clamp(Value, 0.f, 1.f));
}
FORCEINLINE float USensorDistanceAndAngleTest::ModifyAngleScore(const float Value) const
{
	return AngleCurve.EditorCurveData.Eval(FMath::Clamp(Value, 0.f, 1.f));
}
