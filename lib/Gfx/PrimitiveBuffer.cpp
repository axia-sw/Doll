#define DOLL_TRACE_FACILITY doll::kLog_GfxPrimitiveBuffer
#include "../BuildSettings.hpp"

#include "doll/Gfx/PrimitiveBuffer.hpp"
#include "doll/Gfx/API-GL.hpp"

#include "doll/Core/Memory.hpp"
#include "doll/Core/Logger.hpp"

//#include <gl/GL.h>

namespace doll
{

	class CVertexLayoutCache
	{
	public:
		CVertexLayoutCache()
		: m_prioCache()
		, m_cCached( 0 )
		{
		}
		~CVertexLayoutCache()
		{
		}

		UPtr getLayout( U32 uFlags )
		{
			for( U32 i = 0; i < m_cCached; ++i ) {
				if( m_cached[ i ].uFlags == uFlags ) {
					m_cached[ i ].link.toFront();
					return m_cached[ i ].vertexLayout;
				}
			}

			if( m_cCached == sizeof( m_cached )/sizeof( m_cached[0] ) ) {
				SCacheItem *const p = m_prioCache.tail();
				AX_ASSERT_NOT_NULL( p );

				gfx_r_destroyLayout( p->vertexLayout );
				return fillLayout( *p, uFlags );
			}
			
			return fillLayout( m_cached[ m_cCached++ ], uFlags );
		}

	private:
		struct SCacheItem
		{
			UPtr vertexLayout;
			U32  uFlags;

			TIntrLink<SCacheItem> link;
		};

		TIntrList<SCacheItem> m_prioCache;
		SCacheItem m_cached[ 16 ];
		UPtr       m_cCached;

		UPtr fillLayout( SCacheItem &x, U32 uFlags )
		{
			if( !( x.vertexLayout = gfx_r_createLayout( sizeof( PrimitiveConfig::Vertex ) ) ) ) {
				return 0;
			}

			UPtr offset = 0;
			switch( uFlags & kVFMask_Pos ) {
			case kVF_XY:
				gfx_r_layoutVertex( x.vertexLayout, kVectorSize2 );
				offset = 2*4;
				break;
			case kVF_XYZ:
				gfx_r_layoutVertex( x.vertexLayout, kVectorSize3 );
				offset = 3*4;
				break;
			case kVF_XYZW:
				gfx_r_layoutVertex( x.vertexLayout, kVectorSize4 );
				offset = 4*4;
				break;
			}

			if( uFlags & kVF_DiffuseBit ) {
				gfx_r_layoutColor( x.vertexLayout, kVectorSize4, kVectorTypeU8 );
			}
			offset += 4;

			const U32 cTextures = ( uFlags & kVFMask_Tex )>>kVFShft_Tex;
			for( U32 i = 0; i < cTextures; ++i ) {
				gfx_r_layoutTexCoord( x.vertexLayout, kVectorSize2, kVectorTypeF32, offset );
				offset += 2*4;
			}

			// ### FIXME ### Check return value of gfx_r_finishLayout()
			gfx_r_finishLayout( x.vertexLayout );

			x.uFlags = uFlags;
			x.link.setNode( &x );
			x.link.unlink();
			m_prioCache.addHead( x.link );

			return x.vertexLayout;
		}
	};

	static UPtr getVertexLayout( U32 uFlags )
	{
		static CVertexLayoutCache cache;
		return cache.getLayout( uFlags );
	}

	EResult PrimitiveBuffer::setPrimitiveType( ETopology pt )
	{
		if( primType!=pt ) {
			submit();
		}

		primType = pt;
		return kSuccess;
	}
	EResult PrimitiveBuffer::setVertexFormat( U32 fmt )
	{
		if( vertexFormat==fmt ) {
			return kSuccess;
		}

		EResult res = submit();
		if( !AX_VERIFY_MSG( res==kSuccess, "Failed to submit" ) ) {
			return res;
		}

		vertexFormat = fmt;
		return kSuccess;
	}
	EResult PrimitiveBuffer::setTexture( UPtr uTexture )
	{
		if( lastTexture == uTexture ) {
			return kSuccess;
		}

		const EResult ret = submit();
		if( !AX_VERIFY_MSG( ret == kSuccess, "Failed to submit" ) ) {
			return ret;
		}

		if( uTexture != 0 ) {
			gfx_r_enableTexture2D();
		} else {
			gfx_r_disableTexture2D();
		}
		gfx_r_setTexture( uTexture );

		lastTexture = uTexture;
		return kSuccess;
	}

	EResult PrimitiveBuffer::submit()
	{
		U32 remains = 0;
		const U32 primCount = getPrimitiveCount( &remains );
		if( !primCount ) {
			return kSuccess;
		}

		gfx_r_setLayout( getVertexLayout( vertexFormat ) );
		gfx_r_drawMem( primType, getVertexCount(), sizeof( PrimitiveConfig::Vertex ), &buffer[0] );

		U32 remainingSize = remains*VERTEX_SIZE;
		if( remains>0 ) {
#if DOLL__SECURE_LIB
			memmove_s
			(
				buffer,
				MAX_BUFFER,
				( ( const char * )buffer ) + size - remainingSize,
				remainingSize
			);
#else
			memmove
			(
				buffer,
				( ( const char * )buffer ) + size - remainingSize,
				remainingSize
			);
#endif
		}

		size = remainingSize;
		return kSuccess;
	}

#define DO_SUBMIT() \
		EResult ret__ = submitFullBuffer();\
		if( ret__!=kSuccess )\
			return ret__

	EResult PrimitiveBuffer::color3f( F32 r, F32 g, F32 b )
	{
		if( !AX_VERIFY_MSG( !isBufferFull(), "Too many vertices" ) ) {
			return kError_Overflow;
		}

		lastColor = floatColorToDword(r, g, b, 1.0f);

		vertexFormat |= kVF_DiffuseBit;
		getCurrentVertex()->diffuse = lastColor;

		return kSuccess;
	}
	EResult PrimitiveBuffer::color4f( F32 r, F32 g, F32 b, F32 a )
	{
		if( !AX_VERIFY_MSG( !isBufferFull(), "Too many vertices" ) ) {
			return kError_Overflow;
		}

		lastColor = floatColorToDword(r, g, b, a);

		vertexFormat |= kVF_DiffuseBit;
		getCurrentVertex()->diffuse = lastColor;

		return kSuccess;
	}
	EResult PrimitiveBuffer::color( U32 uColor )
	{
		if( !AX_VERIFY_MSG( !isBufferFull(), "Too many vertices" ) ) {
			return kError_Overflow;
		}

		lastColor = uColor;

		vertexFormat |= kVF_DiffuseBit;
		getCurrentVertex()->diffuse = lastColor;

		return kSuccess;
	}
	EResult PrimitiveBuffer::texcoord2f(F32 u, F32 v)
	{
		if( !AX_VERIFY_MSG( !isBufferFull(), "Too many vertices" ) ) {
			return kError_Overflow;
		}

		//v = 1.0f - v;

		lastU = u;
		lastV = v;

		vertexFormat |= kVF_Tex1;
		getCurrentVertex()->u = u;
		getCurrentVertex()->v = v;

		return kSuccess;
	}
	EResult PrimitiveBuffer::vertex2f( F32 x, F32 y )
	{
		DO_SUBMIT();

		PrimitiveConfig::set( getCurrentVertex(), x + offsetX, y + offsetY );

		size += VERTEX_SIZE;

		getCurrentVertex()->diffuse = lastColor;
		getCurrentVertex()->u = lastU;
		getCurrentVertex()->v = lastV;

		return kSuccess;
	}

	EResult PrimitiveBuffer::addVertices( const Vertex *verts, UPtr numVerts )
	{
		AX_ASSERT_MSG( verts!=NULL, "Vertex pointer must be provided" );
		AX_ASSERT_MSG( numVerts > 0, "Vertices must be provided" );

		if( !AX_VERIFY_MSG( canAddVertices( numVerts ), "Full" ) ) {
			return kError_Overflow;
		}

		const U32 transferSize = U32(VERTEX_SIZE*numVerts);
		memcpy( &buffer[ size ], ( const void * )verts, transferSize );
		size += transferSize;

		return kSuccess;
	}

}
