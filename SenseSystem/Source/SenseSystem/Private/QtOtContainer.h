// Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "AbstractTree.h"
#include "HAL/Platform.h"
#include "Misc/ScopeRWLock.h"
#include "Misc/MemStack.h"
#include "Math/Box2D.h"
#include "Math/Box.h"
#include "Math/NumericLimits.h"
#include "Containers/Array.h"
#include "Containers/SparseArray.h"

#include "SenseSystem.h"
#include "SensedStimulStruct.h"


struct FIndexRemoveControl
{
	TArray<uint16> RemIDs;
	uint32 bTrack = false;
	bool bRemove = false;
};

struct FTreeDrawSetup
{
	FTreeDrawSetup() {}
	FTreeDrawSetup(const FLinearColor C, const float T, const ESceneDepthPriorityGroup D) : Color(C.ToFColor(true)), Thickness(T), DrawDepth(D) {}
	FTreeDrawSetup(const FColor C, const float T, const ESceneDepthPriorityGroup D) : Color(C), Thickness(T), DrawDepth(D) {}
	FColor Color = FColor::White;
	float Thickness = 1.f;
	ESceneDepthPriorityGroup DrawDepth = ESceneDepthPriorityGroup::SDPG_World;
};

/** abstract QuadTree - OcTree */
class SENSESYSTEM_API IContainerTree
{
public:
	IContainerTree() = default;
	virtual ~IContainerTree() = default;
	using Real = FVector::FReal;

	virtual TSparseArray<FSensedStimulus>& GetCompDataPool() = 0;
	virtual const TSparseArray<FSensedStimulus>& GetCompDataPool() const = 0;

	void Lock() {RWLock.WriteLock();}
	void UnLock() {RWLock.WriteUnlock();}
protected:

	mutable FIndexRemoveControl IndexRemoveControl;
	mutable FRWLock RWLock;

public:
#if WITH_EDITOR
	bool IsRemoveControlClear() const;
#endif

	void MarkRemoveControl() const;
	void ResetRemoveControl() const;
	void ResetRemoveControl(int32 Idx) const;

	/** FStimulusTagResponse */
	virtual bool Remove(uint16 InObjID) = 0;

	virtual uint16 Insert(const FSensedStimulus& ComponentData, FBox InBox) = 0;
	virtual uint16 Insert(FSensedStimulus&& ComponentData, FBox InBox) = 0;
	virtual void Update(uint16 InObjID, FBox NewBox) = 0;
	/** end FStimulusTagResponse */

	/** virtual Tree */
	virtual void Clear() = 0;
	virtual void Collapse() = 0;

	virtual void GetInBoxIDs(FBox Box, TArray<uint16>& Out, uint64 InBitChannels = MAX_uint64) const = 0;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TArray<uint16>& Out, uint64 InBitChannels = MAX_uint64) const = 0;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TArray<uint16>& Out, uint64 InBitChannels = MAX_uint64) const = 0;

	virtual void GetInBoxIDs(FBox Box, TSet<uint16>& Out, uint64 InBitChannels = MAX_uint64) const = 0;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TSet<uint16>& Out, uint64 InBitChannels = MAX_uint64) const = 0;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TSet<uint16>& Out, uint64 InBitChannels = MAX_uint64) const  = 0;

	virtual FBox GetMaxIntersect(FBox Box) const = 0;

	virtual void DrawTree(const class UWorld* World, FTreeDrawSetup TreeNode, FTreeDrawSetup Link, FTreeDrawSetup ElemNode, float LifeTime) const {}
	/** end virtual Tree */

	FORCEINLINE int32 Num() const { return GetCompDataPool().Num(); }

	FSensedStimulus GetSensedStimulusCopy_TS(uint16 InObjID) const;
	FSensedStimulus GetSensedStimulusCopy_Simple_TS(uint16 InObjID) const;

	/** TMap<Id, Hash> */
	TArray<uint16> CheckHash_TS(const TMap<uint16, uint32>& InArr) const;
	TArray<uint16, TMemStackAllocator<>> CheckHashStack_TS(const TMap<uint16, uint32>& InArr) const;
	TSet<uint16> CheckHashSet_TS(const TMap<uint16, uint32>& InArr) const;

	FORCEINLINE const FSensedStimulus& GetSensedStimulus(const uint16 InObjID) const { return GetCompDataPool()[InObjID]; }
	FORCEINLINE FSensedStimulus& GetSensedStimulus(const uint16 InObjID) { return GetCompDataPool()[InObjID]; }
	FORCEINLINE FSensedStimulus GetSensedStimulusCopy(const uint16 InObjID) const { return GetCompDataPool()[InObjID]; }

	void SetAge_TS(uint16 ID, float AgeValue);
	void SetScore_TS(uint16 ID, float ScoreValue);
	void SetChannels_TS(uint16 ID, uint64 Channels);
	void SetSensedPoints_TS(uint16 ID, const TArray<FSensedPoint>& InSensedPoints, float InCurrentTime);
	void SetSensedPoints_TS(uint16 ID, const FSensedPoint& InSensedPoints, float InCurrentTime);

	void SetSensedPoints_TS(uint16 ID, TArray<FSensedPoint>&& InSensedPoints, float InCurrentTime);
	void SetSensedPoints_TS(uint16 ID, FSensedPoint&& InSensedPoints, float InCurrentTime);
};

/** QuadTree */
class SENSESYSTEM_API FSenseSys_QuadTree final : public IContainerTree
{
private:
	using TreeType = TTree_Base<FSensedStimulus, FVector2D, uint16, 2>;
	TreeType Tree;

	virtual TSparseArray<FSensedStimulus>& GetCompDataPool() override { return Tree.GetElementPool(); }
	virtual const TSparseArray<FSensedStimulus>& GetCompDataPool() const override { return Tree.GetElementPool(); }

public:
	using Real = FVector::FReal;
	
	FSenseSys_QuadTree( //
		const Real MinimumQuadSize,
		const int32 InNodeCantSplit = 8,
		const int32 OtCount = 128,
		const int32 ObjCount = 128)
		: Tree(MinimumQuadSize, InNodeCantSplit, OtCount, ObjCount)
	{
#if WITH_EDITOR
		UE_LOG(LogSenseSys, Log, TEXT("SenseSys_QuadTree created"));
#endif
	}

	virtual ~FSenseSys_QuadTree() override { Clear(); }


	/**FStimulusTagResponse*/
	virtual bool Remove(uint16 InObjID) override;
	virtual uint16 Insert(FSensedStimulus&& ComponentData, FBox InBox) override;
	virtual uint16 Insert(const FSensedStimulus& ComponentData, FBox InBox) override;
	virtual void Update(uint16 InObjID, FBox NewBox) override;
	virtual void Clear() override;
	virtual void Collapse() override;

	virtual void GetInBoxIDs(FBox Box, TArray<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TArray<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TArray<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;

	virtual void GetInBoxIDs(FBox Box, TSet<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TSet<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TSet<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;

	virtual void DrawTree(const class UWorld* World, FTreeDrawSetup TreeNode, FTreeDrawSetup Link, FTreeDrawSetup ElemNode, float LifeTime) const override;

	virtual FBox GetMaxIntersect(FBox Box) const override;

private:
	struct FInRadiusPredicate
	{
		FInRadiusPredicate(const TreeType& InTree, const FVector& InCenter, const Real InRadius, const FBox2D& InBox, const uint64 InBitChannels = MAX_uint64)
			: Tree(InTree)
			, Center(InCenter)
			, RSquared(InRadius * InRadius)
			, Box(InBox)
			, BitChannels(InBitChannels)
		{}
		FInRadiusPredicate(const TreeType& InTree, const FVector& InCenter, const Real InRadius, const uint64 InBitChannels = MAX_uint64)
			: Tree(InTree)
			, Center(InCenter)
			, RSquared(InRadius * InRadius)
			, Box(FBox2D(FVector2D(Center.X - InRadius, Center.Y - InRadius), FVector2D(Center.X + InRadius, Center.Y + InRadius)))
			, BitChannels(InBitChannels)
		{}

		FORCEINLINE bool operator()(const uint16 ObjID) const
		{
			const auto& B = Tree.GetElementBox(ObjID);
			return
				B.IsIntersect(Box.Min, Box.Max) &&
				B.SphereAABBIntersection(Center, RSquared) && 
				(BitChannels & Tree.GetElement(ObjID).BitChannels);
		}

		const TreeType& Tree;
		const FVector2D Center;
		const Real RSquared;
		const FBox2D Box;
		const uint64 BitChannels;
	};

	struct FInBoxPredicate
	{
		FInBoxPredicate(const TreeType& InQt, const FBox2D& InBox, const uint64 InBitChannels = MAX_uint64)
			: Tree(InQt), Box(InBox), BitChannels(InBitChannels)
		{}

		FORCEINLINE bool operator()(const uint16 ObjID) const
		{
			const auto& B = Tree.GetElementBox(ObjID);
			return
				B.IsIntersect(Box.Min, Box.Max) && 
				(BitChannels & Tree.GetElement(ObjID).BitChannels);
		}

		const TreeType& Tree;
		const FBox2D Box;
		const uint64 BitChannels;
	};
};

/** OcTree */
class SENSESYSTEM_API FSenseSys_OcTree final : public IContainerTree
{
private:
	using TreeType = TTree_Base<FSensedStimulus, FVector, uint16, 3>;
	TreeType Tree;

	virtual TSparseArray<FSensedStimulus>& GetCompDataPool() override { return Tree.GetElementPool(); }
	virtual const TSparseArray<FSensedStimulus>& GetCompDataPool() const override { return Tree.GetElementPool(); }

public:

	using Real = FVector::FReal;

	FSenseSys_OcTree( //
		const Real MinimumCubeSize,
		const int32 InNodeCantSplit = 8,
		const int32 OtCount = 128,
		const int32 ObjCount = 128)
		: Tree(MinimumCubeSize, InNodeCantSplit, OtCount, ObjCount)
	{
#if WITH_EDITOR
		UE_LOG(LogSenseSys, Log, TEXT("SenseSys_OcTree created"));
#endif
	}

	virtual ~FSenseSys_OcTree() override { Clear(); }


	virtual bool Remove(uint16 InObjID) override;
	virtual uint16 Insert(FSensedStimulus&& ComponentData, FBox InBox) override;
	virtual uint16 Insert(const FSensedStimulus& ComponentData, FBox InBox) override;
	virtual void Update(uint16 InObjID, FBox NewBox) override;
	virtual void Clear() override;
	virtual void Collapse() override;

	virtual void GetInBoxIDs(FBox Box, TArray<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TArray<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TArray<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;

	virtual void GetInBoxIDs(FBox Box, TSet<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TSet<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TSet<uint16>& Out, uint64 InBitChannels = MAX_uint64) const override;

	virtual void DrawTree(const class UWorld* World, FTreeDrawSetup TreeNode, FTreeDrawSetup Link, FTreeDrawSetup ElemNode, float LifeTime) const override;

	virtual FBox GetMaxIntersect(FBox Box) const override;

private:
	struct FInRadiusPredicate
	{
		FInRadiusPredicate(const TreeType& InTree, const FVector& InCenter, const Real InRadius, const uint64 InBitChannels = MAX_uint64)
			: Tree(InTree)
			, Center(InCenter)
			, RSquared(InRadius * InRadius)
			, Box(FBox(FVector(Center.X - InRadius, Center.Y - InRadius, Center.Z - InRadius),
				  FVector(Center.X + InRadius, Center.Y + InRadius, Center.Z + InRadius)))
			, BitChannels(InBitChannels)
		{}

		FInRadiusPredicate(const TreeType& InTree, const FVector& InCenter, const Real InRadius, const FBox& Box, const uint64 InBitChannels = MAX_uint64)
			: Tree(InTree)
			, Center(InCenter)
			, RSquared(InRadius * InRadius)
			, Box(Box)
			, BitChannels(InBitChannels)
		{}

		FORCEINLINE bool operator()(const uint16 ObjID) const
		{
			const auto& B = Tree.GetElementBox(ObjID);
			return
				B.IsIntersect(Box.Min, Box.Max) &&
				B.SphereAABBIntersection(Center, RSquared) &&
				(BitChannels & Tree.GetElement(ObjID).BitChannels);
		}

		const TreeType& Tree;
		const FVector Center;
		const Real RSquared;
		const FBox Box;
		const uint64 BitChannels;
	};

	struct FInBoxPredicate
	{
		FInBoxPredicate(const TreeType& InTree, const FBox& InBox, const uint64 InBitChannels = MAX_uint64)
			: Tree(InTree)
			, Box(InBox)
			, BitChannels(InBitChannels)
		{}

		FORCEINLINE bool operator()(const uint16 ObjID) const
		{
			const auto& B = Tree.GetElementBox(ObjID);
			return
				B.IsIntersect(Box.Min, Box.Max) && 
				(BitChannels & Tree.GetElement(ObjID).BitChannels);
		}

		const TreeType& Tree;
		const FBox Box;
		const uint64 BitChannels;
	};
};
