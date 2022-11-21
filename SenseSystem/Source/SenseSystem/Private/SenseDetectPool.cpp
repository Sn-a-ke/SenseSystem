//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "SenseDetectPool.h"


#include "HashSorted.h"
#include "SenseStimulusBase.h"


struct FSortIDPredicate
{
	explicit FSortIDPredicate(const TSparseArray<FSensedStimulus>& InPoolRef) : PoolRef(InPoolRef) {}
	FORCEINLINE bool operator()(const uint16 A, const uint16 B) const { return PoolRef[A].TmpHash < PoolRef[B].TmpHash; }
	FORCEINLINE bool operator()(const uint32 A, const uint16 B) const { return A < PoolRef[B].TmpHash; }
	FORCEINLINE bool operator()(const uint16 A, const uint32 B) const { return PoolRef[A].TmpHash < B; }

private:
	const TSparseArray<FSensedStimulus>& PoolRef;
};

struct FSortScorePredicate
{
	explicit FSortScorePredicate(const TSparseArray<FSensedStimulus>& InPoolRef, const TArray<uint16>& NewCurrentSensed)
		: PoolRef(InPoolRef)
		, Arr(NewCurrentSensed)
	{}
	FORCEINLINE bool operator()(const int32 A, const int32 B) const { return PoolRef[Arr[A]].Score > PoolRef[Arr[B]].Score; };

private:
	const TSparseArray<FSensedStimulus>& PoolRef;
	const TArray<uint16>& Arr;
};

struct FSortHashScorePredicate
{
	explicit FSortHashScorePredicate(const TSparseArray<FSensedStimulus>& InPoolRef, const TArray<uint16>& NewCurrentSensed)
		: PoolRef(InPoolRef)
		, Arr(NewCurrentSensed)
	{}
	FORCEINLINE bool operator()(const int32 A, const int32 B) const { return PoolRef[Arr[A]].TmpHash < PoolRef[Arr[B]].TmpHash; };

private:
	const TSparseArray<FSensedStimulus>& PoolRef;
	const TArray<uint16>& Arr;
};

/********/

struct FValidPoolIdx
{
	explicit FValidPoolIdx(const TSparseArray<FSensedStimulus>& In) : PoolRef(In) {}
	const TSparseArray<FSensedStimulus>& PoolRef;
	FORCEINLINE bool operator()(const uint16 ElemID) const { return !PoolRef[ElemID].StimulusComponent.IsValid(); }
};

struct FInvalidRemoveIdx
{
	explicit FInvalidRemoveIdx(TSparseArray<FSensedStimulus>& In) : PoolRef(In) {}
	TSparseArray<FSensedStimulus>& PoolRef;
	FORCEINLINE bool operator()(const uint16 ElemID) const
	{
		if (!PoolRef[ElemID].StimulusComponent.IsValid())
		{
			PoolRef.RemoveAt(ElemID);
			return true;
		}
		return false;
	}
};

/********/


uint16 FSenseDetectPool::ContainsIn(const FSensedStimulus& InElem, const TArray<uint16>& InArr) const
{
	if (InArr.Num() && InElem.TmpHash != MAX_uint32)
	{
		const int32 Idx = Algo::BinarySearch(InArr, InElem.TmpHash, FSortIDPredicate(ObjPool));
		if (Idx != INDEX_NONE)
		{
			const uint16 F = InArr[Idx];
			if (ObjPool[F].TmpHash == InElem.TmpHash)
			{
				return F;
			}
		}
	}
	return MAX_uint16;
}

TArray<FSensedStimulus> FSenseDetectPool::GetArray_Copy(const TArray<uint16>& Arr) const
{
	TArray<FSensedStimulus> Out;
	Out.Reserve(Arr.Num());
	for (const uint16 It : Arr)
	{
		const FSensedStimulus& Elem = ObjPool[It];
		if (Elem.TmpHash != MAX_uint32)
		{
			Out.Add(Elem);
		}
	}
	Out.Shrink();
	return MoveTemp(Out);
}


void FSenseDetectPool::NewSensedUpdate(const EOnSenseEvent Ost, const bool bOverrideSenseState, const bool bNewSenseForcedByBestScore)
{
	if (bOverrideSenseState)
	{
		NewSensed(Ost, bNewSenseForcedByBestScore);
	}
	else
	{
		AddSensed(Ost, bNewSenseForcedByBestScore);
	}
}


void FSenseDetectPool::BestScoreUpdt(TArray<uint16>& InArr, const bool bNewSenseForcedByBestScore)
{

#if WITH_EDITOR //todo remove this
	for (const auto It : InArr)
	{
		verifyf(
			ObjPool[It].Score >= Best_Sense.MinBestScore,
			TEXT("An object with a Score lower than acceptable MinBestScore has entered the SenseDetectPool, please report an error in the plugin, "
				 "with data on the setting of the current sensor, "));
	}
#endif

	auto& BestScore = Best_Sense.ScoreIDs;

	BestIdsByPredicate( //
		Best_Sense.TrackBestScoreCount,
		InArr,
		FSortScorePredicate(ObjPool, InArr),
		BestScore);

	if (InArr.Num() > 0 && InArr.Num() > BestScore.Num() && Best_Sense.TrackBestScoreCount > 0 && bNewSenseForcedByBestScore)
	{
		Algo::Sort(BestScore, FSortHashScorePredicate(ObjPool, InArr)); //sort IDs by hash
		for (int32 i = 0; i < BestScore.Num(); i++)
		{
			InArr[i] = InArr[BestScore[i]]; //override arr and IDs
			BestScore[i] = i;				//build new ID by score
		}
		if (InArr.Num() > BestScore.Num())
		{
			InArr.RemoveAt(BestScore.Num(), InArr.Num() - BestScore.Num(), true); // cut Arr by IDs Num
		}
		Algo::Sort(BestScore, FSortScorePredicate(ObjPool, InArr)); //sort back to score
	}
}


void FSenseDetectPool::NewSensed(const EOnSenseEvent Ost, const bool bNewSenseForcedByBestScore)
{
	ResetArr(ESensorArrayByType::SenseForget);

	const auto SortPred = FSortIDPredicate(ObjPool);
	{
		Algo::Sort(DetectNew, SortPred);

		FPlatformMisc::MemoryBarrier();

		NewCurrent = DetectNew;
		NewCurrent.Append(DetectCurrent);
		Algo::Sort(NewCurrent, SortPred);

		BestScoreUpdt(NewCurrent, bNewSenseForcedByBestScore);

		FPlatformMisc::MemoryBarrier();

		ArraySorted::ArrayMinusArray_SortedPredicate(DetectNew, NewCurrent, SortPred);

		EmptyArr(DetectNew);
		DetectCurrent.Empty();
	}

	TArray<uint16> TmpNewCurrentSensed = NewCurrent;

	if (Ost >= EOnSenseEvent::SenseNew)
	{
		ArraySorted::ArrayMinusArray_SortedPredicate(NewCurrent, Current, SortPred);
		ArraySorted::ArrayMinusArray_SortedPredicate(Current, TmpNewCurrentSensed, SortPred);

		if (Ost >= EOnSenseEvent::SenseLost)
		{
			LostCurrent = Current;
			ArraySorted::ArrayMinusArray_SortedPredicate(Lost, TmpNewCurrentSensed, SortPred);

			if (Ost >= EOnSenseEvent::SenseForget)
			{
				ArraySorted::Merge_SortedPredicate(Lost, LostCurrent, SortPred);
			}
			else
			{
				EmptyArr(ESensorArrayByType::SenseForget);
				ResetArr(ESensorArrayByType::SenseLost);
				Lost = LostCurrent;
			}
		}
		else
		{
			ResetArr(ESensorArrayByType::SenseCurrent);
			EmptyArr(ESensorArrayByType::SenseForget);
			EmptyArr(ESensorArrayByType::SenseLost);
			LostCurrent.Empty();
		}
	}
	else
	{
		ArraySorted::ArrayMinusArray_SortedPredicate(Current, TmpNewCurrentSensed, SortPred);
		ResetArr(ESensorArrayByType::SenseCurrent);
		EmptyArr(ESensorArrayByType::SenseForget);
		EmptyArr(ESensorArrayByType::SenseLost);
		LostCurrent.Empty();
		NewCurrent.Empty();
	}

	Current = MoveTemp(TmpNewCurrentSensed);
}

void FSenseDetectPool::AddSensed(const EOnSenseEvent Ost, const bool bNewSenseForcedByBestScore)
{
	const auto SortPred = FSortIDPredicate(ObjPool);
	Algo::Sort(DetectNew, SortPred);

	TArray<uint16> TmpNewCurrentSensed = DetectNew;
	TmpNewCurrentSensed.Append(DetectCurrent);
	DetectCurrent.Empty();
	Algo::Sort(TmpNewCurrentSensed, SortPred);

	BestScoreUpdt(TmpNewCurrentSensed, bNewSenseForcedByBestScore); //cut by min score DetectNew

	/*if (bNewSenseForcedByBestScore)//todo AddSensed bNewSenseForcedByBestScore need this check?
	{
		ArraySorted::ArrayMinusArray_SortedPredicate(_Current_Sensed, TmpNewCurrentSensed, SortPred);
		ArraySorted::ArrayMinusArray_SortedPredicate(_New_Sensed, TmpNewCurrentSensed, SortPred);
		EmptyArr(_Current_Sensed, InChannelSetup.GetSenseBitChannel());
		EmptyArr(_New_Sensed, InChannelSetup.GetSenseBitChannel());
	}*/

	ArraySorted::ArrayMinusArray_SortedPredicate(DetectNew, TmpNewCurrentSensed, SortPred);
	EmptyArr(DetectNew);

	ArraySorted::Merge_SortedPredicate(Current, TmpNewCurrentSensed, SortPred);

	//BestScoreUpdt(Current, false);

	if (Ost >= EOnSenseEvent::SenseNew)
	{
		ArraySorted::Merge_SortedPredicate(NewCurrent, MoveTemp(TmpNewCurrentSensed), SortPred);
	}
	else
	{
		NewCurrent.Reset();
	}
}


void FSenseDetectPool::EmptyUpdate(const EOnSenseEvent Ost, const bool bOverrideSenseState /*= true*/)
{
	ResetArr(ESensorArrayByType::SenseForget);

	EmptyArr(DetectNew);
	DetectCurrent.Empty();

	if (bOverrideSenseState)
	{
		Best_Sense.ScoreIDs.Empty();

		if (Ost >= EOnSenseEvent::SenseNew)
		{
			NewCurrent.Empty();
			if (Ost >= EOnSenseEvent::SenseLost)
			{
				LostCurrent = Current;
				Current.Reset();
				if (Ost >= EOnSenseEvent::SenseForget)
				{
					ArraySorted::Merge_SortedPredicate(Lost, LostCurrent, FSortIDPredicate(ObjPool));
				}
				else
				{
					EmptyArr(ESensorArrayByType::SenseForget);
					ResetArr(ESensorArrayByType::SenseLost);
					Lost = LostCurrent;
				}
			}
			else
			{
				ResetArr(ESensorArrayByType::SenseCurrent);
				EmptyArr(ESensorArrayByType::SenseForget);
				EmptyArr(ESensorArrayByType::SenseLost);
				LostCurrent.Empty();
			}
		}
		else
		{
			EmptyArr(ESensorArrayByType::SenseCurrent);
			EmptyArr(ESensorArrayByType::SenseForget);
			EmptyArr(ESensorArrayByType::SenseLost);
			LostCurrent.Empty();
			NewCurrent.Empty();
		}
	}
	else
	{
		if (Ost >= EOnSenseEvent::SenseNew && NewCurrent.Num())
		{
			ArrayHelpers::Filter_Sorted_V2(NewCurrent, FValidPoolIdx(GetPool()), false);
		}
		if (Current.Num())
		{
			ArrayHelpers::Filter_Sorted_V2(Current, FInvalidRemoveIdx(GetPool()), false);
		}

		{
			TArray<uint16> TmpNewCurrentSensed = Current;
			BestScoreUpdt(TmpNewCurrentSensed, false);
			if (Current.Num() != TmpNewCurrentSensed.Num())
			{
				ArraySorted::ArrayMinusArray_SortedPredicate(Current, TmpNewCurrentSensed, FSortIDPredicate(ObjPool));
				EmptyArr(ESensorArrayByType::SenseCurrent /*Current*/);
			}
			Current = MoveTemp(TmpNewCurrentSensed);
		}
	}
}


void FSenseDetectPool::NewAgeUpdate(const float CurrentTime, const EOnSenseEvent Ost)
{
	if (Ost >= EOnSenseEvent::SenseForget)
	{
		if (Lost.Num())
		{
			TArray<uint16> Rem;
			Rem.Reserve(Lost.Num());
			const int32 LostNum = Lost.Num();
			int32 r = -1;
			for (int32 i = 0; i < LostNum; i++)
			{
				FSensedStimulus& SS = ObjPool[Lost[i]];
				const auto Ssc = SS.StimulusComponent.Get();
				if (SS.StimulusComponent.IsValid() && Ssc && Ssc->IsRegisteredForSense() && Ssc->bEnable)
				{
					if (SS.Age == 0.f || CurrentTime >= (SS.Age + SS.SensedTime))
					{
						Forget.Add(Lost[i]); //remove
					}
					else
					{
						r++;
						Lost[r] = Lost[i]; //false
					}
				}
				else
				{
					Rem.Add(Lost[i]);
				}
			}

			r++;
			if (r < Lost.Num())
			{
				Lost.RemoveAt(r, Lost.Num() - r, true);
			}
			ArraySorted::ArrayMinusArray_SortedPredicate(LostCurrent, Rem, FSortIDPredicate(GetPool()));
			EmptyArr(MoveTemp(Rem));
		}
		
		Forget.Shrink();
	}
	else
	{
		EmptyArr(ESensorArrayByType::SenseForget); //todo NewAgeUpdate Ost < EOnSenseEvent::SenseForget

		//EmptyArr(ESensorArrayByType::SenseLost);
		//_LostCurrent_Sensed.Empty();
	}

	//ObjPool.ShrinkPool();
}

void FSenseDetectPool::EmptyArr(TArray<uint16>& A, const uint64 Channel)
{
	if (A.Num())
	{
		for (const uint16 i : A)
		{
			ObjPool[i].BitChannels |= ~Channel;
			if (ObjPool[i].BitChannels == 0)
			{
				ObjPool.RemoveAt(i);
			}
		}
		A.Empty();
	}
}

#if WITH_EDITOR

void FSenseDetectPool::CheckSort() const
{
	const auto& SortPred = FSortIDPredicate(GetPool());

	check(Algo::IsSorted(NewCurrent, SortPred));
	check(Algo::IsSorted(Current, SortPred));
	check(Algo::IsSorted(LostCurrent, SortPred));
	check(Algo::IsSorted(Lost, SortPred));
	//check(Algo::IsSorted(_Forget_Sensed, SortPred));
}

bool FSenseDetectPool::CheckPoolDup()
{
	/*
	const auto& Array = GetPool().GetArray();
	for (int32 i = 0; i < Array.Num(); i++)
	{
		const FSetElementId Idi = FSetElementId(i);
		if (Array[Idi].StimulusComponent)
		{
			for (int32 j = 0; j < Array.Num(); j++)
			{
				const FSetElementId Idj(j);
				if (j != i && Array[Idj].StimulusComponent && Array[Idi] == Array[Idj])
				{
					checkNoEntry();
					return false;
				}
			}
		}
	}
	*/
	return true;
}

bool FSenseDetectPool::CheckPool()
{
	/*
	const auto& Array = GetPool().GetArray();
	for (const auto& Elem : Array)
	{
		if (Elem.TmpHash == MAX_uint32) continue;
		if (Elem.StimulusComponent == nullptr) continue;
		if (ContainsIn(Elem, Current)) continue;
		if (ContainsIn(Elem, Lost)) continue;
		if (ContainsIn(Elem, Forget)) continue;
		//if (ContainsIn(Elem, _New_Sensed)) continue;
		//if (ContainsIn(Elem, _LostCurrent_Sensed)) continue;

		checkNoEntry();
		return false;
	}
	*/
	return CheckPoolDup();
}

#endif
