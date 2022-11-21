//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "SenseSystemBPLibrary.h"

#include "SenseStimulusBase.h"
#include "SenseReceiverComponent.h"
#include "SenseStimulusInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"

#include "Engine/Engine.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"

//#include "Async/ParallelFor.h"


USenseSystemBPLibrary::USenseSystemBPLibrary(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{}


USenseStimulusBase* USenseSystemBPLibrary::GetStimulusFromActor(const AActor* Actor, const bool bInterfaceOnly /*= false*/)
{
	USenseStimulusBase* Out = nullptr;
	if (IsValid(Actor))
	{
		if (!Actor->IsUnreachable())
		{
			/*
			// cpp interface implementation
			const ISenseStimulusInterface* ISenseStimulus = Cast<ISenseStimulusInterface>(Actor);
			if (ISenseStimulus)
			{
				return ISenseStimulus->IGetActorStimulus();
			}
			else
			*/
			if (Actor->GetClass()->ImplementsInterface(USenseStimulusInterface::StaticClass()))
			{
				Out = ISenseStimulusInterface::Execute_IGetActorStimulus(Actor);
			}
		}

		if (!Out && !bInterfaceOnly)
		{
			return Actor->FindComponentByClass<USenseStimulusBase>();
		}
	}
	return Out;
}

USenseReceiverComponent* USenseSystemBPLibrary::GetReceiverFromActor(const AActor* Actor)
{
	if (IsValid(Actor))
	{
		return Actor->FindComponentByClass<USenseReceiverComponent>();
	}
	return nullptr;
}

USenseStimulusBase* USenseSystemBPLibrary::GetStimulusFromActor_BP(const AActor* Actor)
{
	return GetStimulusFromActor(Actor, false);
}

USenseReceiverComponent* USenseSystemBPLibrary::GetReceiverFromActor_BP(const AActor* Actor)
{
	return GetReceiverFromActor(Actor);
}


bool FSkeletalBoneIDCache::Init(const class USkeletalMeshComponent* SkeletalMeshComponent, const TArray<FName>& BoneNames)
{
	bool bNoErrors = false;
	if (SkeletalMeshComponent)
	{
		SkelMesh = SkeletalMeshComponent->SkeletalMesh;
		if (SkelMesh)
		{
			bNoErrors = true;
			BoneIDs.Reserve(BoneNames.Num());
			for (const FName& BnName : BoneNames)
			{
				const int32 BoneIndex = SkeletalMeshComponent->GetBoneIndex(BnName); //SkeletalMeshComponent->GetSocketLocation(BnName);
				if (BoneIndex != INDEX_NONE)
				{
					BoneIDs.Add(BoneIndex);
				}
				else
				{
					bNoErrors = false;
				}
			}
		}
	}
	return bNoErrors;
}

bool FSkeletalBoneIDCache::GetBoneLocations(const class USkeletalMeshComponent* SkeletalMeshComponent, TArray<FVector>& Out) const
{
	if (IsValid(SkeletalMeshComponent) && SkelMesh == SkeletalMeshComponent->SkeletalMesh)
	{
		Out.Reserve(BoneIDs.Num());
		for (const int32 BnID : BoneIDs)
		{
			const FVector Loc = SkeletalMeshComponent->GetBoneTransform(BnID).GetLocation();
			Out.Add(Loc);
		}
		return true;
	}
	return false;
}

FSkeletalBoneIDCache::FSkeletalBoneIDCache(const class USkeletalMeshComponent* SkeletalMeshComponent, const TArray<FName>& BoneNames)
{
	if (SkeletalMeshComponent)
	{
		SkelMesh = SkeletalMeshComponent->SkeletalMesh;
		if (SkelMesh)
		{
			BoneIDs.Reserve(BoneNames.Num());
			for (const FName& BnName : BoneNames)
			{
				const int32 BoneIndex = SkeletalMeshComponent->GetBoneIndex(BnName); //SkeletalMeshComponent->GetSocketLocation(BnName);
				if (BoneIndex != INDEX_NONE) BoneIDs.Add(BoneIndex);
			}
		}
	}
}


#if WITH_EDITOR

UBlueprintGeneratedClass* USenseSystemBPLibrary::GetOwnerBlueprintClass(UObject* Obj)
{
	UBlueprintGeneratedClass* BaseClass = nullptr;
	if (IsValid(Obj))
	{
		UObject* PackageObj = Obj;
		for (UObject* Top = PackageObj;;)
		{
			UObject* CurrentOuter = Top->GetOuter();
			if (!CurrentOuter)
			{
				//check(Cast<UBlueprintGeneratedClass>(PackageObj) || Cast<UBlueprintGeneratedClass>(PackageObj->GetClass()));
				break;
			}
			PackageObj = Top;
			Top = CurrentOuter;
		}

		if (PackageObj)
		{
			if (Cast<UBlueprintGeneratedClass>(PackageObj))
			{
				BaseClass = Cast<UBlueprintGeneratedClass>(PackageObj); //if Actor Package
			}
			if (!BaseClass)
			{
				BaseClass = Cast<UBlueprintGeneratedClass>(PackageObj->GetClass());
			}
		}
	}

	return BaseClass;
}

USenseReceiverComponent* USenseSystemBPLibrary::GetSenseReceiverComponent_FromDefaultActorClass(UClass* InClass, const FName& Name)
{
	if (InClass)
	{
		TArray<UActorComponent*> Components;

		{
			TArray<UActorComponent*> Templates;
			if (const AActor* DefaultActor = Cast<AActor>(InClass->GetDefaultObject(true)))
			{
				DefaultActor->GetComponents(USenseReceiverComponent::StaticClass(), Templates);
				Components.Append(Templates);
			}
		}

		if (UBlueprintGeneratedClass* BP_Class = Cast<UBlueprintGeneratedClass>(InClass))
		{
			if (BP_Class->GetInheritableComponentHandler(true))
			{
				TArray<UActorComponent*> Templates;

				BP_Class->GetInheritableComponentHandler()->GetAllTemplates(Templates);
				Components.Append(Templates);
			}

			if (BP_Class->SimpleConstructionScript)
			{
				TArray<UActorComponent*> Templates;

				auto Nodes = BP_Class->SimpleConstructionScript->GetRootNodes();
				for (const USCS_Node* Node : Nodes)
				{
					if (Node && Node->ComponentTemplate->GetFName() == Name &&
						Node->ComponentTemplate->GetClass()->IsChildOf(USenseReceiverComponent::StaticClass()))
					{
						Components.Add(Node->ComponentTemplate);
						break;
					}
				}
				//AActor* DefaultActor = Class_1->SimpleConstructionScript->GetComponentEditorActorInstance();
				//DefaultActor->GetComponents(USenseReceiverComponent::StaticClass(), Templates);
				//Components.Append(Templates);
			}

			UBlueprint* Blueprint = Cast<UBlueprint>(BP_Class->ClassGeneratedBy);
			for (UActorComponent* Component : Blueprint->ComponentTemplates)
			{
				if (Component && Component->GetFName() == Name && Component->GetClass()->IsChildOf(USenseReceiverComponent::StaticClass()))
				{
					Components.Add(Component);
				}
			}
		}

		if (Components.Num() > 0)
		{
			Components = Components.FilterByPredicate(
				//
				[&Name](const UActorComponent* Comp) //
				{
					//
					return Comp->GetFName() == Name && Comp->GetClass()->IsChildOf(USenseReceiverComponent::StaticClass());
				});
		}
		if (Components.Num() > 0)
		{
			return Cast<USenseReceiverComponent>(Components[0]);
		}
	}
	return nullptr;
}

EOwnerBlueprintClassType USenseSystemBPLibrary::GetOwnerBlueprintClassType(UClass* BaseClass)
{
	if (BaseClass)
	{
		if (BaseClass->IsChildOf(AActor::StaticClass()))
		{
			return EOwnerBlueprintClassType::ActorBP;
		}
		if (BaseClass->IsChildOf(USenseReceiverComponent::StaticClass()))
		{
			return EOwnerBlueprintClassType::SenseReceiverComponentBP;
		}
		if (BaseClass->IsChildOf(USensorBase::StaticClass()))
		{
			return EOwnerBlueprintClassType::SensorBaseBP;
		}
	}
	return EOwnerBlueprintClassType::None;
}

UClass* USenseSystemBPLibrary::GetOwnerBlueprintClassType(const EOwnerBlueprintClassType In)
{
	switch (In)
	{
		case EOwnerBlueprintClassType::None: return nullptr;
		case EOwnerBlueprintClassType::ActorBP: return AActor::StaticClass();
		case EOwnerBlueprintClassType::SenseReceiverComponentBP: return USenseReceiverComponent::StaticClass();
		case EOwnerBlueprintClassType::SensorBaseBP: return USensorBase::StaticClass();
		default:
		{
			checkNoEntry();
			UE_ASSUME(0);
			return nullptr;
		}
	}
}


#endif

void USenseSystemBPLibrary::ForceGarbageCollection()
{
	if (GEngine)
	{
		GEngine->ForceGarbageCollection(true);
	}
}