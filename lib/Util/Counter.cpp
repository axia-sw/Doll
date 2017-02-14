#define DOLL_TRACE_FACILITY doll::kLog_UtilCounter

#include "doll/Util/Counter.hpp"

#include "doll/Math/Basic.hpp"

namespace doll
{

	U64                        CFrameCounter::guCurrentTick = 0;
	TIntrList< CFrameCounter > CFrameCounter::gFrameCounters;
	ax::CQuickMutex            CFrameCounter::gListAccessor;

	CFrameCounter::CFrameCounter()
	: mpfnCalculate( NULL )
	, mpParm( NULL )
	, mMode( EFrameCounterMode::Normal )
	, muStart( 0 )
	, muEnd( 0 )
	, mfCurrent( 0.0 )
	, mSiblings( this )
	{
		mfRange[ 0 ] = 0.0;
		mfRange[ 1 ] = 1.0;

		{
			gListAccessor.lock();
			gFrameCounters.addTail( mSiblings );
			gListAccessor.unlock();
		}
	}
	CFrameCounter::~CFrameCounter()
	{
		gListAccessor.lock();
		gFrameCounters.unlink( mSiblings );
		gListAccessor.unlock();
	}

	static F64 DOLL_API calcLinear_f( Void *, F64 fValue, F64 )
	{
		return fValue;
	}
	static F64 DOLL_API calcSlerp_f( Void *, F64 fValue, F64 )
	{
		return slerp( 0.0, 1.0, float( fValue ) );
	}
	static F64 DOLL_API calcAccel_f( Void *, F64 fValue, F64 )
	{
		return fValue*fValue;
	}
	static F64 DOLL_API calcDecel_f( Void *, F64 fValue, F64 )
	{
		const F64 fInverted = 1.0 - fValue;

		return fInverted*fInverted;
	}
	static F64 DOLL_API calcAccelDecel_f( Void *, F64 fValue, F64 )
	{
		if( fValue < 0.5 ) {
			return fValue*fValue;
		}

		const F64 fInverted = 1.0 - fValue;
		return fInverted*fInverted;
	}

	Void CFrameCounter::setType( EFrameCounter type )
	{
		switch( type )
		{
		case EFrameCounter::UserCallback:
			mpfnCalculate = NULL;
			mpParm = NULL;
			break;

		case EFrameCounter::Linear:
			mpfnCalculate = &calcLinear_f;
			mpParm = NULL;
			break;

		case EFrameCounter::Slerp:
			mpfnCalculate = &calcSlerp_f;
			mpParm = NULL;
			break;

		case EFrameCounter::Accel:
			mpfnCalculate = &calcAccel_f;
			mpParm = NULL;
			break;
		case EFrameCounter::Decel:
			mpfnCalculate = &calcDecel_f;
			mpParm = NULL;
			break;
		case EFrameCounter::AccelDecel:
			mpfnCalculate = &calcAccelDecel_f;
			mpParm = NULL;
			break;
		}
	}
	EFrameCounter CFrameCounter::getType() const
	{
		if( mpfnCalculate == &calcLinear_f ) {
			return EFrameCounter::Linear;
		}
		if( mpfnCalculate == &calcSlerp_f ) {
			return EFrameCounter::Slerp;
		}
		if( mpfnCalculate == &calcAccel_f ) {
			return EFrameCounter::Accel;
		}
		if( mpfnCalculate == &calcDecel_f ) {
			return EFrameCounter::Decel;
		}
		if( mpfnCalculate == &calcAccelDecel_f ) {
			return EFrameCounter::AccelDecel;
		}

		return EFrameCounter::UserCallback;
	}

	Void CFrameCounter::setMode( EFrameCounterMode mode )
	{
		mMode = mode;
	}
	EFrameCounterMode CFrameCounter::getMode() const
	{
		return mMode;
	}

	F64 CFrameCounter::getValue() const
	{
		return mfCurrent;
	}
	Bool CFrameCounter::isActive() const
	{
		const U64 uElapsed = muStart - guCurrentTick;
		const U64 uTotal = muEnd - muStart;

		switch( mMode )
		{
		case EFrameCounterMode::Normal:
		case EFrameCounterMode::Clamp:
		case EFrameCounterMode::ClampAtStart:
			return uElapsed > uTotal;

		case EFrameCounterMode::LoopOnce:
			return uElapsed > uTotal*2;

		case EFrameCounterMode::WrapAround:
		case EFrameCounterMode::Loop:
			return uTotal > 0 ;
		}

		return false;
	}

	Void CFrameCounter::setCalculateCallback( FnCalculateFrameCounter pfnCalculate, Void *pParameter )
	{
		mpfnCalculate = pfnCalculate;
		mpParm = pParameter;
	}
	FnCalculateFrameCounter CFrameCounter::getCalculateFunction() const
	{
		return mpfnCalculate;
	}
	Void *CFrameCounter::getCalculateParameter() const
	{
		return mpParm;
	}

	Void CFrameCounter::setRange( F64 fStart, F64 fEnd )
	{
		mfRange[ 0 ] = fStart;
		mfRange[ 1 ] = fEnd;
	}
	F64 CFrameCounter::getRangeStart() const
	{
		return mfRange[ 0 ];
	}
	F64 CFrameCounter::getRangeEnd() const
	{
		return mfRange[ 1 ];
	}

	Void CFrameCounter::activate( F64 fTimeInSeconds )
	{
		activateMicroseconds( secondsToMicroseconds( ( axtm_fp_t )fTimeInSeconds ) );
	}
	Void CFrameCounter::activateMicroseconds( U64 uTimeInMicroseconds )
	{
		muStart = guCurrentTick;
		muEnd = muStart + uTimeInMicroseconds;
	}
	Void CFrameCounter::deactivate()
	{
		muStart = 0;
		muEnd = 0;
	}

	static F64 calculateValueFromMode( EFrameCounterMode Mode, F64 fElapsed )
	{
		// check the mode
		switch( Mode )
		{
		case EFrameCounterMode::Normal:
			return fElapsed;

		case EFrameCounterMode::Clamp:
			if( fElapsed > 1.0 ) {
				return 1.0;
			}

			return fElapsed;

		case EFrameCounterMode::ClampAtStart:
			if( fElapsed > 1.0 ) {
				return 0.0;
			}

			return fElapsed;

		case EFrameCounterMode::WrapAround:
			return fmod( fElapsed, 1.0 );

		case EFrameCounterMode::LoopOnce:
			if( fElapsed > 2.0 ) {
				return 0.0;
			}

			if( fElapsed > 1.0 ) {
				return fElapsed - 1.0;
			}

			return fElapsed;

		case EFrameCounterMode::Loop:
			{
				const F64 fWrapped = fmod( fElapsed, 2.0 );

				if( fWrapped > 1.0 ) {
					return fWrapped - 1.0;
				}

				return fWrapped;
			}
		}

		// unknown
		return 0.0;
	}

	Void CFrameCounter::update()
	{
		// avoid division by zero and uninitialized counters
		if( muEnd == muStart || !mpfnCalculate ) {
			return;
		}

		// amount of time that has passed in microseconds
		const U64 uElapsed = guCurrentTick - muStart;
		// amount of time that has passed in terms of how far along this counter is
		const F64 fElapsed = F64( uElapsed )/F64( muEnd - muStart );

		// adjusted elapsed time value based on the mode
		const F64 fValue = calculateValueFromMode( mMode, fElapsed );

		// calculate the new value
		const F64 fcalculated = mpfnCalculate( mpParm, fValue, mfCurrent );

		// store the new value
		mfCurrent = mfRange[ 0 ] + fcalculated*( mfRange[ 1 ] - mfRange[ 0 ] );
	}

	Void CFrameCounter::updateAll()
	{
		guCurrentTick = microseconds();

		gListAccessor.lock();
		for( CFrameCounter *pCounter = gFrameCounters.head(); pCounter != nullptr; pCounter = pCounter->mSiblings.next() ) {
			pCounter->update();
		}
		gListAccessor.unlock();
	}

}
