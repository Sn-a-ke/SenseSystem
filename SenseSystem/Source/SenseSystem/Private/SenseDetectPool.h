//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Platform.h"
#include "Containers/Array.h"
#include "Containers/SparseArray.h"
#include <algorithm>

#include "SensedStimulStruct.h"


class FSenseDetectPool
{
	TSparseArray<FSensedStimulus> ObjPool;

public:
	FSenseDetectPool() { ObjPool.Reserve(128); }
	~FSenseDetectPool() {}

	const TSparseArray<FSensedStimulus>& GetPool() const;
	TSparseArray<FSensedStimulus>& GetPool();

	TArray<uint16> Current;
	TArray<uint16> Lost;
	TArray<uint16> Forget;

	TArray<uint16> NewCurrent;
	TArray<uint16> LostCurrent;

	/**best sensed detection struct*/
	FTrackBestSenseScore Best_Sense;

	TArray<uint16> DetectNew;
	TArray<uint16> DetectCurrent;


public:
	void Add(const float CurrentTime, FSensedStimulus SS, const uint16 ID)
	{
		if (SS.TmpHash != MAX_uint32)
		{
			if (ID == MAX_uint16)
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

	uint16 ContainsInCurrentSense(const FSensedStimulus& InElem) const;
	uint16 ContainsInLostSense(const FSensedStimulus& InElem) const;

	uint16 ContainsIn(const FSensedStimulus& InElem, const TArray<uint16>& InArr) const;

	void NewSensedUpdate(EOnSenseEvent Ost, bool bOverrideSenseState, bool bNewSenseForcedByBestScore);
	void EmptyUpdate(EOnSenseEvent Ost, bool bOverrideSenseState = true);

	void NewSensed(EOnSenseEvent Ost, bool bNewSenseForcedByBestScore);
	void AddSensed(EOnSenseEvent Ost, bool bNewSenseForcedByBestScore);

	void NewAgeUpdate(float CurrentTime, EOnSenseEvent Ost);

	TArray<FSensedStimulus> GetArray_Copy(const TArray<uint16>& Arr) const;
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

private:

	FORCEINLINE uint16 AddToPool(FSensedStimulus&& Elem)
	{
		return ObjPool.Add(MoveTemp(Elem));
	}
	FORCEINLINE uint16 AddToPool(const uint16 PoolId, FSensedStimulus&& Elem)
	{
		FSensedStimulus& Obj = ObjPool[PoolId];
		Elem.FirstSensedTime = Obj.FirstSensedTime;
		Obj = MoveTemp(Elem);
		return PoolId;
	}

	void BestScoreUpdt(TArray<uint16>& InArr, bool bNewSenseForcedByBestScore);


	TArray<uint16>& BySenseEvent_Ref(ESensorArrayByType SenseEvent);
	const TArray<uint16>& BySenseEvent_Ref(ESensorArrayByType SenseEvent) const;

	void ResetArr(TArray<uint16>& A, int32 Size);
	void ResetArr(TArray<uint16>& A) { ResetArr(A, A.Num()); }
	void EmptyArr(TArray<uint16>& A);
	void EmptyArr(TArray<uint16>&& A);

	UE_DEPRECATED(4.23, "unused")
	void EmptyArr(TArray<uint16>& A, const uint64 Channel);

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

FORCEINLINE TArray<uint16>& FSenseDetectPool::BySenseEvent_Ref(const ESensorArrayByType SenseEvent)
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
FORCEINLINE const TArray<uint16>& FSenseDetectPool::BySenseEvent_Ref(const ESensorArrayByType SenseEvent) const
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

FORCEINLINE void FSenseDetectPool::ResetArr(TArray<uint16>& A, const int32 Size)
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

FORCEINLINE void FSenseDetectPool::EmptyArr(TArray<uint16>& A)
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
FORCEINLINE void FSenseDetectPool::EmptyArr(TArray<uint16>&& A)
{
	if (A.Num())
	{
		for (auto i : A)
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

FORCEINLINE uint16 FSenseDetectPool::ContainsInCurrentSense(const FSensedStimulus& InElem) const
{
	return ContainsIn(InElem, Current);
}
FORCEINLINE uint16 FSenseDetectPool::ContainsInLostSense(const FSensedStimulus& InElem) const
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