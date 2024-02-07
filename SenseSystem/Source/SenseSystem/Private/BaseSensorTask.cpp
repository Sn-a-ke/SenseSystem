//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "BaseSensorTask.h"
#include "SenseManager.h"
#include "Sensors/SensorBase.h"
#include "HAL/Platform.h"
#include "UObject/UObjectGlobals.h"


FSenseRunnable::FSenseRunnable(const double InWaitTime, const int32 InCounter) : WaitTime(InWaitTime), CounterLimit(InCounter)
{
	m_Kill = false;
	m_Pause = true;
	WaitState = FGenericPlatformProcess::GetSynchEventFromPool(false);

	Thread = FRunnableThread::Create(this, TEXT("FSenseRunnable"), 0, TPri_BelowNormal);
#if WITH_EDITOR
	if (bSenseThreadStateLog)
	{
		UE_LOG(LogSenseSys, Log, TEXT("SenseThread Created"));
	}
#endif
}

FSenseRunnable::~FSenseRunnable()
{
	if (WaitState)
	{
		//Cleanup the FEvent
		FGenericPlatformProcess::ReturnSynchEventToPool(WaitState);
		WaitState = nullptr;
	}
	if (Thread)
	{
		//Cleanup the worker thread
		delete Thread;
		Thread = nullptr;
	}
#if WITH_EDITOR
	if (bSenseThreadStateLog)
	{
		UE_LOG(LogSenseSys, Log, TEXT("SenseThread Destroyed"));
	}
#endif
}

uint32 FSenseRunnable::Run()
{
	while (!m_Kill)
	{
		if (m_Pause)
		{
			WaitState->Wait();
			if (m_Kill)
			{
				return 0;
			}
		}

		if (!m_Kill)
		{
			if (!UpdateQueue() || CounterLimit <= Counter)
			{
				Counter = 0;
				WaitState->Wait(FTimespan::FromSeconds(WaitTime));
			}
		}
		else
		{
			PauseThread();
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
		PauseThread();
	}
	if (IsThreadPaused())
	{
		return true;
	}

	bool bPop = true;
	{
		USensorBase* const Sensor = Queue.Peek();
		if (IsValid(Sensor) && Sensor->IsValidForTest_Short())
		{
			if (Sensor->UpdateState.Get() == ESensorState::ReadyToUpdate)
			{
				bPop = Sensor->UpdateSensor();
			}
			else
			{
				Sensor->UpdateState = ESensorState::NotUpdate; //skip
				bPop = true;
			}
		}
	}

	if (bPop)
	{
		Queue.Pop();
		Counter++;
		if (SensorQueue.IsEmpty() && HighSensorQueue.IsEmpty())
		{
			PauseThread();
		}
	}
	return bPop;
}

bool FSenseRunnable::AddQueueSensors(USensorBase* Sensor, const bool bHighPriority)
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
	ContinueThread();
	return true;
}
