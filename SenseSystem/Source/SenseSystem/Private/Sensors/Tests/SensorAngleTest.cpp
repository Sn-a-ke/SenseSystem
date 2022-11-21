//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/Tests/SensorAngleTest.h"
#include "Sensors/SensorBase.h"
#include "Math/UnrealMathUtility.h"

#if WITH_EDITORONLY_DATA
	#include "DrawDebugHelpers.h"
	#include "SceneManagement.h"
#endif


USensorAngleTest::USensorAngleTest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bTestBySingleLocation = true;

	ScoreModifyCurve.GetRichCurve()->AddKey(0.f, 1.f, false);
	ScoreModifyCurve.GetRichCurve()->AddKey(1.f, 0.f, false);
}

#if WITH_EDITOR
void USensorAngleTest::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
	const FName PropertyName = (e.Property != nullptr) ? e.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(USensorAngleTest, MaxAngle) || PropertyName == GET_MEMBER_NAME_CHECKED(USensorAngleTest, MaxAngleLost))
	{
		if (MaxAngleLost < MaxAngle)
		{
			MaxAngleLost = MaxAngle;
		}
	}
}
#endif

void USensorAngleTest::SetAngleParam(const float Max_Angle, const float Max_AngleLost)
{
	MaxAngle = FMath::Clamp(FMath::Abs(Max_Angle), 0.f, 180.f);
	MaxAngleLost = FMath::Clamp(FMath::Abs(Max_AngleLost), 0.f, 180.f);
	InitializeCacheTest();
}

EUpdateReady USensorAngleTest::GetReadyToTest()
{
	return Super::GetReadyToTest();
}


bool USensorAngleTest::PreTest()
{
	Super::PreTest();

	if (ScoreModifyCurve.ExternalCurve)
	{
		ScoreModifyCurve.EditorCurveData = *(ScoreModifyCurve.GetRichCurve());
		ScoreModifyCurve.ExternalCurve = nullptr;
	}

	const FTransform& T = GetSensorTransform();


#if SENSESYSTEM_ENABLE_VECTORINTRINSICS

	const FVector Forward = T.GetRotation().GetForwardVector();
	TmpSelfForward = VectorLoadFloat3_W0(&Forward);

#else

	TmpSelfForward = GetSensorTransform().GetRotation().GetForwardVector();

#endif

	return true;
}


ESenseTestResult USensorAngleTest::RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_AngleTest);

#if SENSESYSTEM_ENABLE_VECTORINTRINSICS
	// in range 10000 test  (VECTORINTRINSICS : 0.034500-Milliseconds) VS (Base : 0.080800-Milliseconds)
	const FVectorTransformConst T = FVectorTransformConst(GetSensorTransform());
	const VectorRegister Delta = VectorSubtract(VectorLoadFloat3_W0(&TestLocation), T.GetLocation());
	const float CosAngle = VectorGetComponent(VectorDot3(TmpSelfForward, VectorNormalize(Delta)), 0);

#else

	const FVector Delta = TestLocation - GetSensorTransform().GetLocation();
	const float CosAngle = FVector::DotProduct(TmpSelfForward, Delta.GetSafeNormal());

#endif

	if (CosAngle >= MaxAngleLostCos)
	{
		constexpr float Hpi = (180.0f) / PI;
		const float Angle = FMath::Acos(CosAngle) * Hpi;
		ScoreResult *= ModifyScoreByCurve(Angle / MaxAngleLost);

		return (MinScore > ScoreResult || Angle > MaxAngle) //
			? ESenseTestResult::NotLost						//
			: ESenseTestResult::Sensed;						//
	}
	return ESenseTestResult::Lost;
}

void USensorAngleTest::InitializeCacheTest()
{
	Super::InitializeCacheTest();
	constexpr float DegToRad = PI / (180.f);
	MaxAngleCos = FMath::Cos(MaxAngle * DegToRad);
	MaxAngleLostCos = FMath::Cos(MaxAngleLost * DegToRad);
}


#if WITH_EDITORONLY_DATA
void USensorAngleTest::DrawTest(const FSceneView* View, FPrimitiveDrawInterface* PDI) const
{
	if (MaxAngle > 0.0f)
	{
		const FTransform OriginalT = GetSensorTransform();
		TArray<FVector> Verts;

		{
			FTransform T = OriginalT;
			float DrawMaxAngle = MaxAngle;
			if (DrawMaxAngle > 90.f)
			{
				DrawMaxAngle = 180.f - DrawMaxAngle;
				T.ConcatenateRotation(FQuat(FVector::UpVector, PI));
			}
			DrawWireCone(PDI, Verts, T, DrawDistance, DrawMaxAngle, 12, FColor::Yellow, SDPG_Foreground);
		}

		if (FMath::Abs(MaxAngleLost - MaxAngle) > KINDA_SMALL_NUMBER)
		{
			Verts.Reset();
			FTransform T = OriginalT;
			float DrawMaxAngleLost = MaxAngleLost;
			if (DrawMaxAngleLost > 90.f)
			{
				DrawMaxAngleLost = 180.f - DrawMaxAngleLost;
				T.ConcatenateRotation(FQuat(FVector::UpVector, PI));
			}
			DrawWireCone(PDI, Verts, T, DrawDistance, DrawMaxAngleLost, 12, FColor::Cyan, SDPG_Foreground);
		}
	}
}

void USensorAngleTest::DrawDebug(const float Duration) const
{
	#if ENABLE_DRAW_DEBUG
	if (MaxAngle > KINDA_SMALL_NUMBER && GetSensorOwner())
	{
		if (const UWorld* World = GetSensorOwner()->GetWorld())
		{
			const float DrawDist = DrawDistance;
			constexpr uint8 Depth = SDPG_Foreground;

			FTransform OriginalT = GetSensorTransform();
			const FVector X = OriginalT.GetUnitAxis(EAxis::X);
			const FQuat R = FRotationMatrix::MakeFromX(X).ToQuat();
			OriginalT.SetRotation(R);

			{
		#if SENSESYSTEM_ENABLE_VECTORINTRINSICS
				FVector Forward;
				VectorStoreFloat3(TmpSelfForward, &Forward);
		#else
				FVector Forward = TmpSelfForward;
		#endif

				float DrawMaxAngle = MaxAngle;
				if (DrawMaxAngle > 90.f)
				{
					Forward = -Forward;
					DrawMaxAngle = 180.f - DrawMaxAngle;
				}
				const FColor Color = FColor::Yellow;
				DrawMaxAngle = FMath::DegreesToRadians(DrawMaxAngle);
				DrawDebugCone(World, OriginalT.GetLocation(), Forward, DrawDist, DrawMaxAngle, DrawMaxAngle, 12, Color, false, Duration, Depth, 1.5f);
			}

			if (FMath::Abs(MaxAngleLost - MaxAngle) > KINDA_SMALL_NUMBER)
			{
		#if SENSESYSTEM_ENABLE_VECTORINTRINSICS
				FVector Forward;
				VectorStoreFloat3(TmpSelfForward, &Forward);
		#else
				FVector Forward = TmpSelfForward;
		#endif

				float DrawMaxAngleLost = MaxAngleLost;
				if (DrawMaxAngleLost > 90.f)
				{
					Forward = -Forward;
					DrawMaxAngleLost = 180.f - DrawMaxAngleLost;
				}
				const FColor Color = FColor::Cyan;
				DrawMaxAngleLost = FMath::DegreesToRadians(DrawMaxAngleLost);
				DrawDebugCone(World, OriginalT.GetLocation(), Forward, DrawDist, DrawMaxAngleLost, DrawMaxAngleLost, 12, Color, false, Duration, Depth, 1.5f);
			}
		}
	}
	#endif
}
#endif
