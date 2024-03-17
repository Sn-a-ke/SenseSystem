//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Sensors/Tests/SensorLocationTestBase.h"

#include "FrustumTest.generated.h"


struct FFrustumTestData final
{
	FFrustumTestData() {}
	FFrustumTestData(FVector2D Point1, FVector2D Point2, FVector2D ViewportSize, float FOVAngle, float FarPlaneDistance);
	FFrustumTestData(float FOVAngle, float AspectRatio, float FarPlaneDistance);

	float MaxRadius = 0.f;
	float MaxCosAngle = -1.f;
	FVector::FReal MaxManhattanDistance = 0.f;

	FVector2D Center = FVector2D::ZeroVector;
	FBox2D Bound = FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);

	FORCEINLINE bool IsInsideBound(const FVector& In) const { return IsInsideBound(FVector2D(In.Y, In.Z)); }
	FORCEINLINE bool IsInsideBound(const FVector2D In) const { return Bound.IsInside(In); }

private:
	void InitDefault(float FarDistance);
};


/**
 * Camera Frustum Test
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class SENSESYSTEM_API UFrustumTest : public USensorLocationTestBase
{
	GENERATED_BODY()
public:
	UFrustumTest(const FObjectInitializer& ObjectInitializer);
	virtual ~UFrustumTest() override;
	virtual void BeginDestroy() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

#if WITH_EDITORONLY_DATA
	virtual void DrawTest(const FSceneView* View, FPrimitiveDrawInterface* PDI) const override;
	virtual void DrawDebug(float Duration) const override;
#endif


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest", meta = (ClampMin = "0.001", ClampMax = "360.0", UIMin = "0.001", UIMax = "360.0"))
	float FOVAngle = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float AspectRatio = 1.7777f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float FarPlaneDistance = 1000.f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest")
	bool bScoreDistance = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SensorTest", meta = (EditCondition = "bScore"))
	bool bScoreScreenDistance = true;


	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest")
	void SetFrustumParam(float FieldOfView, float Aspect_Ratio, float FarDistancePlane);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SensorTest")
	void SetFrustumSelectionRectangleParam(FVector2D Point1, FVector2D Point2, FVector2D ViewportSize, float FOV, float FarDistance);

	/*USensorTestBase*/
	virtual void InitializeFromSensor() override;
	virtual EUpdateReady GetReadyToTest() override;
	virtual bool PreTest() override;
	virtual ESenseTestResult RunTest(FSensedStimulus& SensedStimulus) const override;

	virtual FBox GetSensorTestBoundBox() const override { return AABB_Box; }
	virtual float GetSensorTestRadius() const override { return TmpData.MaxRadius; }

protected:
	virtual void InitializeCacheTest() override;
	virtual ESenseTestResult RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const override;

private:
	float ScoreByScreenSpaceManhattanDistance(const FVector& IntersectPoint) const;

	FFrustumTestData TmpData;
	FBox AABB_Box;

	FVector TmpSelfForward = FVector::ForwardVector;

};

FORCEINLINE float UFrustumTest::ScoreByScreenSpaceManhattanDistance(const FVector& IntersectPoint) const
{
	const FVector::FReal AbsX = FMath::Abs(IntersectPoint.Y - TmpData.Center.X);
	const FVector::FReal AbsY = FMath::Abs(IntersectPoint.Z - TmpData.Center.Y);
	return 1.f - ((AbsX + AbsY) / TmpData.MaxManhattanDistance);
}