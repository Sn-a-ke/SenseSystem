# Quick start

- 1. [SenseManager for UE4.21 and lower](#1-sensemanager-for-ue421-and-lower)
- 2. [SenseReceiverComponent](#2-sensereceivercomponent)
- 3. [SenseStimulusComponent](#3-sensestimuluscomponent)
- 4. [Implement Interface](#4-implement-interface)
- 5. [Customization Sensor and SensorTest in Blueprint](#5-customization-sensor-and-sensortest-in-blueprint)
- 6. [Customization SensorTest in Cpp](#6-customization-sensortest-in-cpp)
- 7. [Customization Sensorin Cpp](#7-customization-sensor-and-sensortest-in-cpp)
- 8. [DebugDraw](#8-debugdraw) # todo
- 9. [Profiling](#9-profiling) # todo

## 1. SenseManager for UE4.21 and lower

First, where to start: SenseManager implementation required. The current implementation prior to
UE4.22 uses GameInstance as the owner of the SenseManager object. With UE4.22, SenseManager is
implemented as a GameInstanceSubSystem, and no action is required to create it.

Create an object from the SenseManager class and save it into a variable:

![Quick start 1.1](../images/quick-start-1.1.png)

Next, you need to implement the SenseManagerInterface,
getter interface

![Quick start 1.2](../images/quick-start-1.2.png)

add a single function implementation using one of the described methods
**Get Sense Manager** ( USenseManager* **GetSenseManager** ())
Which returns a pointer to a **SenseManager object**

![Quick start 1.3](../images/quick-start-1.3.png)

This completes the operation with the GameInstance setup.

## 2. SenseReceiverComponent

add to actor Sense Receiver Component

![Quick start 2.1](../images/quick-start-2.1.png)

add sensors to component

![Quick start 2.2](../images/quick-start-2.2.png)

adjust the parameters of the sensor, in particular the received channels on which corresponds to
the stimulus

![Quick start 2.3](../images/quick-start-2.3.png)

## 3. SenseStimulusComponent

Add to detectable actor Sense Stimulus Component

![Quick start 3.1](../images/quick-start-3.1.png)

add the required sensor tag

![Quick start 3.2](../images/quick-start-3.2.png)

Add any implementation in the receiver or stimulus to the necessary events.
eg:

![Quick start 3.3](../images/quick-start-3.3.png)

All that remains is to add or spawn actors on the level.

## 4. Implement Interface


See [section 3](./sensesystem-pdf.md#3-sensesystem-architecture).

## 5. Customization Sensor and SensorTest in Blueprint

![Quick start 5.1](../images/quick-start-5.1.png)

See an example in:
**SenseSysExample\Content\ExampleCustomization\CustomActiveSensor.uasset
SenseSysExample\Content\ExampleCustomization\CustomSensorTest_Location.uasset**

## 6. CustomSensorTest

A special class has been allocated for blueprints: **SensorTestBlueprintBase**
there are three functions available for redefinition..
- 1. **GetReadyToTest** - Preparing the sensor for the test
  - a. **Ready** - sensor ready for test
  - b. **Skip** - the sensor will skip the test
  - c. **Fail** - an error occurred while preparing the test,the results will be reset, and the test itself will be skipped, a message will be displayed in the log.
- 2. **RunTest** - implementation of the test itself, if thesensor is configured to call not in the game stream,
then it is forbidden to call thread-unsafe functions in the method
  - a. input parameters:
    - * **StimulusComponent** - SenseStimulusComponent
    - * **Location** - Object location
    - * **CurrentScore** - current score
  - b. output parameters:
    - * **Score** - score after test
    - * **ESenseTestResult**
        - 1. **Sensed** - detection was successful
        - 2. **NotLost** - detection failed but the object will notbe lost if it was previously detected
        - 3. **Lost** - detection failed
 - 3. Initialize - имплементация инициализации


## 7. CustomSensor

It makes sense to create a custom sensor when it has a unique set of tests, or unique actions. All
classes are available for blueprint inheritance.

## 8. Customization Sensor and SensorTest in Cpp

See an example in:

```
SenseSysExample\Source\SenseSysExample\Public\MyCustomSensorTest.h
SenseSysExample\Source\SenseSysExample\Public\MyCustomActiveSensor.h
```
