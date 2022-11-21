//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/Tests/SensorBoxTest.h"
#include "Sensors/SensorBase.h"

#if WITH_EDITORONLY_DATA
	#include "DrawDebugHelpers.h"
	#include "SceneManagement.h"
#endif

USensorBoxTest::USensorBoxTest()
{
	//ScoreModifyCurve.GetRichCurve()->AddKey(0.f, 1.f, false);
	//ScoreModifyCurve.GetRichCurve()->AddKey(1.f, 1.f, false);
}

void USensorBoxTest::InitAABBBoxAndSensorTransform()
{
	if (bOrientedBox)
	{
		if (bIgnoreVerticalRotation)
		{
			FVector Forward = SensorTransform.GetRotation().GetForwardVector();
			Forward.Z = 0.f;
			const FQuat Q = FRotationMatrix::MakeFromX(Forward.GetSafeNormal()).ToQuat();
			SensorTransform.SetRotation(Q);
		}

		const FTransform& T = GetSensorTransform();
		const FVector Loc = T.GetLocation();
		AABB_Box = FBox(Loc, Loc);

		AABB_Box += T.TransformPositionNoScale(BoxExtent);
		AABB_Box += T.TransformPositionNoScale(FVector(BoxExtent.X, -BoxExtent.Y, BoxExtent.Z));
		AABB_Box += T.TransformPositionNoScale(FVector(BoxExtent.X, BoxExtent.Y, -BoxExtent.Z));
		AABB_Box += T.TransformPositionNoScale(FVector(BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z));

		AABB_Box += T.TransformPositionNoScale(-BoxExtent);
		AABB_Box += T.TransformPositionNoScale(FVector(-BoxExtent.X, -BoxExtent.Y, BoxExtent.Z));
		AABB_Box += T.TransformPositionNoScale(FVector(-BoxExtent.X, BoxExtent.Y, -BoxExtent.Z));
		AABB_Box += T.TransformPositionNoScale(FVector(-BoxExtent.X, BoxExtent.Y, BoxExtent.Z));
	}
	else
	{
		AABB_Box = FBox::BuildAABB(GetSensorTransform().GetLocation(), BoxExtent);
	}
}

void USensorBoxTest::SetBoxParam(const FVector Extent, const bool OrientedBox, const bool IgnoreVerticalRotation, const bool Score)
{
	BoxExtent = Extent;
	bOrientedBox = OrientedBox;
	bIgnoreVerticalRotation = IgnoreVerticalRotation;
	bScore = Score;
	InitAABBBoxAndSensorTransform();
	InitializeCacheTest();
}

void USensorBoxTest::InitializeCacheTest()
{
	Super::InitializeCacheTest();
	if (bScore)
	{
		MaxManhattanDistance = BoxExtent.X + BoxExtent.Y + BoxExtent.Z;
	}
}

EUpdateReady USensorBoxTest::GetReadyToTest()
{
	if (!AABB_Box.GetExtent().Equals(FVector::ZeroVector, KINDA_SMALL_NUMBER))
	{
		return Super::GetReadyToTest();
	}
	return EUpdateReady::Fail;
}

void USensorBoxTest::InitializeFromSensor()
{
	check(GetOuter() == GetSensorOwner());
	SensorTransform = GetSensorOwner()->GetSensorTransform();
	InitAABBBoxAndSensorTransform();
	InitializeCacheTest();

	if (IsValid(GetSensorOwner()))
	{
		InitializeFromSensorBP(GetSensorOwner());
	}
}

bool USensorBoxTest::PreTest()
{
	Super::PreTest();
	InitAABBBoxAndSensorTransform();
	return true;
}

ESenseTestResult USensorBoxTest::RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_BoxTest);
	if (AABB_Box.IsInsideOrOn(TestLocation)) //AABB
	{
		FVector TestLoc;
		if (bOrientedBox)
		{
			TestLoc = GetSensorTransform().InverseTransformPositionNoScale(TestLocation);
			if (TestLoc.X > BoxExtent.X || TestLoc.X < -BoxExtent.X ||
				TestLoc.Y > BoxExtent.Y || TestLoc.Y < -BoxExtent.Y ||
				TestLoc.Z > BoxExtent.Z || TestLoc.Z < -BoxExtent.Z)
			{
				return ESenseTestResult::Lost;
			}
		}
		else if (bScore)
		{
			TestLoc = TestLocation - GetSensorTransform().GetLocation();
		}

		if (bScore)
		{
			ScoreResult = 1.f - (FMath::Abs(TestLoc.X) + FMath::Abs(TestLoc.Y) + FMath::Abs(TestLoc.Z)) / MaxManhattanDistance;
			return (MinScore > ScoreResult) //
				? ESenseTestResult::NotLost //
				: ESenseTestResult::Sensed; //
		}

		return ESenseTestResult::Sensed;
	}
	return ESenseTestResult::Lost;
}


#if WITH_EDITORONLY_DATA
void USensorBoxTest::DrawTest(const FSceneView* View, FPrimitiveDrawInterface* PDI) const
{
	if (!BoxExtent.Equals(FVector::ZeroVector, KINDA_SMALL_NUMBER))
	{
		const FTransform T = GetSensorTransform();
		const FVector Loc = T.GetLocation();
		const FQuat Q = bOrientedBox ? T.GetRotation() : FQuat::Identity;
		const auto BoxColor = FColor::Yellow;
		DrawOrientedWireBox(PDI, Loc, Q.GetForwardVector(), Q.GetRightVector(), Q.GetUpVector(), BoxExtent, BoxColor, SDPG_World);
	}
}

void USensorBoxTest::DrawDebug(const float Duration) const
{
	#if ENABLE_DRAW_DEBUG
	if (!BoxExtent.Equals(FVector::ZeroVector, KINDA_SMALL_NUMBER))
	{
		if (const UWorld* World = GetSensorOwner()->GetWorld())
		{
			const FTransform T = GetSensorTransform();
			const FVector Loc = T.GetLocation();
			const FQuat Q = T.GetRotation();
			const auto BoxColor = FColor::Yellow;
			const uint8 DepthPriorityGroup = SDPG_Foreground;

			DrawDebugBox(World, Loc, BoxExtent, Q, BoxColor, false, Duration, DepthPriorityGroup, 1.5f);
		}
	}
	#endif
}
#endif