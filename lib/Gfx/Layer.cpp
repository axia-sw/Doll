#include "doll/Gfx/Layer.hpp"
#include "doll/Gfx/Sprite.hpp"
#include "doll/Gfx/RenderCommands.hpp"
#include "doll/Gfx/API-GL.hpp"

#include <gl/GL.h>

namespace doll
{

	MLayers &MLayers::get()
	{
		static MLayers instance;
		return instance;
	}

	MLayers::MLayers()
	: m_Hierarchy()
	{
	}
	MLayers::~MLayers()
	{
	}

	Void MLayers::renderGL( CGfxFrame *pFrame )
	{
		pFrame->setDefaultState();
		RLayer *const pCurrentLayer = gfx_getCurrentLayer();
		for( RLayer *pLayer = head(); pLayer != nullptr; pLayer = pLayer->next() ) {
			pLayer->renderGL( m_Renderer, pFrame );
		}
		gfx_setCurrentLayer( pCurrentLayer );
	}
	
	RLayer *MLayers::newLayer( RLayer &Parent )
	{
		RLayer *const pLayer = new RLayer( Parent.m_Hierarchy.children, &Parent );
		if( !AX_VERIFY_MEMORY( pLayer ) ) {
			return nullptr;
		}

		pLayer->m_Renderer.pFrame = Parent.m_Renderer.pFrame;
		return pLayer;
	}
	RLayer *MLayers::newLayer( CGfxFrame *pFrame )
	{
		RLayer *const pLayer = new RLayer( m_Hierarchy.rootLayers, nullptr );
		if( !AX_VERIFY_MEMORY( pLayer ) ) {
			return nullptr;
		}

		pLayer->m_Renderer.pFrame = !pFrame ? m_Renderer.pDefaultFrame : pFrame;
		return pLayer;
	}
	
	SRect MLayers::getFrameTransform( CGfxFrame *pFrame ) const
	{
		CGfxFrame *const pUsedVP = pFrame != nullptr ? pFrame : m_Renderer.pDefaultFrame;
		if( !pUsedVP ) {
			return SRect();
		}

		return SRect( 0, 0, pUsedVP->getResX(), pUsedVP->getResY() );
	}
	
	Void MLayers::setLayerAsRoot( RLayer &Layer )
	{
		Layer.m_Hierarchy.siblings.unlink();
		Layer.m_Hierarchy.pParent = nullptr;
		m_Hierarchy.rootLayers.addTail( Layer.m_Hierarchy.siblings );
	}

	Bool MLayers::isDefaultLayerMade() const
	{
		return m_Hierarchy.pDefaultLayer != nullptr;
	}
	RLayer *MLayers::getDefaultLayer()
	{
		if( m_Hierarchy.pDefaultLayer == nullptr ) {
			m_Hierarchy.pDefaultLayer = newLayer();
		}

		return m_Hierarchy.pDefaultLayer;
	}
	const RLayer *MLayers::getDefaultLayer() const
	{
		return m_Hierarchy.pDefaultLayer;
	}
	const RLayer *MLayers::getDefaultLayer_const() const
	{
		return m_Hierarchy.pDefaultLayer;
	}
	
	//--------------------------------------------------------------------//
	
	RLayer::RLayer( List &ParentList, RLayer *pParent )
	: TPoolObject()
	, m_Hierarchy( *this, ParentList, pParent )
	, m_View()
	, m_Layout()
	, m_Renderer()
	, m_Properties()
	{
	}
	RLayer::~RLayer()
	{
	}

	Void RLayer::renderGL( MLayers::SRenderer &Renderer, CGfxFrame *pView )
	{
		// Skip this viewport if it's either not visible or not meant to be rendered from the given viewport
		if( !isVisible() || ( isRoot() && getGLFrame() != pView ) ) {
			return;
		}

		// Save the old position and size (effects may temporarily alter)
		const SRect OldShape = m_View.shape;
		
		// run the effects on this viewport
		runEffects();

		// Push the current viewport in the stack -- this maintains global transformation details
		Renderer.pushViewport( m_View.shape );
		
		// Apply the GL viewport
		const SViewport VP = Renderer.topViewport();
		gfx_r_setViewport
		(
			max( VP.shape.x1, 0 ),
			max( VP.shape.y1, 0 ),
			VP.shape.resX(),
			VP.shape.resY()
		);

		// Set the scissor rectangle
		gfx_r_setScissor
		(
			max( VP.shape.x1, 0 ),
			max( VP.shape.y1, 0 ),
			VP.shape.resX(),
			VP.shape.resY()
		);

		// Render this layer
		gfx_setCurrentLayer( this );
		prerenderGL( pView );
		gfx_drawQueueNowGL( pView );
		if( m_Properties.bAutoclear ) {
			gfx_clearQueue();
		}
		postrenderGL( pView );

		// Render child layers
		for( RLayer *pLayer = head(); pLayer != nullptr; pLayer = pLayer->next() ) {
			pLayer->renderGL( Renderer, pView );
		}

		// Restore the current viewport in the stack
		Renderer.popViewport();

		// Restore the old position and size (in the event they were changed by running effects)
		m_View.shape = OldShape;
	}
	Void RLayer::renderGroupsGL( detail::SGroupList &List, CGfxFrame *pView )
	{
		AX_ASSERT_NOT_NULL( pView );
		
		const U32 w = pView->getResX();
		const U32 h = pView->getResY();

#if 0
		//SetDefaultState( pView );
		const HRESULT hr = pDevice->SetFVF( SVertex2DSprite::FVF_DEFAULT );
		ASSERT( SUCCEEDED( hr ), "Invalid D3D op" );
#endif

		for( detail::SGroupNode *p = List.list.head(); p != nullptr; p = p->link.next() ) {
			if( !p->pGroup ) {
				continue;
			}

			p->pGroup->render_gl( w, h );
			//p->pGroup->render_d3d9( pDevice, w, h );
		}
	}

	SIntVector2 RLayer::globalToLocalNoOffset( const SIntVector2 &InGlobal ) const
	{
		return InGlobal - getGlobal().origin();
	}
	SIntVector2 RLayer::localToGlobalNoOffset( const SIntVector2 &InLocal ) const
	{
		return InLocal + getGlobal().origin();
	}
	SIntVector2 RLayer::globalToLocal( const SIntVector2 &InGlobal ) const
	{
		return InGlobal - ( getGlobal().origin() + m_View.offset );
	}
	SIntVector2 RLayer::localToGlobal( const SIntVector2 &InLocal ) const
	{
		return InLocal + ( getGlobal().origin() + m_View.offset );
	}

	SRect RLayer::getLocal() const
	{
		return m_View.shape;
	}
	SRect RLayer::getGlobal() const
	{
		return getLocal().moved( getParentGlobal().origin() );
	}
	SRect RLayer::getParentGlobal() const
	{
		if( m_Hierarchy.pParent != nullptr ) {
			const RLayer &Prnt = *m_Hierarchy.pParent;
			return Prnt.getGlobal().moved( Prnt.m_View.offset );
		}

		return g_layerMgr->getFrameTransform( m_Renderer.pFrame );
	}

	Bool RLayer::isDescendantOf( const RLayer *pLayer ) const
	{
		if( !pLayer ) {
			return true;
		}

		for( const RLayer *pTest = this; pTest != nullptr; pTest = pTest->m_Hierarchy.pParent ) {
			if( pTest->m_Hierarchy.pParent == pLayer ) {
				return true;
			}
		}

		return false;
	}
	Void RLayer::setParent( RLayer *pNewParent )
	{
		if( !pNewParent ) {
			g_layerMgr->setLayerAsRoot( *this );
			return;
		}

		if( pNewParent->isDescendantOf( this ) ) {
			pNewParent->setParent( m_Hierarchy.pParent );
		}

		m_Hierarchy.siblings.unlink();
		pNewParent->m_Hierarchy.children.addTail( m_Hierarchy.siblings );
		m_Hierarchy.pParent = pNewParent;
	}

	Void RLayer::moveTop()
	{
		m_Hierarchy.siblings.toBack();
	}
	Void RLayer::moveBottom()
	{
		m_Hierarchy.siblings.toFront();
	}
	Void RLayer::moveUp()
	{
		m_Hierarchy.siblings.toNext();
	}
	Void RLayer::moveDown()
	{
		m_Hierarchy.siblings.toPrior();
	}

	Void RLayer::addPrerenderSpriteGroup( RSpriteGroup &group )
	{
		detail::SGroupNode *const node = new detail::SGroupNode( &group );
		if( !AX_VERIFY_MEMORY( node ) ) {
			return;
		}

		m_Renderer.preGroups.addNode( *node );
	}
	Void RLayer::addPostrenderSpriteGroup( RSpriteGroup &group )
	{
		detail::SGroupNode *const node = new detail::SGroupNode( &group );
		if( !AX_VERIFY_MEMORY( node ) ) {
			return;
		}

		m_Renderer.postGroups.addNode( *node );
	}

	Void RLayer::removePrerenderSpriteGroup( RSpriteGroup &group )
	{
		detail::SGroupNode *const node = m_Renderer.preGroups.findNode( &group );
		if( !node ) {
			return;
		}

		m_Renderer.preGroups.removeNode( *node );
		delete node;
	}
	Void RLayer::removePostrenderSpriteGroup( RSpriteGroup &group )
	{
		detail::SGroupNode *const node = m_Renderer.postGroups.findNode( &group );
		if( !node ) {
			return;
		}

		m_Renderer.postGroups.removeNode( *node );
		delete node;
	}

	Void RLayer::removeAllPrerenderSpriteGroups()
	{
		while( m_Renderer.preGroups.list.isUsed() ) {
			auto node = m_Renderer.preGroups.list.head();
			if( !node ) {
				return;
			}

			m_Renderer.preGroups.list.unlink( node->link );
			delete node;
		}
	}
	Void RLayer::removeAllPostrenderSpriteGroups()
	{
		while( m_Renderer.postGroups.list.isUsed() ) {
			auto node = m_Renderer.postGroups.list.head();
			if( !node ) {
				return;
			}

			m_Renderer.postGroups.list.unlink( node->link );
			delete node;
		}
	}

	uintptr RLayer::getPrerenderSpriteGroupCount() const
	{
		return static_cast< uintptr >( m_Renderer.preGroups.getCount() );
	}
	uintptr RLayer::getPostrenderSpriteGroupCount() const
	{
		return static_cast< uintptr >( m_Renderer.postGroups.getCount() );
	}

	RSpriteGroup *RLayer::getPrerenderSpriteGroup( uintptr uIndex )
	{
		detail::SGroupNode *const pNode = m_Renderer.preGroups.getNode( uIndex );
		if( !pNode ) {
			return nullptr;
		}

		return pNode->pGroup;
	}
	RSpriteGroup *RLayer::getPostrenderSpriteGroup( uintptr uIndex )
	{
		detail::SGroupNode *const pNode = m_Renderer.postGroups.getNode( uIndex );
		if( !pNode ) {
			return nullptr;
		}

		return pNode->pGroup;
	}

	Void RLayer::prerenderGL( CGfxFrame *pView )
	{
		renderGroupsGL( m_Renderer.preGroups, pView );
	}
	Void RLayer::postrenderGL( CGfxFrame *pView )
	{
		renderGroupsGL( m_Renderer.postGroups, pView );
	}

	Void RLayer::setGLFrame( CGfxFrame *pFrame )
	{
		m_Renderer.pFrame = pFrame;
	}
	CGfxFrame *RLayer::getGLFrame() const
	{
		return m_Renderer.pFrame;
	}

	Void RLayer::setVisible( Bool bVisible )
	{
		m_Properties.bIsVisible = bVisible;
	}
	Bool RLayer::isVisible() const
	{
		return m_Properties.bIsVisible;
	}

	Void RLayer::setAutoclear( Bool bAutoclear )
	{
		m_Properties.bAutoclear = bAutoclear;
	}
	Bool RLayer::getAutoclear() const
	{
		return m_Properties.bAutoclear;
	}
	
	Void RLayer::setUserPointer( Void *pUserData )
	{
		m_Properties.pUserPointer = pUserData;
	}
	Void *RLayer::getUserPointer() const
	{
		return m_Properties.pUserPointer;
	}

	Void RLayer::setLayoutFlags( uint32 uLayoutFlags )
	{
		m_Layout.uFlags = uLayoutFlags;
	}
	uint32 RLayer::getLayoutFlags() const
	{
		return m_Layout.uFlags;
	}

	Void RLayer::setLayoutDistance( const SRect &Distance )
	{
		m_Layout.distances = Distance;
	}
	const SRect &RLayer::getLayoutDistance() const
	{
		return m_Layout.distances;
	}

	Void RLayer::setPosition( const SIntVector2 &Pos )
	{
		m_View.desiredShape.positionMe( Pos );
		m_View.shape = calculateAspectAdjustments( m_View.desiredShape );

		//const SIntVector2 Global = localToGlobal( SIntVector2( 0,  0 ) );
		//m_Renderer.Primitives.setOffset( float( Global.x ), float( Global.y ) );
	}
	Void RLayer::setOffset( const SIntVector2 &Off )
	{
		m_View.offset = Off;
	}
	Void RLayer::setSize( const SIntVector2 &Res )
	{
		m_View.desiredShape.resizeMe( Res );
		m_View.shape = calculateAspectAdjustments( m_View.desiredShape );
		reflow();
	}

	SIntVector2 RLayer::getPosition() const
	{
		return m_View.shape.origin();
	}
	SIntVector2 RLayer::getOffset() const
	{
		return m_View.offset;
	}
	SIntVector2 RLayer::getSize() const
	{
		return m_View.shape.size();
	}
	
	Void RLayer::setAspect( float fAspectRatio, EAspect aspectMode )
	{
		m_View.fAspectRatio = fAspectRatio;
		m_View.aspectMode = aspectMode;
		m_View.shape = calculateAspectAdjustments( m_View.desiredShape );
	}
	float RLayer::getAspectRatio() const
	{
		return ( float )m_View.fAspectRatio;
	}
	EAspect RLayer::getAspectMode() const
	{
		return m_View.aspectMode;
	}
	SRect RLayer::calculateAspectAdjustments( const SRect &Local ) const
	{
		const SIntVector2 NewSize = m_View.shape.size().aspectResized( Local.size(), m_View.fAspectRatio, m_View.aspectMode );

		SRect NewRect;

		NewRect.x1 = Local.x1 + ( Local.resX()/2 - NewSize.x/2 );
		NewRect.y1 = Local.y1 + ( Local.resY()/2 - NewSize.y/2 );
		NewRect.x2 = NewRect.x1 + NewSize.x;
		NewRect.y2 = NewRect.y1 + NewSize.y;

		return NewRect;
	}

	Void RLayer::reflow()
	{
		const SRect Host = m_View.shape.moved( -m_View.shape.origin() );

		if( m_Layout.cachedHost.size() == Host.size() ) {
			return;
		}
		m_Layout.cachedHost = Host;

		for( RLayer *pLayer = head(); pLayer != nullptr; pLayer = pLayer->next() ) {
			pLayer->reflowWithin( Host );
			pLayer->reflow();
		}
	}
	Void RLayer::reflowWithin( const SRect &Host )
	{
		const SRect Src = m_View.shape;
		SRect Dst;

		const uint32 uFlags = m_Layout.uFlags;

		// fix horizontal alignment
		switch( uFlags & kLayoutF_AlignX ) {
		// no alignment
		case 0:
 			Dst.x1 = Src.x1;
			Dst.x2 = Src.x2;
			break;

		// left aligned
		case kLayoutF_AlignX1:
			Dst.x1 = Host.x1 + m_Layout.distances.x1;
			Dst.x2 = Dst.x1 + Src.resX();
			break;

		// right aligned
		case kLayoutF_AlignX2:
			Dst.x2 = Host.x2 - m_Layout.distances.x2;
			Dst.x1 = Dst.x1 - Src.resX();
			break;

		// left + right aligned
		default:
			Dst.x1 = Host.x1 + m_Layout.distances.x1;
			Dst.x2 = Host.x2 - m_Layout.distances.x2;

			if( uFlags & kLayoutF_KeepResX ) {
				Dst.x1 = Dst.x1 + Dst.resX()/2 - Src.resX()/2;
				Dst.x2 = Dst.x1 + Src.resX();
			}

			break;
		}

		// fix vertical alignment
		switch( uFlags & kLayoutF_AlignY ) {
		// no alignment
		case 0:
			Dst.y1 = Src.y1;
			Dst.y2 = Src.y2;
			break;

		// top aligned
		case kLayoutF_AlignY1:
			Dst.y1 = Host.y1 + m_Layout.distances.y1;
			Dst.y2 = Dst.y1 + Src.resY();
			break;

		// bottom aligned
		case kLayoutF_AlignY2:
			Dst.y2 = Host.y2 - m_Layout.distances.y2;
			Dst.y1 = Dst.y2 - Src.resY();
			break;

		// top + bottom aligned
		default:
			Dst.y1 = Host.y1 + m_Layout.distances.y1;
			Dst.y2 = Host.y2 - m_Layout.distances.y2;

			if( uFlags & kLayoutF_KeepResY ) {
				Dst.y1 = Dst.y1 + Dst.resY()/2 - Src.resY()/2;
				Dst.y2 = Dst.y1 + Src.resY();
			}

			break;
		}

		// apply the new position
		if( Dst.x1 != Src.x1 || Dst.y1 != Src.y1 ) {
			setPosition( Dst.origin() );
		}

		// apply the new size
		if( Dst.x2 != Src.x2 || Dst.y2 != Src.y2 ) {
			setSize( Dst.size() );
		}
	}
	
	Void RLayer::addEffectToBack( ILayerEffect &effect )
	{
		m_Renderer.effects.addTail( &effect );
	}
	Void RLayer::addEffectToFront( ILayerEffect &effect )
	{
		m_Renderer.effects.addHead( &effect );
	}
	Void RLayer::removeEffect( ILayerEffect &effect )
	{
		for( ax::TList< ILayerEffect * >::Iterator i = m_Renderer.effects.begin(); i != m_Renderer.effects.end(); ++i ) {
			ILayerEffect *const pLayerEffect = *i;
			if( pLayerEffect == &effect ) {
				m_Renderer.effects.remove( i );
				return;
			}
		}
	}
	Void RLayer::removeAllEffectInstances( ILayerEffect &effect )
	{
		ax::TList< ILayerEffect * >::Iterator i = m_Renderer.effects.begin();
		while( i != m_Renderer.effects.end() ) {
			ILayerEffect *const pLayerEffect = *i;
			if( pLayerEffect == &effect ) {
				i = m_Renderer.effects.remove( i );
			} else {
				i.advance();
			}
		}
	}
	Void RLayer::removeAllEffects()
	{
		m_Renderer.effects.clear();
	}
	Void RLayer::runEffects()
	{
#define HAS_REMOVE_LAYERS 0

#if HAS_REMOVE_LAYERS
		Bool bNeedToRemoveLayers = false;
#endif

		for( ILayerEffect *const &pLayerEffect : m_Renderer.effects ) {
			if( !pLayerEffect ) {
				continue;
			}

			pLayerEffect->run( *this );
#if HAS_REMOVE_LAYERS
			if( pLayerEffect->getRemoveOnInactive() && !pLayerEffect->isActive() ) {
				bNeedToRemoveLayers = true;
			}
#endif
		}

#if HAS_REMOVE_LAYERS
		if( !bNeedToRemoveLayers ) {
			return;
		}

		TList< ILayerEffect * >::Iterator i = m_Renderer.effects.begin();
		while( i != m_Renderer.effects.end() ) {
			ILayerEffect *const pLayerEffect = *i;

			if( !pLayerEffect || !pLayerEffect->getRemoveOnInactive() || pLayerEffect->isActive() ) {
				i.advance();
				continue;
			}

			i = m_Renderer.effects.remove( i );
		}
#endif
	}


	DOLL_FUNC RLayer *DOLL_API gfx_newLayer()
	{
		RLayer *layer = g_layerMgr->newLayer( gfx_r_getFrame() );
		if( !AX_VERIFY_MEMORY( layer ) ) {
			return nullptr;
		}

		return layer;
	}
	DOLL_FUNC RLayer *DOLL_API gfx_deleteLayer( RLayer *layer )
	{
		delete layer;
		return nullptr;
	}

	DOLL_FUNC RLayer *DOLL_API gfx_getDefaultLayer()
	{
		return g_layerMgr->getDefaultLayer();
	}

	DOLL_FUNC RLayer *DOLL_API gfx_getFirstLayer()
	{
		return g_layerMgr->head();
	}
	DOLL_FUNC RLayer *DOLL_API gfx_getLastLayer()
	{
		return g_layerMgr->tail();
	}
	DOLL_FUNC RLayer *DOLL_API gfx_getLayerBefore( RLayer *layer )
	{
		if( !layer ) {
			return NULL;
		}

		return layer->prev();
	}
	DOLL_FUNC RLayer *DOLL_API gfx_getLayerAfter( RLayer *layer )
	{
		if( !layer ) {
			return NULL;
		}

		return layer->next();
	}
	DOLL_FUNC Void DOLL_API gfx_moveLayerUp( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->moveUp();
	}
	DOLL_FUNC Void DOLL_API gfx_moveLayerDown( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->moveDown();
	}

	DOLL_FUNC Void DOLL_API gfx_addLayerPrerenderSpriteGroup( RLayer *layer, RSpriteGroup *group )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) || !AX_VERIFY_NOT_NULL( group ) ) {
			return;
		}

		layer->addPrerenderSpriteGroup( *group );
	}
	DOLL_FUNC Void DOLL_API gfx_addLayerPostrenderSpriteGroup( RLayer *layer, RSpriteGroup *group )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) || !AX_VERIFY_NOT_NULL( group ) ) {
			return;
		}

		layer->addPostrenderSpriteGroup( *group );
	}

	DOLL_FUNC Void DOLL_API gfx_removeLayerPrerenderSpriteGroup( RLayer *layer, RSpriteGroup *group )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		if( !group ) {
			return;
		}

		layer->removePrerenderSpriteGroup( *group );
	}
	DOLL_FUNC Void DOLL_API gfx_removeLayerPostrenderSpriteGroup( RLayer *layer, RSpriteGroup *group )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		if( !group ) {
			return;
		}

		layer->removePostrenderSpriteGroup( *group );
	}
	DOLL_FUNC Void DOLL_API gfx_removeAllLayerPrerenderSpriteGroups( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->removeAllPrerenderSpriteGroups();
	}
	DOLL_FUNC Void DOLL_API gfx_removeAllLayerPostrenderSpriteGroups( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->removeAllPostrenderSpriteGroups();
	}

	DOLL_FUNC UPtr DOLL_API gfx_getLayerPrerenderSpriteGroupCount( const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return 0;
		}

		return layer->getPrerenderSpriteGroupCount();
	}
	DOLL_FUNC UPtr DOLL_API gfx_getLayerPostrenderSpriteGroupCount( const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return 0;
		}

		return layer->getPostrenderSpriteGroupCount();
	}
	DOLL_FUNC RSpriteGroup *DOLL_API gfx_getLayerPrerenderSpriteGroup( RLayer *layer, UPtr index )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return nullptr;
		}

		return layer->getPrerenderSpriteGroup( index );
	}
	DOLL_FUNC RSpriteGroup *DOLL_API gfx_getLayerPostrenderSpriteGroup( RLayer *layer, UPtr index )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return nullptr;
		}

		return layer->getPostrenderSpriteGroup( index );
	}

	DOLL_FUNC Bool DOLL_API gfx_isLayerDescendantOf( const RLayer *layer, const RLayer *ancestor )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return false;
		}

		return layer->isDescendantOf( ancestor );
	}
	DOLL_FUNC Void DOLL_API gfx_setLayerParent( RLayer *layer, RLayer *parent )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setParent( parent );
	}
	DOLL_FUNC RLayer *DOLL_API gfx_getLayerParent( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return nullptr;
		}

		return layer->getParent();
	}

	DOLL_FUNC Void DOLL_API gfx_setLayerVisible( RLayer *layer, Bool visible )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setVisible( visible );
	}
	DOLL_FUNC Bool DOLL_API gfx_getLayerVisible( const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return false;
		}

		return layer->isVisible();
	}
	DOLL_FUNC Void DOLL_API gfx_setLayerLayout( RLayer *layer, U32 layoutFlags )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setLayoutFlags( layoutFlags );
	}
	DOLL_FUNC U32 DOLL_API gfx_getLayerLayout( const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return 0;
		}

		return layer->getLayoutFlags();
	}
	DOLL_FUNC Void DOLL_API gfx_setLayerLayoutDistance( RLayer *layer, S32 left, S32 top, S32 right, S32 bottom )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setLayoutDistance( SRect( left, top, right, bottom ) );
	}
	DOLL_FUNC Void DOLL_API gfx_setLayerLayoutDistanceRect( RLayer *layer, const SRect &dist )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setLayoutDistance( dist );
	}
	DOLL_FUNC Bool DOLL_API gfx_getLayerLayoutDistance( SRect &dst, const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			dst = SRect();
			return false;
		}

		dst = layer->getLayoutDistance();
		return true;
	}

	DOLL_FUNC Void DOLL_API gfx_setLayerPosition( RLayer *layer, S32 x, S32 y )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setPosition( SIntVector2( x, y ) );
	}
	DOLL_FUNC Void DOLL_API gfx_setLayerPositionVec( RLayer *layer, const SIntVector2 &pos )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setPosition( pos );
	}
	DOLL_FUNC Bool DOLL_API gfx_getLayerPosition( SIntVector2 &dst, const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			dst = SIntVector2();
			return false;
		}

		dst = layer->getPosition();
		return true;
	}
	DOLL_FUNC S32 DOLL_API gfx_getLayerPositionX( const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return 0;
		}

		return layer->getPosition().x;
	}
	DOLL_FUNC S32 DOLL_API gfx_getLayerPositionY( const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return 0;
		}

		return layer->getPosition().y;
	}
	DOLL_FUNC Void DOLL_API gfx_setLayerSize( RLayer *layer, S32 w, S32 h )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setSize( SIntVector2( w, h ) );
	}
	DOLL_FUNC Void DOLL_API gfx_setLayerSizeVec( RLayer *layer, const SIntVector2 &res )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setSize( res );
	}
	DOLL_FUNC Bool DOLL_API gfx_getLayerSize( SIntVector2 &dst, const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			dst = SIntVector2();
			return false;
		}

		dst = layer->getSize();
		return true;
	}
	DOLL_FUNC S32 DOLL_API gfx_getLayerSizeX( const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return 0;
		}

		return layer->getSize().x;
	}
	DOLL_FUNC S32 DOLL_API gfx_getLayerSizeY( const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return 0;
		}

		return layer->getSize().y;
	}
	DOLL_FUNC Bool DOLL_API gfx_layerClientToScreen( SIntVector2 &dst, RLayer *layer, const SIntVector2 &local )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			dst = local;
			return false;
		}

		dst = layer->localToGlobal( local );
		return true;
	}
	DOLL_FUNC Bool DOLL_API gfx_layerScreenToClient( SIntVector2 &dst, RLayer *layer, const SIntVector2 &global )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			dst = SIntVector2();
			return false;
		}

		dst = layer->globalToLocal( global );
		return true;
	}
	DOLL_FUNC Bool DOLL_API gfx_getLayerShape( SRect &dst, const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			dst = SRect();
			return false;
		}

		dst = layer->getLocal();
		return true;
	}
	DOLL_FUNC Bool DOLL_API gfx_getLayerScreenShape( SRect &dst, const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			dst = SRect();
			return false;
		}

		dst = layer->getGlobal();
		return true;
	}
	DOLL_FUNC Bool DOLL_API gfx_getLayerParentScreenShape( SRect &dst, const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			dst = SRect();
			return false;
		}

		dst = layer->getParentGlobal();
		return true;
	}
	DOLL_FUNC Void DOLL_API gfx_reflowLayer( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->reflow();
	}

	DOLL_FUNC RLayer *DOLL_API gfx_getFirstLayerChild( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return nullptr;
		}

		return layer->head();
	}
	DOLL_FUNC RLayer *DOLL_API gfx_getLastLayerChild( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return nullptr;
		}

		return layer->tail();
	}

	DOLL_FUNC Void DOLL_API gfx_setLayerUserPointer( RLayer *layer, Void *userp )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setUserPointer( userp );
	}
	DOLL_FUNC Void *DOLL_API gfx_getLayerUserPointer( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return nullptr;
		}

		return layer->getUserPointer();
	}

	DOLL_FUNC Void DOLL_API gfx_enableLayerAutoclear( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setAutoclear( true );
	}
	DOLL_FUNC Void DOLL_API gfx_disableLayerAutoclear( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setAutoclear( false );
	}
	DOLL_FUNC Void DOLL_API gfx_toggleLayerAutoclear( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setAutoclear( layer->getAutoclear() ^ true );
	}
	DOLL_FUNC Bool DOLL_API gfx_isLayerAutoclearEnabled( const RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return false;
		}

		return layer->getAutoclear();
	}

	DOLL_FUNC Void DOLL_API gfx_moveLayerTop( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->moveTop();
	}
	DOLL_FUNC Void DOLL_API gfx_moveLayerBottom( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->moveBottom();
	}

	DOLL_FUNC Void DOLL_API gfx_setLayerFrame( RLayer *layer, CGfxFrame *pFrame )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return;
		}

		layer->setGLFrame( pFrame );
	}
	DOLL_FUNC CGfxFrame *DOLL_API gfx_getLayerFrame( RLayer *layer )
	{
		if( !AX_VERIFY_NOT_NULL( layer ) ) {
			return nullptr;
		}

		return layer->getGLFrame();
	}

	DOLL_FUNC Void DOLL_API gfx_addLayerEffect( RLayer *pLayer, ILayerEffect *pLayerEffect )
	{
		if( !AX_VERIFY_NOT_NULL( pLayer ) || !AX_VERIFY_NOT_NULL( pLayerEffect ) ) {
			return;
		}

		pLayer->addEffectToBack( *pLayerEffect );
	}
	DOLL_FUNC Void DOLL_API gfx_addLayerEffectToFront( RLayer *pLayer, ILayerEffect *pLayerEffect )
	{
		if( !AX_VERIFY_NOT_NULL( pLayer ) || !AX_VERIFY_NOT_NULL( pLayerEffect ) ) {
			return;
		}

		pLayer->addEffectToFront( *pLayerEffect );
	}
	DOLL_FUNC Void DOLL_API gfx_removeLayerEffect( RLayer *pLayer, ILayerEffect *pLayerEffect )
	{
		if( !AX_VERIFY_NOT_NULL( pLayer ) || !AX_VERIFY_NOT_NULL( pLayerEffect ) ) {
			return;
		}

		pLayer->removeEffect( *pLayerEffect );
	}
	DOLL_FUNC Void DOLL_API gfx_removeAllLayerEffectInstances( RLayer *pLayer, ILayerEffect *pLayerEffect )
	{
		if( !AX_VERIFY_NOT_NULL( pLayer ) || !AX_VERIFY_NOT_NULL( pLayerEffect ) ) {
			return;
		}

		pLayer->removeAllEffectInstances( *pLayerEffect );
	}
	DOLL_FUNC Void DOLL_API gfx_removeAllLayerEffects( RLayer *pLayer )
	{
		if( !AX_VERIFY_NOT_NULL( pLayer ) ) {
			return;
		}

		pLayer->removeAllEffects();
	}

	DOLL_FUNC Void DOLL_API gfx_setLayerAspect( RLayer *pLayer, F32 fRatio, EAspect mode )
	{
		if( !AX_VERIFY_NOT_NULL( pLayer ) || !AX_VERIFY_MSG( fRatio > 1e-4f, "Invalid aspect ratio" ) ) {
			return;
		}

		pLayer->setAspect( fRatio, mode );
	}
	DOLL_FUNC F32 DOLL_API gfx_getLayerAspectRatio( const RLayer *pLayer )
	{
		if( !AX_VERIFY_NOT_NULL( pLayer ) ) {
			return 0.0f;
		}

		return pLayer->getAspectRatio();
	}
	DOLL_FUNC EAspect DOLL_API gfx_getLayerAspectMode( const RLayer *pLayer )
	{
		if( !AX_VERIFY_NOT_NULL( pLayer ) ) {
			return EAspect::None;
		}

		return pLayer->getAspectMode();
	}

}
