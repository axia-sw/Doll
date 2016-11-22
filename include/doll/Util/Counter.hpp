#pragma once

#include "../Core/Defs.hpp"
#include "../Core/Memory.hpp"
#include "../Core/MemoryTags.hpp"

namespace doll
{

	class CFrameCounter;

	typedef F64( __stdcall *FnCalculateFrameCounter )( Void *pParm, F64 fCurrent, F64 fPrior );

	enum class EFrameCounter
	{
		UserCallback				= 0,

		Linear						= 1,
		Slerp						= 2,
		Accel						= 3,
		Decel						= 4,
		AccelDecel					= 5
	};
	enum class EFrameCounterMode
	{
		Normal						= 0,

		Clamp						= 1,
		ClampAtStart				= 2,
		WrapAround					= 3,
		LoopOnce					= 4,
		Loop						= 5
	};

	class CFrameCounter: public TPoolObject< CFrameCounter, kTag_Counter >
	{
	public:
		static U64 guCurrentTick;

		static Void updateAll();

		CFrameCounter();
		~CFrameCounter();

		Void setType( EFrameCounter type );
		EFrameCounter getType() const;

		Void setMode( EFrameCounterMode mode );
		EFrameCounterMode getMode() const;

		F64 getValue() const;
		Bool isActive() const;

		Void setCalculateCallback( FnCalculateFrameCounter pfnCalculate, Void *pParameter = NULL );
		FnCalculateFrameCounter getCalculateFunction() const;
		Void *getCalculateParameter() const;

		Void setRange( F64 fStart, F64 fEnd );
		F64 getRangeStart() const;
		F64 getRangeEnd() const;
		inline F64 getRange() const { return getRangeEnd() - getRangeStart(); }

		Void activate( F64 fTimeInSeconds );
		Void activateMicroseconds( U64 uTimeInMicroseconds );
		Void deactivate();

	private:
		static TIntrList< CFrameCounter > gFrameCounters;
		static ax::CQuickMutex            gListAccessor;

		FnCalculateFrameCounter    mpfnCalculate;
		Void *                     mpParm;
		EFrameCounterMode          mMode;
		U64                        muStart;
		U64                        muEnd;
		F64                        mfCurrent;
		F64                        mfRange[ 2 ];
		TIntrLink< CFrameCounter > mSiblings;

		Void update();
	};

}
