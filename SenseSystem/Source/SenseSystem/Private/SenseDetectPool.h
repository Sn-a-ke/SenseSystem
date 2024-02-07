//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/NumericLimits.h"
#include "HAL/Platform.h"
#include "Containers/Array.h"
#include "Containers/SparseArray.h"
#include <algorithm>

#include "SenseSystem.h"
#include "SensedStimulStruct.h"
using ElementIndexType = FSenseSystemModule::ElementIndexType;

class FSenseDetectPool
{
	TSparseArray<FSensedStimulus> ObjPool;
public:
	FSenseDetectPool() { ObjPool.Reserve(128); }
	~FSenseDetectPool() {}

	const TSparseArray<FSensedStimulus>& GetPool() const;
	TSparseArray<FSensedStimulus>& GetPool();

	TArray<ElementIndexType> Current;
	TArray<ElementIndexType> Lost;
	TArray<ElementIndexType> Forget;

	TArray<ElementIndexType> NewCurrent;
	TArray<ElementIndexType> LostCurrent;

	FTrackBestSenseScore Best_Sense;

	TArray<ElementIndexType> DetectNew;
	TArray<ElementIndexType> DetectCurrent;


public:
	void Add(const float CurrentTime, FSensedStimulus SS, const ElementIndexType ID)
	{
		if (SS.TmpHash != MAX_uint32)
		{
			if (ID == TNumericLimits<ElementIndexType>::Max())
			{
				SS.FirstSensedTime = CurrentTime;
				DetectNew.Add(AddToPool(MoveTemp(SS)));
			
			}
			else
			{
				check(ObjPool.IsValidIndex(ID));
				DetectCurrent.Add(AddToPool(ID, MoveTemp(SS)));
			}
		}
	}

	ElementIndexType ContainsInCurrentSense(const FSensedStimulus& InElem) const;
	ElementIndexType ContainsInLostSense(const FSensedStimulus& InElem) const;

	ElementIndexType ContainsIn(const FSensedStimulus& InElem, const TArray<ElementIndexType>& InArr) const;

	void NewSensedUpdate(EOnSenseEvent Ost, bool bOverrideSenseState, bool bNewSenseForcedByBestScore);
	void EmptyUpdate(EOnSenseEvent Ost, bool bOverrideSenseState = true);

	void NewSensed(EOnSenseEvent Ost, bool bNewSenseForcedByBestScore);
	void AddSensed(EOnSenseEvent Ost, bool bNewSenseForcedByBestScore);

	void NewAgeUpdate(float CurrentTime, EOnSenseEvent Ost);

	TArray<FSensedStimulus> GetArray_Copy(const TArray<ElementIndexType>& Arr) const;
	TArray<FSensedStimulus> GetArrayCopy_SenseEvent(ESensorArrayByType SenseEvent) const;

	void ResetArr(ESensorArrayByType SenseEvent);
	void EmptyArr(ESensorArrayByType SenseEvent);

	void Empty();

	friend FArchive& operator<<(FArchive& Ar, FSenseDetectPool& Sd)
	{
		//Ar << Sd.ObjPool;
		//Ar << Sd.Current;
		//Ar << Sd.Lost;
		//Ar << Sd.Forget;
		//Ar << Sd.NewCurrent;
		//Ar << Sd.LostCurrent;
		//Ar << Sd.Best_Sense;
		//Ar << Sd.DetectNew;
		//Ar << Sd.DetectCurrent;
		return Ar;
	}

	template<typename T, typename SortTPredicate>
	static void BestIdsByPredicate(const int32 Count, const TArray<T>& InArr, SortTPredicate Predicate, TArray<int32>& Out);

	bool LostIndex(const ElementIndexType ID);

private:

	FORCEINLINE ElementIndexType AddToPool(FSensedStimulus&& Elem)
	{
		return ObjPool.Add(MoveTemp(Elem));
	}
	FORCEINLINE ElementIndexType AddToPool(const ElementIndexType PoolId, FSensedStimulus&& Elem)
	{
		FSensedStimulus& Obj = ObjPool[PoolId];
		Elem.FirstSensedTime = Obj.FirstSensedTime;
		Obj = MoveTemp(Elem);
		return PoolId;
	}

	void BestScoreUpdt(TArray<ElementIndexType>& InArr, bool bNewSenseForcedByBestScore);


	TArray<ElementIndexType>& BySenseEvent_Ref(ESensorArrayByType SenseEvent);
	const TArray<ElementIndexType>& BySenseEvent_Ref(ESensorArrayByType SenseEvent) const;

	void ResetArr(TArray<ElementIndexType>& A, int32 Size);
	void ResetArr(TArray<ElementIndexType>& A) { ResetArr(A, A.Num()); }
	void EmptyArr(TArray<ElementIndexType>& A);
	void EmptyArr(TArray<ElementIndexType>&& A);


#if WITH_EDITOR
public:
	void CheckSort() const;
	bool CheckPoolDup();
	bool CheckPool();
#endif
};

FORCEINLINE void FSenseDetectPool::Empty()
{
	ObjPool.Empty();
	NewCurrent.Empty();
	Current.Empty();
	LostCurrent.Empty();
	Forget.Empty();
	Lost.Empty();
	Best_Sense.ScoreIDs.Empty();
}

FORCEINLINE const TSparseArray<FSensedStimulus>& FSenseDetectPool::GetPool() const
{
	return ObjPool;
}
FORCEINLINE TSparseArray<FSensedStimulus>& FSenseDetectPool::GetPool()
{
	return ObjPool;
}

FORCEINLINE TArray<ElementIndexType>& FSenseDetectPool::BySenseEvent_Ref(const ESensorArrayByType SenseEvent)
{
	switch (SenseEvent)
	{
		case ESensorArrayByType::SensedNew: return NewCurrent;
		case ESensorArrayByType::SenseCurrent: return Current;
		case ESensorArrayByType::SenseCurrentLost: return LostCurrent;
		case ESensorArrayByType::SenseForget: return Forget;
		case ESensorArrayByType::SenseLost: return Lost;
	}
	checkNoEntry();
	return Current;
}
FORCEINLINE const TArray<ElementIndexType>& FSenseDetectPool::BySenseEvent_Ref(const ESensorArrayByType SenseEvent) const
{
	switch (SenseEvent)
	{
		case ESensorArrayByType::SensedNew: return NewCurrent;
		case ESensorArrayByType::SenseCurrent: return Current;
		case ESensorArrayByType::SenseCurrentLost: return LostCurrent;
		case ESensorArrayByType::SenseForget: return Forget;
		case ESensorArrayByType::SenseLost: return Lost;
	}
	checkNoEntry();
	return Current;
}
FORCEINLINE TArray<FSensedStimulus> FSenseDetectPool::GetArrayCopy_SenseEvent(const ESensorArrayByType SenseEvent) const
{
	return GetArray_Copy(BySenseEvent_Ref(SenseEvent));
}

FORCEINLINE void FSenseDetectPool::ResetArr(TArray<ElementIndexType>& A, const int32 Size)
{
	for (const auto i : A)
	{
		ObjPool.RemoveAt(i);
	}
	A.Reset(Size);
}
FORCEINLINE void FSenseDetectPool::ResetArr(const ESensorArrayByType SenseEvent)
{
	check(SenseEvent != ESensorArrayByType::SensedNew && SenseEvent != ESensorArrayByType::SenseCurrentLost);
	ResetArr(BySenseEvent_Ref(SenseEvent));
}

FORCEINLINE void FSenseDetectPool::EmptyArr(TArray<ElementIndexType>& A)
{
	if (A.Num())
	{
		for (const auto i : A)
		{
			ObjPool.RemoveAt(i);
		}
		A.Empty();
	}
}
FORCEINLINE void FSenseDetectPool::EmptyArr(TArray<ElementIndexType>&& A)
{
	if (A.Num())
	{
		for (const auto i : A)
		{
			ObjPool.RemoveAt(i);
		}
		A.Empty();
	}
}

FORCEINLINE void FSenseDetectPool::EmptyArr(const ESensorArrayByType SenseEvent)
{
	check(SenseEvent != ESensorArrayByType::SensedNew && SenseEvent != ESensorArrayByType::SenseCurrentLost);
	EmptyArr(BySenseEvent_Ref(SenseEvent));
}

FORCEINLINE ElementIndexType FSenseDetectPool::ContainsInCurrentSense(const FSensedStimulus& InElem) const
{
	return ContainsIn(InElem, Current);
}
FORCEINLINE ElementIndexType FSenseDetectPool::ContainsInLostSense(const FSensedStimulus& InElem) const
{
	return ContainsIn(InElem, Lost);
}


template<typename T, typename SortTPredicate>
inline void FSenseDetectPool::BestIdsByPredicate(const int32 Count, const TArray<T>& InArr, SortTPredicate Predicate, TArray<int32>& Out)
{
	const int32 InArrNum = InArr.Num();
	if (InArrNum > 0 && Count > 0)
	{
		if (Count == 1) // O(N)
		{
			Out.Init(0, 1);
			for (int32 i = 1; i < InArrNum; ++i)
			{
				if (Predicate(i, Out[0]))
				{
					Out[0] = i;
				}
			}
		}
		else
		{
			Out.Reset(InArrNum);
			for (int32 i = 0; i < InArrNum; ++i)
			{
				Out.Add(i);
			}

			const int32 SortNum = FMath::Min(Count, Out.Num());
			if (Count < 10 && Out.Num() != SortNum) // O(N log K)
			{
				std::partial_sort(Out.GetData(), Out.GetData() + SortNum, Out.GetData() + InArrNum, Predicate);
			}
			else
			{
				if (Out.Num() != SortNum) // O(N)
				{
					std::nth_element(Out.GetData(), Out.GetData() + SortNum, Out.GetData() + InArrNum, Predicate);
				}
				TArrayView<int32> ArrayView = TArrayView<int32>(Out.GetData(), SortNum);
				Algo::Sort(ArrayView, Predicate); // O(K log K)
			}

			if (Out.Num() > Count)
			{
				Out.RemoveAt(Count, Out.Num() - Count, true);
			}
		}
	}
	else
	{
		Out.Empty();
	}
}