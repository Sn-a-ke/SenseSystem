//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "SenseSystem.h"

#include "CoreMinimal.h"

#include "Async/AsyncWork.h"
#include "Async/Async.h"

#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/ThreadingBase.h"
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
class FSenseRunnable final
	: public FRunnable
	//, FSingleThreadRunnable
{
public:
	FSenseRunnable(double InWaitTime = 0.0001, int32 InCounter = 10);
	virtual ~FSenseRunnable() override;

#if WITH_EDITOR
	bool bSenseThreadPauseLog = false;
	bool bSenseThreadStateLog = true;
#endif

	void EnsureCompletion();

	//FRunnable interface.
	virtual bool Init() override;
	virtual void Stop() override;
	virtual void Exit() override;
	virtual uint32 Run() override;

	//virtual class FSingleThreadRunnable* GetSingleThreadInterface() { return nullptr; }
	//virtual void Tick() override {};

	//FSenseRunnable AddQueueSensors
	bool AddQueueSensors(USensorBase* Sensor, bool bHighPriority = false);

private:
	const double WaitTime;
	const int32 CounterLimit;
	int32 Counter = 0;
	uint32 SenseThreadId = 0;

	bool UpdateQueue();

	//Thread to run the worker FRunnable on
	FRunnableThread* Thread;
	FEvent* WorkEvent;

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


FORCEINLINE bool FSenseRunnable::Init()
{
	SenseThreadId = Thread->GetThreadID();
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
	WorkEvent->Trigger();
}

FORCEINLINE void FSenseRunnable::Exit()
{
	Stop();
	Thread->Kill();
#if WITH_EDITOR
	if (bSenseThreadStateLog)
	{
		UE_LOG(LogSenseSys, Log, TEXT("SenseThread Exit"));
	}
#endif
	SenseThreadId = 0;
}
