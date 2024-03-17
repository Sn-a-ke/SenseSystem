//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"

#include "SenseObstacleInterface.generated.h"


/**
 * SenseObstacleInterface
 */
UINTERFACE(BlueprintType)
class SENSESYSTEM_API USenseObstacleInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * SenseObstacleInterface
 */
class SENSESYSTEM_API ISenseObstacleInterface 
{
	GENERATED_BODY()
public:
	/**Not Blueprint Thread Safe*/
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "SenseSystem|SenseObstacleInterface")
	float GetTransparency(FName SensorTag, class USceneComponent* HitComponent, FVector HitLocation) const;
};
