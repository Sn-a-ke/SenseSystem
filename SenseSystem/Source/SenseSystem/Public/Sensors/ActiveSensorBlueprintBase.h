//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "Sensors/ActiveSensor.h"

#include "ActiveSensorBlueprintBase.generated.h"


/**
 * Base Class for Active Sensor Blueprint
 */
UCLASS(abstract, Blueprintable, BlueprintType, EditInlineNew, HideDropdown)
class SENSESYSTEM_API UActiveSensorBlueprintBase : public UActiveSensor
{
	GENERATED_BODY()

public:
	UActiveSensorBlueprintBase(const FObjectInitializer& ObjectInitializer);
	virtual ~UActiveSensorBlueprintBase() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

	/**Initialize Sensor Blueprint Implementation*/
	UFUNCTION(BlueprintImplementableEvent, Category = "SenseSystem|Sensor", meta = (DisplayName = "InitializeFromReceiver"))
	void InitializeFromReceiver_BP(USenseReceiverComponent* InSenseReceiver);

	virtual void InitializeFromReceiver(USenseReceiverComponent* FromReceiver) override;
};
