#pragma once
#include <Windows.h>

class Timer
{
public:
	Timer();

	float GameTime() const;
	float DeltaTime() const;

	void Reset();
	void Start();
	void Stop();
	void Tick();

private:
	double mSecondsPerCount = 0.0;
	double mDeltaTime = -1.0;

	__int64 mBaseTime = 0;
	__int64 mPausedTime = 0;
	__int64 mStopTime = 0;
	__int64 mPrevTime = 0;
	__int64 mCurrTime = 0;

	bool mStopped = false;
};