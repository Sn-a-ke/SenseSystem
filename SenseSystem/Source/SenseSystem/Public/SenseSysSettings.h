//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SenseSysHelpers.h"
#include "UObject/NoExportTypes.h"

#include "SenseSysSettings.generated.h"


/**
* SuccessState
*/
UENUM(BlueprintType)
enum class ESenseSys_QtOtSwitch : uint8
{
	// OcTree32 max nodes MAX_uint32 - 1 = 4294967294
	OcTree = 0 UMETA(DisplayName = "OcTree"),

	// QuadTree32 max nodes MAX_uint32 - 1= 4294967294
	QuadTree   UMETA(DisplayName = "QuadTree"),

	// OcTree16 max nodes MAX_uint16 - 1 = 65534
	//OcTree16   UMETA(DisplayName = "OcTree16"),

	// QuadTree16 max nodes MAX_uint16 - 1 = 65534
	//QuadTree16 UMETA(DisplayName = "QuadTree16")
};

/**
*	SensedPoint
*/
USTRUCT(BlueprintType)
struct SENSESYSTEM_API FSensorTagSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Config, EditAnywhere, Category = "SenseSystem", meta = (ClampMin = "10.0", ClampMax = "100000.0", UIMin = "10.0", UIMax = "100000.0"))
	float MinimumQuadTreeSize = 100.f;

	//QuadTree - 4. Octree - 8
	UPROPERTY(Config, EditAnywhere, Category = "SenseSystem", meta = (ClampMin = "4", ClampMax = "128", UIMin = "4", UIMax = "128"))
	int32 NodeCantSplit = 4;

	UPROPERTY(Config, EditAnywhere, Category = "SenseSystem")
	ESenseSys_QtOtSwitch QtOtSwitch = ESenseSys_QtOtSwitch::QuadTree;

	UPROPERTY(Config, EditAnywhere, Category = "SenseSystem")
	FSenseSysDebugDraw SenseSysDebugDraw;

	//todo draw color
};


/**
 *	SenseSysSettings
 */
//UCLASS(Config = EditorPerProjectUserSettings)
UCLASS(config = Game, defaultconfig)
class SENSESYSTEM_API USenseSysSettings : public UObject
{
	GENERATED_BODY()
public:
	
	UPROPERTY(Config, EditAnywhere, Category = "SenseSystem")
	TMap<FName, FSensorTagSettings> SensorTagSettings;

	UPROPERTY(Config, EditAnywhere, Category = "SenseSystem")
	int32 CountPerOneCyclesUpdate = 10;

	UPROPERTY(Config, EditAnywhere, Category = "SenseSystem")
	float WaitTimeBetweenCyclesUpdate = 0.0001f;
};
