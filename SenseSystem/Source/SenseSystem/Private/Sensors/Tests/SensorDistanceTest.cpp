//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/Tests/SensorDistanceTest.h"
#include "Sensors/SensorBase.h"
#include "Math/UnrealMathUtility.h"

#if WITH_EDITORONLY_DATA
	#include "DrawDebugHelpers.h"
	#include "SceneManagement.h"
#endif

USensorDistanceTest::USensorDistanceTest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bTestBySingleLocation = true;

	ScoreModifyCurve.GetRichCurve()->AddKey(0.f, 1.f, false);
	ScoreModifyCurve.GetRichCurve()->AddKey(1.f, 0.f, false);
	const float _key = MaxDistance / MaxDistanceLost;
	ScoreModifyCurve.GetRichCurve()->AddKey(_key, 0.1f, false);
}

#if WITH_EDITOR
void USensorDistanceTest::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
	InitializeCacheTest();
}
#endif

#if WITH_EDITORONLY_DATA
void USensorDistanceTest::DrawTest(const FSceneView* View, FPrimitiveDrawInterface* PDI) const
{
	if (MaxDistance > KINDA_SMALL_NUMBER)
	{
		DrawWireSphere(PDI, GetSensorTransform(), FColor::Yellow, MaxDistance, 16, SDPG_Foreground);
		if (FMath::Abs(MaxDistanceLost - MaxDistance) > KINDA_SMALL_NUMBER)
		{
			DrawWireSphere(PDI, GetSensorTransform(), FColor::Cyan, MaxDistanceLost, 16, SDPG_Foreground);
		}
	}
}

void USensorDistanceTest::DrawDebug(const float Duration) const
{
	#if ENABLE_DRAW_DEBUG
	if (MaxDistance > KINDA_SMALL_NUMBER && GetSensorOwner())
	{
		if (const UWorld* World = GetSensorOwner()->GetWorld())
		{
			DrawDebugSphere(World, GetSensorTransform().GetLocation(), MaxDistance, 16, FColor::Yellow, false, Duration, SDPG_Foreground, 1.5f);
			if (FMath::Abs(MaxDistanceLost - MaxDistance) > KINDA_SMALL_NUMBER)
			{
				DrawDebugSphere(World, GetSensorTransform().GetLocation(), MaxDistanceLost, 16, FColor::Cyan, false, Duration, SDPG_Foreground, 1.5f);
			}
		}
	}
	#endif
}
#endif


EUpdateReady USensorDistanceTest::GetReadyToTest()
{
	return Super::GetReadyToTest();
}

bool USensorDistanceTest::PreTest()
{
	Super::PreTest();

	if (ScoreModifyCurve.ExternalCurve)
	{
		ScoreModifyCurve.EditorCurveData = *(ScoreModifyCurve.GetRichCurve());
		ScoreModifyCurve.ExternalCurve = nullptr;
	}

	const FVector V = GetSensorTransform().GetLocation();
	AABB_Box = FBox(V - MaxDistanceLost, V + MaxDistanceLost); //AABB
	return true;
}

ESenseTestResult USensorDistanceTest::RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_DistanceTest);

	ESenseTestResult OutResult = ESenseTestResult::Lost;
	if (AABB_Box.IsInsideOrOn(TestLocation)) //AABB
	{
#if SENSESYSTEM_ENABLE_VECTORINTRINSICS

		const FVectorTransformConst T = FVectorTransformConst(GetSensorTransform());
		const VectorRegister Delta = VectorSubtract(T.GetLocation(), VectorLoadFloat3_W0(&TestLocation));
		const float DistSquared = VectorGetComponent(VectorDot3(Delta, Delta), 0);

#else

		const float DistSquared = (GetSensorTransform().GetLocation() - TestLocation).SizeSquared(); //Distance Squared

#endif

		if (DistSquared <= MaxDistanceLostSquared)
		{
			const float Dist = FMath::Sqrt(DistSquared);
			ScoreResult *= ModifyScoreByCurve(Dist / MaxDistanceLost);
			OutResult =										   //
				(MinScore > ScoreResult || Dist > MaxDistance) //
				? ESenseTestResult::NotLost					   //
				: ESenseTestResult::Sensed;					   //
		}
		else
		{
			ScoreResult = 0;
		}
	}
	return OutResult;
}

void USensorDistanceTest::InitializeCacheTest()
{
	if (MaxDistanceLost < MaxDistance)
	{
		MaxDistanceLost = MaxDistance;
	}
	MaxDistanceSquared = MaxDistance * MaxDistance;
	MaxDistanceLostSquared = MaxDistanceLost * MaxDistanceLost;
}
