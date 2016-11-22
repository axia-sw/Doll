#pragma once

#include "../Core/Defs.hpp"
#include "IntVector2.hpp"

namespace doll {

	struct SRect;
	struct SBox;
	
	/*!
	 *	\brief Rectangle
	 *
	 *	Represents an area from (x1,y1) to (x2 - 1,y2 - 1) inclusively. 
	 */
	struct SRect
	{
		S32 x1;
		S32 y1;
		S32 x2;
		S32 y2;

		inline SRect()
		: x1( 0 )
		, y1( 0 )
		, x2( 0 )
		, y2( 0 )
		{
		}
		inline SRect( S32 x1, S32 y1, S32 x2, S32 y2 )
		: x1( x1 )
		, y1( y1 )
		, x2( x2 )
		, y2( y2 )
		{
		}
		inline SRect( const SBox & );
		inline SRect( const SIntVector2 &topLeft, const SIntVector2 &bottomRight )
		: x1( topLeft.x )
		, y1( topLeft.y )
		, x2( bottomRight.x )
		, y2( bottomRight.y )
		{
		}
		inline SRect( const SRect &Other )
		: x1( Other.x1 )
		, y1( Other.y1 )
		, x2( Other.x2 )
		, y2( Other.y2 )
		{
		}

		/*! Puts the members in the correct order */
		inline SRect &fixMe()
		{
			if( x2 < x1 ) { const S32 x3 = x1; x1 = x2; x2 = x3; }
			if( y2 < y1 ) { const S32 y3 = y1; y1 = y2; y2 = y3; }

			return *this;
		}
		/*! Duplicates this rectangle but with the members in the correct order */
		inline SRect fixed() const
		{
			return SRect( *this ).fixMe();
		}

		/*! Increases this rectangle's area by combining with another rectangle */
		inline SRect &combineWith( const SRect &other )
		{
			if( x1 > other.x1 ) { x1 = other.x1; }
			if( y1 > other.y1 ) { y1 = other.y1; }
			if( x2 < other.x2 ) { x2 = other.x2; }
			if( y2 < other.y2 ) { y2 = other.y2; }

			return *this;
		}
		/*! Creates a rectangle that tightly fits the space of this rectangle and the other rectangle */
		inline SRect combined( const SRect &other ) const
		{
			return SRect( *this ).combineWith( other );
		}

		/*! Restrict this rectangle by a containing rectangle */
		inline SRect &restrictBy( const SRect &other )
		{
			if( x1 < other.x1 ) { x1 = other.x1; }
			if( y1 < other.y1 ) { y1 = other.y1; }
			if( x2 > other.x2 ) { x2 = other.x2; }
			if( y2 > other.y2 ) { y2 = other.y2; }

			return *this;
		}
		/*! Creates a rectangle that matches the current rectangle restricted by another */
		inline SRect restricted( const SRect &other ) const
		{
			return SRect( *this ).restrictBy( other );
		}

		/*! Center this rectangle within another rectangle (retaining the same size) */
		inline SRect &centerMe( const SRect &within )
		{
			const S32 w1 = x2 - x1;
			const S32 h1 = y2 - y1;

			const S32 w2 = within.x2 - within.x1;
			const S32 h2 = within.y2 - within.y1;

			x1 = within.x1 + w2/2 - w1/2;
			y1 = within.y1 + h2/2 - h1/2;
			x2 = x1 + w1;
			y2 = y1 + h1;

			return *this;
		}
		/*! Center this rectangle on an arbitrary point */
		inline SRect &centerMe( const SIntVector2 &at )
		{
			return positionMe( at - size()/2 );
		}
		/*! Creates a rectangle that matches the current rectangle centered within another */
		inline SRect centered( const SRect &within ) const
		{
			return SRect( *this ).centerMe( within );
		}
		/*! Creates a rectangle that matches the current rectangle centered on a point */
		inline SRect centered( const SIntVector2 &at ) const
		{
			return SRect( *this ).centerMe( at );
		}

		/*! Retrieve the top-left point as a vector */
		inline SIntVector2 topLeft() const
		{
			return SIntVector2( x1, y1 );
		}
		/*! Retrieve the top-right point as a vector */
		inline SIntVector2 topRight() const
		{
			return SIntVector2( x2, y1 );
		}
		/*! Retrieve the bottom-right point as a vector */
		inline SIntVector2 bottomRight() const
		{
			return SIntVector2( x2, y2 );
		}
		/* Retrieve the bottom-left point as a vector */
		inline SIntVector2 bottomLeft() const
		{
			return SIntVector2( x1, y2 );
		}
		/*! Retrieve the center of this rectangle as a vector */
		inline SIntVector2 center() const
		{
			return SIntVector2( x1 + ( x2 - x1 ), y1 + ( y2 - y1 ) );
		}
		/*! Retrieve the position of this rectangle as a vector (top left) */
		inline SIntVector2 origin() const
		{
			return SIntVector2( x1, y1 );
		}
		/*! Retrieve the size of this rectangle as a vector */
		inline SIntVector2 size() const
		{
			return SIntVector2( x2 - x1, y2 - y1 );
		}
		/*! Determine whether a given point is inside this rectangle */
		inline bool contains( const SIntVector2 &v ) const
		{
			return x1 <= v.x && y1 <= v.y && x2 >= v.x && y2 >= v.y;
		}
		/*! Determine whether a given rectangle is completely contained within this rectangle */
		inline bool contains( const SRect &r ) const
		{
			return contains( r.topLeft() ) && contains( r.topRight() ) && contains( r.bottomRight() ) && contains( r.bottomLeft() );
		}
		/*! Determine whether a given rectangle intersects with this rectangle */
		inline bool intersects( const SRect &r ) const
		{
			return contains( r.topLeft() ) || contains( r.topRight() ) || contains( r.bottomRight() ) || contains( r.bottomLeft() );
		}
		/*! Convert a vector to one relative to this rectangle */
		inline SIntVector2 relative( const SIntVector2 &v ) const
		{
			return v - topLeft();
		}
		/*! Convert a rectangle to one relative to this rectangle */
		inline SRect relative( const SRect &r ) const
		{
			SRect temp;

			temp.x1 = r.x1 - x1;
			temp.y1 = r.y1 - y1;
			temp.x2 = temp.x1 + r.resX();
			temp.y2 = temp.y1 + r.resY();

			return temp;
		}
		/*! Move this rectangle by the given vector */
		inline SRect &moveMe( const SIntVector2 &distance )
		{
			x1 += distance.x;
			y1 += distance.y;
			x2 += distance.x;
			y2 += distance.y;
			return *this;
		}
		/*! Duplicate this rectangle and move it by the given vector */
		inline SRect moved( const SIntVector2 &distance ) const
		{
			return SRect( *this ).moveMe( distance );
		}

		/*! Resize this rectangle */
		inline SRect &resizeMe( const SIntVector2 &size )
		{
			x2 = x1 + size.x;
			y2 = y1 + size.y;
			return *this;
		}
		/*! Duplicate this rectangle and resize it by the given vector */
		inline SRect resized( const SIntVector2 &size ) const
		{
			return SRect( *this ).resizeMe( size );
		}

		/*! Position this rectangle */
		inline SRect &positionMe( const SIntVector2 &pos )
		{
			x2 = pos.x + resX();
			y2 = pos.y + resY();
			x1 = pos.x;
			y1 = pos.y;

			return *this;
		}
		/*! Duplicate this rectangle and position it at the given vector */
		inline SRect positioned( const SIntVector2 &pos ) const
		{
			return SRect( *this ).positionMe( pos );
		}

		/*! Retrieve the width of this rectangle. Same as resX(). */
		inline S32 width() const { return x2 - x1; }
		/*! Retrieve the height of this rectangle. Same as resY(). */
		inline S32 height() const { return y2 - y1; }

		/*! Retrieve the x-resolution of this rectangle. Same as width(). */
		inline S32 resX() const { return x2 - x1; }
		/*! Retrieve the y-resolution of this rectangle. Same as height(). */
		inline S32 resY() const { return y2 - y1; }

		/*! Retrieve the x-origin of this rectangle. */
		inline S32 posX() const { return x1; }
		/*! Retrieve the y-origin of this rectangle. */
		inline S32 posY() const { return y1; }

		inline bool operator==( const SRect &Other ) const
		{
			return x1 == Other.x1 && y1 == Other.y1 && x2 == Other.x2 && y2 == Other.y2;
		}
		inline bool operator!=( const SRect &Other ) const
		{
			return x1 != Other.x1 || y1 != Other.y1 || x2 != Other.x2 || y2 != Other.y2;
		}

		inline operator SBox() const;
		inline operator SBox &()
		{
			return reinterpret_cast< SBox & >( *this );
		}
		inline operator const SBox &() const
		{
			return reinterpret_cast< const SBox & >( *this );
		}
	};

	inline SIntVector2 &SIntVector2::clampMe( const SRect &bounds )
	{
		return clampMe( bounds.topLeft(), bounds.bottomRight() );
	}

	/*!
	 *	\brief Box. (Same as rectangle, but uses a different constructor.)
	 *
	 *	Represents an area from (x1,y1) to (x2 - 1,y2 - 1) inclusively.
	 */
	struct SBox: public SRect
	{
		inline SBox()
		: SRect()
		{
		}
		inline SBox( S32 x, S32 y, S32 w, S32 h )
		: SRect( x, y, x + w, y + h )
		{
		}
		inline SBox( const SRect &r )
		: SRect( r )
		{
		}
		inline SBox( const SBox &r )
		: SRect( r.x1, r.y1, r.x2, r.y2 )
		{
		}
		inline SBox( const SIntVector2 &origin, const SIntVector2 &size )
		: SRect( origin.x, origin.y, origin.x + size.x, origin.y + size.y )
		{
		}
	};

	inline SRect::SRect( const SBox &b )
	: x1( b.x1 )
	, y1( b.y1 )
	, x2( b.x2 )
	, y2( b.y2 )
	{
	}
	inline SRect::operator SBox() const
	{
		return SBox( *this );
	}

}
