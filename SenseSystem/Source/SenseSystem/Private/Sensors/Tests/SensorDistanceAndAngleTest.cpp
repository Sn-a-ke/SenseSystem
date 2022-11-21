//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/Tests/SensorDistanceAndAngleTest.h"
#include "Sensors/SensorBase.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/World.h"

#if WITH_EDITORONLY_DATA
	#include "DrawDebugHelpers.h"
	#include "SceneManagement.h"
#endif


#if SENSESYSTEM_ENABLE_VECTORINTRINSICS
static FORCEINLINE VectorRegister VectorRotateAngleAxis(const float AngleRad, const VectorRegister& V, const VectorRegister& Ax)
{
	float s, c;
	FMath::SinCos(&s, &c, 0.5f * AngleRad);
	const VectorRegister Quat = VectorMultiplyAdd(Ax, MakeVectorRegister(s, s, s, 0.f), MakeVectorRegister(0.f, 0.f, 0.f, c));
	return VectorQuaternionRotateVector(Quat, V);
}
#endif


USensorDistanceAndAngleTest::USensorDistanceAndAngleTest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bTestBySingleLocation = true;

	if (!DistanceCurve.ExternalCurve)
	{
		DistanceCurve.GetRichCurve()->AddKey(0.f, 1.f, false);
		DistanceCurve.GetRichCurve()->AddKey(1.f, 0.f, false);
		const float D_Key = MaxDistance / MaxDistanceLost;
		const float A_Key = MaxAngle / MaxAngleLost;
		DistanceCurve.GetRichCurve()->AddKey(D_Key, 0.1f, false);
		DistanceCurve.GetRichCurve()->AddKey(A_Key, 0.1f, false);
	}
	if (!AngleCurve.ExternalCurve)
	{
		AngleCurve.GetRichCurve()->AddKey(0.f, 1.f, false);
		AngleCurve.GetRichCurve()->AddKey(1.f, 0.f, false);
	}
}

#if WITH_EDITOR
void USensorDistanceAndAngleTest::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
	InitializeCacheTest();
}
#endif

void USensorDistanceAndAngleTest::SetDistanceAndAngleParam(
	const float Max_Angle,
	const float Max_AngleLost,
	const float Max_Distance,
	const float Max_DistanceLost,
	const float Min_Distance)
{
	MaxAngle = FMath::Clamp(FMath::Abs(Max_Angle), 0.f, 180.f);
	MaxAngleLost = FMath::Clamp(FMath::Abs(Max_AngleLost), 0.f, 180.f);
	MaxDistance = FMath::Abs(Max_Distance);
	MaxDistanceLost = FMath::Abs(Max_DistanceLost);
	MinDistance = FMath::Abs(Min_Distance);
	InitializeCacheTest();
}

bool USensorDistanceAndAngleTest::PreTest()
{
	Super::PreTest();

	if (DistanceCurve.ExternalCurve)
	{
		DistanceCurve.EditorCurveData = *(DistanceCurve.GetRichCurve());
		DistanceCurve.ExternalCurve = nullptr;
	}
	if (AngleCurve.ExternalCurve)
	{
		AngleCurve.EditorCurveData = *(AngleCurve.GetRichCurve());
		AngleCurve.ExternalCurve = nullptr;
	}

	const FTransform& Transform = GetSensorTransform();
	const FVector Forward = GetSensorTransform().GetUnitAxis(EAxis::X);
	const FQuat QRotation = FRotationMatrix::MakeFromX(Forward).ToQuat();

	//build AABB box

#if SENSESYSTEM_ENABLE_VECTORINTRINSICS

	const FVectorTransformConst T = FVectorTransformConst(Transform);
	TmpSelfForward = VectorLoadFloat3_W0(&Forward); //updt member
	VectorRegister Max = T.GetLocation();
	VectorRegister Min = Max;
	const VectorRegister Rotation = VectorLoadAligned(&QRotation);
	for (int32 i = 0; i < 4; ++i)
	{
		const VectorRegister RotatedVec = VectorQuaternionRotateVector(Rotation, AABB_Helper[i]);
		const VectorRegister TranslatedVec = VectorAdd(RotatedVec, T.GetLocation());
		Max = VectorMax(TranslatedVec, Max);
		Min = VectorMin(TranslatedVec, Min);
	}

	const VectorRegister MaxDist = VectorSetFloat1(MaxDistanceLost);
	const VectorRegister MinDist = VectorSetFloat1(MinDistance);

	const VectorRegister AABB_Helper_2[6] = //
	{
		MakeVectorRegister(00.f, 00.f,  1.f, 0.f), // FVector::UpVector
		MakeVectorRegister(00.f, 00.f, -1.f, 0.f), // FVector::DownVector
		MakeVectorRegister(00.f,  1.f, 00.f, 0.f), // FVector::RightVector
		MakeVectorRegister(00.f, -1.f, 00.f, 0.f), // FVector::LeftVector
		MakeVectorRegister( 1.f, 00.f, 00.f, 0.f), // FVector::ForwardVector
		MakeVectorRegister(-1.f, 00.f, 00.f, 0.f), // FVector::BackwardVector
	};

	for (int32 i = 0; i < 6; ++i)
	{
		const VectorRegister TranslatedVec2 = VectorMultiplyAdd(AABB_Helper_2[i], MinDist, T.GetLocation());
		Max = VectorMax(TranslatedVec2, Max);
		Min = VectorMin(TranslatedVec2, Min);

		const float CosAngle = VectorGetComponent(VectorDot3(TmpSelfForward, AABB_Helper_2[i]), 0);
		if (CosAngle >= MaxAngleLostCos)
		{
			const VectorRegister TranslatedVec1 = VectorMultiplyAdd(AABB_Helper_2[i], MaxDist, T.GetLocation());
			Max = VectorMax(TranslatedVec1, Max);
			Min = VectorMin(TranslatedVec1, Min);
		}
	}

	FVector MinV, MaxV;
	VectorStoreFloat3(Max, &MaxV);
	VectorStoreFloat3(Min, &MinV);
	AABB_Box = FBox(MinV, MaxV); //updt member

#else
	const FVector Loc = Transform.GetLocation();
	AABB_Box = FBox(Loc, Loc);
	TmpSelfForward = Forward;
	for (int32 i = 0; i < 4; ++i)
	{
		AABB_Box += Loc + QRotation.RotateVector(AABB_Helper[i]);
	}

	const FVector AABB_Helper_2[6] = //
		{
			FVector::UpVector,		//
			FVector::DownVector,	//
			FVector::RightVector,	//
			FVector::LeftVector,	//
			FVector::ForwardVector, //
			FVector::BackwardVector //
		};
	for (int32 i = 0; i < 6; ++i)
	{
		const float CosAngle = FVector::DotProduct(TmpSelfForward, AABB_Helper_2[i]);
		if (CosAngle >= MaxAngleLostCos)
		{
			AABB_Box += Transform.GetLocation() + (AABB_Helper_2[i] * MaxDistanceLost);
			AABB_Box += Transform.GetLocation() + (AABB_Helper_2[i] * MinDistance);
		}
	}

#endif

	return true;
}

ESenseTestResult USensorDistanceAndAngleTest::RunTestForLocation(const FSensedStimulus& SensedStimulus, const FVector& TestLocation, float& ScoreResult) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_DistanceAndAngleTest);

	ESenseTestResult OutResult = ESenseTestResult::Lost;
	if (AABB_Box.IsInsideOrOn(TestLocation))
	{
#if SENSESYSTEM_ENABLE_VECTORINTRINSICS

		const FVectorTransformConst T = FVectorTransformConst(GetSensorTransform());
		const VectorRegister Delta = VectorSubtract(VectorLoadFloat3_W0(&TestLocation), T.GetLocation());
		const float DistSquared = VectorGetComponent(VectorDot3(Delta, Delta), 0);

#else

		const FVector Delta = TestLocation - GetSensorTransform().GetLocation();
		const float DistSquared = Delta.SizeSquared();

#endif

		if (DistSquared <= MaxDistanceLostSquared)
		{
#if SENSESYSTEM_ENABLE_VECTORINTRINSICS

			VectorRegister Norm;
			if (DistSquared == 1.f)
			{
				Norm = Delta;
			}
			else if (DistSquared < KINDA_SMALL_NUMBER)
			{
				Norm = GlobalVectorConstants::Float1000;
			}
			else
			{
				const float InvSqrtDist = FMath::InvSqrt(DistSquared);
				Norm = VectorMultiply(Delta, VectorSetFloat1(InvSqrtDist));
			}
			const float CosAngle = VectorGetComponent(VectorDot3(TmpSelfForward, Norm), 0);

			//check(FMath::IsNearlyEqual(
			//	FVector::DotProduct(TmpSelfForward, NormalWithSizeSquared(TestLocation - Location, DistSquared)),
			//	CosAngle,
			//	KINDA_SMALL_NUMBER));

#else

			const float CosAngle = FVector::DotProduct(TmpSelfForward, NormalWithSizeSquared(Delta, DistSquared));

#endif

			if (CosAngle >= MaxAngleLostCos)
			{
				constexpr float Hpi = (180.0f) / PI;
				//QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_DistanceAndAngleTest_DistSqrtAngleAcos);
				const float Dist = FMath::Sqrt(DistSquared);
				const float Angle = FMath::Acos(CosAngle) * Hpi;

				//QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_DistanceAndAngleTest_Score);
				ScoreResult *= ModifyDistanceScore(Dist / MaxDistanceLost);
				if (MinDistanceSquared > 0.f && MinDistanceSquared < DistSquared)
				{
					ScoreResult *= ModifyAngleScore(Angle / MaxAngleLost);
				}
				else
				{
					ScoreResult *= ModifyAngleScore(Angle / 180.f);
				}

				OutResult =															   //
					(MinScore > ScoreResult || Dist > MaxDistance || Angle > MaxAngle) //
					? ESenseTestResult::NotLost										   //
					: ESenseTestResult::Sensed;										   //
			}
			else if (MinDistanceSquared > 0.f && MinDistanceSquared >= DistSquared)
			{
				const float Dist = FMath::Sqrt(DistSquared);
				ScoreResult *= ModifyDistanceScore(Dist / MaxDistanceLost);
				OutResult = ESenseTestResult::NotLost;
			}
		}
	}
	return OutResult;
}


void USensorDistanceAndAngleTest::InitializeCacheTest()
{
	Super::InitializeCacheTest();
	if (MaxDistanceLost < MaxDistance)
	{
		MaxDistanceLost = MaxDistance;
	}
	if (MaxAngleLost < MaxAngle)
	{
		MaxAngleLost = MaxAngle;
	}
	MinDistanceSquared = MinDistance * MinDistance;
	MaxDistanceSquared = MaxDistance * MaxDistance;
	MaxDistanceLostSquared = MaxDistanceLost * MaxDistanceLost;

	MaxAngleCos = FMath::Cos(MaxAngle * PI / (180.f));
	MaxAngleLostCos = FMath::Cos(MaxAngleLost * PI / (180.f));

#if SENSESYSTEM_ENABLE_VECTORINTRINSICS

	const float Angle = FMath::DegreesToRadians(MaxAngleLost);
	const VectorRegister Vec = MakeVectorRegister(MaxDistanceLost, 0.f, 0.f, 0.f);
	AABB_Helper[0] = VectorRotateAngleAxis(Angle, Vec, MakeVectorRegister(0.f, 01.f, 00.f, 0.f));
	AABB_Helper[1] = VectorRotateAngleAxis(Angle, Vec, MakeVectorRegister(0.f, -1.f, 00.f, 0.f));
	AABB_Helper[2] = VectorRotateAngleAxis(Angle, Vec, MakeVectorRegister(0.f, 00.f, 01.f, 0.f));
	AABB_Helper[3] = VectorRotateAngleAxis(Angle, Vec, MakeVectorRegister(0.f, 00.f, -1.f, 0.f));

#else

	const FVector V = FVector(MaxDistanceLost, 0.f, 0.f);
	AABB_Helper[0] = V.RotateAngleAxis(MaxAngleLost, FVector::RightVector);
	AABB_Helper[1] = V.RotateAngleAxis(MaxAngleLost, FVector::LeftVector);
	AABB_Helper[2] = V.RotateAngleAxis(MaxAngleLost, FVector::UpVector);
	AABB_Helper[3] = V.RotateAngleAxis(MaxAngleLost, FVector::DownVector);

#endif
}


#if WITH_EDITORONLY_DATA
void USensorDistanceAndAngleTest::DrawTest(const FSceneView* View, FPrimitiveDrawInterface* PDI) const
{
	if (MaxAngle > KINDA_SMALL_NUMBER)
	{
		constexpr uint8 Depth = SDPG_Foreground;

		FTransform Transform = GetSensorTransform();
		const FVector XDir = Transform.GetUnitAxis(EAxis::X);
		const FQuat QRotation = FRotationMatrix::MakeFromX(XDir).ToQuat();
		Transform.SetRotation(QRotation);

		const FQuat Q180 = FQuat(FVector::UpVector, PI);
		TArray<FVector> Verts;
		{
			const FColor Color = FColor::Yellow;
			float DrawMaxAngle = MaxAngle;
			FTransform T = Transform;
			if (DrawMaxAngle > 90.f)
			{
				DrawMaxAngle = 180.f - DrawMaxAngle;
				T.ConcatenateRotation(Q180);
			}

			DrawWireCone(PDI, Verts, T, MaxDistance, DrawMaxAngle, 12, Color, Depth);
			if (MinDistance > 0.f)
			{

				DrawWireSphere(PDI, Transform, FColor::Cyan, MinDistance, 12, SDPG_Foreground);
			}

			const int32 ArcCount = static_cast<int32>(Verts.Num() / 2);
			for (int32 i = 0; i < ArcCount; i += 3)
			{
				FVector Y = Verts[i] - Verts[ArcCount + i];
				Y.Normalize();
				DrawArc(PDI, T.GetTranslation(), XDir, Y, -MaxAngle, MaxAngle, MaxDistance, 10, Color, Depth);
			}
		}
		{
			Verts.Reset();
			const FColor Color = FColor::Cyan;
			FTransform T = Transform;
			float DrawMaxAngleLost = MaxAngleLost;
			if (DrawMaxAngleLost > 90.f)
			{
				DrawMaxAngleLost = 180.f - DrawMaxAngleLost;
				T.ConcatenateRotation(Q180);
			}

			DrawWireCone(PDI, Verts, T, MaxDistanceLost, DrawMaxAngleLost, 12, Color, Depth);

			const int32 ArcCount = static_cast<int32>(Verts.Num() / 2);
			for (int32 i = 0; i < ArcCount; i += 3)
			{
				FVector Y = Verts[i] - Verts[ArcCount + i];
				Y.Normalize();
				DrawArc(PDI, T.GetTranslation(), XDir, Y, -MaxAngleLost, MaxAngleLost, MaxDistanceLost, 10, Color, Depth);
			}
		}

		/*if(true) //draw flat
		{
			const FVector L = OriginalT.GetLocation();
			const float CosAngle = FVector::DotProduct(TmpSelfForward, FVector::UpVector);
			if (FMath::Abs(CosAngle) >= MaxAngleLostCos)
			{
				FVector Vr = L + R.RotateVector(AABB_Helper[2]);
				FVector Vl = L + R.RotateVector(AABB_Helper[3]);
				Vr.Z = L.Z;
				Vl.Z = L.Z;
				PDI->DrawLine(L, Vr, FColor::Red, Depth, 1.5f, 0.f, true);
				PDI->DrawLine(L, Vl, FColor::Red, Depth, 1.5f, 0.f, true);
				int32 Dir = 0;
			}
		}*/
	}
}

void USensorDistanceAndAngleTest::DrawDebug(const float Duration) const
{
	#if ENABLE_DRAW_DEBUG
	if (MaxAngle > KINDA_SMALL_NUMBER && GetSensorOwner())
	{
		if (const UWorld* World = GetSensorOwner()->GetWorld())
		{
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

				int32 ArcCount = 8;
				if (DrawMaxAngle > 90.f)
				{
					Forward = -Forward;
					DrawMaxAngle = 180.f - DrawMaxAngle;
					ArcCount = 16;
				}
				const FColor Color = FColor::Yellow;
				DrawMaxAngle = FMath::DegreesToRadians(DrawMaxAngle);
				DrawDebugCone(World, OriginalT.GetLocation(), Forward, MaxDistance, DrawMaxAngle, DrawMaxAngle, 12, Color, false, Duration, Depth, 1.5f);

				if (MinDistance > 0.f)
				{
					const FVector XAxis = OriginalT.GetRotation().GetAxisX();
					const FVector YAxis = OriginalT.GetRotation().GetAxisY();
					const FVector ZAxis = OriginalT.GetRotation().GetAxisZ();

					DrawDebugCircle(World, OriginalT.GetLocation(), MinDistance, 36, FColor::Cyan, false, Duration, SDPG_Foreground, 1.5f, XAxis, YAxis, false);
					DrawDebugCircle(World, OriginalT.GetLocation(), MinDistance, 36, FColor::Cyan, false, Duration, SDPG_Foreground, 1.5f, XAxis, ZAxis, false);
					DrawDebugCircle(World, OriginalT.GetLocation(), MinDistance, 36, FColor::Cyan, false, Duration, SDPG_Foreground, 1.5f, YAxis, ZAxis, false);
				}
				{
					const float Ang = MaxAngle * 2.f;
					const float StepAng = Ang / ArcCount;
					FVector StartHorizon = FVector::ForwardVector.RotateAngleAxis(MaxAngle, -FVector::UpVector) * MaxDistance;
					FVector StartVert = FVector::ForwardVector.RotateAngleAxis(MaxAngle, -FVector::RightVector) * MaxDistance;
					for (int32 i = 0; i < ArcCount; i++)
					{
						const FVector EndHorizon = StartHorizon.RotateAngleAxis(StepAng, FVector::UpVector);
						const FVector EndVert = StartVert.RotateAngleAxis(StepAng, FVector::RightVector);
						// clang-format off
						DrawDebugLine(World, OriginalT.TransformPositionNoScale(StartHorizon), OriginalT.TransformPositionNoScale(EndHorizon),
							Color, false, Duration, Depth, 1.5f);
						DrawDebugLine(World, OriginalT.TransformPositionNoScale(StartVert), OriginalT.TransformPositionNoScale(EndVert),
							Color, false, Duration, Depth, 1.5f);
						// clang-format on
						StartHorizon = EndHorizon;
						StartVert = EndVert;
					}
				}
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


				int32 ArcCount = 8;
				if (DrawMaxAngleLost > 90.f)
				{
					Forward = -Forward;
					DrawMaxAngleLost = 180.f - DrawMaxAngleLost;
					ArcCount = 16;
				}
				const FColor Color = FColor::Cyan;
				DrawMaxAngleLost = FMath::DegreesToRadians(DrawMaxAngleLost);
				// clang-format off
				DrawDebugCone(World, OriginalT.GetLocation(), Forward, MaxDistanceLost, DrawMaxAngleLost, DrawMaxAngleLost,
					12, Color, false, Duration, Depth, 1.5f);
				// clang-format on
				{
					const float Ang = MaxAngleLost * 2.f;
					const float StepAng = Ang / ArcCount;
					FVector StartHorizon = FVector::ForwardVector.RotateAngleAxis(MaxAngleLost, -FVector::UpVector) * MaxDistanceLost;
					FVector StartVert = FVector::ForwardVector.RotateAngleAxis(MaxAngleLost, -FVector::RightVector) * MaxDistanceLost;
					for (int32 i = 0; i < ArcCount; i++)
					{
						const FVector EndHorizon = StartHorizon.RotateAngleAxis(StepAng, FVector::UpVector);
						const FVector EndVert = StartVert.RotateAngleAxis(StepAng, FVector::RightVector);
						// clang-format off
						DrawDebugLine(World, OriginalT.TransformPositionNoScale(StartHorizon), OriginalT.TransformPositionNoScale(EndHorizon),
							Color, false, Duration, Depth, 1.5f);
						DrawDebugLine(World, OriginalT.TransformPositionNoScale(StartVert), OriginalT.TransformPositionNoScale(EndVert),
							Color, false, Duration, Depth, 1.5f);
						// clang-format on
						StartHorizon = EndHorizon;
						StartVert = EndVert;
					}
				}
			}
		}
	}
	#endif
}
#endif
