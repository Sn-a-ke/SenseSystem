//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "SenseStimulusComponent.h"
#include "SenseManager.h"
#include "Sensors/SensorBase.h"
#include "SenseStimulusInterface.h"
#include "SenseReceiverComponent.h"
#include "HashSorted.h"
#include "Algo/MinElement.h"

#include "GameFramework/Actor.h"

USenseStimulusComponent::USenseStimulusComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bOwnerSenseStimulusInterface = false;

	// manual forceinline InitOwnerInterface in constructor
	const AActor* Owner = GetOwner();
	if (IsValid(Owner))
	{
		bOwnerSenseStimulusInterface =						//
			Cast<ISenseStimulusInterface>(Owner) != nullptr //
			|| Owner->GetClass()->ImplementsInterface(USenseStimulusInterface::StaticClass());

		//if (!bOwnerSenseStimulusInterface)
		//{
		//	InterfaceOwnerBitFlags = 0;
		//}
	}
}

USenseStimulusComponent::~USenseStimulusComponent()
{}


#if WITH_EDITORONLY_DATA
void USenseStimulusComponent::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
}
#endif


void USenseStimulusComponent::OnRegister()
{
	Super::OnRegister();
	InitOwnerInterface();
}

void USenseStimulusComponent::InitOwnerInterface()
{
	const AActor* Owner = GetOwner();
	if (IsValid(Owner))
	{
		bOwnerSenseStimulusInterface =
			Cast<ISenseStimulusInterface>(Owner) != nullptr || Owner->GetClass()->ImplementsInterface(USenseStimulusInterface::StaticClass());
	}

	if (bOwnerSenseStimulusInterface)
	{
		check(GetOwner() && GetOwner()->GetClass()->ImplementsInterface(USenseStimulusInterface::StaticClass()));
	}
}

void USenseStimulusComponent::SetInterfaceFlag(ESSInterfaceFlag Flag, const bool bValue)
{
	if (bValue)
	{
		InterfaceOwnerBitFlags |= static_cast<uint8>(Flag);
	}
	else
	{
		InterfaceOwnerBitFlags &= ~static_cast<uint8>(Flag);
	}
}

FVector USenseStimulusComponent::GetSingleSensePoint(const FName SensorTag) const
{

#if WITH_EDITOR
	check(IsInGameThread());
#endif

	if (bOwnerSenseStimulusInterface && (InterfaceOwnerBitFlags & static_cast<uint8>(ESSInterfaceFlag::ESS_SinglePointInterface)))
	{

#if WITH_EDITOR
		check(GetOwner() && GetOwner()->GetClass()->ImplementsInterface(USenseStimulusInterface::StaticClass()));
#endif

		return ISenseStimulusInterface::Execute_IGetSingleSensePoint(GetOwner(), SensorTag);
	}
	return USenseStimulusBase::GetSingleSensePoint(SensorTag);
}

TArray<FVector> USenseStimulusComponent::GetSensePoints(const FName SensorTag) const
{

#if WITH_EDITOR
	check(IsInGameThread());
#endif

	if (bOwnerSenseStimulusInterface && (InterfaceOwnerBitFlags & static_cast<uint8>(ESSInterfaceFlag::ESS_SensePointsInterface)))
	{
#if WITH_EDITOR
		check(GetOwner() && GetOwner()->GetClass()->ImplementsInterface(USenseStimulusInterface::StaticClass()));
#endif
		return ISenseStimulusInterface::Execute_IGetSensePoints(GetOwner(), SensorTag);
	}
	return USenseStimulusBase::GetSensePoints(SensorTag);
}
