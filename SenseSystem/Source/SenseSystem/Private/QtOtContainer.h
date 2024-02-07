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

template<typename IndexType = int32>
struct FIndexRemoveControl
{
	TArray<IndexType> RemIDs;
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
	using TreeIndexType = int32;
	using ElementIndexType = FSenseSystemModule::ElementIndexType;

	template<typename IndexType = ElementIndexType>
	static constexpr ElementIndexType MaxIndex()
	{
		return TNumericLimits<IndexType>::Max();
	}

	virtual TSparseArray<FSensedStimulus>& GetCompDataPool() = 0;
	virtual const TSparseArray<FSensedStimulus>& GetCompDataPool() const = 0;

	void Lock() const { RWLock.WriteLock(); }
	void UnLock() const { RWLock.WriteUnlock(); }

protected:
	mutable FIndexRemoveControl<ElementIndexType> IndexRemoveControl;
	mutable FRWLock RWLock;

public:
#if WITH_EDITOR
	bool IsRemoveControlClear() const;
#endif

	void MarkRemoveControl() const;
	void ResetRemoveControl() const;
	void ResetRemoveControl(int32 Idx) const;

	/** FStimulusTagResponse */
	virtual bool Remove(ElementIndexType InObjID) = 0;

	virtual ElementIndexType Insert(const FSensedStimulus& ComponentData, FBox InBox) = 0;
	virtual ElementIndexType Insert(FSensedStimulus&& ComponentData, FBox InBox) = 0;
	virtual void Update(ElementIndexType InObjID, FBox NewBox) = 0;
	/** end FStimulusTagResponse */

	/** virtual Tree */
	virtual void Clear() = 0;
	virtual void Collapse() = 0;

	virtual void GetInBoxIDs(FBox Box, TArray<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const = 0;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TArray<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const = 0;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TArray<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const = 0;

	virtual void GetInBoxIDs(FBox Box, TSet<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const = 0;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TSet<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const = 0;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TSet<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const = 0;

	virtual FBox GetMaxIntersect(FBox Box) const = 0;

	virtual void DrawTree(const class UWorld* World, FTreeDrawSetup TreeNode, FTreeDrawSetup Link, FTreeDrawSetup ElemNode, float LifeTime) const {}
	/** end virtual Tree */

	FORCEINLINE int32 Num() const { return GetCompDataPool().Num(); }

	FSensedStimulus GetSensedStimulusCopy_TS(ElementIndexType InObjID) const;
	FSensedStimulus GetSensedStimulusCopy_Simple_TS(ElementIndexType InObjID) const;

	/** TMap<Id, Hash> */
	TArray<ElementIndexType> CheckHash_TS(const TMap<ElementIndexType, uint32>& InArr) const;
	TArray<ElementIndexType, TMemStackAllocator<>> CheckHashStack_TS(const TMap<ElementIndexType, uint32>& InArr) const;
	TSet<ElementIndexType> CheckHashSet_TS(const TMap<ElementIndexType, uint32>& InArr) const;

	FORCEINLINE const FSensedStimulus& GetSensedStimulus(const ElementIndexType InObjID) const { return GetCompDataPool()[InObjID]; }
	FORCEINLINE FSensedStimulus& GetSensedStimulus(const ElementIndexType InObjID) { return GetCompDataPool()[InObjID]; }
	FORCEINLINE FSensedStimulus GetSensedStimulusCopy(const ElementIndexType InObjID) const { return GetCompDataPool()[InObjID]; }

	void SetAge_TS(ElementIndexType ID, float AgeValue);
	void SetScore_TS(ElementIndexType ID, float ScoreValue);
	void SetChannels_TS(ElementIndexType ID, uint64 Channels);
	void SetSensedPoints_TS(ElementIndexType ID, const TArray<FSensedPoint>& InSensedPoints, float InCurrentTime);
	void SetSensedPoints_TS(ElementIndexType ID, const FSensedPoint& InSensedPoints, float InCurrentTime);

	void SetSensedPoints_TS(ElementIndexType ID, TArray<FSensedPoint>&& InSensedPoints, float InCurrentTime);
	void SetSensedPoints_TS(ElementIndexType ID, FSensedPoint&& InSensedPoints, float InCurrentTime);
};

/** QuadTree */
class SENSESYSTEM_API FSenseSys_QuadTree final : public IContainerTree
{
private:
	using ElementIndexType = IContainerTree::ElementIndexType;
	using TreeIndexType = IContainerTree::TreeIndexType;
	using TreeType = TTree_Base<FSensedStimulus, FVector2D, ElementIndexType, TreeIndexType, 2U>;
	TreeType Tree;

	virtual TSparseArray<FSensedStimulus>& GetCompDataPool() override { return Tree.GetElementPool(); }
	virtual const TSparseArray<FSensedStimulus>& GetCompDataPool() const override { return Tree.GetElementPool(); }

public:
	using Real = FVector::FReal;

	explicit FSenseSys_QuadTree( //
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
	virtual bool Remove(ElementIndexType InObjID) override;
	virtual ElementIndexType Insert(FSensedStimulus&& ComponentData, FBox InBox) override;
	virtual ElementIndexType Insert(const FSensedStimulus& ComponentData, FBox InBox) override;
	virtual void Update(ElementIndexType InObjID, FBox NewBox) override;
	virtual void Clear() override;
	virtual void Collapse() override;

	virtual void GetInBoxIDs(FBox Box, TArray<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TArray<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TArray<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;

	virtual void GetInBoxIDs(FBox Box, TSet<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TSet<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TSet<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;

	virtual void DrawTree(const class UWorld* World, FTreeDrawSetup TreeNode, FTreeDrawSetup Link, FTreeDrawSetup ElemNode, float LifeTime) const override;

	virtual FBox GetMaxIntersect(FBox Box) const override;

private:
	struct FInRadiusPredicate
	{
		FInRadiusPredicate(const TreeType& InTree, const FVector& InCenter, const Real InRadius, const FBox2D& InBox, const uint64 InBitChannels = MAX_uint64)
			: Tree(InTree)
			, Center(InCenter)
			, Radius(InRadius)
			, Box(InBox)
			, BitChannels(InBitChannels)
		{}
		FInRadiusPredicate(const TreeType& InTree, const FVector& InCenter, const Real InRadius, const uint64 InBitChannels = MAX_uint64)
			: Tree(InTree)
			, Center(InCenter)
			, Radius(InRadius)
			, Box(FBox2D(FVector2D(Center.X - InRadius, Center.Y - InRadius), FVector2D(Center.X + InRadius, Center.Y + InRadius)))
			, BitChannels(InBitChannels)
		{}

		FORCEINLINE bool operator()(const ElementIndexType ObjID) const
		{
			const auto& B = Tree.GetElementBox(ObjID);
			return B.IsIntersect(Box.Min, Box.Max) && B.SphereAABBIntersection(Center, Radius) && (BitChannels & Tree.GetElement(ObjID).BitChannels);
		}

		const TreeType& Tree;
		const FVector2D Center;
		const Real Radius;
		const FBox2D Box;
		const uint64 BitChannels;
	};

	struct FInBoxPredicate
	{
		FInBoxPredicate(const TreeType& InQt, const FBox2D& InBox, const uint64 InBitChannels = MAX_uint64) : Tree(InQt), Box(InBox), BitChannels(InBitChannels)
		{}

		FORCEINLINE bool operator()(const ElementIndexType ObjID) const
		{
			const auto& B = Tree.GetElementBox(ObjID);
			return B.IsIntersect(Box.Min, Box.Max) && (BitChannels & Tree.GetElement(ObjID).BitChannels);
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
	using ElementIndexType = IContainerTree::ElementIndexType;
	using TreeIndexType = IContainerTree::TreeIndexType;
	using TreeType = TTree_Base<FSensedStimulus, FVector, ElementIndexType, TreeIndexType, 3U>;
	TreeType Tree;

	virtual TSparseArray<FSensedStimulus>& GetCompDataPool() override { return Tree.GetElementPool(); }
	virtual const TSparseArray<FSensedStimulus>& GetCompDataPool() const override { return Tree.GetElementPool(); }

public:
	using Real = FVector::FReal;

	explicit FSenseSys_OcTree( //
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


	virtual bool Remove(ElementIndexType InObjID) override;
	virtual ElementIndexType Insert(FSensedStimulus&& ComponentData, FBox InBox) override;
	virtual ElementIndexType Insert(const FSensedStimulus& ComponentData, FBox InBox) override;
	virtual void Update(ElementIndexType InObjID, FBox NewBox) override;
	virtual void Clear() override;
	virtual void Collapse() override;

	virtual void GetInBoxIDs(FBox Box, TArray<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TArray<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TArray<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;

	virtual void GetInBoxIDs(FBox Box, TSet<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInRadiusIDs(Real Radius, FVector Center, TSet<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;
	virtual void GetInBoxRadiusIDs(FBox Box, FVector Center, Real Radius, TSet<ElementIndexType>& Out, uint64 InBitChannels = MAX_uint64) const override;

	virtual void DrawTree(const class UWorld* World, FTreeDrawSetup TreeNode, FTreeDrawSetup Link, FTreeDrawSetup ElemNode, float LifeTime) const override;

	virtual FBox GetMaxIntersect(FBox Box) const override;

private:
	struct FInRadiusPredicate
	{
		FInRadiusPredicate(const TreeType& InTree, const FVector& InCenter, const Real InRadius, const uint64 InBitChannels = MAX_uint64)
			: Tree(InTree)
			, Center(InCenter)
			, RSquared(InRadius * InRadius)
			, Box(FBox(
				  FVector(Center.X - InRadius, Center.Y - InRadius, Center.Z - InRadius),
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

		FORCEINLINE bool operator()(const ElementIndexType ObjID) const
		{
			const auto& B = Tree.GetElementBox(ObjID);
			return B.IsIntersect(Box.Min, Box.Max) && B.SphereAABBIntersection(Center, RSquared) && (BitChannels & Tree.GetElement(ObjID).BitChannels);
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

		FORCEINLINE bool operator()(const ElementIndexType ObjID) const
		{
			const auto& B = Tree.GetElementBox(ObjID);
			return B.IsIntersect(Box.Min, Box.Max) && (BitChannels & Tree.GetElement(ObjID).BitChannels);
		}

		const TreeType& Tree;
		const FBox Box;
		const uint64 BitChannels;
	};
};
