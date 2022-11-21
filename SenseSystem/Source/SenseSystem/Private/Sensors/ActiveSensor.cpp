//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/ActiveSensor.h"
#include "SenseManager.h"
#include "SenseStimulusBase.h"
#include "GameFramework/Actor.h"


UActiveSensor::UActiveSensor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UActiveSensor::~UActiveSensor()
{
}

void UActiveSensor::BeginDestroy()
{
	Super::BeginDestroy();
	CallbackTargetsSingleResult.Empty();
	Callback.Empty();
}

#if WITH_EDITOR
void UActiveSensor::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
}
#endif

void UActiveSensor::InitializeFromReceiver(USenseReceiverComponent* FromReceiver)
{
	Super::InitializeFromReceiver(FromReceiver);
	bOverrideSenseState = SensorType == ESensorType::Active;
}

void UActiveSensor::OnSensorUpdated()
{

	Super::OnSensorUpdated();

	if (SensorType == ESensorType::Manual)
	{
		bOverrideSenseState = IsOverrideSenseState();
	}

	if (Callback.Num() > 0)
	{
		for (int32 i = 0; i < Callback.Num(); i++)
		{
			Callback[i].ExecuteIfBound(this);
		}
		Callback.Empty();
		bRequestManualUpdate = false;
	}

	if (CallbackTargetsSingleResult.Num() > 0)
	{
		for (FChannelSetup& CS : ChannelSetup) //todo ActiveSensor CallbackTargetsSingleResult by channel
		{
			const FSensedStimulus* Tmp = CS.GetBestSenseStimulus();
			if (Tmp && Tmp->StimulusComponent.IsValid())
			{
				const auto Actor = Tmp->StimulusComponent.Get()->GetOwner();
				if (Actor)
				{
					for (int32 i = 0; i < CallbackTargetsSingleResult.Num(); i++)
					{
						CallbackTargetsSingleResult[i].ExecuteIfBound(Actor);
					}
				}
			}
		}
		CallbackTargetsSingleResult.Empty();
		bRequestManualUpdate = false;
	}
	//if (bRequestManualUpdate)
	//{
	//	bOverrideSenseState = false;
	//}
}

void UActiveSensor::OnAgeUpdated()
{
	bRequestManualUpdate = false;
	Super::OnAgeUpdated();
	//if (bRequestManualUpdate)
	//{
	//	bOverrideSenseState = false;
	//}
}


void UActiveSensor::RunManualSensor(const bool bOverride_SenseState /*= true*/)
{
	if (bEnable && IsInitialized() && SensorType == ESensorType::Manual && UpdateState == ESensorState::NotUpdate)
	{
		bRequestManualUpdate = true;
		bOverrideSenseState = bOverride_SenseState;
		TrySensorUpdate();
	}
}

void UActiveSensor::AsyncManualSensor(UPARAM(DisplayName = "Event") FOnSensorAsyncComplete Delegate, const bool bOverride_SenseState)
{
	if (bEnable && IsInitialized())
	{
		bRequestManualUpdate = true;
		bOverrideSenseState = bOverride_SenseState;

		Callback.Add(Delegate);
		if (UpdateState == ESensorState::NotUpdate)
		{
			TrySensorUpdate();
		}
	}
}

void UActiveSensor::AsyncManualSensorDetect_BestActor(UPARAM(DisplayName = "Event") FOnSenseTestActor Delegate, const bool bOverride_SenseState)
{
	if (bEnable && IsInitialized())
	{
		bRequestManualUpdate = true;
		bOverrideSenseState = bOverride_SenseState;
		CallbackTargetsSingleResult.Add(Delegate);
		if (UpdateState == ESensorState::NotUpdate)
		{
			TrySensorUpdate();
		}
	}
}
