//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/Tests/SensorTestBase.h"
#include "Sensors/SensorBase.h"
#include "SenseStimulusComponent.h"
#include "SenseReceiverComponent.h"
#include "GameFramework/Actor.h"
#include "SenseObstacleInterface.h"
#include "SenseStimulusInterface.h"
#include "SenseSystemBPLibrary.h"


USensorTestBase::USensorTestBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrivateSensorOwner = GetTypedOuter<USensorBase>();
}

USensorTestBase::~USensorTestBase()
{
	/* */
}

#if WITH_EDITOR
void USensorTestBase::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
}
#endif

FName USensorTestBase::GetSensorTag() const
{
	check(GetSensorOwner() != nullptr);
	return GetSensorOwner()->SensorTag;
}

void USensorTestBase::BeginDestroy()
{
	Super::BeginDestroy();
}

bool USensorTestBase::IsObstacleInterface(const AActor* Actor)
{
	if (IsValid(Actor) && Actor->GetClass()->ImplementsInterface(USenseObstacleInterface::StaticClass()) && !Actor->IsUnreachable())
	{
		return true;
	}
	return false;
}

bool USensorTestBase::IsStimulusInterface(const AActor* Actor)
{
	if (IsValid(Actor) && Actor->GetClass()->ImplementsInterface(USenseStimulusInterface::StaticClass()) && !Actor->IsUnreachable())
	{
		return true;
	}
	return false;
}

void USensorTestBase::InitializeFromSensor()
{
	check(GetOuter() == GetSensorOwner());
	//Sensor->OnSensorChanged.AddUObject(this, &USensorTestBase::OnSensorChanged);
	SensorTransform = GetSensorOwner()->GetSensorTransform();
	InitializeCacheTest();

	if (IsValid(GetSensorOwner()))
	{
		InitializeFromSensorBP(GetSensorOwner());
	}
}

bool USensorTestBase::PreTest()
{
	checkSlow(GetSensorOwner() && IsValid(GetSensorOwner()));

	if (const USensorBase* Sensor = GetSensorOwner())
	{
		SensorTransform = Sensor->GetSensorTransform();
	}
	return true;
}

ESenseTestResult USensorTestBase::RunTest(FSensedStimulus& SensedStimulus) const
{
	return ESenseTestResult::Lost;
}

class UWorld* USensorTestBase::GetWorld() const
{
	const USensorBase* SensorOwner = GetSensorOwner();
	if (IsValid(SensorOwner))
	{
		return GetSensorOwner()->GetWorld();
	}
	return nullptr;
}

const FTransform& USensorTestBase::GetReceiverTransform() const
{
	const USenseReceiverComponent* Comp = GetSenseReceiverComponent();
	if (IsValid(Comp))
	{
		return Comp->GetComponentTransform();
	}
	return FTransform::Identity;
}

USenseReceiverComponent* USensorTestBase::GetSenseReceiverComponent() const
{
	check(GetSensorOwner() != nullptr);
	const USensorBase* SensorOwner = GetSensorOwner();
	if (IsValid(SensorOwner))
	{
		return GetSensorOwner()->GetSenseReceiverComponent();
	}
	return nullptr;
}

AActor* USensorTestBase::GetReceiverActor() const
{
	check(GetSensorOwner() != nullptr);
	const USenseReceiverComponent* ReceiverComp = GetSenseReceiverComponent();
	if (IsValid(ReceiverComp))
	{
		AActor* Owner = ReceiverComp->GetOwner();
		if (IsValid(Owner))
		{
			return Owner;
		}
	}
	return nullptr;
}

//USenseStimulusBase* USensorTestBase::GetActorStimulus(const AActor* Actor)
//{
//	return USenseSystemBPLibrary::GetStimulusFromActor(Actor);
//}