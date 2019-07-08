#include "Timer.h"

Timer::Timer()
{
	__int64 freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)& freq);
	this->mSecondsPerCount = 1.0 / (double)freq;
}

float Timer::GameTime() const
{
	// If we are stopped, dont count time passed since stop
	// also, subtract paused time from stoptime
	if (this->mStopped)
	{
		return (float)(((mStopTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}
	else
	{
		return (float)(((mCurrTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}
}

float Timer::DeltaTime() const
{
	return (float)this->mDeltaTime;
}

void Timer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)& currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}

void Timer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)& startTime);

	// If previously stopped, accumulate paused time
	if (this->mStopped)
	{
		mPausedTime += (startTime - mStopTime);

		mPrevTime = startTime;
		mStopTime = 0;
		mStopped = false;
	}
}

void Timer::Stop()
{
	if (!this->mStopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)& currTime);
		mStopTime = currTime;
		mStopped = true;
	}
}

void Timer::Tick()
{
	if (this->mStopped)
		this->mDeltaTime = 0.0;
	else
	{
		// Get current time
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)& currTime);
		this->mCurrTime = currTime;

		// delta time
		this->mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

		mPrevTime = mCurrTime;

		// Prevents weird stuff
		if (mDeltaTime < 0.0)
		{
			mDeltaTime = 0.0;
		}
	}
}
