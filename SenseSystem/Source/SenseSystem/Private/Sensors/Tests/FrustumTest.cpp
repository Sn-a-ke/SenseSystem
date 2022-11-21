//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/Tests/FrustumTest.h"
#include "Engine/Engine.h"
#include "UObject/Object.h"
#include "Math/UnrealMathUtility.h"
#include "SensedStimulStruct.h"
#include "Sensors/SensorBase.h"
#include "Sensors/Tests/SensorTestBase.h"

#if WITH_EDITORONLY_DATA
	#include "DrawDebugHelpers.h"
	#include "SceneManagement.h"
#endif


UFrustumTest::UFrustumTest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bTestBySingleLocation = true;
	TmpData = FFrustumTestData(FOVAngle, AspectRatio, FarPlaneDistance);
}

UFrustumTest::~UFrustumTest()
{}

void UFrustumTest::BeginDestroy()
{
	Super::BeginDestroy();
}

#if WITH_EDITOR
void UFrustumTest::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	USensorTestBase::PostEditChangeProperty(e);
	SetFrustumParam(FOVAngle, AspectRatio, FarPlaneDistance);
	TmpData = FFrustumTestData(FOVAngle, AspectRatio, FarPlaneDistance);
	InitializeCacheTest();
}
#endif

#if WITH_EDITORONLY_DATA
void UFrustumTest::DrawTest(const FSceneView* View, FPrimitiveDrawInterface* PDI) const
{
	if (FarPlaneDistance > KINDA_SMALL_NUMBER && FOVAngle > 1.f)
	{
		const auto FrustumColor = FColor::Yellow;
		constexpr uint8 DepthPriorityGroup = SDPG_Foreground;
		const FTransform& T = GetSensorTransform();
		FVector Verts[5];
		Verts[0] = T.GetLocation();
		Verts[1] = T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Max.X, TmpData.Bound.Max.Y));
		Verts[2] = T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Max.X, TmpData.Bound.Min.Y));
		Verts[3] = T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Min.X, TmpData.Bound.Min.Y));
		Verts[4] = T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Min.X, TmpData.Bound.Max.Y));

		PDI->DrawLine(Verts[0], Verts[1], FrustumColor, DepthPriorityGroup);
		PDI->DrawLine(Verts[0], Verts[2], FrustumColor, DepthPriorityGroup);
		PDI->DrawLine(Verts[0], Verts[3], FrustumColor, DepthPriorityGroup);
		PDI->DrawLine(Verts[0], Verts[4], FrustumColor, DepthPriorityGroup);

		PDI->DrawLine(Verts[1], Verts[2], FrustumColor, DepthPriorityGroup);
		PDI->DrawLine(Verts[2], Verts[3], FrustumColor, DepthPriorityGroup);
		PDI->DrawLine(Verts[3], Verts[4], FrustumColor, DepthPriorityGroup);
		PDI->DrawLine(Verts[4], Verts[1], FrustumColor, DepthPriorityGroup);
	}
}
void UFrustumTest::DrawDebug(const float Duration) const
{
	#if ENABLE_DRAW_DEBUG
	if (FarPlaneDistance > KINDA_SMALL_NUMBER && FOVAngle > 1.f && GetSensorOwner())
	{
		if (const UWorld* World = GetSensorOwner()->GetWorld())
		{
			const auto FrustumColor = FColor::Yellow;
			constexpr uint8 DepthPriorityGroup = SDPG_Foreground;
			const FTransform& T = GetSensorTransform();
			FVector Verts[5];
			Verts[0] = T.GetLocation();
			Verts[1] = T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Max.X, TmpData.Bound.Max.Y));
			Verts[2] = T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Max.X, TmpData.Bound.Min.Y));
			Verts[3] = T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Min.X, TmpData.Bound.Min.Y));
			Verts[4] = T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Min.X, TmpData.Bound.Max.Y));

			DrawDebugLine(World, Verts[0], Verts[1], FrustumColor, false, Duration, DepthPriorityGroup, 1.5f);
			DrawDebugLine(World, Verts[0], Verts[2], FrustumColor, false, Duration, DepthPriorityGroup, 1.5f);
			DrawDebugLine(World, Verts[0], Verts[3], FrustumColor, false, Duration, DepthPriorityGroup, 1.5f);
			DrawDebugLine(World, Verts[0], Verts[4], FrustumColor, false, Duration, DepthPriorityGroup, 1.5f);

			DrawDebugLine(World, Verts[1], Verts[2], FrustumColor, false, Duration, DepthPriorityGroup, 1.5f);
			DrawDebugLine(World, Verts[2], Verts[3], FrustumColor, false, Duration, DepthPriorityGroup, 1.5f);
			DrawDebugLine(World, Verts[3], Verts[4], FrustumColor, false, Duration, DepthPriorityGroup, 1.5f);
			DrawDebugLine(World, Verts[4], Verts[1], FrustumColor, false, Duration, DepthPriorityGroup, 1.5f);
		}
	}
	#endif
}
#endif

void UFrustumTest::SetFrustumParam(const float FieldOfView, const float Aspect_Ratio, const float FarDistancePlane)
{
	FOVAngle = FieldOfView;
	AspectRatio = Aspect_Ratio;
	FarPlaneDistance = FarDistancePlane;

	TmpData = FFrustumTestData(FOVAngle, AspectRatio, FarPlaneDistance);
	InitializeCacheTest();
}

void UFrustumTest::SetFrustumSelectionRectangleParam(
	const FVector2D Point1,
	const FVector2D Point2,
	const FVector2D ViewportSize,
	const float FOV,
	const float FarDistance)
{
	AspectRatio = ViewportSize.X / ViewportSize.Y;
	FOVAngle = FOV;
	FarPlaneDistance = FarDistance;

	TmpData = FFrustumTestData(Point1, Point2, ViewportSize, FOVAngle, FarPlaneDistance);
	InitializeCacheTest();
}

void UFrustumTest::InitializeFromSensor()
{
	check(GetOuter() == GetSensorOwner());
	SensorTransform = GetSensorOwner()->GetSensorTransform();

	SetFrustumParam(FOVAngle, AspectRatio, FarPlaneDistance);
	InitializeCacheTest();

	if (IsValid(GetSensorOwner()))
	{
		InitializeFromSensorBP(GetSensorOwner());
	}
}

void UFrustumTest::InitializeCacheTest()
{
	Super::InitializeCacheTest();
}


EUpdateReady UFrustumTest::GetReadyToTest()
{
	if (FarPlaneDistance > KINDA_SMALL_NUMBER && FOVAngle > KINDA_SMALL_NUMBER)
	{
		return Super::GetReadyToTest();
	}
	return EUpdateReady::Fail;
}

bool UFrustumTest::PreTest()
{
	Super::PreTest();

	const FTransform& T = GetSensorTransform();

	TmpSelfForward = T.GetRotation().GetForwardVector();

	const FVector L = T.GetLocation();
	AABB_Box = FBox(L, L);
	AABB_Box += T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Max.X, TmpData.Bound.Max.Y));
	AABB_Box += T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Min.X, TmpData.Bound.Min.Y));
	AABB_Box += T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Max.X, TmpData.Bound.Min.Y));
	AABB_Box += T.TransformPositionNoScale(FVector(FarPlaneDistance, TmpData.Bound.Min.X, TmpData.Bound.Max.Y));

	return true;
}

ESenseTestResult UFrustumTest::RunTest(FSensedStimulus& SensedStimulus) const
{
	return Super::RunTest(SensedStimulus);
}

ESenseTestResult UFrustumTest::RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_FrustumTest);
	if (AABB_Box.IsInsideOrOn(TestLocation)) //AABB
	{
		const FVector Delta = TestLocation - GetSensorTransform().GetLocation();
		const float Dot = FVector::DotProduct(TmpSelfForward, Delta);

		if (Dot > 0.f && Dot <= FarPlaneDistance)
		{
			const FVector RelativeReceiverLoc = GetSensorTransform().InverseTransformVectorNoScale(Delta);
			const FVector::FReal RelativeReceiverLocX_Far = FarPlaneDistance / RelativeReceiverLoc.X;
			const FVector IntersectPoint = RelativeReceiverLocX_Far * RelativeReceiverLoc;

			if (TmpData.IsInsideBound(IntersectPoint))
			{
				float LocScore = 0.f;
				if (bScoreDistance)
				{
					LocScore += RelativeReceiverLocX_Far;
				}
				if (bScoreScreenDistance)
				{
					const float S = ScoreByScreenSpaceManhattanDistance(IntersectPoint);
					LocScore = bScoreDistance ? (LocScore + S) * 0.5f : S;
				}
				ScoreResult *= LocScore;

				return (MinScore > ScoreResult) ? ESenseTestResult::NotLost : ESenseTestResult::Sensed;
			}
		}
	}
	ScoreResult = 0;
	return ESenseTestResult::Lost;
}


FFrustumTestData::FFrustumTestData(
	const FVector2D Point1,
	const FVector2D Point2,
	const FVector2D ViewportSize,
	const float FOVAngle,
	const float FarPlaneDistance)
{
	const float AspectRatio = ViewportSize.X / ViewportSize.Y;
	const float HozHalfAngleInRadians = FMath::DegreesToRadians(FOVAngle * 0.5f);

	if (FOVAngle > 0 && AspectRatio > KINDA_SMALL_NUMBER)
	{
		const float BoundX = FarPlaneDistance * FMath::Tan(HozHalfAngleInRadians);
		const float BoundY = BoundX / AspectRatio;

		Bound.Max.X = FMath::GetMappedRangeValueUnclamped(FVector2D(0.0, ViewportSize.X), FVector2D(-1.0, 1.0), Point1.X) * BoundX;
		Bound.Max.Y = FMath::GetMappedRangeValueUnclamped(FVector2D(0.0, ViewportSize.Y), FVector2D(1.0, -1.0), Point1.Y) * BoundY;

		Bound.Min.X = FMath::GetMappedRangeValueUnclamped(FVector2D(0.0, ViewportSize.X), FVector2D(-1.0, 1.0), Point2.X) * BoundX;
		Bound.Min.Y = FMath::GetMappedRangeValueUnclamped(FVector2D(0.0, ViewportSize.Y), FVector2D(1.0, -1.0), Point2.Y) * BoundY;
	}

	InitDefault(FarPlaneDistance);
}

FFrustumTestData::FFrustumTestData(const float FOVAngle, const float AspectRatio, const float FarPlaneDistance)
{
	const float HozHalfAngleInRadians = FMath::DegreesToRadians(FOVAngle * 0.5f);
	if (FOVAngle > 0)
	{
		Bound.Max.X = FarPlaneDistance * FMath::Tan(HozHalfAngleInRadians);
		Bound.Max.Y = Bound.Max.X / AspectRatio;
		Bound.Min = (-Bound.Max);
	}

	InitDefault(FarPlaneDistance);
}

void FFrustumTestData::InitDefault(const float FarDistance)
{
	{
		const FVector2D TmpMin = Bound.Min;

		Bound.Min.X = FMath::Min(Bound.Min.X, Bound.Max.X);
		Bound.Min.Y = FMath::Min(Bound.Min.Y, Bound.Max.Y);
		Bound.Max.X = FMath::Max(Bound.Max.X, TmpMin.X);
		Bound.Max.Y = FMath::Max(Bound.Max.Y, TmpMin.Y);
		Center = Bound.GetCenter();
		const FVector2D Extent = Bound.GetExtent();
		MaxManhattanDistance = Extent.X + Extent.Y;
	}
	{
		const FVector V1(FarDistance, Bound.Max.X, Bound.Max.Y);
		const FVector V2(FarDistance, Bound.Min.X, Bound.Min.Y);
		const FVector& VMax = Bound.Max.SizeSquared() > Bound.Min.SizeSquared() ? V1 : V2;
		MaxRadius = VMax.Size();
	}
	{
		const FVector2D::FReal MaxX = FMath::Max(FMath::Abs(Bound.Max.X), FMath::Abs(Bound.Min.X));
		const FVector2D::FReal MaxY = FMath::Max(FMath::Abs(Bound.Max.Y), FMath::Abs(Bound.Min.Y));
		MaxCosAngle = FVector::DotProduct(FVector::ForwardVector, FVector(FarDistance, MaxX, MaxY).GetSafeNormal());
	}
}
