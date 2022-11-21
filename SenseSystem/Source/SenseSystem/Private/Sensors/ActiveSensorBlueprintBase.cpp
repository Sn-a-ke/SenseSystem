//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/ActiveSensorBlueprintBase.h"


UActiveSensorBlueprintBase::UActiveSensorBlueprintBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UActiveSensorBlueprintBase::~UActiveSensorBlueprintBase()
{
}

#if WITH_EDITOR
void UActiveSensorBlueprintBase::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);
}
#endif

void UActiveSensorBlueprintBase::InitializeFromReceiver(USenseReceiverComponent* FromReceiver)
{
	UActiveSensor::InitializeFromReceiver(FromReceiver);
	InitializeFromReceiver_BP(FromReceiver);
}
