#define DOLL_TRACE_FACILITY doll::kLog_UtilMetrics

#include "doll/Util/Metrics.hpp"

#include "doll/OS/Monitor.hpp"

#include "doll/Math/Basic.hpp"

namespace doll
{
	
	template< typename tElement >
	static Void push( TMutArr< tElement > &arr, const tElement &item )
	{
		arr.append( item );
	}
	template< typename tElement >
	static Void pop( TMutArr< tElement > &arr )
	{
		if( arr.isUsed() ) {
			arr.removeLast();
		}
	}
	template< typename tElement >
	static const tElement &top( const TMutArr< tElement > &arr )
	{
		static const tElement dummy = tElement();
		if( arr.isUsed() ) {
			return arr.last();
		}

		return dummy;
	}

	static S32 roundToInt32( F64 fValue )
	{
		const F64 fMultiplier = fValue < 0.0 ? -1.0 : 1.0;
		const S32 iMultiplier = fValue < 0.0 ? -1 : 1;
		const F64 fUnit = fValue*fMultiplier;

		if( fmod( fUnit, 1.0 ) >= 0.5 ) {
			return S32( fUnit + 1.0 )*iMultiplier;
		}

		return S32( fUnit )*iMultiplier;
	}

	MMetrics &MMetrics::get()
	{
		static MMetrics instance;
		return instance;
	}

	MMetrics::MMetrics()
	: m_dotsPerInch()
	, m_sizeStack()
	, m_fontSizeStack()
	{
		queryDotsPerInch();

		pushSize( 100 );
		pushFontSize( 12 );
	}

	Void MMetrics::pushSize( const SIntVector2 &Size )
	{
		push( m_sizeStack, Size );
	}
	Void MMetrics::PopSize()
	{
		pop( m_sizeStack );
	}
	Void MMetrics::pushFontSize( F64 fSize )
	{
		push( m_fontSizeStack, SFontSize( fSize ) );
	}
	Void MMetrics::pushFontSize( F64 fSize, F64 fXSize )
	{
		push( m_fontSizeStack, SFontSize( fSize, fXSize ) );
	}
	Void MMetrics::popFontSize()
	{
		pop( m_fontSizeStack );
	}

	const SIntVector2 &MMetrics::getCurrentSize() const
	{
		return top( m_sizeStack );
	}
	const SFontSize &MMetrics::getCurrentFontSize() const
	{
		return top( m_fontSizeStack );
	}
	F64 MMetrics::getCurrentFontHeight() const
	{
		return top( m_fontSizeStack ).fHeight;
	}
	F64 MMetrics::getCurrentFontXHeight() const
	{
		return top( m_fontSizeStack ).fXHeight;
	}

	S32 MMetrics::percentageToPixels( F64 fUnit, EAxis Axis ) const
	{
		return roundToInt32( fUnit/100.0*( Axis == EAxis::X ? getCurrentSize().x : getCurrentSize().y ) );
	}
	S32 MMetrics::inchesToPixels( F64 fUnit, EAxis Axis ) const
	{
		return roundToInt32( fUnit*( Axis == EAxis::X ? m_dotsPerInch.x : m_dotsPerInch.y ) );
	}
	S32 MMetrics::centimetersToPixels( F64 fUnit, EAxis Axis ) const
	{
		return millimetersToPixels( fUnit*10.0, Axis );
	}
	S32 MMetrics::millimetersToPixels( F64 fUnit, EAxis Axis ) const
	{
		// 25.4 millimeters per inch (exactly)
		return inchesToPixels( fUnit/25.4, Axis );
	}
	S32 MMetrics::emsToPixels( F64 fUnit, EAxis Axis ) const
	{
		return pointsToPixels( fUnit*getCurrentFontHeight(), Axis );
	}
	S32 MMetrics::exsToPixels( F64 fUnit, EAxis Axis ) const
	{
		return pointsToPixels( fUnit*getCurrentFontXHeight(), Axis );
	}
	S32 MMetrics::pointsToPixels( F64 fUnit, EAxis Axis ) const
	{
		return inchesToPixels( fUnit/72.0, Axis );
	}
	S32 MMetrics::picasToPixels( F64 fUnit, EAxis Axis ) const
	{
		return pointsToPixels( fUnit*12.0, Axis );
	}
	S32 MMetrics::measurementToPixels( F64 fUnit, EUnit UnitType, EAxis Axis ) const
	{
		switch( UnitType )
		{
		case EUnit::Percentage:
			return percentageToPixels( fUnit, Axis );
		case EUnit::Inch:
			return inchesToPixels( fUnit, Axis );
		case EUnit::Centimeter:
			return centimetersToPixels( fUnit, Axis );
		case EUnit::Millimeter:
			return millimetersToPixels( fUnit, Axis );
		case EUnit::Em:
			return emsToPixels( fUnit, Axis );
		case EUnit::Ex:
			return exsToPixels( fUnit, Axis );
		case EUnit::Point:
			return pointsToPixels( fUnit, Axis );
		case EUnit::Pica:
			return picasToPixels( fUnit, Axis );
		case EUnit::Pixel:
			break;
		}

		return roundToInt32( fUnit );
	}
	S32 MMetrics::textToPixels( Str units, EAxis Axis ) const
	{
		F64 fMultiplier = 1.0;

		const char *const e = units.getEnd();
		const char *s = units.get();
		while( s < e && *s <= ' ' ) {
			++s;
		}

		if( s < e && *s == '-' ) {
			++s;
			fMultiplier = -1.0f;
		}

		const char *p = s;
		while( p < e && *p >= '0' && *p <= '9' ) {
			++p;
		}
		if( p < e && *p == '.' ) {
			++p;
			while( p < e && *p >= '0' && *p <= '9' ) {
				++p;
			}
		}
		if( p < e && *p == 'e' ) {
			if( *( p + 1 ) == '-' || *( p + 1 ) == '+' ) {
				p += 2;
				while( p < e && *p >= '0' && *p <= '9' ) {
					++p;
				}
			}
		}
		if( s == p ) {
			return 0;
		}

		char szBuf[ 128 ];
		axstr_cpyn( szBuf, s, ( axstr_size_t )( p - s ) );

		const F64 fUnit = atof( szBuf )*fMultiplier;

		if( strncmp( p, "%", 1 ) == 0 ) {
			return percentageToPixels( fUnit, Axis );
		}
		if( strncmp( p, "in", 2 ) == 0 ) {
			return inchesToPixels( fUnit, Axis );
		}
		if( strncmp( p, "cm", 2 ) == 0 ) {
			return centimetersToPixels( fUnit, Axis );
		}
		if( strncmp( p, "mm", 2 ) == 0 ) {
			return millimetersToPixels( fUnit, Axis );
		}
		if( strncmp( p, "em", 2 ) == 0 ) {
			return emsToPixels( fUnit, Axis );
		}
		if( strncmp( p, "ex", 2 ) == 0 ) {
			return exsToPixels( fUnit, Axis );
		}
		if( strncmp( p, "pt", 2 ) == 0 ) {
			return pointsToPixels( fUnit, Axis );
		}
		if( strncmp( p, "pc", 2 ) == 0 ) {
			return picasToPixels( fUnit, Axis );
		}

		return roundToInt32( fUnit );
	}

	const SIntVector2 &MMetrics::getDotsPerInch() const
	{
		return m_dotsPerInch;
	}

	Void MMetrics::queryDotsPerInch()
	{
		m_dotsPerInch = os_getPrimaryMonitorInfo().dotsPerInch;
	}

	//--------------------------------------------------------------------//

	DOLL_FUNC S32 DOLL_API util_getDpiX()
	{
		return g_metrics->getDotsPerInch().x;
	}
	DOLL_FUNC S32 DOLL_API util_getDpiY()
	{
		return g_metrics->getDotsPerInch().y;
	}
	DOLL_FUNC S32 DOLL_API util_getDpi( S32 *pOutX, S32 *pOutY )
	{
		if( !AX_VERIFY_NOT_NULL( pOutX ) || !AX_VERIFY_NOT_NULL( pOutY ) ) {
			return 0;
		}

		const auto &dpi = g_metrics->getDotsPerInch();

		*pOutX = dpi.x;
		*pOutY = dpi.y;

		return dpi.x;
	}

	DOLL_FUNC S32 DOLL_API util_unitsToPixels( Str input, S32 axis )
	{
		if( !AX_VERIFY( input.isUsed() ) || !AX_VERIFY_MSG( axis >= 0 && axis <= 1, "Invalid axis" ) ) {
			return 0;
		}

		return g_metrics->textToPixels( input, ( axis == 0 ? EAxis::X : EAxis::Y ) );
	}
	DOLL_FUNC S32 DOLL_API util_unitsToPixelsX( Str input )
	{
		if( !AX_VERIFY( input.isUsed() ) ) {
			return 0;
		}

		return g_metrics->textToPixels( input, EAxis::X );
	}
	DOLL_FUNC S32 DOLL_API util_unitsToPixelsY( Str input )
	{
		if( !AX_VERIFY( input.isUsed() ) ) {
			return 0;
		}

		return g_metrics->textToPixels( input, EAxis::Y );
	}

	DOLL_FUNC Void DOLL_API util_pushPercentageSize( S32 resX, S32 resY )
	{
		g_metrics->pushSize( SIntVector2( resX, resY ) );
	}
	DOLL_FUNC Void DOLL_API util_popPercentageSize()
	{
		g_metrics->PopSize();
	}
	DOLL_FUNC S32 DOLL_API util_getPercentageSize( S32 *pOutResX, S32 *pOutResY )
	{
		if( !AX_VERIFY_NOT_NULL( pOutResX ) || !AX_VERIFY_NOT_NULL( pOutResY ) ) {
			return 0;
		}

		const SIntVector2 &Res = g_metrics->getCurrentSize();

		*pOutResX = Res.x;
		*pOutResY = Res.y;

		return 1;
	}
	DOLL_FUNC S32 DOLL_API util_getPercentageSizeX()
	{
		return g_metrics->getCurrentSize().x;
	}
	DOLL_FUNC S32 DOLL_API util_getPercentageSizeY()
	{
		return g_metrics->getCurrentSize().y;
	}

	DOLL_FUNC Void DOLL_API util_pushFontSize( F64 fHeight, F64 fXHeight )
	{
		if( fXHeight*fXHeight < 0.000001 ) {
			fXHeight = fHeight/2.0;
		}

		g_metrics->pushFontSize( fHeight, fXHeight );
	}
	DOLL_FUNC Void DOLL_API util_popFontSize()
	{
		g_metrics->popFontSize();
	}
	DOLL_FUNC F64 DOLL_API util_getFontSize()
	{
		return g_metrics->getCurrentFontHeight();
	}
	DOLL_FUNC F64 DOLL_API util_getFontXSize()
	{
		return g_metrics->getCurrentFontXHeight();
	}

}
