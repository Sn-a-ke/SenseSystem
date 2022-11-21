//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/PassiveSensor.h"


UPassiveSensor::UPassiveSensor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UPassiveSensor::~UPassiveSensor()
{
}

void UPassiveSensor::BeginDestroy()
{
	Super::BeginDestroy();
}
