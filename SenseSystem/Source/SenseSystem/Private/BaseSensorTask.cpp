//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "BaseSensorTask.h"
#include "SenseManager.h"
#include "Sensors/SensorBase.h"
#include "HAL/Platform.h"
#include "UObject/UObjectGlobals.h"


FSenseRunnable::FSenseRunnable(const double InWaitTime, const int32 InCounter) : WaitTime(InWaitTime), CounterLimit(InCounter)
{
	m_Kill = false;
	Thread = FRunnableThread::Create(			 //
		this,									 //
		TEXT("FSenseRunnable"),					 //
		0,										 //
		EThreadPriority::TPri_Lowest,			 //
		FPlatformAffinity::GetNoAffinityMask()); //

	WorkEvent = FPlatformProcess::GetSynchEventFromPool();
}

FSenseRunnable::~FSenseRunnable()
{
	FPlatformProcess::ReturnSynchEventToPool(WorkEvent);
	if (Thread)
	{
		//Cleanup the worker thread
		delete Thread;
		Thread = nullptr;
	}
}

uint32 FSenseRunnable::Run()
{
	while (!m_Kill)
	{
		if (m_Kill)
		{
			return 0;
		}
		if (!m_Kill)
		{
			WorkEvent->Wait(1);
			if (!UpdateQueue() || CounterLimit <= Counter)
			{
				Counter = 0;
			}
		}
		else
		{
			Thread->SetThreadPriority(EThreadPriority::TPri_Lowest);
		}
	}
	return 0;
}

bool FSenseRunnable::UpdateQueue()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_FSenseRunnableTick);

	if (m_Kill)
	{
		return false;
	}

	FSensorQueue& Queue = HighSensorQueue.IsEmpty() ? SensorQueue : HighSensorQueue;

	if (Queue.IsEmpty())
	{
		Thread->SetThreadPriority(EThreadPriority::TPri_Lowest);
	}
	if (Thread->GetThreadPriority() == EThreadPriority::TPri_Lowest /*IsThreadPaused()*/)
	{
		return true;
	}

	bool bPop = true;
	USensorBase* const Sensor = Queue.Peek();
	if (LIKELY(IsValid(Sensor) && Sensor->IsValidForTest_Short()))
	{
		if (LIKELY(Sensor->UpdateState.Get() == ESensorState::ReadyToUpdate))
		{
			bPop = Sensor->UpdateSensor();
		}
		else
		{
			Sensor->UpdateState = ESensorState::NotUpdate; //skip
			bPop = true;
		}
	}

	if (LIKELY(bPop))
	{
		Queue.Pop();
		Counter++;
		if (SensorQueue.IsEmpty() && HighSensorQueue.IsEmpty())
		{
			Thread->SetThreadPriority(EThreadPriority::TPri_Lowest);
		}
	}
	return bPop;
}

bool FSenseRunnable::AddQueueSensors(USensorBase* Sensor, const bool bHighPriority)
{
	if (Sensor)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_SenseSys_AddQueueSensors);
		if (bHighPriority)
		{
			HighSensorQueue.Enqueue(Sensor);
		}
		else
		{
			SensorQueue.Enqueue(Sensor);
		}
		Thread->SetThreadPriority(EThreadPriority::TPri_Normal);
		return true;
	}
	return false;
}
