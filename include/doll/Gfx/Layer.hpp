#pragma once

#include "../Core/Defs.hpp"
#include "../Core/Memory.hpp"
#include "../Core/MemoryTags.hpp"

#include "../Types/IntVector2.hpp"
#include "../Types/Rect.hpp"

#include "PrimitiveBuffer.hpp"
#include "LayerEffect.hpp"

namespace doll
{

	class RSpriteGroup;
	
	class RLayer;
	class MLayers;

	class ILayerEffect;

	typedef TIntrList< RLayer > LayerList;
	typedef TIntrLink< RLayer > LayerLink;

	typedef ax::TList< ILayerEffect * > LayerEffectList;

	class CGfxFrame;
	
	enum ELayout : U32
	{
		kLayoutF_AlignX1  = 1,
		kLayoutF_AlignY1  = 2,
		kLayoutF_AlignX2  = 4,
		kLayoutF_AlignY2  = 8,
		kLayoutF_KeepResX = 16,
		kLayoutF_KeepResY = 32,
		
		kLayoutF_AlignX   = kLayoutF_AlignX1  | kLayoutF_AlignX2,
		kLayoutF_AlignY   = kLayoutF_AlignY1  | kLayoutF_AlignY2,
		kLayoutF_Align    = kLayoutF_AlignX   | kLayoutF_AlignY,
		kLayoutF_KeepRes  = kLayoutF_KeepResX | kLayoutF_KeepResY
	};
	
	struct SViewport
	{
		SRect       shape;
		SIntVector2 origin;

		inline SViewport()
		: shape()
		, origin()
		{
		}
	};

	namespace detail
	{

		struct SGroupNode
		{
			RSpriteGroup *          pGroup;
			TIntrLink< SGroupNode > link;

			inline SGroupNode( RSpriteGroup *pGroup )
			: pGroup( pGroup )
			, link( this )
			{
			}
			inline ~SGroupNode()
			{
			}
		};
		struct SGroupList
		{
			TIntrList< SGroupNode > list;
			UPtr                    count;
			UPtr                    currentIndex;
			SGroupNode *            currentNode;

			inline SGroupList()
			: list()
			, count( 0 )
			, currentIndex( 0 )
			, currentNode( NULL )
			{
			}
			inline ~SGroupList()
			{
			}

			inline Void addNode( SGroupNode &node )
			{
				list.addTail( node.link );
				++count;
			}
			inline Void removeNode( SGroupNode &node )
			{
				currentIndex = 0;
				currentNode  = nullptr;

				list.unlink( node.link );
			}

			inline SGroupNode *findNode( RSpriteGroup *pGroup )
			{
				SGroupNode *p;

				for( p = list.head(); p != NULL; p = p->link.next() ) {
					if( p->pGroup == pGroup ) {
						return p;
					}
				}

				return nullptr;
			}

			inline UPtr getCount() const
			{
				return count;
			}
			inline SGroupNode *getNode( UPtr index )
			{
				if( index >= count ) {
					return NULL;
				}

				if( currentNode == NULL ) {
					currentIndex = 0;
					currentNode = list.head();
				}

				while( currentIndex < index ) {
					currentNode = currentNode->link.next();
					++currentIndex;
				}

				while( currentIndex > index ) {
					currentNode = currentNode->link.prev();
					--currentIndex;
				}

				return currentNode;
			}
		};
	
	}
	
	//--------------------------------------------------------------------//

	class MLayers
	{
	public:
		struct SHierarchy
		{
			TIntrList< RLayer > rootLayers;
			RLayer *            pDefaultLayer;

			inline SHierarchy()
			: rootLayers()
			, pDefaultLayer( nullptr )
			{
			}
		};
		struct SRenderer
		{
			CGfxFrame *          pDefaultFrame;
			TMutArr< SViewport > viewStack;

			inline SRenderer()
			: pDefaultFrame( nullptr )
			, viewStack()
			{
			}

			SViewport topViewport() const;
			Bool pushViewport( const SRect &localShape );
			Void popViewport();
		};

		static MLayers &get();

		const SHierarchy &hierarchy() const;
		const SRenderer &renderer() const;

		Void renderGL( CGfxFrame *pFrame );
		Void setDefaultFrame( CGfxFrame *pFrame );

		RLayer *newLayer( RLayer &Parent );
		RLayer *newLayer( CGfxFrame *pFrame = nullptr );

		SRect getFrameTransform( CGfxFrame *pFrame = nullptr ) const;
		Void setLayerAsRoot( RLayer &Layer );

		RLayer *head();
		const RLayer *head() const;
		const RLayer *head_const() const;

		RLayer *tail();
		const RLayer *tail() const;
		const RLayer *tail_const() const;

		Bool isDefaultLayerMade() const;
		RLayer *getDefaultLayer();
		const RLayer *getDefaultLayer() const;
		const RLayer *getDefaultLayer_const() const;

	private:
		SHierarchy m_Hierarchy;
		SRenderer  m_Renderer;

		MLayers();
		~MLayers();
	};
	static ax::TManager< MLayers > g_layerMgr;
	
	//--------------------------------------------------------------------//

	class RLayer: public TPoolObject< RLayer, kTag_Layer >
	{
	friend class MLayers;
	public:
		typedef LayerLink			Link;
		typedef LayerList			List;

		struct SHierarchy
		{
			// Pointer to the layer owning this one, or NULL if this is a root layer
			RLayer *pParent;
			// Links to the prior and next siblings in the parent's (or root's) list
			Link    siblings;
			// List for the layers owned by this
			List    children;

			SHierarchy( RLayer &ThisLayer, List &LayerList, RLayer *pParent );
		};
		struct SView
		{
			// Local region of the view prior to aspect adjustment
			SRect       desiredShape;
			// Aspect-adjusted region of the view
			SRect       shape;
			// Current offset in the shape
			SIntVector2 offset;
			// Total region covered by the view
			SRect       virtualShape;
			// Aspect ratio for the view
			F64         fAspectRatio;
			// Aspect mode of the view -- affects the final shape
			EAspect     aspectMode;
			
			SView();
			Void reset();
		};
		struct SLayout
		{
			// Combination of layout flags (ELayout, kLayoutF_)
			U32   uFlags;
			// The individual distances from each edge of the host
			SRect distances;
			// Cached host (to determine whether a reflow is needed again)
			SRect cachedHost;
			
			SLayout();
			Void reset();

			inline Bool any( uint32 uMask ) const
			{
				return ( uFlags & uMask ) != 0;
			}
			inline Bool all( uint32 uMask ) const
			{
				return ( uFlags & uMask ) == uMask;
			}
		};
		struct SRenderer
		{
			// Handle to the viewport used by the hosting engine
			CGfxFrame *        pFrame;
			// List of all the effects in use on this layer
			LayerEffectList    effects;
			// Pre-render sprite groups (appears in background)
			detail::SGroupList preGroups;
			// Post-render sprite groups (appears in foreground)
			detail::SGroupList postGroups;
			// Set of main rendering commands
			TMutArr<U8>        commands;
			// Output from rendering commands
			PrimitiveBuffer    primitives;
			// Stored text data (for text rendering commands)
			MutStr             textBuffer;

			SRenderer();
			Void reset();
		};
		struct SProperties
		{
			// Miscellaneous user pointer (not used by us)
			Void * pUserPointer;
			// Whether the layer should be displayed or not
			Bool   bIsVisible;
			// Controls whether the command/primitive buffers are cleared after rendering
			Bool   bAutoclear;
			// Name assigned by user (good for debugging)
			MutStr name;

			SProperties();
			Void reset();
		};
		
		//------------------------------------------------------------//
		
		~RLayer();

		const SHierarchy &hierarchy() const;
		const SView &view() const;
		const SLayout &layout() const;
		const SRenderer &renderer() const;

		SProperties &properties();
		const SProperties &properties() const;

		SIntVector2 globalToLocalNoOffset( const SIntVector2 &InGlobal ) const;
		SIntVector2 localToGlobalNoOffset( const SIntVector2 &InLocal ) const;
		SIntVector2 globalToLocal( const SIntVector2 &InGlobal ) const;
		SIntVector2 localToGlobal( const SIntVector2 &InLocal ) const;

		SRect getLocal() const;
		SRect getGlobal() const;
		SRect getParentGlobal() const;

		Bool isDescendantOf( const RLayer *pLayer ) const;
		Void setParent( RLayer *pNewParent );

		Bool isRoot() const;

		RLayer *getParent();
		const RLayer *getParent() const;
		const RLayer *getParent_const() const;

		RLayer *prev();
		const RLayer *prev() const;
		const RLayer *prev_const() const;

		RLayer *next();
		const RLayer *next() const;
		const RLayer *next_const() const;

		RLayer *head();
		const RLayer *head() const;
		const RLayer *head_const() const;

		RLayer *tail();
		const RLayer *tail() const;
		const RLayer *tail_const() const;

		TMutArr<U8> &renderCommands();
		PrimitiveBuffer &renderPrimitives();

		Void moveTop();
		Void moveBottom();
		Void moveUp();
		Void moveDown();

		Void addPrerenderSpriteGroup( RSpriteGroup &Group );
		Void addPostrenderSpriteGroup( RSpriteGroup &Group );

		Void removePrerenderSpriteGroup( RSpriteGroup &Group );
		Void removePostrenderSpriteGroup( RSpriteGroup &Group );

		Void removeAllPrerenderSpriteGroups();
		Void removeAllPostrenderSpriteGroups();

		Void prerenderGL( CGfxFrame *pFrame );
		Void postrenderGL( CGfxFrame *pFrame );

		uintptr getPrerenderSpriteGroupCount() const;
		uintptr getPostrenderSpriteGroupCount() const;

		RSpriteGroup *getPrerenderSpriteGroup( uintptr uIndex );
		RSpriteGroup *getPostrenderSpriteGroup( uintptr uIndex );

		Void setGLFrame( CGfxFrame *pFrame );
		CGfxFrame *getGLFrame() const;

		Void setVisible( Bool bVisible = true );
		Bool isVisible() const;

		Void setAutoclear( Bool bAutoclear = true );
		Bool getAutoclear() const;
		
		Void setUserPointer( Void *pUserData );
		Void *getUserPointer() const;

		Void setLayoutFlags( uint32 uLayoutFlags );
		uint32 getLayoutFlags() const;

		Void setLayoutDistance( const SRect &Distance );
		const SRect &getLayoutDistance() const;

		Void setPosition( const SIntVector2 &Pos );
		Void setOffset( const SIntVector2 &Off );
		Void setSize( const SIntVector2 &Res );

		SIntVector2 getPosition() const;
		SIntVector2 getOffset() const;
		SIntVector2 getSize() const;
		
		Void setAspect( float fAspectRatio, EAspect aspectMode );
		float getAspectRatio() const;
		EAspect getAspectMode() const;
		SRect calculateAspectAdjustments( const SRect &Local ) const;

		Void reflow();

		Void addEffectToBack( ILayerEffect &Effect );
		Void addEffectToFront( ILayerEffect &Effect );
		Void removeEffect( ILayerEffect &Effect );
		Void removeAllEffectInstances( ILayerEffect &Effect );
		Void removeAllEffects();
		Void runEffects();

		Void setName( Str name );
		Str getName() const;

	protected:
		RLayer( List &ParentList, RLayer *pParent = nullptr );
		Void renderGL( MLayers::SRenderer &Renderer, CGfxFrame *pFrame );

	private:
		SHierarchy					m_Hierarchy;
		SView						m_View;
		SLayout						m_Layout;
		SRenderer					m_Renderer;
		SProperties					m_Properties;
		
		Void renderGroupsGL( detail::SGroupList &List, CGfxFrame *pFrame );
		Void reflowWithin( const SRect &Host );

		RLayer( const RLayer & ) AX_DELETE_FUNC;
		RLayer &operator=( const RLayer & ) AX_DELETE_FUNC;
	};
	
	//--------------------------------------------------------------------//

	inline RLayer::SHierarchy::SHierarchy( RLayer &ThisLayer, List &LayerList, RLayer *pParent )
	: pParent( pParent )
	, siblings( &ThisLayer )
	, children()
	{
		LayerList.addTail( siblings );
	}

	inline RLayer::SView::SView()
	{
		reset();
	}
	inline Void RLayer::SView::reset()
	{
		desiredShape.x1 = 0;
		desiredShape.y1 = 0;
		desiredShape.x2 = 0;
		desiredShape.y2 = 0;
		shape.x1 = 0;
		shape.y1 = 0;
		shape.x2 = 0;
		shape.y2 = 0;
		offset.x = 0;
		offset.y = 0;
		virtualShape.x1 = 0;
		virtualShape.y1 = 0;
		virtualShape.x2 = 0;
		virtualShape.y2 = 0;
		fAspectRatio = 1.0;
		aspectMode = EAspect::None;
	}

	inline RLayer::SLayout::SLayout()
	{
		reset();
	}
	inline Void RLayer::SLayout::reset()
	{
		uFlags = kLayoutF_Align;
		distances.x1 = 0;
		distances.y1 = 0;
		distances.x2 = 0;
		distances.y2 = 0;
	}

	inline RLayer::SRenderer::SRenderer()
	: effects()
	, preGroups()
	, postGroups()
	, commands()
	, textBuffer()
	{
		reset();
	}
	inline Void RLayer::SRenderer::reset()
	{
		pFrame = nullptr;
	}

	inline RLayer::SProperties::SProperties()
	{
		reset();
	}
	inline Void RLayer::SProperties::reset()
	{
		pUserPointer = nullptr;
		bIsVisible = true;
		bAutoclear = false;
	}
	
	inline const RLayer::SHierarchy &RLayer::hierarchy() const
	{
		return m_Hierarchy;
	}
	inline const RLayer::SView &RLayer::view() const
	{
		return m_View;
	}
	inline const RLayer::SLayout &RLayer::layout() const
	{
		return m_Layout;
	}
	inline const RLayer::SRenderer &RLayer::renderer() const
	{
		return m_Renderer;
	}

	inline RLayer::SProperties &RLayer::properties()
	{
		return m_Properties;
	}
	inline const RLayer::SProperties &RLayer::properties() const
	{
		return m_Properties;
	}

	inline Bool RLayer::isRoot() const
	{
		return m_Hierarchy.pParent == nullptr;
	}

	inline RLayer *RLayer::getParent()
	{
		return m_Hierarchy.pParent;
	}
	inline const RLayer *RLayer::getParent() const
	{
		return m_Hierarchy.pParent;
	}
	inline const RLayer *RLayer::getParent_const() const
	{
		return m_Hierarchy.pParent;
	}

	inline RLayer *RLayer::prev()
	{
		return m_Hierarchy.siblings.prev();
	}
	inline const RLayer *RLayer::prev() const
	{
		return m_Hierarchy.siblings.prev();
	}
	inline const RLayer *RLayer::prev_const() const
	{
		return m_Hierarchy.siblings.prev();
	}

	inline RLayer *RLayer::next()
	{
		return m_Hierarchy.siblings.next();
	}
	inline const RLayer *RLayer::next() const
	{
		return m_Hierarchy.siblings.next();
	}
	inline const RLayer *RLayer::next_const() const
	{
		return m_Hierarchy.siblings.next();
	}

	inline RLayer *RLayer::head()
	{
		return m_Hierarchy.children.head();
	}
	inline const RLayer *RLayer::head() const
	{
		return m_Hierarchy.children.head();
	}
	inline const RLayer *RLayer::head_const() const
	{
		return m_Hierarchy.children.head();
	}

	inline RLayer *RLayer::tail()
	{
		return m_Hierarchy.children.tail();
	}
	inline const RLayer *RLayer::tail() const
	{
		return m_Hierarchy.children.tail();
	}
	inline const RLayer *RLayer::tail_const() const
	{
		return m_Hierarchy.children.tail();
	}

	inline TMutArr<U8> &RLayer::renderCommands()
	{
		return m_Renderer.commands;
	}
	inline PrimitiveBuffer &RLayer::renderPrimitives()
	{
		return m_Renderer.primitives;
	}

	inline Void RLayer::setName( Str name )
	{
		AX_EXPECT_MEMORY( m_Properties.name.tryAssign( name ) );
	}
	inline Str RLayer::getName() const
	{
		return m_Properties.name;
	}

	//--------------------------------------------------------------------//
	
	inline SViewport MLayers::SRenderer::topViewport() const
	{
		if( viewStack.isEmpty() ) {
			SViewport Viewport;

			Viewport.shape = g_layerMgr->getFrameTransform( pDefaultFrame );
			Viewport.origin = Viewport.shape.origin();

			return Viewport;
		}

		return viewStack.last();
	}
	inline Bool MLayers::SRenderer::pushViewport( const SRect &LocalShape )
	{
		const SViewport priorView = topViewport();

		SViewport globalView;

		globalView.shape = LocalShape.moved( priorView.origin );
		globalView.origin = globalView.shape.origin();

		globalView.shape.restrictBy( priorView.shape );

		return viewStack.append( globalView );
	}
	inline Void MLayers::SRenderer::popViewport()
	{
		if( viewStack.isEmpty() ) {
			return;
		}

		viewStack.removeLast();
	}
	
	inline const MLayers::SHierarchy &MLayers::hierarchy() const
	{
		return m_Hierarchy;
	}
	inline const MLayers::SRenderer &MLayers::renderer() const
	{
		return m_Renderer;
	}

	inline Void MLayers::setDefaultFrame( CGfxFrame *pFrame )
	{
		m_Renderer.pDefaultFrame = pFrame;
	}

	inline RLayer *MLayers::head()
	{
		return m_Hierarchy.rootLayers.head();
	}
	inline const RLayer *MLayers::head() const
	{
		return m_Hierarchy.rootLayers.head();
	}
	inline const RLayer *MLayers::head_const() const
	{
		return m_Hierarchy.rootLayers.head();
	}

	inline RLayer *MLayers::tail()
	{
		return m_Hierarchy.rootLayers.tail();
	}
	inline const RLayer *MLayers::tail() const
	{
		return m_Hierarchy.rootLayers.tail();
	}
	inline const RLayer *MLayers::tail_const() const
	{
		return m_Hierarchy.rootLayers.tail();
	}

	//--------------------------------------------------------------------//

	DOLL_FUNC RLayer *DOLL_API gfx_newLayer();
	DOLL_FUNC RLayer *DOLL_API gfx_deleteLayer( RLayer *layer );

	DOLL_FUNC RLayer *DOLL_API gfx_getDefaultLayer();

	DOLL_FUNC RLayer *DOLL_API gfx_getFirstLayer();
	DOLL_FUNC RLayer *DOLL_API gfx_getLastLayer();
	DOLL_FUNC RLayer *DOLL_API gfx_getLayerBefore( RLayer *layer );
	DOLL_FUNC RLayer *DOLL_API gfx_getLayerAfter( RLayer *layer );
	DOLL_FUNC Void DOLL_API gfx_moveLayerUp( RLayer *layer );
	DOLL_FUNC Void DOLL_API gfx_moveLayerDown( RLayer *layer );

	DOLL_FUNC Void DOLL_API gfx_addLayerPrerenderSpriteGroup( RLayer *layer, RSpriteGroup *group );
	DOLL_FUNC Void DOLL_API gfx_addLayerPostrenderSpriteGroup( RLayer *layer, RSpriteGroup *group );

	DOLL_FUNC Void DOLL_API gfx_removeLayerPrerenderSpriteGroup( RLayer *layer, RSpriteGroup *group );
	DOLL_FUNC Void DOLL_API gfx_removeLayerPostrenderSpriteGroup( RLayer *layer, RSpriteGroup *group );
	DOLL_FUNC Void DOLL_API gfx_removeAllLayerPrerenderSpriteGroups( RLayer *layer );
	DOLL_FUNC Void DOLL_API gfx_removeAllLayerPostrenderSpriteGroups( RLayer *layer );

	DOLL_FUNC UPtr DOLL_API gfx_getLayerPrerenderSpriteGroupCount( const RLayer *layer );
	DOLL_FUNC UPtr DOLL_API gfx_getLayerPostrenderSpriteGroupCount( const RLayer *layer );
	DOLL_FUNC RSpriteGroup *DOLL_API gfx_getLayerPrerenderSpriteGroup( RLayer *layer, UPtr index );
	DOLL_FUNC RSpriteGroup *DOLL_API gfx_getLayerPostrenderSpriteGroup( RLayer *layer, UPtr index );

	DOLL_FUNC Bool DOLL_API gfx_isLayerDescendantOf( const RLayer *layer, const RLayer *ancestor );
	DOLL_FUNC Void DOLL_API gfx_setLayerParent( RLayer *layer, RLayer *parent );
	DOLL_FUNC RLayer *DOLL_API gfx_getLayerParent( RLayer *layer );

	DOLL_FUNC Void DOLL_API gfx_setLayerVisible( RLayer *layer, Bool visible );
	DOLL_FUNC Bool DOLL_API gfx_getLayerVisible( const RLayer *layer );
	DOLL_FUNC Void DOLL_API gfx_setLayerLayout( RLayer *layer, U32 layoutFlags );
	DOLL_FUNC U32 DOLL_API gfx_getLayerLayout( const RLayer *layer );
	DOLL_FUNC Void DOLL_API gfx_setLayerLayoutDistance( RLayer *layer, S32 left, S32 top, S32 right, S32 bottom );
	DOLL_FUNC Void DOLL_API gfx_setLayerLayoutDistanceRect( RLayer *layer, const SRect &dist );
	DOLL_FUNC Bool DOLL_API gfx_getLayerLayoutDistance( SRect &dst, const RLayer *layer );
	inline SRect DOLL_API gfx_getLayerLayoutDistance( const RLayer *layer )
	{
		SRect r;
		return gfx_getLayerLayoutDistance( r, layer ), r;
	}

	DOLL_FUNC Void DOLL_API gfx_setLayerPosition( RLayer *layer, S32 x, S32 y );
	DOLL_FUNC Void DOLL_API gfx_setLayerPositionVec( RLayer *layer, const SIntVector2 &pos );
	DOLL_FUNC Bool DOLL_API gfx_getLayerPosition( SIntVector2 &dst, const RLayer *layer );
	inline SIntVector2 DOLL_API gfx_getLayerPosition( const RLayer *layer )
	{
		SIntVector2 r;
		return gfx_getLayerPosition( r, layer ), r;
	}
	DOLL_FUNC S32 DOLL_API gfx_getLayerPositionX( const RLayer *layer );
	DOLL_FUNC S32 DOLL_API gfx_getLayerPositionY( const RLayer *layer );
	DOLL_FUNC Void DOLL_API gfx_setLayerSize( RLayer *layer, S32 w, S32 h );
	DOLL_FUNC Void DOLL_API gfx_setLayerSizeVec( RLayer *layer, const SIntVector2 &res );
	DOLL_FUNC Bool DOLL_API gfx_getLayerSize( SIntVector2 &dst, const RLayer *layer );
	inline SIntVector2 DOLL_API gfx_getLayerSize( const RLayer *layer )
	{
		SIntVector2 r;
		return gfx_getLayerSize( r, layer ), r;
	}
	DOLL_FUNC S32 DOLL_API gfx_getLayerSizeX( const RLayer *layer );
	DOLL_FUNC S32 DOLL_API gfx_getLayerSizeY( const RLayer *layer );
	DOLL_FUNC Bool DOLL_API gfx_layerClientToScreen( SIntVector2 &dst, RLayer *layer, const SIntVector2 &local );
	DOLL_FUNC Bool DOLL_API gfx_layerScreenToClient( SIntVector2 &dst, RLayer *layer, const SIntVector2 &global );
	inline SIntVector2 DOLL_API gfx_layerClientToScreen( RLayer *layer, const SIntVector2 &local )
	{
		SIntVector2 r;
		return gfx_layerClientToScreen( r, layer, local ), r;
	}
	inline SIntVector2 DOLL_API gfx_layerScreenToClient( RLayer *layer, const SIntVector2 &global )
	{
		SIntVector2 r;
		return gfx_layerScreenToClient( r, layer, global ), r;
	}
	DOLL_FUNC Bool DOLL_API gfx_getLayerShape( SRect &dst, const RLayer *layer );
	DOLL_FUNC Bool DOLL_API gfx_getLayerScreenShape( SRect &dst, const RLayer *layer );
	DOLL_FUNC Bool DOLL_API gfx_getLayerParentScreenShape( SRect &dst, const RLayer *layer );
	inline SRect DOLL_API gfx_getLayerShape( const RLayer *layer )
	{
		SRect r;
		return gfx_getLayerShape( r, layer ), r;
	}
	inline SRect DOLL_API gfx_getLayerScreenShape( const RLayer *layer )
	{
		SRect r;
		return gfx_getLayerScreenShape( r, layer ), r;
	}
	inline SRect DOLL_API gfx_getLayerParentScreenShape( const RLayer *layer )
	{
		SRect r;
		return gfx_getLayerParentScreenShape( r, layer ), r;
	}
	DOLL_FUNC Void DOLL_API gfx_reflowLayer( RLayer *layer );

	DOLL_FUNC RLayer *DOLL_API gfx_getFirstLayerChild( RLayer *layer );
	DOLL_FUNC RLayer *DOLL_API gfx_getLastLayerChild( RLayer *layer );

	DOLL_FUNC Void DOLL_API gfx_setLayerUserPointer( RLayer *layer, Void *userp );
	DOLL_FUNC Void *DOLL_API gfx_getLayerUserPointer( RLayer *layer );

	DOLL_FUNC Void DOLL_API gfx_enableLayerAutoclear( RLayer *layer );
	DOLL_FUNC Void DOLL_API gfx_disableLayerAutoclear( RLayer *layer );
	DOLL_FUNC Void DOLL_API gfx_toggleLayerAutoclear( RLayer *layer );
	DOLL_FUNC Bool DOLL_API gfx_isLayerAutoclearEnabled( const RLayer *layer );

	DOLL_FUNC Void DOLL_API gfx_moveLayerTop( RLayer *layer );
	DOLL_FUNC Void DOLL_API gfx_moveLayerBottom( RLayer *layer );

	DOLL_FUNC Void DOLL_API gfx_setLayerFrame( RLayer *layer, CGfxFrame *pFrame );
	DOLL_FUNC CGfxFrame *DOLL_API gfx_getLayerFrame( RLayer *layer );

	DOLL_FUNC Void DOLL_API gfx_addLayerEffect( RLayer *pLayer, ILayerEffect *pLayerEffect );
	DOLL_FUNC Void DOLL_API gfx_addLayerEffectToFront( RLayer *pLayer, ILayerEffect *pLayerEffect );
	DOLL_FUNC Void DOLL_API gfx_removeLayerEffect( RLayer *pLayer, ILayerEffect *pLayerEffect );
	DOLL_FUNC Void DOLL_API gfx_removeAllLayerEffectInstances( RLayer *pLayer, ILayerEffect *pLayerEffect );
	DOLL_FUNC Void DOLL_API gfx_removeAllLayerEffects( RLayer *pLayer );

	DOLL_FUNC Void DOLL_API gfx_setLayerAspect( RLayer *pLayer, F32 fRatio, EAspect mode );
	DOLL_FUNC F32 DOLL_API gfx_getLayerAspectRatio( const RLayer *pLayer );
	DOLL_FUNC EAspect DOLL_API gfx_getLayerAspectMode( const RLayer *pLayer );

}
