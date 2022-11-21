//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "SenseSystem.h"

#include "CoreMinimal.h"

#include "Async/AsyncWork.h"
#include "Async/Async.h"

#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"

#include "Containers/Queue.h"


class USensorBase;


/**
* SensorQueue
*/
class FSensorQueue : private TQueue<USensorBase*>
{
public:
	FSensorQueue() {}
	~FSensorQueue() {}

	bool Enqueue(USensorBase* Item);
	USensorBase* Dequeue();
	USensorBase* Peek() const;
	bool Pop();
	void Empty();
	bool IsEmpty() const;
	int32 Num() const;

private:
	int32 QueueNum = 0;
};


/**
 * SenseRunnable Thread
 */
class FSenseRunnable final : public FRunnable
{
public:
	FSenseRunnable(double InWaitTime = 0.0001f, int32 InCounter = 10);
	virtual ~FSenseRunnable() override;

#if WITH_EDITOR
	bool bSenseThreadPauseLog = false;
	bool bSenseThreadStateLog = true;
#endif

	void EnsureCompletion();
	void PauseThread();
	void ContinueThread();
	bool IsThreadPaused() const;

	//FRunnable interface.
	virtual bool Init() override;
	virtual void Stop() override;
	virtual void Exit() override;
	virtual uint32 Run() override;

	//FSenseRunnable AddQueueSensors
	bool AddQueueSensors(USensorBase* Sensor, bool bHighPriority = false);

private:
	const double WaitTime;
	const int32 CounterLimit;
	int32 Counter = 0;

	bool UpdateQueue();

	//Thread to run the worker FRunnable on
	FRunnableThread* Thread;
	FEvent* WaitState;

	//As the name states those members are Thread safe
	FThreadSafeBool m_Kill;
	FThreadSafeBool m_Pause;

	FSensorQueue SensorQueue;
	FSensorQueue HighSensorQueue;
};


FORCEINLINE bool FSensorQueue::Enqueue(USensorBase* Item)
{
	QueueNum++;
	return TQueue::Enqueue(Item);
}

FORCEINLINE USensorBase* FSensorQueue::Dequeue()
{
	USensorBase* Ptr = nullptr;
	if (TQueue::Dequeue(Ptr))
	{
		QueueNum--;
	}
	return Ptr;
}

FORCEINLINE USensorBase* FSensorQueue::Peek() const
{
	USensorBase* Ptr = nullptr;
	TQueue::Peek(Ptr);
	return Ptr;
}

FORCEINLINE bool FSensorQueue::Pop()
{
	QueueNum--;
	return TQueue::Pop();
}

FORCEINLINE void FSensorQueue::Empty()
{
	TQueue::Empty();
}

FORCEINLINE bool FSensorQueue::IsEmpty() const
{
	checkSlow(!TQueue::IsEmpty() || QueueNum > 0);
	return TQueue::IsEmpty();
}

FORCEINLINE int32 FSensorQueue::Num() const
{
	return QueueNum;
}


FORCEINLINE void FSenseRunnable::EnsureCompletion()
{
	Stop();
	if (Thread)
	{
		Thread->WaitForCompletion();
	}
}

FORCEINLINE void FSenseRunnable::PauseThread()
{
	if (!IsThreadPaused())
	{
		m_Pause = true;
#if WITH_EDITOR
		if (bSenseThreadPauseLog)
		{
			UE_LOG(LogSenseSys, Log, TEXT("SenseThread PausedThread"));
		}
#endif
	}
}

FORCEINLINE void FSenseRunnable::ContinueThread()
{
	if (IsThreadPaused())
	{
		m_Pause = false;
		if (WaitState) /* && bForcedPause == false*/
		{
			WaitState->Trigger();
		}
#if WITH_EDITOR
		if (bSenseThreadPauseLog)
		{
			UE_LOG(LogSenseSys, Log, TEXT("SenseThread ContinueThread"));
		}
#endif
	}
}

FORCEINLINE bool FSenseRunnable::IsThreadPaused() const
{
	return m_Pause;
}


FORCEINLINE bool FSenseRunnable::Init()
{
#if WITH_EDITOR
	if (bSenseThreadStateLog)
	{
		UE_LOG(LogSenseSys, Log, TEXT("SenseThread Initialized"));
	}
#endif
	return true;
}

FORCEINLINE void FSenseRunnable::Stop()
{
	m_Kill = true;
	m_Pause = false;
	if (WaitState)
	{
		WaitState->Trigger();
	}
}

FORCEINLINE void FSenseRunnable::Exit()
{
#if WITH_EDITOR
	if (bSenseThreadStateLog)
	{
		UE_LOG(LogSenseSys, Log, TEXT("SenseThread Exit"));
	}
#endif
}
