// Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "QtOtContainer.h"

#include "DrawDebugHelpers.h"


#if WITH_EDITOR
bool IContainerTree::IsRemoveControlClear() const
{
	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	return IndexRemoveControl.bTrack == 0;
}
#endif

void IContainerTree::MarkRemoveControl() const
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);

	if (IndexRemoveControl.bTrack == 0)
	{
		IndexRemoveControl.bRemove = false;
		IndexRemoveControl.RemIDs.Empty(8);
	}
	IndexRemoveControl.bTrack++;
}

void IContainerTree::ResetRemoveControl() const
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);

	IndexRemoveControl.bTrack--;
	if (IndexRemoveControl.bTrack == 0)
	{
		IndexRemoveControl.bRemove = false;
		IndexRemoveControl.RemIDs.Empty(8);
	}
}


void IContainerTree::ResetRemoveControl(const int32 Idx) const
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);
	IndexRemoveControl.RemIDs.RemoveAtSwap(Idx);
	if (IndexRemoveControl.RemIDs.Num() == 0)
	{
		IndexRemoveControl.bRemove = false;
	}
}

FSensedStimulus IContainerTree::GetSensedStimulusCopy_TS(const ElementIndexType InObjID) const
{
	int32 ID = INDEX_NONE;
	{
		FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);
		if (IndexRemoveControl.bTrack && IndexRemoveControl.bRemove)
		{
			ID = IndexRemoveControl.RemIDs.IndexOfByKey(InObjID);
		}
		if (ID == INDEX_NONE)
		{
			if (GetCompDataPool().IsValidIndex(InObjID))
			{
				return GetSensedStimulus(InObjID);
			}
			return FSensedStimulus();
		}
	}
	ResetRemoveControl(ID);
	return FSensedStimulus();
}

FSensedStimulus IContainerTree::GetSensedStimulusCopy_Simple_TS(const ElementIndexType InObjID) const
{
	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);
	if (GetCompDataPool().IsValidIndex(InObjID))
	{
		return GetSensedStimulus(InObjID);
	}
	return FSensedStimulus();
}

TArray<IContainerTree::ElementIndexType> IContainerTree::CheckHash_TS(const TMap<ElementIndexType, uint32>& InArr) const
{
	TArray<ElementIndexType> OutIDs;
	OutIDs.Reserve(InArr.Num());

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);
	const TSparseArray<FSensedStimulus>& P = GetCompDataPool();
	for (const auto& Elem : InArr)
	{
		const ElementIndexType ID = Elem.Key;
		if (P.IsValidIndex(ID) && Elem.Value == P[ID].TmpHash)
		{
			OutIDs.Add(ID);
		}
	}
	return MoveTemp(OutIDs);
}

TArray<IContainerTree::ElementIndexType, TMemStackAllocator<>> IContainerTree::CheckHashStack_TS(const TMap<ElementIndexType, uint32>& InArr) const
{
	TArray<ElementIndexType, TMemStackAllocator<>> OutIDs;
	OutIDs.Reserve(InArr.Num());

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	for (const auto& Elem : InArr)
	{
		if (GetCompDataPool().IsValidIndex(Elem.Key) && Elem.Value == GetSensedStimulus(Elem.Key).TmpHash)
		{
			OutIDs.Add(Elem.Key);
		}
	}
	return MoveTemp(OutIDs);
}

TSet<IContainerTree::ElementIndexType> IContainerTree::CheckHashSet_TS(const TMap<ElementIndexType, uint32>& InArr) const
{
	TSet<ElementIndexType> OutIDs;
	OutIDs.Reserve(InArr.Num());

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	for (const auto& Elem : InArr)
	{
		if (GetCompDataPool().IsValidIndex(Elem.Key) && Elem.Value == GetSensedStimulus(Elem.Key).TmpHash)
		{
			OutIDs.Add(Elem.Key);
		}
	}
	return MoveTemp(OutIDs);
}


void IContainerTree::SetAge_TS(const ElementIndexType ID, const float AgeValue)
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);
	if (GetCompDataPool().IsValidIndex(ID))
	{
		GetSensedStimulus(ID).Age = AgeValue;
	}
}

void IContainerTree::SetScore_TS(const ElementIndexType ID, const float ScoreValue)
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);
	if (GetCompDataPool().IsValidIndex(ID))
	{
		GetSensedStimulus(ID).Score = ScoreValue;
	}
}

void IContainerTree::SetChannels_TS(const ElementIndexType ID, const uint64 Channels)
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);
	if (GetCompDataPool().IsValidIndex(ID))
	{
		GetSensedStimulus(ID).BitChannels = Channels;
	}
}

void IContainerTree::SetSensedPoints_TS(const ElementIndexType ID, const TArray<FSensedPoint>& InSensedPoints, const float InCurrentTime)
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);
	if (GetCompDataPool().IsValidIndex(ID))
	{
		GetSensedStimulus(ID).SensedPoints = InSensedPoints;
		GetSensedStimulus(ID).SensedTime = InCurrentTime;
	}
}

void IContainerTree::SetSensedPoints_TS(const ElementIndexType ID, const FSensedPoint& InSensedPoints, const float InCurrentTime)
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);
	if (GetCompDataPool().IsValidIndex(ID))
	{
		GetSensedStimulus(ID).SensedPoints[0] = InSensedPoints;
		GetSensedStimulus(ID).SensedTime = InCurrentTime;
	}
}


void IContainerTree::SetSensedPoints_TS(const ElementIndexType ID, TArray<FSensedPoint>&& InSensedPoints, const float InCurrentTime)
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);
	if (GetCompDataPool().IsValidIndex(ID))
	{
		GetSensedStimulus(ID).SensedPoints = InSensedPoints;
		GetSensedStimulus(ID).SensedTime = InCurrentTime;
	}
}

void IContainerTree::SetSensedPoints_TS(const ElementIndexType ID, FSensedPoint&& InSensedPoints, const float InCurrentTime)
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);
	if (GetCompDataPool().IsValidIndex(ID))
	{
		GetSensedStimulus(ID).SensedPoints[0] = InSensedPoints;
		GetSensedStimulus(ID).SensedTime = InCurrentTime;
	}
}


IContainerTree::ElementIndexType FSenseSys_QuadTree::Insert(const FSensedStimulus& ComponentData, const FBox InBox)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_QuadTree_Insert);

	FRWScopeLock SRWLock(RWLock, SLT_Write);

	return Tree.Insert(ComponentData, TreeHelper::ToBox2D(InBox));
}
IContainerTree::ElementIndexType FSenseSys_QuadTree::Insert(FSensedStimulus&& ComponentData, const FBox InBox)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_QuadTree_Insert);

	FRWScopeLock SRWLock(RWLock, SLT_Write);

	return Tree.Insert(MoveTemp(ComponentData), TreeHelper::ToBox2D(InBox));
}

void FSenseSys_QuadTree::Update(const ElementIndexType InObjID, const FBox NewBox)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_QuadTree_Update);

	FRWScopeLock SRWLock(RWLock, SLT_Write);

	if (InObjID != MaxIndex())
	{
		Tree.Update(InObjID, TreeHelper::ToBox2D(NewBox));
	}
}

bool FSenseSys_QuadTree::Remove(const ElementIndexType InObjID)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_QuadTree_Remove);

	FRWScopeLock SRWLock(RWLock, SLT_Write);

	if (IndexRemoveControl.bTrack)
	{
		IndexRemoveControl.RemIDs.Add(InObjID);
		IndexRemoveControl.bRemove = true;
	}
	check(InObjID != MaxIndex());
	Tree.Remove(InObjID);

	return true;
}

void FSenseSys_QuadTree::Clear()
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);

	Tree.Clear();
}

void FSenseSys_QuadTree::Collapse()
{
	FRWScopeLock SRWLock(RWLock, SLT_Write);

	Tree.CollapseQt();
}


IContainerTree::ElementIndexType FSenseSys_OcTree::Insert(const FSensedStimulus& ComponentData, const FBox InBox)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_Insert);

	FRWScopeLock SRWLock(RWLock, SLT_Write);

	return Tree.Insert(ComponentData, InBox);
}
IContainerTree::ElementIndexType FSenseSys_OcTree::Insert(FSensedStimulus&& ComponentData, const FBox InBox)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_Insert);

	FRWScopeLock SRWLock(RWLock, SLT_Write);

	return Tree.Insert(MoveTemp(ComponentData), InBox);
}

void FSenseSys_OcTree::Update(const ElementIndexType InObjID, const FBox NewBox)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_Update);

	FRWScopeLock SRWLock(RWLock, SLT_Write);

	if (InObjID != MaxIndex())
	{
		Tree.Update(InObjID, NewBox);
	}
}

bool FSenseSys_OcTree::Remove(const ElementIndexType InObjID)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_Remove);

	FRWScopeLock SRWLock(RWLock, SLT_Write);

	if (IndexRemoveControl.bTrack)
	{
		IndexRemoveControl.RemIDs.Add(InObjID);
		IndexRemoveControl.bRemove = true;
	}
	check(InObjID != MaxIndex());
	Tree.Remove(InObjID);
	return true;
}

void FSenseSys_OcTree::Clear()
{

	FRWScopeLock SRWLock(RWLock, SLT_Write);

	Tree.Clear();
}

void FSenseSys_OcTree::Collapse()
{

	FRWScopeLock SRWLock(RWLock, SLT_Write);

	Tree.CollapseQt();
}


void FSenseSys_QuadTree::GetInBoxIDs(const FBox Box, TArray<ElementIndexType>& Out, const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInBox);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const FBox2D Box2D(FVector2D(Box.Min), FVector2D(Box.Max));
	const auto InBox = FInBoxPredicate(Tree, Box2D, InBitChannels);
	Tree.GetElementsIDs(Box2D, InBox, Out);
}
void FSenseSys_QuadTree::GetInRadiusIDs(const FSenseSys_QuadTree::Real Radius, const FVector Center, TArray<ElementIndexType>& Out, const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInRadius);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const auto InRadius = FInRadiusPredicate(Tree, Center, Radius, InBitChannels);
	Tree.GetElementsIDs(InRadius.Box, InRadius, Out);
}
void FSenseSys_QuadTree::GetInBoxRadiusIDs(
	const FBox Box,
	const FVector Center,
	const FSenseSys_QuadTree::Real Radius,
	TArray<ElementIndexType>& Out,
	const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInBoxRadius);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const FBox2D Box2D(FVector2D(Box.Min), FVector2D(Box.Max));
	const auto InRadius = FInRadiusPredicate(Tree, Center, Radius, Box2D, InBitChannels);
	Tree.GetElementsIDs(InRadius.Box, InRadius, Out);
}

void FSenseSys_QuadTree::GetInBoxIDs(const FBox Box, TSet<ElementIndexType>& Out, const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInBox);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const FBox2D Box2D(FVector2D(Box.Min), FVector2D(Box.Max));
	const auto InBox = FInBoxPredicate(Tree, Box2D, InBitChannels);
	Tree.GetElementsIDs(Box2D, InBox, Out);
}
void FSenseSys_QuadTree::GetInRadiusIDs(const FSenseSys_QuadTree::Real Radius, const FVector Center, TSet<ElementIndexType>& Out, const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInRadius);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const auto InRadius = FInRadiusPredicate(Tree, Center, Radius, InBitChannels);
	Tree.GetElementsIDs(InRadius.Box, InRadius, Out);
}
void FSenseSys_QuadTree::GetInBoxRadiusIDs(const FBox Box, const FVector Center, const FSenseSys_QuadTree::Real Radius, TSet<ElementIndexType>& Out, const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInBoxRadius);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const FBox2D Box2D(FVector2D(Box.Min), FVector2D(Box.Max));
	const auto InRadius = FInRadiusPredicate(Tree, Center, Radius, Box2D, InBitChannels);
	Tree.GetElementsIDs(InRadius.Box, InRadius, Out);
}

FBox FSenseSys_QuadTree::GetMaxIntersect(const FBox Box) const
{
	const TreeIndexType MaxIntersect = Tree.GetMaxIntersect(TreeHelper::ToBox2D(Box));
	if (MaxIntersect != MaxIndex())
	{
		const FBox2D Box2D = Tree.GetTreeBox(MaxIntersect);
		return TreeHelper::ToBox(Box2D);
	}
	return FBox(FVector::ZeroVector, FVector::ZeroVector);
}


void FSenseSys_OcTree::GetInBoxIDs(const FBox Box, TArray<ElementIndexType>& Out, const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInBox);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const auto InBox = FInBoxPredicate(Tree, Box, InBitChannels);
	Tree.GetElementsIDs(Box, InBox, Out);
}
void FSenseSys_OcTree::GetInRadiusIDs(const FSenseSys_OcTree::Real Radius, const FVector Center, TArray<ElementIndexType>& Out, const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInBox);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const auto InRadius = FInRadiusPredicate(Tree, Center, Radius, InBitChannels);
	Tree.GetElementsIDs(InRadius.Box, InRadius, Out);
}
void FSenseSys_OcTree::GetInBoxRadiusIDs(
	const FBox Box,
	const FVector Center,
	const FSenseSys_OcTree::Real Radius,
	TArray<ElementIndexType>& Out,
	const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInBoxRadius);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const auto InRadius = FInRadiusPredicate(Tree, Center, Radius, Box, InBitChannels);
	Tree.GetElementsIDs(InRadius.Box, InRadius, Out);
}

void FSenseSys_OcTree::GetInBoxIDs(const FBox Box, TSet<ElementIndexType>& Out, const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInBox);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const auto InBox = FInBoxPredicate(Tree, Box, InBitChannels);
	Tree.GetElementsIDs(Box, InBox, Out);
}
void FSenseSys_OcTree::GetInRadiusIDs(const FSenseSys_OcTree::Real Radius, const FVector Center, TSet<ElementIndexType>& Out, const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInBox);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const auto InRadius = FInRadiusPredicate(Tree, Center, Radius, InBitChannels);
	Tree.GetElementsIDs(InRadius.Box, InRadius, Out);
}
void FSenseSys_OcTree::GetInBoxRadiusIDs(const FBox Box, const FVector Center, const FSenseSys_OcTree::Real Radius, TSet<ElementIndexType>& Out, const uint64 InBitChannels) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_OcTree_GetInBoxRadius);

	FRWScopeLock SRWLock(RWLock, SLT_ReadOnly);

	const auto InRadius = FInRadiusPredicate(Tree, Center, Radius, Box, InBitChannels);
	Tree.GetElementsIDs(InRadius.Box, InRadius, Out);
}

FBox FSenseSys_OcTree::GetMaxIntersect(const FBox Box) const
{
	const TreeIndexType MaxIntersect = Tree.GetMaxIntersect(Box);
	if (MaxIntersect != MaxIndex())
	{
		return FBox(Tree.GetTreeBox(MaxIntersect));
	}
	return FBox(FVector::ZeroVector, FVector::ZeroVector);
}

void FSenseSys_QuadTree::DrawTree(const class UWorld* World, const FTreeDrawSetup TreeNode, const FTreeDrawSetup Link, const FTreeDrawSetup ElemNode, const float LifeTime) const
{
#if ENABLE_DRAW_DEBUG
	Tree.DrawTree(
		World,
		LifeTime,
		TreeNode.Color,
		TreeNode.Thickness,
		TreeNode.DrawDepth,
		Link.Color,
		Link.Thickness,
		Link.DrawDepth,
		ElemNode.Color,
		ElemNode.Thickness,
		ElemNode.DrawDepth);
#endif //ENABLE_DRAW_DEBUG
}

void FSenseSys_OcTree::DrawTree(const class UWorld* World, const FTreeDrawSetup TreeNode, const FTreeDrawSetup Link, const FTreeDrawSetup ElemNode, const float LifeTime) const
{
#if ENABLE_DRAW_DEBUG
	Tree.DrawTree(
		World,
		LifeTime,
		TreeNode.Color,
		TreeNode.Thickness,
		TreeNode.DrawDepth,
		Link.Color,
		Link.Thickness,
		Link.DrawDepth,
		ElemNode.Color,
		ElemNode.Thickness,
		ElemNode.DrawDepth);
#endif //ENABLE_DRAW_DEBUG
}
