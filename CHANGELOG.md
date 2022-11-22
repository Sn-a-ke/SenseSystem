# CHANGELOG

CHANGELOG pattern, see here : https://keepachangelog.com/en/1.0.0/

## 7.1 v1.04 - 2022-10-06

https://youtu.be/oniC1fxWqFUand some bugfix.
Individual per channel settings and updates

## 7.2 v1.10 - 2022-10-06

### Added

- **QuadTree** added to quickly get participants in thesystem for the sensor from the current level,
  which significantly increased system performance. It is very suitable for relatively flat worlds, for non-planar
  ones a little worse, but it will still be effective (This will be in future updates.)
  https://i.imgur.com/Uj4jMmm.gifv
- Added **system settings** in project settings
  minimum squares cell - the optimal cell size depends on how tight the targets for sensing are
  placed at, ideal - so that in one square there are less than 8-16 objects
  debug settings for sensors with matching default tag
- Added **auxiliary structure** for faster obtaining the position of sockets or bones in the skeleton
  mesh
- Added **SetScore** and **SetAge**

### Changed

- **Default Age and Score** moved to **StimulusSensorNameResponse**
- **Refactoring** the system made it less prone to multithreadingerrors, as well as more productive.
  SenseSys 1.04 :https://i.imgur.com/d9YAEzD.mp4
  SenseSys 1.10 :https://i.imgur.com/DAYLBNf.mp4

### Removed

- **IGetSensedAge** and **IGetSensedScore** functions removed from the interface
  **SetScore** and **SetAge** added instead

### Fixed

- **ParallelTestUpdate** - the experimental function disabled by default, enables parallel testing when
  updating. Higher load on all processor threads, which can reduce the maximum FPS, but the average FPS
  can be more stable.

### 7.3 v1.20 - 2022-10-06

### Added

- Added Project settings for sensors by tag
**1. SenseSystem Project Settings**

```
individual settings for sensors by tag.
NodeCanSplit -the number of objects for the tree,exceeding which the tree will be split
into subtrees, default values QuadTree - 4 , OcTree- 8
QtOtSwitch - selection of tree type:
```

- **QuadTree** - for flat levels, when the sensing targetsare located on the map with a
  small variation in height, and the target's height does not greatly affect the result.
- **OcTree** - for volumetric levels where sensing targetsare located on the map with a
  large variation in height (e.g. flying targets).
- **CountPerOneCyclesUpdate** - the number of sensors thatthe "sensor thread" processes
  for breaking
- **WaitTimeBetweenCyclesUpdate** - time in seconds, a shortpause between updates of the
  "sensor thread"

**2.** Added default functions for debugging - **DrawDebugSensor** (...)

**3.** Added ability to disable rendering of default sensors (when many sensors in one actor, rendering
all - prevents configure them)

```
https://i.imgur.com/NTKk2Wo.gif
```

**4. OnSenseStateChanged -** for StimulusComponent addedSensor tag by which the state changes,
now the state can be separated for each tag separately
**5. ReceiveStimulusFlag** for **StimulusComponent** moved fromcomponent properties to
**StimulusSensorNameResponse**.

```
which allows you to customize the receipt of calls from sensors by tag individually for each.
the inactive ReceiveStimulusFlag in the componentis left for backward compatibility - recompile
and save your blueprints to automatically transfer the flag values to the
StimulusSensorNameResponse.
```

```
6. TestBySingleLocation - in the sensor test now doesnot affect the performance of the sensor
update, with the advent of QuadTree, OcTree trees and test check Bounds.
https://i.imgur.com/lDgAknN.png
https://i.imgur.com/udPogI0.png
```

```
7. SensorTest now has virtual functions
```

- FBox GetSensorTestBoundBox ()
- float GetSensorTestRadius ()
  for the test to work correctly, one or both of them must be overridden
  https://i.imgur.com/tH3GpjW.png
  **8.** Added **ExampleLight** to the sample project - an exampleof target visibility by its illumination.
  **9.** Refactoring the system with more loyal memory use when updating the sensor. which gives a
  performance boost.
  **10.** Bug fix.

## 7.4 v1.24 - 2022-10-06

**1.** Replacing arrays with channels with 64-bit flag https://i.imgur.com/8rWPTO6.mp4
**2.** Channel zero is now invalid - and means no value, it's more
sequential channel logic.
**3.** Updating the values ​​of your saved resources will be updated
automatically when you open and save an asset (channels will be shifted by +1)
**4.** Updated functions for setting channel values
**5.** All old variables and functions associated with channels and arrays are marked
as DEPRECATED

**Although this update will cause some inconvenience - to re-save
current assets, I tried to keep these inconveniences to a minimum so that it was
automatic. However, if you are using channel values ​​somewhere in external code,
it needs to be updated.**