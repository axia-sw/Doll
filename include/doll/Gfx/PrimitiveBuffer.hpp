#pragma once

#include "../Core/Defs.hpp"
#include "Vertex.hpp"

#include <math.h>

namespace doll
{

	enum class PrimitiveMode
	{
		NoTransform,
		Transform
	};

	static const PrimitiveMode kPrimitiveMode = PrimitiveMode::Transform;

	namespace detail
	{

		template< PrimitiveMode _mode_ >
		struct PrimitiveConfig
		{
		};

		template<>
		struct PrimitiveConfig< PrimitiveMode::NoTransform >
		{
			typedef SVertex2D Vertex;
			static const U32 kFormat_Colored  = Vertex::kFmtPos | kVF_DiffuseBit;
			static const U32 kFormat_Textured = Vertex::kFmtPos | kVF_DiffuseBit | kVF_Tex1;

			AX_FORCEINLINE static void set( Vertex *p, F32 x, F32 y ) {
				p->x = x;
				p->y = y;
				p->z = 0.0f;
				p->w = 1.0f;
			}
		};
		template<>
		struct PrimitiveConfig< PrimitiveMode::Transform >
		{
			typedef SVertex2DSprite Vertex;
			static const U32 kFormat_Colored  = Vertex::kFmtPos | kVF_DiffuseBit;
			static const U32 kFormat_Textured = Vertex::kFmtPos | kVF_DiffuseBit | kVF_Tex1;

			AX_FORCEINLINE static void set( Vertex *p, F32 x, F32 y ) {
				p->x = x;
				p->y = y;
				//p->z = 1.0f;
			}
		};

	}

	typedef detail::PrimitiveConfig< kPrimitiveMode > PrimitiveConfig;

	class PrimitiveBuffer
	{
	public:
		typedef PrimitiveConfig::Vertex Vertex;

		enum constants_t {
			MAX_BUFFER = 131072,
			VERTEX_SIZE = sizeof( Vertex ),

			MAX_VERTEX_COUNT = MAX_BUFFER/VERTEX_SIZE,
		};

		inline PrimitiveBuffer()
		: lastColor( 0xFFFFFFFF )
		, lastU( 0 )
		, lastV( 0 )
		, offsetX( 0 )
		, offsetY( 0 )
		, lastTexture( 0 )
		, size( 0 )
		{
			reset();
		}
		inline ~PrimitiveBuffer()
		{
		}

		inline Void reset()
		{
			primType = kTopologyTriangleList;
			vertexFormat = Vertex::kFmtPos | kVF_DiffuseBit;
			lastTexture = 0;
		}

		inline U32 getVertexCount() const
		{
			return size/VERTEX_SIZE;
		}
		inline Bool isBufferFull() const
		{
			return size + VERTEX_SIZE > MAX_BUFFER;
		}
		inline Bool canAddVertices(UPtr count) const
		{
			return size + VERTEX_SIZE*count <= MAX_BUFFER;
		}
		inline U32 getPrimitiveCount( U32 *remains = nullptr ) const
		{
#define SET_REMAINS( n ) if( remains!=NULL ) *remains = n
			unsigned int vertCount = getVertexCount();

			switch( primType )
			{
			case kTopologyPointList:
				SET_REMAINS( 0 );
				return vertCount;

			case kTopologyLineList:
				SET_REMAINS( vertCount%2 );
				return vertCount/2;

			case kTopologyLineStrip:
				SET_REMAINS( 1 );
				if( vertCount < 2 ) {
					return 0;
				}
				return vertCount - 1;

			case kTopologyTriangleList:
				SET_REMAINS( vertCount%3 );
				return vertCount/3;

			case kTopologyTriangleStrip:
				if( vertCount < 3 ) {
					SET_REMAINS( 0 );
					return 0;
				}
				SET_REMAINS( 2 );
				return vertCount - 2;

			case kTopologyTriangleFan:
				SET_REMAINS( 0 );
				if( vertCount < 3 ) {
					return 0;
				}
				return vertCount - 2;
			}

			SET_REMAINS( 0 );

			return 0;
#undef SET_REMAINS
		}

		EResult setPrimitiveType( ETopology pt );
		EResult setVertexFormat( U32 fmt );
		EResult setTexture( UPtr uTexture );
		EResult submit();

		EResult color3f( F32 r, F32 g, F32 b );
		EResult color4f( F32 r, F32 g, F32 b, F32 a );
		EResult color( U32 uColor );
		EResult texcoord2f( F32 u, F32 v );
		EResult vertex2f( F32 x, F32 y );

		EResult addVertices( const Vertex *verts, UPtr numVerts );

		inline void setOffset( F32 x, F32 y )
		{
			offsetX = x;
			offsetY = y;
		}
		inline void getOffset( F32 *x, F32 *y ) const
		{
			if( x != nullptr ) {
				*x = offsetX;
			}
			if( y != nullptr ) {
				*y = offsetY;
			}
		}

		inline EResult vertex2i( int x, int y )
		{
			return vertex2f( floorf( F32( x ) + 0.5f ), floorf( F32( y ) + 0.5f ) );
		}

	private:
		ETopology primType;
		U32       vertexFormat;
		U32       lastColor;
		F32       lastU, lastV;
		F32       offsetX, offsetY;
		UPtr      lastTexture;

		char      buffer[ MAX_BUFFER ];
		U32       size;

		inline Vertex *getCurrentVertex()
		{
			return ( Vertex * )&buffer[ size ];
		}
		inline EResult submitFullBuffer()
		{
			if( !isBufferFull() ) {
				return kSuccess;
			}

			return submit();
		}

		static inline U32 floatColorToDword( F32 r, F32 g, F32 b, F32 a )
		{
			const U32 x = ( U32 )( U8 )( r*255.0f );
			const U32 y = ( U32 )( U8 )( g*255.0f );
			const U32 z = ( U32 )( U8 )( b*255.0f );
			const U32 w = ( U32 )( U8 )( a*255.0f );

			return ( U32 )( w<<24 | x<<16 | y<<8 | z );
		}
	};

}
