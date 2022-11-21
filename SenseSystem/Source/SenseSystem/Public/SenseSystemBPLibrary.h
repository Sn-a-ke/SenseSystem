//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SensedStimulStruct.h"
#include "SenseSysHelpers.h"

#include "SenseSystemBPLibrary.generated.h"


class USenseStimulusBase;
class USenseReceiverComponent;
class USkinnedAsset;


/**
 * StimulusTagResponse struct
 */
USTRUCT(BlueprintType)
struct SENSESYSTEM_API FSkeletalBoneIDCache //96 byte
{
	GENERATED_USTRUCT_BODY()

	FSkeletalBoneIDCache() {}
	FSkeletalBoneIDCache(const class USkeletalMeshComponent* SkeletalMeshComponent, const TArray<FName>& BoneNames);

	UPROPERTY(BlueprintReadOnly, Category = "SkeletalBoneIDCache") 
	TObjectPtr<USkinnedAsset> SkelMesh = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "SkeletalBoneIDCache") 
	TArray<int32> BoneIDs;

	bool Init(const class USkeletalMeshComponent* SkeletalMeshComponent, const TArray<FName>& BoneNames);
	bool GetBoneLocations(const class USkeletalMeshComponent* SkeletalMeshComponent, TArray<FVector>& Out) const;
};


/*
*	Function library class.
*/
UCLASS()
class USenseSystemBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
public:
	static USenseStimulusBase* GetStimulusFromActor(const AActor* Actor, bool bInterfaceOnly = false);

	static USenseReceiverComponent* GetReceiverFromActor(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem", meta = (DisplayName = "GetStimulusFromActor", Keywords = "SenseSystem Get Stimulus From Actor"))
	static USenseStimulusBase* GetStimulusFromActor_BP(const AActor* Actor);
	UFUNCTION(BlueprintCallable, Category = "SenseSystem", meta = (DisplayName = "GetReceiverFromActor", Keywords = "SenseSystem Get Receiver From Actor"))
	static USenseReceiverComponent* GetReceiverFromActor_BP(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SkeletalBoneIDCache", meta = (Keywords = "SenseSystem Create Bone ID Position Location Cache"))
	static FSkeletalBoneIDCache CreateBoneIDCache(const class USkeletalMeshComponent* SkeletalMeshComponent, const TArray<FName>& BoneNames, bool& bSuccess)
	{
		FSkeletalBoneIDCache Out(SkeletalMeshComponent, BoneNames);
		bSuccess = Out.BoneIDs.Num() > 0; //Out.Init(SkeletalMeshComponent, BoneNames);
		return Out;
	}

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SkeletalBoneIDCache", meta = (Keywords = "SenseSystem Get Bone Locations Bone ID Cache"))
	static TArray<FVector> GetBoneLocations_BoneIDCache(
		const class USkeletalMeshComponent* SkeletalMeshComponent,
		const FSkeletalBoneIDCache& SkeletalBoneIDCache,
		bool& bSuccess)
	{
		TArray<FVector> Out;
		bSuccess = SkeletalBoneIDCache.GetBoneLocations(SkeletalMeshComponent, Out);
		return Out;
	}
	UFUNCTION(BlueprintCallable, Category = "SenseSystem|SkeletalBoneIDCache", meta = (Keywords = "SenseSystem Convert BitFlag 64 to Array"))
	static TArray<uint8> BitFlag64_ToArray(const FBitFlag64_SenseSys& In) { return FBitFlag64_SenseSys::To_ArrayBitFlagChannel(In.Value); }

#if WITH_EDITOR
	static class UBlueprintGeneratedClass* GetOwnerBlueprintClass(UObject* Obj);
	static class USenseReceiverComponent* GetSenseReceiverComponent_FromDefaultActorClass(class UClass* InClass, const FName& Name);
	static EOwnerBlueprintClassType GetOwnerBlueprintClassType(UClass* BaseClass);
	static UClass* GetOwnerBlueprintClassType(EOwnerBlueprintClassType In);
#endif

	UFUNCTION(BlueprintCallable, Category = "SenseSystem|Utils")
	static void ForceGarbageCollection();
};
