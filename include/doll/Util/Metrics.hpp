#pragma once

#include "../Core/Defs.hpp"
#include "../Types/IntVector2.hpp"
#include "../Types/Rect.hpp"

namespace doll
{

	struct SFontSize
	{
		F64 fHeight;
		F64 fXHeight;

		inline SFontSize()
		: fHeight( 0.0 )
		, fXHeight( 0.0 )
		{
		}
		inline SFontSize( F64 fHeight )
		: fHeight( fHeight )
		, fXHeight( fHeight/2.0 )
		{
		}
		inline SFontSize( F64 fHeight, F64 fXHeight )
		: fHeight( fHeight )
		, fXHeight( fXHeight )
		{
		}
	};

	enum class EUnit
	{
		// [px] Dot on the monitor
		Pixel,
		// [%] Percentage of the currently pushed size
		Percentage,
		// [in] Inch
		Inch,
		// [cm] Centimeter
		Centimeter,
		// [mm] Millimeter
		Millimeter,
		// [em] Multiple of the current font height
		Em,
		// [ex] Multiple of the current font x-height (usually around height/2)
		Ex,
		// [pt] 1/72 of an inch
		Point,
		// [pc] 12 points
		Pica
	};
	enum class EAxis
	{
		X,
		Y
	};

	class MMetrics
	{
	public:
		static MMetrics &get();

		// Size in pixels
		Void pushSize( const SIntVector2 &Size );
		Void PopSize();

		// Font height in points
		Void pushFontSize( F64 fSize );
		// Font height and x-height (usually height/2) in points
		Void pushFontSize( F64 fSize, F64 fXSize );
		Void popFontSize();

		// Retrieve the current size in pixels
		const SIntVector2 &getCurrentSize() const;
		// Retrieve the current font size
		const SFontSize &getCurrentFontSize() const;
		// Retrieve the current font height in points
		F64 getCurrentFontHeight() const;
		// Retrieve the current font x-height (usually height/2) in points
		F64 getCurrentFontXHeight() const;

		S32 percentageToPixels( F64 fUnit, EAxis Axis = EAxis::X ) const;
		S32 inchesToPixels( F64 fUnit, EAxis Axis = EAxis::X ) const;
		S32 centimetersToPixels( F64 fUnit, EAxis Axis = EAxis::X ) const;
		S32 millimetersToPixels( F64 fUnit, EAxis Axis = EAxis::X ) const;
		S32 emsToPixels( F64 fUnit, EAxis Axis = EAxis::X ) const;
		S32 exsToPixels( F64 fUnit, EAxis Axis = EAxis::X ) const;
		S32 pointsToPixels( F64 fUnit, EAxis Axis = EAxis::X ) const;
		S32 picasToPixels( F64 fUnit, EAxis Axis = EAxis::X ) const;
		S32 measurementToPixels( F64 fUnit, EUnit UnitType, EAxis Axis = EAxis::X ) const;
		S32 textToPixels( Str units, EAxis Axis = EAxis::X ) const;

		const SIntVector2 &getDotsPerInch() const;

	private:
		MMetrics();

		SIntVector2            m_dotsPerInch;
		TMutArr< SIntVector2 > m_sizeStack;
		TMutArr< SFontSize >   m_fontSizeStack;

		Void queryDotsPerInch();
	};
#ifdef DOLL__BUILD
	static ax::TManager< MMetrics >	g_metrics;
#endif

	//--------------------------------------------------------------------//

	DOLL_FUNC S32 DOLL_API util_getDpiX();
	DOLL_FUNC S32 DOLL_API util_getDpiY();
	DOLL_FUNC S32 DOLL_API util_getDpi( S32 *pOutX, S32 *pOutY );

	DOLL_FUNC S32 DOLL_API util_unitsToPixels( Str input, S32 axis );
	DOLL_FUNC S32 DOLL_API util_unitsToPixelsX( Str input );
	DOLL_FUNC S32 DOLL_API util_unitsToPixelsY( Str input );

	DOLL_FUNC Void DOLL_API util_pushPercentageSize( S32 resX, S32 resY );
	DOLL_FUNC Void DOLL_API util_popPercentageSize();
	DOLL_FUNC S32 DOLL_API util_getPercentageSize( S32 *pOutResX, S32 *pOutResY );
	DOLL_FUNC S32 DOLL_API util_getPercentageSizeX();
	DOLL_FUNC S32 DOLL_API util_getPercentageSizeY();

	DOLL_FUNC Void DOLL_API util_pushFontSize( F64 fHeight, F64 fXHeight );
	DOLL_FUNC Void DOLL_API util_popFontSize();
	DOLL_FUNC F64 DOLL_API util_getFontSize();
	DOLL_FUNC F64 DOLL_API util_getFontXSize();

}
