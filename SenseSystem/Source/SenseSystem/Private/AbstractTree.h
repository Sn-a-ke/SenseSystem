// Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "HAL/Platform.h"
#include "Math/NumericLimits.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"
#include "Misc/AssertionMacros.h"
#include "Templates/Function.h"
#include "Templates/TypeHash.h"
#include "Templates/UnrealTemplate.h"

#include "Containers/Array.h"
#include "Containers/ContainerAllocationPolicies.h"
#include "Containers/SparseArray.h"
#include "Containers/StaticArray.h"
#include "Containers/UnrealString.h"

#include "DrawDebugHelpers.h"


#if !defined(IF_CONSTEXPR)
	#define IF_CONSTEXPR if constexpr
#endif


/* Vector 2d-3d conversion Vector2dFrom3d, Vector3dFrom2d */
namespace TreeHelper
{
	using namespace UE::Math;

	template<uint32 InSize>
	struct FVectorSpace
	{
		FVectorSpace() {}
		static constexpr int32 Size = InSize;
		static constexpr int32 GetInt32 = static_cast<uint32>(Size);
		static constexpr bool IsEqual(uint32 Value) { return Size == Value; }
		static constexpr uint32 SubTravelNum()
		{
			if constexpr (IsEqual(1))
			{
				return 2;
	}
			if constexpr (IsEqual(2))
			{
				return 4;
			}
			if constexpr (IsEqual(3))
			{
				return 8;
			}
			checkNoEntry();
			return 0;
		}
	};

	static FORCEINLINE FVector2D Vector2dFrom3d(const FVector& V)
	{
		return FVector2D{V.X, V.Y};
	}
	static FORCEINLINE FVector Vector3dFrom2d(const FVector2D& V, FVector::FReal Val = 0.0)
	{
		return FVector{V.X, V.Y, Val};
	}

	static FORCEINLINE FBox2D ToBox2D(const FBox& InBox)
	{
		return FBox2D(FVector2D(InBox.Min[0], InBox.Min[1]), FVector2D(InBox.Max[0], InBox.Max[1]));
	}
	static FORCEINLINE FBox ToBox(const FBox2D& InBox)
	{
		return FBox(FVector(InBox.Min[0], InBox.Min[1], 0), FVector(InBox.Max[0], InBox.Max[1], 0));
	}

	/*
		None         = 0,
		Top-Left     = 1,
		Top-Right    = 2,
		Bottom-Left  = 4,
		Bottom-Right = 8

		https://i.imgur.com/un40fel.png
		2d
		0--------*--------1
		|  +y+x  |  +y-x  |
		|  000   |  001   |
		|  TL 0  |  TR 1  |
		o--------C--------o - Z
		|  -y+x  |  -y-x  |
		|  010   |  011   |
		|  BL 2  |  BR 3  |
		2--------o--------3

		   -z
		3d +z
		4--------*--------5
		|        |        |
		|   100  |   101  |
		|    4   |    5   |
		o--------C--------o - Z
		|        |        |
		|   110  |   111  |
		|    6   |    7   |
		6--------o--------7

		// i0: 0000
		// i1: 0001
		// i2: 0010
		// i3: 0011
		// i4: 0100
		// i5: 0101
		// i6: 0110
		// i7: 0111
	*/
} // namespace TreeHelper
using namespace TreeHelper;

/**	Tree Box */
template<typename PointType, uint32 InVectorSpace>
struct TTreeBox
{
	using Real = typename PointType::FReal;
	using VSpace = FVectorSpace<InVectorSpace>;

	TTreeBox()
		: Min(0) //
		, Max(0)
		, Center(0)
	{}
	explicit TTreeBox(Real HalfSize)
		: Min(PointType{-HalfSize, -HalfSize}) //
		, Max(PointType{HalfSize, HalfSize})
		, Center((Max + Min) / 2)
	{}
	explicit TTreeBox(PointType Point)
		: Min(Point) //
		, Max(Point)
		, Center(Point)
	{}
	TTreeBox(const PointType& InMin, const PointType& InMax)
	{
		for (int32 i = 0; i < VSpace::GetInt32; ++i)
		{
			min[i] = FMath::Min<Real>(InMin[i], InMax[i]);
			max[i] = FMath::Max<Real>(InMin[i], InMax[i]);
		}
		Center = (InMax + InMin) / 2;
	}

	TTreeBox(const PointType& Location, Real MinimumQuadSize)
	{
		const Real* RESTRICT L = reinterpret_cast<const Real*>(&Location);
		Real* RESTRICT Mi = reinterpret_cast<Real*>(&min);
		Real* RESTRICT Ma = reinterpret_cast<Real*>(&max);
		Real* RESTRICT Ce = reinterpret_cast<Real*>(&Center);
		MinimumQuadSize = FMath::Abs(MinimumQuadSize);
		for (int32 i = 0; i < VSpace::GetInt32; ++i)
		{
			const Real Value = L[i] / MinimumQuadSize;
			int32 Li = FMath::CeilToInt(Value);
			Li += static_cast<int32>(Li == FMath::FloorToInt(Value));
			Mi[i] = (Li - 1) * MinimumQuadSize;
			if (Mi[i] >= L[i]) // unreliable CeilToInt
			{
				Ma[i] = Mi[i];
				Mi[i] = (Li - 2) * MinimumQuadSize;
			}
			else
			{
				Ma[i] = Li * MinimumQuadSize;
			}
			Ce[i] = (Mi[i] + Ma[i]) / 2;
		}
#if WITH_EDITOR
		checkf(
			IsInside(Location),
			TEXT("Error! TTreeBox(PointType Location, Real MinimumQuadSize):"
				 "\nIsInside(Location) -> Location[i] > Min[i] || Location[i] <= Max[i] = false, "
				 "\nTTreeBox::Min =\t %s  <"
				 "\nLocation =\t\t %s  <="
				 "\nTTreeBox::Max =\t %s"
				 "\nMinimumQuadSize = %f"),
			*PointTypeToString(min),
			*PointTypeToString(Location),
			*PointTypeToString(max),
			MinimumQuadSize);
#endif
	}

	template<typename OtherBoxType>
	TTreeBox(OtherBoxType OtherBox) //
		: TTreeBox(OtherBox.Min, OtherBox.Max)
	{}

	static TTreeBox BuildAABB(PointType Origin, PointType Extent) //
	{
		return TTreeBox(Origin - Extent, Origin + Extent);
	}


	PointType min;
	PointType max;
	PointType Center;

public:
#if WITH_EDITOR
	static FString PointTypeToString(const PointType& In)
	{
		FString Out;
		for (int32 i = 0; i < VSpace::GetInt32; ++i)
		{
			Out.Append(FString::Printf(TEXT("D_%d = %f"), i, In[i]));
			if (i != (VSpace::GetInt32 - 1))
			{
				Out.Append(FString::Printf(TEXT(", ")));
			}
		}
		return Out;
	}
#endif

public:
	FORCEINLINE const PointType& Min() const { return min; }
	FORCEINLINE const PointType GetCenter() const { return Center; }
	FORCEINLINE const PointType& Max() const { return max; }
	FORCEINLINE PointType GetSize() const { return (max - min); }
	FORCEINLINE PointType GetExtent() const { return GetSize() / 2; }

	FORCEINLINE bool IsInside(const PointType& TestPoint) const
	{
		for (int32 i = 0; i < VSpace::GetInt32; ++i)
		{
			if (TestPoint[i] <= min[i] || TestPoint[i] > max[i])
			{
				return false;
			}
		}
		return true;
	}
	FORCEINLINE bool IsInside(const TTreeBox& Box) const { return (IsInside(Box.Min()) && IsInside(Box.Max())); }

	FORCEINLINE bool IsIntersect(const PointType& BoxMin, const PointType& BoxMax) const
	{
		for (int32 i = 0; i < VSpace::GetInt32; ++i)
		{
			if (min[i] >= BoxMax[i] && BoxMin[i] > max[i])
			{
				return false;
			}
		}
		return true;
	}
	FORCEINLINE bool IsIntersect(const TTreeBox& Box) const { return IsIntersect(Box.Min(), Box.Max()); }

	FORCEINLINE bool operator==(const TTreeBox& Box) const { return min == Box.Min() && max == Box.Max(); }
	FORCEINLINE bool operator!=(const TTreeBox& Box) const { return !(*this == Box); }
	friend FArchive& operator<<(FArchive& Ar, TTreeBox& In)
	{
		Ar << In.min;
		Ar << In.max;
		Ar << In.Center;
		return Ar;
	}

	FORCEINLINE operator PointType() const { return GetCenter(); }

	FORCEINLINE bool SphereAABBIntersection(const PointType& SphereCenter, const Real Radius) const
	{
		Real Dist = 0.0;
		for (int32 i = 0; i < VSpace::GetInt32; ++i)
		{
			if (SphereCenter[i] < min[i])
			{
				Dist += SphereCenter[i] - min[i];
			}
			else if (SphereCenter[i] > max[i])
			{
				Dist += SphereCenter[i] - max[i];
			}
		}
		return Dist <= Radius;
	}

	FORCEINLINE operator FBox2D() const
	{
		/*
		//if (VSpace::Size == 1U)
		//{
		//	return FBox2D(min, max);
		//}
		*/
		if (VSpace::Size == 2U)
		{
			return FBox2D(min, max);
		}
		if (VSpace::Size == 3U)
		{
			return FBox2D(FVector2D{min[0], min[1]}, FVector2D{max[0], max[1]});
		}
		checkNoEntry();
		return FBox2D();
	}
	FORCEINLINE operator FBox() const
	{
		if (VSpace::Size == 3U)
		{
			return FBox(min, max);
		}
		if (VSpace::Size == 2U)
		{
			return FBox(FVector{min[0], min[1], 0}, FVector{max[0], max[1], 0});
		}
		checkNoEntry();
		return FBox();
	}
};


/**	Tree Node */
template<
	typename TreeElementIdxType, // Element Idx in Element pool
	typename IndexQtType,		 // Tree Idx in Tree pool
	typename PointType,			 // PointType = Vector[n]
	uint32 VectorSpace = 3U,	 // Vector[VectorSpace]
	int32 InlineNodeNum = 36>	 // --
class TTreeNode
{
public:
	using VSpace = FVectorSpace<VectorSpace>;

	static constexpr IndexQtType MaxIndexQt = TNumericLimits<IndexQtType>::Max();
	static constexpr int32 SubNodesNum = VSpace::SubTravelNum();

	static_assert(SubNodesNum > 0, "Error TTreeNode SubNodesNum == 0 !");
	static_assert(VectorSpace > 1U && VectorSpace < 4U, "TTreeNode: DimensionSize error");

	using Real = typename PointType::FReal;
	using ElementNodeType = TreeElementIdxType;
	using BoxType = TTreeBox<PointType, VectorSpace>;

	TTreeNode() : SubNodes(TStaticArray<IndexQtType, SubNodesNum>(InPlace, MaxIndexQt)), TreeBox(0) {}
	TTreeNode(IndexQtType InParent, const BoxType& Box) //
		: SubNodes(TStaticArray<IndexQtType, SubNodesNum>(InPlace, MaxIndexQt))
		, Parent(InParent)
		, TreeBox(Box)
	{}
	TTreeNode(IndexQtType InParent, BoxType&& Box)
		: SubNodes(TStaticArray<IndexQtType, SubNodesNum>(InPlace, MaxIndexQt))
		, Parent(InParent)
		, TreeBox(MoveTemp(Box))
	{}

	FORCEINLINE bool operator==(const TTreeNode& Other) const { return Self_ID == Other.Self_ID; }
	FORCEINLINE friend uint32 GetTypeHash(const TTreeNode& In) { return GetTypeHash(In.Self_ID); }

	TTreeNode& operator=(const TTreeNode& Other)
	{
		SubNodes = Other.SubNodes;
		Self_ID = Other.Self_ID;
		Parent = Other.Parent;
		ContainsCount = Other.ContainsCount;
		TreeBox = Other.TreeBox;
		Nodes = Other.Nodes;
		return *this;
	}

	TStaticArray<IndexQtType, SubNodesNum> SubNodes;
	IndexQtType Self_ID = MaxIndexQt;
	IndexQtType Parent = MaxIndexQt;

	int32 ContainsCount = 0;
	BoxType TreeBox;
	TArray<ElementNodeType, TInlineAllocator<InlineNodeNum>> Nodes; // 16 + InlineAllocator aligned


	FORCEINLINE bool IsLeaf() const { return SubNodes[0] == MaxIndexQt; }
	FORCEINLINE int32 Num() const { return ContainsCount; }

	FORCEINLINE const BoxType& GetTreeBox() const { return TreeBox; }
	FORCEINLINE BoxType& GetTreeBox() { return TreeBox; }

	FORCEINLINE bool IsIntersect(const BoxType& InBox) const { return GetTreeBox().IsIntersect(InBox); }
	FORCEINLINE bool IsIntersect(const PointType& Point) const { return GetTreeBox().IsInside(Point); }

	FORCEINLINE bool IsInside(const BoxType& InBox) const { return GetTreeBox().IsInside(InBox); }
	FORCEINLINE bool IsInside(const PointType& Point) const { return GetTreeBox().IsInside(Point); }

	IndexQtType GetByQuadName(const uint8 QuadName) const
	{
		for (int32 i = 0; i < SubNodesNum; i++)
		{
			if (QuadName == static_cast<uint8>(1U) << i)
			{
				return SubNodes[i];
			}
		}
		return MaxIndexQt;
	}
	IndexQtType& GetByQuadNameRef(const uint8 QuadName)
	{
		for (int32 i = 0; i < SubNodesNum; i++)
		{
			if (QuadName == static_cast<uint8>(1U) << i)
			{
				return SubNodes[i];
			}
		}
		checkNoEntry();
		UE_ASSUME(0);
		return SubNodes[0];
	}

	FORCEINLINE uint8 GetQuad(const PointType& P) const
	{
#if WITH_EDITOR
		check(!IsLeaf());
#endif
		const auto BoxCenter = GetTreeBox().GetCenter();
		return GetIDByPos(BoxCenter, P);
	}

	uint8 GetQuads(const BoxType& Box) const
	{
		uint8 QuadNames = 0;

#if WITH_EDITOR
		check(!IsLeaf());
#endif

		/*
		//example VectorSpace = 4
		//QuadNames |= GetIDByPos_t(BoxCenter, PointType(InBox.Max()));
		//QuadNames |= GetIDByPos_t(BoxCenter, PointType(InBox.Min()));
		//QuadNames |= GetIDByPos_t(BoxCenter, PointType(InBox.Max()[0], InBox.Min()[1]));
		//QuadNames |= GetIDByPos_t(BoxCenter, PointType(InBox.Min()[0], InBox.Max()[1]));
		
		     Max  Max    Min   Min
		   .  X  .  Y  .  X  .  Y
		 ---------------------------
		 0 |  x  |  y  |     |     |
		 ---------------------------
		 1 |     |  y  |  x  |     |
		 ---------------------------
		 2 |     |     |  x  |  y  |
		 ---------------------------
		 3 |  x  |     |     |  y  |
		 ---------------------------			
		*/

		const auto BoxCenter = GetTreeBox().GetCenter();
		for (int32 i = 0; i < SubNodesNum; ++i)
		{
			PointType v = PointType();
			for (int32 j = 0; j < VSpace::GetInt32; j++)
			{
				v[j] = ((i >> j) & 1) //
					? Box.Min()[j]
					: Box.Max()[j];
			}
			QuadNames |= GetIDByPos(BoxCenter, v);
		}

		return QuadNames;
	}

	static uint8 GetIDByPos(const PointType& Center, const PointType& Pos)
	{
		uint8 BitIdx = 0;
		auto Delta = Pos - Center;
		const Real* RESTRICT d = reinterpret_cast<const Real*>(&Delta);

		for (int32 i = VSpace::GetInt32 - 1; i >= 0; --i)
		{
			BitIdx = BitIdx << 1;
			if (Delta[i] < 0.0)
			{
				BitIdx |= 1;
			}
		}
		return static_cast<uint8>(1U) << BitIdx;
	}
};


/** Tree Base */
template<
	typename ElementType,			   //
	typename PointType,				   //
	typename ElementIndexType = int32, //
	typename IndexQtType = int32,	   //
	uint32 VectorSpace = 3U>		   //
class TTree_Base
{
private:
	using VSpace = FVectorSpace<VectorSpace>;

public:
	static constexpr uint32 InlineAllocatorSize =									 //
		(VSpace::Size == 2U)														 //
		? (std::is_same_v<IndexQtType, uint32> || std::is_same_v<IndexQtType, int32> //
			   ? 28U
			   : (std::is_same_v<IndexQtType, uint16> ? 36U : 0U))					 //
		: (std::is_same_v<IndexQtType, uint32> || std::is_same_v<IndexQtType, int32> //
			   ? 10U
			   : (std::is_same_v<IndexQtType, uint16> ? 24U : 0U));

	using TreeElementIdxType = ElementIndexType;
	using TreeNodeType = TTreeNode<TreeElementIdxType, IndexQtType, PointType, VSpace::Size, InlineAllocatorSize>;
	using BoxType = typename TreeNodeType::BoxType;
	using Real = typename PointType::FReal;

	static constexpr IndexQtType MaxIndexQt = TreeNodeType::MaxIndexQt;

	static constexpr int32 SubNodesNum = VSpace::SubTravelNum();

private:
	/** Tree Data */
	struct TreeIdxBox
	{
		TreeIdxBox(IndexQtType InId, const BoxType& InBox) : Box(InBox), TreeID(InId) {}
		TreeIdxBox(IndexQtType InId, BoxType&& InBox) : Box(MoveTemp(InBox)), TreeID(InId) {}
		BoxType Box;
		IndexQtType TreeID;
	};

	static constexpr bool bElementVector = std::is_same_v<ElementType, PointType>;
	using VectorOrBox = typename TChooseClass<bElementVector, PointType, BoxType>::Result;
	using TreeData = typename TChooseClass<bElementVector, IndexQtType, TreeIdxBox>::Result;


public:
	explicit TTree_Base(
		const Real InMinimumQuadSize = 100.f,
		const int32 InNodeCantSplit = SubNodesNum,
		const int32 TreePoolSize = 256,
		const int32 CompPoolSize = 256)
		: Root(MaxIndexQt)
		, MinimumQuadSize(InMinimumQuadSize)
		, SplitTolerance(InMinimumQuadSize * 1.5f)
		, NodeCantSplit(InNodeCantSplit)
	{
		Pool.Reserve(TreePoolSize);
		ElementPool.Reserve(CompPoolSize);
		Data.Reserve(CompPoolSize);
	}

protected:
	IndexQtType Root = MaxIndexQt;
	const Real MinimumQuadSize;
	const Real SplitTolerance;
	const int32 NodeCantSplit;

	TSparseArray<TreeNodeType> Pool;
	TSparseArray<TreeData> Data;
	TSparseArray<ElementType> ElementPool;

public:
	FORCEINLINE bool IsValidTreeIdx(IndexQtType TreeIdx) const { return TreeIdx != MaxIndexQt; }
	FORCEINLINE const BoxType& GetTreeBox(IndexQtType TreeIdx) const { return Pool[static_cast<int32>(TreeIdx)].GetTreeBox(); }
	FORCEINLINE BoxType& GetTreeBox(IndexQtType TreeIdx) { return Pool[static_cast<int32>(TreeIdx)].GetTreeBox(); }
	FORCEINLINE int32 NumTreeNodes() { return Pool.Num(); }

	FORCEINLINE IndexQtType GetRoot() const { return Root; };
	FORCEINLINE bool IsValidRoot() const { return IsValidTreeIdx(GetRoot()); }
	FORCEINLINE const BoxType& GetRootBox() const { return GetTreeBox(GetRoot()); }
	FORCEINLINE BoxType GetRootBox() { return GetTreeBox(GetRoot()); }

	FORCEINLINE TSparseArray<ElementType>& GetElementPool() { return ElementPool; }
	FORCEINLINE const TSparseArray<ElementType>& GetElementPool() const { return ElementPool; }
	FORCEINLINE int32 NumElements() { return ElementPool.Num(); }

	FORCEINLINE const ElementType& GetElement(const TreeElementIdxType ObjID) const { return GetElementPool()[static_cast<int32>(ObjID)]; }
	FORCEINLINE ElementType& GetElement(const TreeElementIdxType ObjID) { return GetElementPool()[static_cast<int32>(ObjID)]; }

	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<std::is_same_v<T, PointType>, const PointType&> GetElementBox(const TreeElementIdxType ObjID) const
	{
		return GetElement(ObjID);
	}
	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<std::is_same_v<T, PointType>, PointType&> GetElementBox(const TreeElementIdxType ObjID)
	{
		return GetElement(ObjID);
	}

	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<std::is_same_v<T, PointType>, IndexQtType> GetElementTreeID(const TreeElementIdxType ObjID) const
	{
		return Data[static_cast<int32>(ObjID)];
	}
	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<std::is_same_v<T, PointType>, IndexQtType&> GetElementTreeID(const TreeElementIdxType ObjID)
	{
		return Data[static_cast<int32>(ObjID)];
	}


	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<!std::is_same_v<T, PointType>, const BoxType&> GetElementBox(const TreeElementIdxType ObjID) const
	{
		return Data[static_cast<int32>(ObjID)].Box;
	}
	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<!std::is_same_v<T, PointType>, BoxType&> GetElementBox(const TreeElementIdxType ObjID)
	{
		return Data[static_cast<int32>(ObjID)].Box;
	}

	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<!std::is_same_v<T, PointType>, IndexQtType> GetElementTreeID(const TreeElementIdxType ObjID) const
	{
		return Data[static_cast<int32>(ObjID)].TreeID;
	}
	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<!std::is_same_v<T, PointType>, IndexQtType&> GetElementTreeID(const TreeElementIdxType ObjID)
	{
		return Data[static_cast<int32>(ObjID)].TreeID;
	}
	// end element

private:
	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<std::is_same_v<T, PointType>, void> InsertNewData(const TreeElementIdxType ObjID, const PointType&)
	{
		Data.Insert(static_cast<int32>(ObjID), MaxIndexQt);
	}
	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<!std::is_same_v<T, PointType>, void> InsertNewData(const TreeElementIdxType ObjID, const BoxType& InBox)
	{
		Data.Insert(static_cast<int32>(ObjID), TreeData(MaxIndexQt, InBox));
	}

	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<std::is_same_v<T, PointType>, TreeElementIdxType> Insert(const PointType& Element)
	{
		const int32 Idx = this->Insert(Element, Element);
		return static_cast<TreeElementIdxType>(Idx);
	}

public:
#if WITH_EDITOR
	bool CheckNum(const IndexQtType Self_ID) const
	{
		const int32 RecNum = NumElements_Recursive(Self_ID);
		return RecNum == Pool[Self_ID].Num() && RecNum == ElementPool.Num() && RecNum == Data.Num();
	}
#endif

	/*
	bool CheckChilds(const IndexQtType Self_ID)
	{
		const TreeNodeType& TreeRef = Pool[Self_ID];

		const bool bCh1 =						 //
			TreeRef.SubNodes[0] != MaxIndexQt && //
			TreeRef.SubNodes[1] != MaxIndexQt && //
			TreeRef.SubNodes[2] != MaxIndexQt && //
			TreeRef.SubNodes[3] != MaxIndexQt;

		const bool bCh2 =						 //
			TreeRef.SubNodes[0] == MaxIndexQt && //
			TreeRef.SubNodes[1] == MaxIndexQt && //
			TreeRef.SubNodes[2] == MaxIndexQt && //
			TreeRef.SubNodes[3] == MaxIndexQt;

		return bCh1 || bCh2;
	}
	*/

	TreeElementIdxType Insert(const ElementType& Element, VectorOrBox InBox)
	{
		const int32 ResIdx = ElementPool.Add(Element);
		const TreeElementIdxType ObjID = ResIdx;
		InsertNewData(ObjID, InBox);

		checkSlow(GetElement(ObjID) == Element);

		if (!IsValidRoot())
		{
			Root = Pool.Add(TreeNodeType(MaxIndexQt, BoxType(PointType(InBox), MinimumQuadSize)));
			Pool[Root].Self_ID = Root;
		}

		bool bNewRoot = false;
		if (!GetRootBox().IsInside(InBox))
		{
			Root = ExtendToParent(Root, InBox);
			bNewRoot = true;
		}

		checkSlow(GetRootBox().IsInside(InBox));

		const IndexQtType NewOtID = Insert_Internal(Root, ObjID, InBox);
		IndexQtType& QtID_Ref = GetElementTreeID(ObjID);
		QtID_Ref = NewOtID; //update current

		if (bNewRoot)
		{
			CollapseQt(true);
		}

#if WITH_EDITOR
		checkSlow(CheckNum(Root));
#endif

		return ObjID;
	}
	TreeElementIdxType Insert(ElementType&& Element, VectorOrBox InBox)
	{
		const int32 ResIdx = ElementPool.Add(MoveTemp(Element));
		const TreeElementIdxType ObjID = ResIdx;
		InsertNewData(ObjID, InBox);

		if (!IsValidRoot())
		{
			Root = Pool.Add(TreeNodeType(MaxIndexQt, BoxType(PointType(InBox), MinimumQuadSize)));
			Pool[Root].Self_ID = Root;
		}

		bool bNewRoot = false;
		if (!GetRootBox().IsInside(InBox))
		{
			Root = ExtendToParent(Root, InBox);
			bNewRoot = true;
		}

		checkSlow(GetRootBox().IsInside(InBox));

		const IndexQtType NewOtID = Insert_Internal(Root, ObjID, InBox);
		IndexQtType& QtID_Ref = GetElementTreeID(ObjID);
		QtID_Ref = NewOtID; //update current

		if (bNewRoot)
		{
			CollapseQt(true);
		}

#if WITH_EDITOR
		checkSlow(CheckNum(Root));
#endif

		return ObjID;
	}


	void Update(const TreeElementIdxType ObjID, const VectorOrBox New)
	{
		check(ObjID != TNumericLimits<TreeElementIdxType>::Max());
		check(IsValidRoot());
		const auto& Old = GetElementBox(ObjID);

		checkSlow(GetElementBox(GetElementTreeID(ObjID)).IsInside(GetElement(ObjID)));

		if (!(Old == New))
		{
			const IndexQtType QtID = GetElementTreeID(ObjID);
#if WITH_EDITOR
			check(QtID != MaxIndexQt);
#endif
			check(Pool[QtID].Num());

			const auto& TreeCell = Pool[QtID];
			if (!TreeCell.IsInside(New) || (!TreeCell.IsLeaf() && TreeCell.GetByQuadName(TreeCell.GetQuad(New)) != MaxIndexQt))
			{
				check(GetRootBox().IsInside(Old));

				if (!Pool[Root].IsInside(New))
				{
					Root = ExtendToParent(Root, New);
					checkSlow(IsValidRoot());
					checkSlow(GetRootBox().IsInside(New));
					checkSlow(GetElementBox(QtID).IsInside(GetElement(ObjID)));
				}

				const IndexQtType NewQtID = UpdateFromDown_Internal(QtID, ObjID, New, Old);
				IndexQtType& QtID_Ref = GetElementTreeID(ObjID);
				QtID_Ref = NewQtID; //update current
			}
			GetElementBox(ObjID) = New;

#if WITH_EDITOR
			const IndexQtType CheckQtID = GetElementTreeID(ObjID);
			const auto& Check = GetElementBox(ObjID);
			check(CheckQtID != MaxIndexQt);
			check(Check == New);
			check(GetTreeBox(CheckQtID).IsInside(Check));
			check(CheckNum(Root));
#endif
		}
	}

	void Remove(const TreeElementIdxType ObjID)
	{
		check(IsValidRoot());
		check(ObjID != TNumericLimits<TreeElementIdxType>::Max());

#if WITH_EDITOR
		check(GetRootBox().IsInside(GetElementBox(ObjID)));
#endif

		const IndexQtType QtID = GetElementTreeID(ObjID);
		Remove_Internal(QtID, ObjID);

		Data.RemoveAt(static_cast<int32>(ObjID));
		ElementPool.RemoveAt(static_cast<int32>(ObjID));

#if WITH_EDITOR
		checkSlow(CheckNum(Root));
#endif
	}

	void Clear()
	{
		ElementPool.Empty();
		Data.Empty();
		Pool.Empty();
	}

	void CollapseQt(const bool bWithSubTree = true)
	{
		if (Root != MaxIndexQt)
		{
			const IndexQtType NewRootQuadTree = CheckCollapseParentDown_Recursive(Root);
			if (NewRootQuadTree != Root)
			{
				DetachFromParent(NewRootQuadTree);
				Empty(Root);
				Pool.RemoveAt(Root);
				Root = NewRootQuadTree;
			}

			if (bWithSubTree)
			{
				CollapseSubTrees_Recursive(Root);
			}

#if WITH_EDITOR
			checkSlow(CheckNum(Root));
#endif
		}
	}

	int32 NumElements_Recursive(const IndexQtType Self_ID) const
	{
		int32 Out = 0;
		NumElements_Recursive(Self_ID, Out);
		return Out;
	}

	IndexQtType GetMaxIntersect(const BoxType Box) const
	{
		if (IsValidRoot() && GetRootBox().IsIntersect(Box))
		{
			const IndexQtType MaxIntersect = GetMaxIntersectTree_Internal(Root, Box);
			if (MaxIntersect != TNumericLimits<IndexQtType>::Max())
			{
				return MaxIntersect;
			}
		}
		return TNumericLimits<IndexQtType>::Max();
	}

	TreeElementIdxType FindNearest(const TreeElementIdxType ObjID) const
	{
		check(IsValidRoot());
		check(ObjID != TNumericLimits<TreeElementIdxType>::Max());

		IndexQtType TreeIdx = GetElementTreeID(ObjID);
		while (Pool[TreeIdx].Num() - 1 <= 0) //self exclude
		{
			TreeIdx = Pool[TreeIdx].GetParent();
		}

		const auto& V = GetElement(ObjID);
		Real MinVal = MAX_flt;
		TreeElementIdxType MinIdx = TNumericLimits<TreeElementIdxType>::Max();

		auto FindNearestLambda = [&](const TreeElementIdxType Idx)
		{
			const auto& V2 = GetElement(Idx);
			const Real TestVal = (V - V2).SizeSquared();
			if (TestVal < MinVal)
			{
				MinVal = TestVal;
				MinIdx = Idx;
			}
		};

		const BoxType Box = BoxType(Pool[TreeIdx].GetTreeBox());
		CallChildLambdaIdx_Recursive(TreeIdx, FindNearestLambda, Box);
		CallParentNodesLambdaId(TreeIdx, FindNearestLambda, Box);
		return MinIdx;
	}

	template<typename Predicate>
	void GetElementsIDs(const BoxType& Box, Predicate FilterPredicate, TArray<TreeElementIdxType>& Out) const
	{
		if (IsValidRoot() && GetRootBox().IsIntersect(Box))
		{
			const IndexQtType MaxIntersect = GetMaxIntersectTree_Internal(GetRoot(), Box);
			if (MaxIntersect == MaxIndexQt)
			{
				return;
			}
			Out.Reserve(Pool[MaxIntersect].Num());
			GetElemID_Recursive(MaxIntersect, Box, MoveTempIfPossible(FilterPredicate), Out);
			Out.Shrink();
		}
	}
	template<typename Predicate>
	void GetElementsIDs(const BoxType& Box, Predicate FilterPredicate, TSet<TreeElementIdxType>& Out) const
	{
		if (IsValidRoot() && GetRootBox().IsIntersect(Box))
		{
			const IndexQtType MaxIntersect = GetMaxIntersectTree_Internal(GetRoot(), Box);
			if (MaxIntersect == MaxIndexQt)
			{
				return;
			}
			Out.Reserve(Pool[MaxIntersect].Num());
			GetElemID_Recursive(MaxIntersect, Box, MoveTempIfPossible(FilterPredicate), Out);
			Out.Shrink();
		}
	}

	template<typename ElemLambda = TFunctionRef<void(const PointType&)>>
	void CallLambdaElement(const BoxType& Box, ElemLambda CallLambda) const
	{
		if (IsValidRoot() && GetRootBox().IsIntersect(Box))
		{
			const IndexQtType MaxIntersect = GetMaxIntersectTree_Internal(GetRoot(), Box);
			if (MaxIntersect == MaxIndexQt)
			{
				return;
			}
			CallLambdaElement_Recursive(MaxIntersect, MoveTemp(CallLambda), Box);
		}
	}

	template<typename ElemLambda = TFunctionRef<void(PointType&)>>
	void CallLambdaElement(const BoxType& Box, ElemLambda CallLambda)
	{
		if (IsValidRoot() && GetRootBox().IsIntersect(Box))
		{
			const IndexQtType MaxIntersect = GetMaxIntersectTree_Internal(GetRoot(), Box);
			if (MaxIntersect == MaxIndexQt)
			{
				return;
			}
			CallLambdaElement_Recursive(MaxIntersect, MoveTemp(CallLambda), Box);
		}
	}


	template<typename Predicate>
	void GetElements(const BoxType& Box, Predicate FilterPredicate, TArray<ElementType>& Out) const
	{
		if (IsValidRoot() && GetRootBox().IsIntersect(Box))
		{
			const IndexQtType MaxIntersect = GetMaxIntersectTree_Internal(GetRoot(), Box);
			if (MaxIntersect == MaxIndexQt)
			{
				return;
			}
			Out.Reserve(Pool[MaxIntersect].Num());
			GetElements_Recursive(MaxIntersect, Box, MoveTemp(FilterPredicate), Out);
			Out.Shrink();
		}
	}

	template<typename T, typename Predicate, typename TConvLambda>
	void GetElements(const BoxType& Box, Predicate FilterPredicate, TConvLambda ConvLambda, TArray<T>& Out)
	{
		if (IsValidRoot() && GetRootBox().IsIntersect(Box))
		{
			const IndexQtType MaxIntersect = GetMaxIntersectTree_Internal(GetRoot(), Box);
			if (MaxIntersect == MaxIndexQt)
			{
				return;
			}
			Out.Reserve(Pool[MaxIntersect].Num());
			GetElements_Recursive<T, Predicate, TConvLambda>(MaxIntersect, Box, FilterPredicate, ConvLambda, Out);
			Out.Shrink();
		}
	}

	// clang-format off
	
	template<uint32 VSpace = VectorSpace>
	std::enable_if_t<VSpace == 2, void> DrawTree(
		const UWorld* World,
		const float LifeTime,
		const FColor Color_1, const float Thickness_1, const uint8 DepthPriority_1,
		const FColor Color_2, const float Thickness_2, const uint8 DepthPriority_2,
		const FColor Color_3, const float Thickness_3, const uint8 DepthPriority_3,
		const float DrawHeight = 0.f) const
	{
#if WITH_EDITORONLY_DATA && ENABLE_DRAW_DEBUG
		if (Root != MaxIndexQt)
		{
			DrawTree_Recursive<VSpace>(
				Root, World, DrawHeight, LifeTime, 
				Color_1, Thickness_1, DepthPriority_1, 
				Color_2, Thickness_2, DepthPriority_2,
				Color_3, Thickness_3, DepthPriority_3);
		}
#endif
	}

	template<uint32 VSpace = VectorSpace>
	std::enable_if_t<VSpace == 3, void> DrawTree(
		const UWorld* World,
		const float LifeTime,
		const FColor Color_1, const float Thickness_1, const uint8 DepthPriority_1,
		const FColor Color_2, const float Thickness_2, const uint8 DepthPriority_2,
		const FColor Color_3, const float Thickness_3, const uint8 DepthPriority_3) const
	{
#if WITH_EDITORONLY_DATA && ENABLE_DRAW_DEBUG
		if (Root != MaxIndexQt)
		{
			DrawTree_Recursive<VSpace>(
				Root, World, LifeTime, 
				Color_1, Thickness_1, DepthPriority_1, 
				Color_2, Thickness_2, DepthPriority_2, 
				Color_3, Thickness_3, DepthPriority_3);
		}
#endif
	}

	// clang-format on

private:
	bool IsCanSplitTree(const TreeNodeType& SelfNode) const
	{
		return SelfNode.IsLeaf() && SelfNode.Num() >= NodeCantSplit && (SelfNode.GetTreeBox().GetSize()[0] - SplitTolerance) > 0.f;
	}

	bool IsCanCollapse(const TreeNodeType& SelfNode) const { return (SelfNode.Parent != MaxIndexQt) && Pool[SelfNode.Parent].Num() <= NodeCantSplit; }


	IndexQtType Insert_Internal(IndexQtType Self_ID, TreeElementIdxType ObjID, const VectorOrBox& InBox)
	{
		while (true)
		{
			TreeNodeType& SelfNode = Pool[Self_ID];
			if (!SelfNode.IsLeaf())
			{
				const auto Q = bElementVector ? SelfNode.GetQuad(InBox) : SelfNode.GetQuads(InBox);
				const IndexQtType TreeId = SelfNode.GetByQuadName(Q);
				check(!bElementVector || (bElementVector && TreeId != MaxIndexQt)) if (bElementVector || TreeId != MaxIndexQt)
				{
					SelfNode.ContainsCount++;
					Self_ID = TreeId; // next loop
					continue;
				}
			}
			else if (IsCanSplitTree(SelfNode))
			{
				Split(Self_ID);
				continue; // next loop
			}

			SelfNode.Nodes.Add(MoveTemp(ObjID));
			SelfNode.ContainsCount++;
			//#if WITH_EDITOR
			//		checkSlow(CheckNum(Self_ID));
			//#endif
			break;
		}
		return Self_ID;
	}

	bool Remove_Internal(IndexQtType Self_ID, const TreeElementIdxType ObjID)
	{
#if WITH_EDITOR
		const bool bRemoved = RemoveNodeForElement(Self_ID, ObjID);
		check(bRemoved);
#else
		RemoveNodeForElement(Self_ID, ObjID);
#endif

		while (true)
		{
			TreeNodeType& SelfNode = Pool[Self_ID];
			SelfNode.ContainsCount--;

			if (SelfNode.Num() == 0)
			{
				Empty(Self_ID);
			}
			else if (SelfNode.Num() <= NodeCantSplit && !SelfNode.IsLeaf())
			{
				CollectChildTreesToSelf(Self_ID);
			}

			if (SelfNode.Parent != MaxIndexQt)
			{
				Self_ID = SelfNode.Parent;
				continue;
			}
			break;
		}
		return true;
	}


	IndexQtType GetMaxIntersectTree_Internal(IndexQtType Self_ID, BoxType Box) const
	{
		//checkNoRecursion();

		Real* const RESTRICT MiB = reinterpret_cast<Real* const>(&Box.min);
		Real* const RESTRICT MaB = reinterpret_cast<Real* const>(&Box.max);

		while (Pool[Self_ID].Num() > 0)
		{
			const TreeNodeType& SelfNode = Pool[Self_ID];
			const auto& SelfBox = SelfNode.GetTreeBox();

			const Real* RESTRICT Mi = reinterpret_cast<const Real*>(&SelfBox.min);
			const Real* RESTRICT Ma = reinterpret_cast<const Real*>(&SelfBox.max);

			for (int32 i = 0; i < VSpace::GetInt32; i++)
			{
				MiB[i] = FMath::Max(Mi[i], MiB[i]);
				MaB[i] = FMath::Min(Ma[i], MaB[i]);
			}

			if (SelfBox.IsIntersect(Box))
			{
				for (const auto& ElemIdx : SelfNode.Nodes)
				{
					if (Box.IsInside(GetElementBox(ElemIdx)))
					{
						return Self_ID;
					}
				}

				if (!SelfNode.IsLeaf() && SelfNode.GetTreeBox().IsInside(Box))
				{
					const IndexQtType TreeId = SelfNode.GetByQuadName(SelfNode.GetQuads(Box));
					if (TreeId != MaxIndexQt)
					{
						Self_ID = TreeId;
						continue;
					}
				}
				return Self_ID;
			}
			break;
		}
		return MaxIndexQt;
	}

	IndexQtType UpdateFromDown_Internal(IndexQtType Self_ID, const TreeElementIdxType ObjID, const VectorOrBox& New, const VectorOrBox Old)
	{
#if WITH_EDITOR
		checkNoRecursion();
#endif

		TreeNodeType& SelfNode = Pool[Self_ID];

		checkSlow(SelfNode.IsInside(Old));
		if (SelfNode.IsInside(New))
		{
			/*if (IsCanCollapse())
			{
				//checkNoEntry();
				//return Pool[Parent).CollectChildTreesToSelf();
			}
			else */

			if ((!SelfNode.IsLeaf() || IsCanSplitTree(SelfNode)) && SelfNode.GetByQuadName(SelfNode.GetQuad(New)) != MaxIndexQt)
			{
				GetElementBox(ObjID) = New;
				Split(Self_ID);
				
				return GetElementTreeID(ObjID);
			}
			return Self_ID;
		}
#if WITH_EDITOR
		const bool bRemoved = RemoveNodeForElement(Self_ID, ObjID);
		check(bRemoved);
#else
		RemoveNodeForElement(Self_ID, ObjID);
#endif

		while (Self_ID != MaxIndexQt)
		{
			TreeNodeType& LoopRef = Pool[Self_ID];
			LoopRef.ContainsCount--;

			if (LoopRef.IsInside(New))
			{
				return Insert_Internal(Self_ID, ObjID, New);
			}

			checkSlow(LoopRef.Parent != MaxIndexQt);

			if (LoopRef.Num() == 0)
			{
				Empty(Self_ID);
			}
			else if (LoopRef.Num() <= NodeCantSplit && !LoopRef.IsLeaf())
			{
				CollectChildTreesToSelf(Self_ID);
			}
			Self_ID = LoopRef.Parent;
		}
		checkNoEntry();
		UE_ASSUME(0);
		return MaxIndexQt;
	}


	void Split(const IndexQtType Self_ID)
	{

		if (Pool[Self_ID].IsLeaf())
		{
			if (Pool.GetMaxIndex() < Pool.Num() + 8)
			{
				Pool.Reserve(Pool.Num() + 64);
			}

			CreateChildLeaves(Pool[Self_ID]);
		}

		TreeNodeType& TreeRef = Pool[Self_ID];
		check(!TreeRef.IsLeaf());
		for (int32 i = TreeRef.Nodes.Num() - 1; i > INDEX_NONE; i--) // node to leaves
		{
			const auto ObjID = TreeRef.Nodes[i];
			const auto& Loc = GetElementBox(ObjID);
			const uint8 Q = bElementVector ? TreeRef.GetQuad(Loc) : TreeRef.GetQuads(Loc);
			const IndexQtType TreeInsertId = TreeRef.GetByQuadName(Q);

			check(!bElementVector || (bElementVector && TreeInsertId != MaxIndexQt));

			if (bElementVector || TreeInsertId != MaxIndexQt)
			{
				const IndexQtType TreeID = Insert_Internal(TreeInsertId, ObjID, Loc);
				if (TreeID != Self_ID)
				{
					IndexQtType& QtID_Ref = GetElementTreeID(ObjID);
					QtID_Ref = TreeID;
					TreeRef.Nodes.RemoveAtSwap(i, 1, false);
				}
			}
		}
	}

	IndexQtType ExtendToParent(IndexQtType Self_ID, const VectorOrBox& InBox)
	{
		while (!Pool[Self_ID].IsInside(InBox))
		{
			if (Pool.GetMaxIndex() < (Pool.Num() + SubNodesNum))
			{
				Pool.Reserve(Pool.Num() + 8 * SubNodesNum);
			}
			TreeNodeType& SelfNode = Pool[Self_ID];

			const auto& Box = SelfNode.GetTreeBox();
			const auto QuadTreeBoxCenter = Box.GetCenter();
			const auto InBoxCenter = PointType(InBox);

			PointType BoxCenter;
			{
				Real* const RESTRICT Bc = reinterpret_cast<Real*>(&BoxCenter);

				const Real* RESTRICT QtBc = reinterpret_cast<const Real*>(&QuadTreeBoxCenter);
				const Real* RESTRICT InBc = reinterpret_cast<const Real*>(&InBoxCenter);

				for (int32 i = 0; i < VSpace::GetInt32; i++)
				{
					Bc[i] = (InBc[i] <= QtBc[i]) ? Box.Min()[i] : Box.Max()[i];
				}
			}

			const PointType Size = Box.GetSize();
			SelfNode.Parent = Pool.Add(TreeNodeType(MaxIndexQt, BoxType(BoxCenter - Size, BoxCenter + Size)));
			TreeNodeType& ParentRef = Pool[SelfNode.Parent];

			ParentRef.GetByQuadNameRef(SelfNode.GetIDByPos(BoxCenter, QuadTreeBoxCenter)) = SelfNode.Self_ID;

			ParentRef.Self_ID = SelfNode.Parent;
			ParentRef.ContainsCount = SelfNode.Num();
			CreateChildLeaves(ParentRef);
			Self_ID = SelfNode.Parent;
		}
		return Self_ID;
	}


	void CreateChildLeaves(TreeNodeType& SelfNode)
	{
		const auto& Box = SelfNode.GetTreeBox();
		const auto Center = Box.GetCenter();
		const auto HalfExtent = Box.GetExtent() * 0.5f;

		for (int32 i = 0; i < SubNodesNum; i++)
		{
			IndexQtType& Param = SelfNode.SubNodes[i];
			if (Param == MaxIndexQt)
			{
				PointType HelP = HalfExtent;
				Real* const RESTRICT h = reinterpret_cast<Real*>(&HelP);
				for (int32 j = 0; j < VSpace::GetInt32; j++)
				{
					if (1 & (i >> j))
					{
						h[j] = -h[j];
					}
				}

				Param = Pool.Add(TreeNodeType(SelfNode.Self_ID, BoxType::BuildAABB(Center + HelP, HalfExtent)));
				Pool[Param].Self_ID = Param;
			}
		}
	}

	void Empty(const IndexQtType Self_ID)
	{
		TreeNodeType& SelfNode = Pool[Self_ID];
		SelfNode.ContainsCount = 0;
		SelfNode.Nodes.Empty();
		EmptyLeaves_Recursive(Self_ID);
	}


	void NumElements_Recursive(const IndexQtType Self_ID, int32& Out) const
	{
		const auto& SelfNode = Pool[Self_ID];
		Out += SelfNode.Nodes.Num();
		if (!SelfNode.IsLeaf())
		{
			for (auto It : SelfNode.SubNodes)
			{
				NumElements_Recursive(It, Out);
			}
		}
	}

	bool RemoveNodeForElement(const IndexQtType Self_ID, const TreeElementIdxType& ObjID)
	{
		auto& SelfNode = Pool[Self_ID];
		if (SelfNode.Num())
		{
			const int32 ElementIdx = SelfNode.Nodes.IndexOfByKey(ObjID);
			if (ElementIdx != INDEX_NONE)
			{
				const bool bShrink = SelfNode.Nodes.Num() > InlineAllocatorSize;
				SelfNode.Nodes.RemoveAtSwap(ElementIdx, 1, bShrink);
				return true;
			}
		}
		return false;
	}

	void DetachFromParent(const IndexQtType Self_ID)
	{
		TreeNodeType& SelfNode = Pool[Self_ID];

		checkSlow(SelfNode.Parent != MaxIndexQt);

		TreeNodeType& ParentTreeRef = Pool[SelfNode.Parent];
		for (auto& It : ParentTreeRef.SubNodes)
		{
			if (It == Self_ID)
			{
				It = MaxIndexQt;
				break;
			}
		}
		SelfNode.Parent = MaxIndexQt;
	}

	bool CollectChildTreesToSelf(const IndexQtType Self_ID)
	{
		auto& SelfNode = Pool[Self_ID];
		if (SelfNode.Num() <= NodeCantSplit)
		{
			if (SelfNode.Num() && !SelfNode.IsLeaf())
			{
#if WITH_EDITOR
				checkSlow(CheckNum(Self_ID));
#endif
				SelfNode.Nodes.Reserve(SelfNode.Num());

				GetChildElements_Recursive(Self_ID, SelfNode.Nodes, false);
				for (const auto& ObjID : SelfNode.Nodes)
				{
					IndexQtType& QtID_Ref = GetElementTreeID(ObjID);
					QtID_Ref = Self_ID;
				}
				checkSlow(SelfNode.Nodes.Num() == SelfNode.Num());
			}
			EmptyLeaves_Recursive(Self_ID);

#if WITH_EDITOR
			checkSlow(CheckNum(Self_ID));
#endif

			return true;
		}
		return false;
	}


	void EmptyLeaves_Recursive(const IndexQtType Self_ID, const bool bExcludeSelf = true)
	{
		TreeNodeType& SelfNode = Pool[Self_ID];
		if (!SelfNode.IsLeaf())
		{
			for (int32 i = 0; i < SubNodesNum; i++)
			{
				const IndexQtType LeafId = SelfNode.SubNodes[i];
				if (LeafId != MaxIndexQt)
				{
					Pool[LeafId].ContainsCount = 0;
					Pool[LeafId].Nodes.Empty();
					EmptyLeaves_Recursive(LeafId, false);

					Pool.RemoveAt(LeafId);
					SelfNode.SubNodes[i] = MaxIndexQt;
				}
			}
		}
		else if (!bExcludeSelf)
		{
			SelfNode.ContainsCount = 0;
			SelfNode.Nodes.Empty();
		}
	}

	void CollapseSubTrees_Recursive(IndexQtType Self_ID)
	{
		const auto& SelfNode = Pool[Self_ID];
		if (!SelfNode.IsLeaf())
		{
			if (SelfNode.Num() <= NodeCantSplit)
			{
				while (Pool[Self_ID].Parent != MaxIndexQt && Pool[Pool[Self_ID].Parent].Num() <= NodeCantSplit)
				{
					Self_ID = Pool[Self_ID].Parent;
				}
				CollectChildTreesToSelf(Self_ID);
			}
			else
			{
				for (auto It : SelfNode.SubNodes)
				{
					CollapseSubTrees_Recursive(It);
				}
			}
		}
	}


	void GetChildElements_Recursive(const IndexQtType Self_ID, TArray<TreeElementIdxType>& Out, const bool bIncludeSelf) const
	{
		const auto& SelfNode = Pool[Self_ID];
		if (bIncludeSelf)
		{
			Out.Append(SelfNode.Nodes);
		}

		if (!SelfNode.IsLeaf())
		{
			for (auto It : SelfNode.SubNodes)
			{
				GetChildElements_Recursive(It, Out, true);
			}
		}
	}

	void GetChildElements_Recursive(const IndexQtType Self_ID, TArray<TreeElementIdxType, TInlineAllocator<InlineAllocatorSize>>& Out, const bool bIncludeSelf)
	{
		auto& SelfNode = Pool[Self_ID];
		if (bIncludeSelf)
		{
			//Out.Append(MoveTemp(SelfNode.Nodes));

			const int32 SourceCount = SelfNode.Nodes.Num();
#if WITH_EDITOR
			check(Out.Max() >= Out.Num() + SourceCount);
#endif

			if (static_cast<bool>(SourceCount))
			{
				const int32 OutCount = Out.Num();
				FMemory::Memmove(Out.GetData() + OutCount, SelfNode.Nodes.GetData(), sizeof(TreeElementIdxType) * SourceCount);
				SelfNode.Nodes.SetNumUnsafeInternal(0);
				Out.SetNumUnsafeInternal(OutCount + SourceCount);
			}
		}

		if (!SelfNode.IsLeaf())
		{
			for (auto It : SelfNode.SubNodes)
			{
				GetChildElements_Recursive(It, Out, true);
			}
		}
	}


	IndexQtType CheckCollapseParentDown_Recursive(const IndexQtType Self_ID) const
	{
		const auto& SelfNode = Pool[Self_ID];
		if (SelfNode.Nodes.Num() == 0 && !SelfNode.IsLeaf())
		{
			const int32 SelfNum = SelfNode.Num();
			IndexQtType CollapseTree = MaxIndexQt;

			for (auto It : SelfNode.SubNodes)
			{
				if (Pool[It].Num() == SelfNum)
				{
					CollapseTree = It;
					break;
				}
			}

			if (CollapseTree != MaxIndexQt)
			{
				return CheckCollapseParentDown_Recursive(CollapseTree);
			}
		}
		return Self_ID;
	}

	IndexQtType CollectChildTreesToSelf_Recursive(const IndexQtType Self_ID)
	{
		while (Pool[Self_ID].Parent != MaxIndexQt && Pool[Pool[Self_ID].Parent].Num() <= NodeCantSplit)
		{
			Self_ID = Pool[Self_ID].Parent;
		}
		CollectChildTreesToSelf(Self_ID);
		return Self_ID;
	}

	//auto Lambda = [](TreeElementIdxType& Obj) {  idx do some; }
	template<typename CallLambdaType = TFunctionRef<void(TreeElementIdxType)>>
	void CallChildLambdaIdx_Recursive(const IndexQtType Self_ID, CallLambdaType CallLambda, const BoxType& Box) const
	{
		const TreeNodeType& SelfNode = Pool[Self_ID];
		if (SelfNode.Num() && SelfNode.GetTreeBox().IsIntersect(Box))
		{
			IF_CONSTEXPR(bElementVector)
			{
				if (!SelfNode.IsLeaf())
				{
					for (auto It : SelfNode.SubNodes)
					{
						CallChildLambdaIdx_Recursive(It, CallLambda, Box);
					}
				}
				else
				{
					for (const auto& ElemIdx : SelfNode.Nodes)
					{
						if (Box.IsInside(GetElementBox(ElemIdx)))
						{
							Invoke(CallLambda, ElemIdx);
						}
					}
				}
			}
			else
			{
				for (const auto& ElemIdx : SelfNode.Nodes)
				{
					if (Box.IsIntersect(GetElementBox(ElemIdx)))
					{
						Invoke(CallLambda, ElemIdx);
					}
				}

				if (!SelfNode.IsLeaf())
				{
					for (auto It : SelfNode.SubNodes)
					{
						CallChildLambdaIdx_Recursive(It, CallLambda, Box);
					}
				}
			}
		}
	}

	template<typename CallLambdaType = TFunctionRef<void(TreeElementIdxType)>>
	void CallChildLambdaIdx_Recursive(const IndexQtType Self_ID, CallLambdaType CallLambda, const BoxType& Box)
	{
		const TreeNodeType& SelfNode = Pool[Self_ID];
		if (SelfNode.Num() && SelfNode.GetTreeBox().IsIntersect(Box))
		{
			IF_CONSTEXPR(bElementVector)
			{
				if (!SelfNode.IsLeaf())
				{
					for (auto It : SelfNode.SubNodes)
					{
						CallChildLambdaIdx_Recursive(It, CallLambda, Box);
					}
				}
				else
				{
					for (const auto& ElemIdx : SelfNode.Nodes)
					{
						if (Box.IsInside(GetElementBox(ElemIdx)))
						{
							Invoke(CallLambda, ElemIdx);
						}
					}
				}
			}
			else
			{
				for (const auto& ElemIdx : SelfNode.Nodes)
				{
					if (Box.IsIntersect(GetElementBox(ElemIdx)))
					{
						Invoke(CallLambda, ElemIdx);
					}
				}

				if (!SelfNode.IsLeaf())
				{
					for (auto It : SelfNode.SubNodes)
					{
						CallChildLambdaIdx_Recursive(It, CallLambda, Box);
					}
				}
			}
		}
	}

	template<typename CallLambdaType>
	void CallParentLambdaIdx(const IndexQtType Self_ID, CallLambdaType CallLambda, const BoxType& Box) const
	{
		while (Pool[Self_ID].GetTreeBox().IsIntersect(Box) && Pool[Self_ID].Parent != MaxIndexQt)
		{
			Self_ID = Pool[Self_ID].Parent;
			const auto& ParentRef = Pool[Self_ID];
			if (ParentRef.Nodes.Num())
			{
				for (const auto& SubNodes : ParentRef.Nodes)
				{
					Invoke(CallLambda, SubNodes);
				}
			}
		}
	}

	template<typename CallLambdaType>
	void CallParentLambdaIdx(const IndexQtType Self_ID, CallLambdaType CallLambda, const BoxType& Box)
	{
		while (Pool[Self_ID].GetTreeBox().IsIntersect(Box) && Pool[Self_ID].Parent != MaxIndexQt)
		{
			Self_ID = Pool[Self_ID].Parent;
			const auto& ParentRef = Pool[Self_ID];
			if (ParentRef.Nodes.Num())
			{
				for (const auto& SubNodes : ParentRef.Nodes)
				{
					Invoke(CallLambda, SubNodes);
				}
			}
		}
	}


	template<typename Predicate, typename IdxContainer /*= TArray<TreeElementIdxType>*/>
	void GetElemID_Recursive(const IndexQtType Self_ID, const BoxType& Box, Predicate FilterPredicate, IdxContainer& Out) const
	{
		CallChildLambdaIdx_Recursive(
			Self_ID,
			[&Out, FilterPredicate](TreeElementIdxType Idx)
			{
				if (Invoke(FilterPredicate, Idx))
				{
					Out.Add(Idx);
				}
			},
			Box);
	}
	//template<typename Predicate>
	//void GetElemID_Recursive(const IndexQtType Self_ID, const BoxType& Box, Predicate FilterPredicate, TSet<TreeElementIdxType>& Out) const
	//{
	//	CallChildLambdaIdx_Recursive(
	//		Self_ID,
	//		[&Out, FilterPredicate](TreeElementIdxType Idx)
	//		{
	//			if (Invoke(FilterPredicate, Idx))
	//			{
	//				Out.Add(Idx);
	//			}
	//		},
	//		Box);
	//}

	template<typename Predicate, typename ElementContainer /*= TArray<ElementType>*/>
	void GetElements_Recursive(const IndexQtType Self_ID, const BoxType& Box, Predicate FilterPredicate, ElementContainer& Out) const
	{
		CallChildLambdaIdx_Recursive(
			Self_ID,
			[&](const TreeElementIdxType Idx)
			{
				if (Invoke(FilterPredicate, Idx))
				{
					Out.Add(GetElement(Idx));
				}
			},
			Box);
	}

	template<typename T, typename Predicate, typename TConvLambda /*= FIdentityFunctor*/, typename ElementContainer /*= TArray<T>*/>
	void GetElements_Recursive(const IndexQtType Self_ID, const BoxType& Box, Predicate FilterPredicate, TConvLambda ConvLambda, ElementContainer& Out)
	{
		CallChildLambdaIdx_Recursive(
			Self_ID,
			[&](const TreeElementIdxType Idx)
			{
				if (Invoke(FilterPredicate, Idx))
				{
					Out.Add(Invoke(ConvLambda, Idx));
				}
			},
			Box);
	}


	template<typename CallLambdaType = TFunctionRef<void(const ElementType&)>>
	void CallLambdaElement_Recursive(const IndexQtType Self_ID, CallLambdaType CallLambda, const BoxType& Box) const
	{
		CallChildLambdaIdx_Recursive(
			Self_ID,
			[&](const TreeElementIdxType Idx) { Invoke(CallLambda, GetElement(Idx)); },
			Box);
	}

	template<typename CallLambdaType = TFunctionRef<void(ElementType&)>>
	void CallLambdaElement_Recursive(const IndexQtType Self_ID, CallLambdaType CallLambda, const BoxType& Box)
	{
		CallChildLambdaIdx_Recursive(
			Self_ID,
			[&](const TreeElementIdxType Idx) { Invoke(CallLambda, GetElement(Idx)); },
			Box);
	}


	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<std::is_same_v<T, PointType>, FVector> GetExtentFrom(const PointType& P) const
	{
		return FVector(0.0);
	}
	template<typename T = ElementType>
	FORCEINLINE std::enable_if_t<!std::is_same_v<T, PointType>, FVector> GetExtentFrom(const BoxType& P) const
	{
		return P.GetExtent();
	}

	// clang-format off
	
	template<uint32 Dim = VectorSpace>
	std::enable_if_t<Dim == 2, void> DrawTree_Recursive(
		const IndexQtType Self_ID,
		const UWorld* World,
		const float DrawHeight,
		const float LifeTime,
		const FColor Color_1, const float Thickness_1, const uint8 DepthPriority_1,
		const FColor Color_2, const float Thickness_2, const uint8 DepthPriority_2,
		const FColor Color_3, const float Thickness_3, const uint8 DepthPriority_3) const
	{
#if ENABLE_DRAW_DEBUG
		const TreeNodeType& TreeRef = Pool[Self_ID];
		if (TreeRef.Num())
		{
			const FVector SelfCen = FVector(TreeRef.GetTreeBox().GetCenter()[0], TreeRef.GetTreeBox().GetCenter()[1], DrawHeight);

			if (TreeRef.Parent == MaxIndexQt)
			{
				const auto& B = TreeRef.GetTreeBox();
				const FBox2D DrawBox = FBox2D(B.min, B.max);
				DrawBorderBox2D(World, DrawBox, DrawHeight, FColor::Red, DepthPriority_1, Thickness_1 * 2.f, LifeTime);
			}
			if (!TreeRef.IsLeaf())
			{
				const auto& Box = TreeRef.GetTreeBox();
				const FVector2D Cen = Box.GetCenter();

				DrawDebugLine(
					World, FVector(Cen.X, Box.max.Y, DrawHeight), FVector(Cen.X, Box.min.Y, DrawHeight), Color_1, false, LifeTime, DepthPriority_1, Thickness_1);
				DrawDebugLine(
					World, FVector(Box.max.X, Cen.Y, DrawHeight), FVector(Box.min.X, Cen.Y, DrawHeight), Color_1, false, LifeTime, DepthPriority_1, Thickness_1);
			}
			{
				const FString Str = FString::Printf(TEXT("Total: %d , Nodes : %d"), TreeRef.Num(), TreeRef.Nodes.Num());
				DrawDebugString(World, SelfCen, Str, nullptr, FColor::Red, LifeTime);
			}

			if (TreeRef.Nodes.Num())
			{
				for (const auto& SubNodes : TreeRef.Nodes)
				{
					FBox2D DrawBox;
					const auto& NodeBox2D = GetElementBox(SubNodes);
					if (bElementVector)
					{
						const auto& Ext = FVector2D::UnitVector * 10.f;
						DrawBox = FBox2D(PointType(NodeBox2D) - Ext, PointType(NodeBox2D) + Ext);
					}
					else
					{
						DrawBox = FBox2D(NodeBox2D.min, NodeBox2D.max);
					}
					DrawBorderBox2D(World, DrawBox, DrawHeight, Color_2, DepthPriority_2, Thickness_2, LifeTime);

					const FVector2D NodeCen2D = PointType(NodeBox2D);
					const FVector NodeCen = FVector(NodeCen2D.X, NodeCen2D.Y, DrawHeight);

					DrawDebugLine(World, SelfCen, NodeCen, Color_3, false, LifeTime, DepthPriority_3, Thickness_3);
				}
			}

			if (!TreeRef.IsLeaf())
			{
				for (auto It : TreeRef.SubNodes)
				{
					DrawTree_Recursive<Dim>(
						It, World, DrawHeight, LifeTime,
						Color_1, Thickness_1, DepthPriority_1,
						Color_2, Thickness_2, DepthPriority_2,
						Color_3, Thickness_3, DepthPriority_3);			
				}
			}
		}
#endif
	}

	template<uint32 Dim = VectorSpace>
	std::enable_if_t<Dim == 3, void> DrawTree_Recursive(
		const IndexQtType Self_ID,
		const UWorld* World,
		const float LifeTime,
		const FColor Color_1, const float Thickness_1, const uint8 DepthPriority_1,
		const FColor Color_2, const float Thickness_2, const uint8 DepthPriority_2,
		const FColor Color_3, const float Thickness_3, const uint8 DepthPriority_3) const
	{
#if ENABLE_DRAW_DEBUG
		const TreeNodeType& TreeRef = Pool[Self_ID];
		if (TreeRef.Num())
		{
			const FVector SelfCen = TreeRef.GetTreeBox().GetCenter();

			{
				const FString Str = FString::Printf(TEXT("Total: %d , Nodes : %d"), TreeRef.Num(), TreeRef.Nodes.Num());
				DrawDebugString(World, SelfCen, Str, nullptr, FColor::Red, LifeTime);
			}
			if (TreeRef.Parent != MaxIndexQt)
			{
				DrawDebugBox(World, SelfCen, TreeRef.GetTreeBox().GetExtent(), Color_1, false, LifeTime, DepthPriority_1, Thickness_1);

				const FVector ParentCen = Pool[TreeRef.Parent].GetTreeBox().GetCenter();
				const FVector V = FVector(SelfCen.X, SelfCen.Y, ParentCen.Z);

				DrawDebugDirectionalArrow(World, ParentCen, V, 5625.f, Color_2, false, LifeTime, DepthPriority_2, Thickness_2);
				DrawDebugDirectionalArrow(World, V, SelfCen, 5625.f, Color_2, false, LifeTime, DepthPriority_2, Thickness_2);
			}
			else
			{
				DrawDebugBox(World, SelfCen, TreeRef.GetTreeBox().GetExtent(), Color_1, false, LifeTime, DepthPriority_1, Thickness_1 * 2.f);
			}

			if (TreeRef.Nodes.Num())
			{
				constexpr Real MinSizeBoxToDraw = 5.f;
				for (const auto& SubIdx : TreeRef.Nodes)
				{
					const auto& Point = GetElementBox(SubIdx);
					const FVector Extent =
						bElementVector
							? FVector(MinSizeBoxToDraw)
							: GetExtentFrom(Point).SizeSquared() > MinSizeBoxToDraw
								? GetExtentFrom(Point)
								: FVector(MinSizeBoxToDraw);

					DrawDebugBox(World, Point, Extent, Color_3, false, LifeTime, DepthPriority_3, Thickness_3);

					DrawDebugDirectionalArrow(World, SelfCen, Point, 5625.f, Color_2, false, LifeTime, DepthPriority_2, Thickness_2);
				}
			}

			if (!TreeRef.IsLeaf())
			{
				for (auto It : TreeRef.SubNodes)
				{
					DrawTree_Recursive<Dim>(
						It, World, LifeTime,
						Color_1, Thickness_1, DepthPriority_1,
						Color_2, Thickness_2, DepthPriority_2,
						Color_3, Thickness_3, DepthPriority_3);
				}
			}
		}
#endif
	}

	
	static void DrawSplitBox2D(
		const class UWorld* InWorld,
		const FBox2D& Box,
		const float HeightZ,
		const FColor Color,
		const uint8 DepthPriority,
		const float Thickness,
		const float LifeTime)
	{
		const FVector2D Cen = Box.GetCenter();
		DrawDebugLine(InWorld, FVector(Cen.X, Box.Max.Y, HeightZ), FVector(Cen.X, Box.Min.Y, HeightZ), Color, false, LifeTime, DepthPriority, Thickness);
		DrawDebugLine(InWorld, FVector(Box.Max.X, Cen.Y, HeightZ), FVector(Box.Min.X, Cen.Y, HeightZ), Color, false, LifeTime, DepthPriority, Thickness);
	}

	static void DrawBorderBox2D(
		const class UWorld* InWorld,
		const FBox2D& Box,
		const float HeightZ,
		const FColor Color,
		const uint8 DepthPriority,
		const float Thickness,
		const float LifeTime)
	{
		DrawDebugLine(InWorld, FVector(Box.Max.X, Box.Max.Y, HeightZ), FVector(Box.Max.X, Box.Min.Y, HeightZ), Color, false, LifeTime, DepthPriority, Thickness);
		DrawDebugLine(InWorld, FVector(Box.Max.X, Box.Min.Y, HeightZ), FVector(Box.Min.X, Box.Min.Y, HeightZ), Color, false, LifeTime, DepthPriority, Thickness);
		DrawDebugLine(InWorld, FVector(Box.Min.X, Box.Min.Y, HeightZ), FVector(Box.Min.X, Box.Max.Y, HeightZ), Color, false, LifeTime, DepthPriority, Thickness);
		DrawDebugLine(InWorld, FVector(Box.Min.X, Box.Max.Y, HeightZ), FVector(Box.Max.X, Box.Max.Y, HeightZ), Color, false, LifeTime, DepthPriority, Thickness);
	}

	// clang-format on
};
