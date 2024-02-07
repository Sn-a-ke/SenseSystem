//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "Sensors/SensorBase.h"

#include "PassiveSensor.generated.h"


/**
 *	Base Class for Passive Sensor
 *	Event Or Delegate Update Sensor
 */
UCLASS(abstract, Blueprintable, BlueprintType, EditInlineNew, HideDropdown)
class SENSESYSTEM_API UPassiveSensor : public USensorBase
{
	GENERATED_BODY()
public:

	UPassiveSensor(const FObjectInitializer& ObjectInitializer);
	virtual ~UPassiveSensor() override;

	virtual void BeginDestroy() override;
	virtual void ReportPassiveEvent(class USenseStimulusBase* StimulusComponent) { ReportSenseStimulusEvent(StimulusComponent); }
	virtual void ReportPassiveEvent(const FSenseSystemModule::ElementIndexType StimulusID) { ReportSenseStimulusEvent(StimulusID); }
	virtual bool IsOverrideSenseState() const override { return false; }
};
