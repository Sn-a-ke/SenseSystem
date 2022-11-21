//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "SenseStimulusInterface.generated.h"


class USenseStimulusComponent;


/**
 * SenseStimulusInterface
 */
UINTERFACE(BlueprintType)
class SENSESYSTEM_API USenseStimulusInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * SenseStimulusInterface
 */
class SENSESYSTEM_API ISenseStimulusInterface
{
	GENERATED_BODY()
public:
	/** (!) Not Blueprint Thread Safe!!!*/
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "SenseSystem|SenseStimulusInterface")
	class USenseStimulusBase* IGetActorStimulus() const;

	/** (!) Not Blueprint Thread Safe!!!*/
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "SenseSystem|SenseStimulusInterface")
	FVector IGetSingleSensePoint(FName SensorTag) const;

	/** (!) Not Blueprint Thread Safe!!!*/
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "SenseSystem|SenseStimulusInterface")
	TArray<FVector> IGetSensePoints(FName SensorTag) const;
};