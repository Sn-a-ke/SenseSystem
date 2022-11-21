//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/Tests/SensorTraceTestBase.h"
#include "SenseReceiverComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Sensors/SensorBase.h"
#include "Sensors/Tests/SensorTestBase.h"
#include "SensedStimulStruct.h"
#include "SenseStimulusBase.h"
#include "SenseObstacleInterface.h"
#include "UObject/GarbageCollection.h"


USensorTraceTestBase::USensorTraceTestBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	USensorTraceTestBase::InitCollisionParams();
}

void USensorTraceTestBase::BeginDestroy()
{
	Collision_Params = FCollisionQueryParams::DefaultQueryParam;
	Super::BeginDestroy();
}

void USensorTraceTestBase::InitializeFromSensor()
{
	InitCollisionParams();
	USensorTestBase::InitializeFromSensor();
}

void USensorTraceTestBase::SetBaseTraceParam(const ETraceTestParam TraceParam, const ECollisionChannel Collision, bool TraceComplex)
{
	TraceTestParam = TraceParam;
	CollisionChannel = Collision;
	bTraceComplex = TraceComplex;
}

void USensorTraceTestBase::InitCollisionParams()
{
	Collision_Params = FCollisionQueryParams(TEXT("SenseSysLineTrace"), SCENE_QUERY_STAT_ONLY(SenseTrace), true);
	Collision_Params.bReturnPhysicalMaterial = false;
	Collision_Params.bReturnFaceIndex = false;
	Collision_Params.bReturnPhysicalMaterial = false;
	Collision_Params.bTraceComplex = bTraceComplex;
	if (GetSensorOwner())
	{
		Collision_Params.AddIgnoredActors(GetSensorOwner()->GetIgnoredActors());
	}
}

bool USensorTraceTestBase::LineMultiTraceTest(
	const UWorld* World, const FCollisionQueryParams& CollisionParams, const FVector& StartTrace, FSensedStimulus& SensedStimulus) const
{
	for (FSensedPoint& It : SensedStimulus.SensedPoints)
	{
		TArray<struct FHitResult> OutHits;
		const bool bHit = World->LineTraceMultiByChannel(OutHits, StartTrace, It.SensedPoint, CollisionChannel, CollisionParams);
		if (!bHit)
		{
			return false;
		}
		It.PointTestResult = ESenseTestResult::Lost;
	}
	return true;
}

bool USensorTraceTestBase::LineBoolTraceTest(
	const UWorld* World, const FCollisionQueryParams& CollisionParams, const FVector& StartTrace, FSensedStimulus& SensedStimulus) const
{
	for (FSensedPoint& It : SensedStimulus.SensedPoints)
	{
		const bool bHit = World->LineTraceTestByChannel(StartTrace, It.SensedPoint, CollisionChannel, CollisionParams);
		if (!bHit)
		{
			return false;
		}
		It.PointTestResult = ESenseTestResult::Lost;
	}
	return true;
}

FHitResult* USensorTraceTestBase::GetBlockingHit(TArray<struct FHitResult>& Hits)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SensorTraceTestBase_GetBlockingHit);

	for (auto& Hit : Hits)
	{
		const auto Component = Hit.Component.Get();
		if (Component)
		{
			const auto Target = Component->GetOwner();
			if (Target)
			{
				if (!IsStimulusInterface(Target) || !IsObstacleInterface(Target))
				{
					return &Hit;
				}
			}
		}
	}
	return nullptr;
}

bool USensorTraceTestBase::ScoreFromTransparency(const TArray<struct FHitResult>& OutHits, float& InScore) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SensorTraceTestBase_ScoreFromTransparency);

	if (OutHits.Num() > 0)
	{
		const FName SenorTag = GetSensorTag();
		FGCScopeGuard GCScope;
		for (const FHitResult& ItHit : OutHits)
		{
			UPrimitiveComponent* Component = ItHit.Component.Get();
			const auto Owner = Component != nullptr ? Component->GetOwner() : nullptr;
			if (IsObstacleInterface(Owner))
			{
				InScore *= ISenseObstacleInterface::Execute_GetTransparency(Owner, SenorTag, Component, ItHit.Location);
			}
			else
			{
				InScore = 0.f;
				return false;
			}
		}
	}
	return true;
}

// void USensorTraceTestBase::OnSensorChanged()
// {
// 	InitCollisionParams();
// }
//FCollisionObjectQueryParams USensorTraceTestBase::GetCollisionObjectParams(const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes) const
//{
//	checkNoEntry();
//	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SensorTraceTestBase_GetCollisionObjectParams);
//
//	TArray<TEnumAsByte<ECollisionChannel>> CollisionObjectTraces;
//	CollisionObjectTraces.AddUninitialized(ObjectTypes.Num());
//
//	for (auto Iterate = ObjectTypes.CreateConstIterator(); Iterate; ++Iterate)
//	{
//		CollisionObjectTraces[Iterate.GetIndex()] = UEngineTypes::ConvertToCollisionChannel(*Iterate);
//	}
//
//	FCollisionObjectQueryParams ObjectParams;
//	for (auto Iterate = CollisionObjectTraces.CreateConstIterator(); Iterate; ++Iterate)
//	{
//		const ECollisionChannel& Channel = (*Iterate);
//		if (FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
//		{
//			ObjectParams.AddObjectTypesToQuery(Channel);
//		}
//		else
//		{
//			UE_LOG(LogBlueprintUserMessages, Warning, TEXT("%d isn't valid object type"), int32(Channel));
//		}
//	}
//
//	return ObjectParams;
//}
//FCollisionQueryParams USensorTraceTestBase::GetCollisionParams(const FSensedStimulus& SensedStimulus) const
//{
//	checkNoEntry();
//	FCollisionQueryParams Params(TEXT("SenseSysTraceMulti"), SCENE_QUERY_STAT_ONLY(SenseTrace), true);
//	Params.AddIgnoredActors(GetIgnoreTraceActors(SensedStimulus));
//	//Params.AddIgnoredComponent()
//	return Params;
//}
//TArray<AActor*> USensorTraceTestBase::GetIgnoreTraceActors(const FSensedStimulus& SensedStimulus) const
//{
//	checkNoEntry();
//	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_SensorTraceTestBase_GetIgnoreTraceActors);
//	if (SensedStimulus.StimulusComponent && GetSensorOwner() != nullptr && GetSensorOwner()->IsValidLowLevel())
//	{
//		auto Ignore = GetSensorOwner()->GetIgnoredActors();
//		if (AActor* IgnoredActor = SensedStimulus.StimulusComponent->GetOwner())
//		{
//			Ignore.Add(IgnoredActor);
//		}
//		return Ignore;
//	}
//	return TArray<AActor*>{};
//}