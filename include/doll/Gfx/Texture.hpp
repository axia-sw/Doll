#pragma once

#include "../Core/Defs.hpp"
#include "../Core/Memory.hpp"
#include "../Core/MemoryTags.hpp"
#include "API.hpp"

namespace doll
{

#ifndef DOLL_TEXTURE_MEMORY_ENABLED
# define DOLL_TEXTURE_MEMORY_ENABLED 0
#endif

	class RTexture;
	class CTextureAtlas;
	class MTextures;

	struct STextureRect
	{
		F32 pos[ 2 ];
		F32 res[ 2 ];
	};
	struct SPixelVec2
	{
		U16 x;
		U16 y;
	};
	struct SPixelRect
	{
		SPixelVec2 off;
		SPixelVec2 res;
	};

	inline U32 gfx_getTexelByteSize( ETextureFormat format )
	{
		switch( format ) {
		case kTexFmtRGBA8:
			return 4;

		case kTexFmtRGB8:
			return 3;
		}

		return 0;
	}

	class CRectangleAllocator
	{
	public:
		CRectangleAllocator();
		~CRectangleAllocator();

		Bool init( const SPixelVec2 &res );
		Void fini();

		Void *allocateId( SPixelRect &rect, U16 textureId, const SPixelVec2 &res );
		Void freeId( Void *p );

	private:
		struct SNode
		{
			U16        textureId;    //0=free space, 1+=texture ID
			SPixelRect rect;         //the space this rectangle occupies
			SNode      *prev, *next; //links
		};
		SNode *    head, *tail;
		SPixelVec2 resolution;

		// this will allocate a blank node by default
		SNode *allocateNode( U16 offX, U16 offY, U16 resX, U16 resY );
		Void freeNode( SNode *node );

		SNode *findBestFitNode( U16 resX, U16 resY ) const;
		Bool splitNode( SNode *node, U16 textureId, U16 resX, U16 resY );

		UPtr findAdjacentFreeNodes( SNode **out_nodes, UPtr maxOutNodes, SNode *node ) const;
		Void mergeAdjacentFreeNodes( SNode *node );
	};

	class CTextureAtlas
	{
	friend class MTextures;
	friend class RTexture;
	public:
		~CTextureAtlas();

		RTexture *reserveTexture( U16 width, U16 height );
		inline const SPixelVec2 &getResolution() const
		{
			return resolution;
		}
		inline UPtr getBackingTexture() const
		{
			return texture;
		}
		inline ETextureFormat getFormat() const
		{
			return format;
		}
		inline UPtr getBytesPerPixel() const
		{
			return UPtr( gfx_getTexelByteSize( format ) );
		}

	protected:
		TIntrLink< CTextureAtlas > mgrAtlas_link;

		CTextureAtlas();

		Bool init( ETextureFormat fmt, U16 resX, U16 resY );
		Void fini();

#if DOLL_TEXTURE_MEMORY_ENABLED
		Bool updateTextures( UPtr numTextures, const RTexture *const *textures );
#else
		Bool updateTexture( const RTexture *texture, ETextureFormat format, const Void *pData );
#endif

		inline Void freeNode( Void *node )
		{
			allocator.freeId( node );
		}

	private:
		UPtr                texture;
		SPixelVec2          resolution;
		ETextureFormat      format;

		CRectangleAllocator allocator;

		TIntrList<RTexture> atlas_list;
	};

	class RTexture: public TPoolObject< RTexture, kTag_Texture >
	{
	friend class CTextureAtlas;
	friend class MTextures;
	public:
		~RTexture();

		Void translateCoordinates( STextureRect &dst, const SPixelRect &src ) const;

		Bool setName( Str newName );
		Str getName() const;

		inline Bool isValid() const
		{
			return atlas!=nullptr;
		}
		Void fini();

		inline const SPixelRect &getAtlasRectangle() const
		{
			return texRect;
		}
		inline size_t getByteCount() const
		{
			if( !atlas ) {
				return 0;
			}

			return texRect.res.x*texRect.res.y*atlas->getBytesPerPixel();
		}

#if DOLL_TEXTURE_MEMORY_ENABLED
		inline Bool copyMemory( const Void *src, size_t len )
		{
			size_t actualLen = getByteCount();

			if( !AX_VERIFY_MSG( len==actualLen, "User length does not match" ) ) {
				return false;
			}

			if( !memory ) {
				memory = ( Void * )new U8[ actualLen ];
				if( !AX_VERIFY_MEMORY( memory ) ) {
					return false;
				}
			}

			memcpy( memory, src, actualLen );
			return true;
		}
		inline Void *getMemoryPointer()
		{
			return memory;
		}
		inline const Void *getMemoryPointer_const() const
		{
			return memory;
		}
		inline const Void *getMemoryPointer() const
		{
			return memory;
		}
#endif

		inline const SPixelVec2 &getResolution() const
		{
			return texRect.res;
		}
		inline UPtr getBackingTexture() const
		{
			if( !atlas ) {
				return 0;
			}

			return atlas->getBackingTexture();
		}

		inline U16 getIdentifier() const
		{
			return ident;
		}

		inline F32 getUnitResX() const
		{
			return 1.0f/( F32 )atlas->getResolution().x;
		}
		inline F32 getUnitResY() const
		{
			return 1.0f/( F32 )atlas->getResolution().y;
		}

	protected:
		RTexture( CTextureAtlas *mainAtlas, const SPixelRect &rect, Void *allocNode, U16 textureId );

		TIntrLink<RTexture> mgrTex_link;
		TIntrLink<RTexture> atlas_link;

	private:
		CTextureAtlas *atlas;
		SPixelRect     texRect;
		Void *         allocNode;
		U16            ident;

#if DOLL_TEXTURE_MEMORY_ENABLED
		Void *         memory;
#endif

		MutStr         name;
	};

	class MTextures
	{
	friend class CTextureAtlas;
	friend class RTexture;
	public:
		static const UPtr MAX_TEXTURES = 65536;

		static MTextures instance;

		RTexture *makeTexture( U16 width, U16 height, const Void *data, ETextureFormat format = kTexFmtRGBA8, CTextureAtlas *specificAtlas = nullptr );
		RTexture *loadTexture( Str filename, CTextureAtlas *specificAtlas = nullptr );

		CTextureAtlas *allocateAtlas( ETextureFormat fmt, U16 resX, U16 resY );
		RTexture *getTextureById( U16 textureId ) const;

		U16 getAllocatedTexturesCount() const;

		inline U16 getDefaultAtlasResolution() const
		{
			return defAtlasRes;
		}

	protected:
		U16 procureTextureId();
		Void setHandleForTextureId( U16 textureId, RTexture *handle );
		Void nullifyTextureId( U16 textureId );

	private:
		MTextures();
		~MTextures();

		Void pushFreeTextureId( U16 textureId );
		U16 popFreeTextureId();

		RTexture *               textures[ MAX_TEXTURES ];    //0 is set to nullptr, always
		U16                      freeIds[ MAX_TEXTURES - 1 ]; //0 is always nullptr, sub 1
		U16                      freeIds_sp;                  //stack pointer

		U16                      defAtlasRes;

		TIntrList<CTextureAtlas> mgrAtlas_list;
		TIntrList<RTexture>      mgrTex_list;
	};
	extern MTextures &g_textureMgr;

	DOLL_FUNC Bool DOLL_API gfx_isTextureResolutionValid( U16 resX, U16 resY );

	DOLL_FUNC CTextureAtlas *DOLL_API gfx_newTextureAtlas( U16 resX, U16 resY, U16 format );
	DOLL_FUNC CTextureAtlas *DOLL_API gfx_deleteTextureAtlas( CTextureAtlas *atlas );

	DOLL_FUNC RTexture *DOLL_API gfx_newTexture( U16 width, U16 height, const void *data, ETextureFormat format );
	DOLL_FUNC RTexture *DOLL_API gfx_newTextureInAtlas( U16 width, U16 height, const void *data, ETextureFormat format, CTextureAtlas *atlas );

	DOLL_FUNC RTexture *DOLL_API gfx_loadTexture( Str filename );
	DOLL_FUNC RTexture *DOLL_API gfx_loadTextureInAtlas( Str filename, CTextureAtlas *atlas );

	DOLL_FUNC RTexture *DOLL_API gfx_deleteTexture( RTexture *textureId );
	DOLL_FUNC U32 DOLL_API gfx_getTextureResX( const RTexture *textureId );
	DOLL_FUNC U32 DOLL_API gfx_getTextureResY( const RTexture *textureId );

}
