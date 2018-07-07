#include "../BuildSettings.hpp"
#include "doll/UX/Widget.hpp"

#include "doll/Gfx/RenderCommands.hpp"

namespace doll
{

	namespace detail
	{

		DOLL_FUNC Void *DOLL_API ux__allocWidget( UPtr cBytes )
		{
			return g_widgets->mem__allocWidget( cBytes );
		}
		DOLL_FUNC NullPtr DOLL_API ux__freeWidget( Void *pWidget )
		{
			return g_widgets->mem__freeWidget( pWidget );
		}

	}

	DOLL_FUNC EWidgetVisitResult DOLL_API ux__cxx_widgetVisitor_f( IWidget *pWidget, IWidget *pParent, Void *pData )
	{
		AX_ASSERT_NOT_NULL( pWidget );
		AX_ASSERT_NOT_NULL( pData );

		return reinterpret_cast< IWidgetVisitor * >( pData )->visit( pWidget, pParent );
	}

	// ====================================================================== //

	MWidgets &MWidgets::get()
	{
		static MWidgets instance;
		return instance;
	}

	MWidgets::MWidgets()
	: m_widgets()
	, m_refreshWidgets()
	, m_pHoverWidget( nullptr )
	, m_pWidgetLayer( nullptr )
	{
	}
	MWidgets::~MWidgets()
	{
		deleteAllWidgets();
	}

	DOLL_FUNC MWidgets *DOLL_API doll__ux_getWidgets_ptr()
	{
		return &MWidgets::get();
	}

	Void *MWidgets::mem__allocWidget( UPtr cBytes )
	{
		return DOLL_ALLOC( *DOLL__DEFAULT_ALLOCATOR, cBytes, kTag_UX );
	}
	NullPtr MWidgets::mem__freeWidget( Void *pWidget )
	{
		DOLL_DEALLOC( *DOLL__DEFAULT_ALLOCATOR, pWidget );
		return nullptr;
	}

	Void MWidgets::install( OSWindow w )
	{
		AX_ASSERT_NOT_NULL( w );
		AX_ASSERT_NOT_NULL( gfx_getDefaultLayer() );

		if( m_pWidgetLayer != nullptr ) {
			return;
		}

		SWndDelegate wndDel;

		wndDel.pfnOnKeyPress     = &onKeyPress_f;
		wndDel.pfnOnKeyRelease   = &onKeyRelease_f;
		wndDel.pfnOnKeyChar      = &onKeyChar_f;

		wndDel.pfnOnMousePress   = &onMousePress_f;
		wndDel.pfnOnMouseRelease = &onMouseRelease_f;
		wndDel.pfnOnMouseWheel   = &onMouseWheel_f;
		wndDel.pfnOnMouseMove    = &onMouseMove_f;
		wndDel.pfnOnMouseExit    = &onMouseExit_f;

		wnd_addDelegate( w, wndDel );

		RLayer *const pDefLayer = gfx_getDefaultLayer();
		AX_EXPECT_NOT_NULL( pDefLayer );

		m_pWidgetLayer = gfx_newLayer();
		AX_EXPECT_MEMORY( m_pWidgetLayer );

		m_pWidgetLayer->setParent( pDefLayer );
		m_pWidgetLayer->setAutoclear( true );
		m_pWidgetLayer->setLayoutFlags( kLayoutF_Align );
		m_pWidgetLayer->setLayoutDistance( SRect() );

		m_pWidgetLayer->setSize( pDefLayer->getSize() );
		m_pWidgetLayer->setPosition( pDefLayer->getPosition() );
	}
	Void MWidgets::frame()
	{
		RLayer *const pCurLayer = gfx_getCurrentLayer();

		IWidget *pNextWidget;
		for( IWidget *pWidget = m_refreshWidgets.head(); pWidget != nullptr; pWidget = pNextWidget ) {
			pNextWidget = pWidget->m_refreshLink.next();

			if( pWidget->m_pLayer != nullptr ) {
				gfx_setCurrentLayer( pWidget->m_pLayer );
				gfx_clearQueue();
			}

			pWidget->validate();
			pWidget->onDraw();
		}

		gfx_setCurrentLayer( pCurLayer );
	}

	Void MWidgets::deleteAllWidgets()
	{
		while( m_widgets.isUsed() ) {
			deleteWidget( m_widgets.head() );
		}
	}

	Bool MWidgets::visit( FnWidgetVisitor pfnVisitor, Void *pData, IWidget *pParent ) const
	{
		AX_ASSERT_NOT_NULL( pfnVisitor );

		TIntrList<IWidget> &widgets =
			const_cast< TIntrList<IWidget> & >
			(
				!pParent ? m_widgets : pParent->m_children
			);

		for( IWidget &widget : widgets ) {
			const EWidgetVisitResult reply = pfnVisitor( &widget, pParent, pData );

			switch( reply ) {
			case EWidgetVisitResult::Break:
				return true;

			case EWidgetVisitResult::Continue:
				break;

			case EWidgetVisitResult::Recurse:
				if( widget.getChildren().isEmpty() || visit( pfnVisitor, pData, &widget ) ) {
					return true;
				}
				break;
			}
		}

		return false;
	}

	struct SWidgetPickData
	{
		static constexpr U32 kMaxStack = 32;

		SIntVector2 posStack[ kMaxStack ] = {};
		U32         uStackIndex           = 0;
		IWidget *   pPicked               = nullptr;
	};
	static EWidgetVisitResult DOLL_API pickWidget_f( IWidget *pWidget, IWidget *pParent, Void *pVisitData )
	{
		AX_PUSH_DISABLE_WARNING_MSVC(6386) // There is no buffer overrun

		AX_ASSERT_NOT_NULL( pVisitData );

		SWidgetPickData &data = *reinterpret_cast< SWidgetPickData * >( pVisitData );
		AX_ASSERT( data.uStackIndex < SWidgetPickData::kMaxStack );

		if( data.pPicked != nullptr && pParent != data.pPicked ) {
			return EWidgetVisitResult::Break;
		}

		const SIntVector2 &pos = data.posStack[ data.uStackIndex ];
		const Bool bPicked = pWidget->pick( pos - pWidget->localToGlobal() );

		if( !bPicked ) {
			return EWidgetVisitResult::Continue;
		}

		data.pPicked = pWidget;

		if( data.uStackIndex + 1 < SWidgetPickData::kMaxStack ) {
			// 6386: buffer overrun ('writable size is 256 bytes, but 264 bytes might be written')
			data.posStack[ ++data.uStackIndex ] = pos;
			return EWidgetVisitResult::Recurse;
		}

		return EWidgetVisitResult::Break;

		AX_POP_DISABLE_WARNING_MSVC()
	}
	IWidget *MWidgets::pick( const SIntVector2 &pos ) const
	{
		SWidgetPickData data;

		data.posStack[ 0 ] = pos;
		data.uStackIndex   = 0;
		data.pPicked       = nullptr;

		visit( &pickWidget_f, data );
		return data.pPicked;
	}

	EWndReply MWidgets::onKeyPress( EKey key, U32 mods, Bool isRepeat )
	{
		for( IWidget &widget : getKeyWidgets() ) {
			const EWidgetReply reply = widget.onKeyPress( key, mods, isRepeat );

			switch( reply ) {
			case EWidgetReply::Handled:
				return EWndReply::Handled;

			case EWidgetReply::Ignored:
				break;
			}
		}

		return EWndReply::NotHandled;
	}
	EWndReply MWidgets::onKeyRelease( EKey key, U32 mods )
	{
		for( IWidget &widget : getKeyWidgets() ) {
			const EWidgetReply reply = widget.onKeyRelease( key, mods );

			switch( reply ) {
			case EWidgetReply::Handled:
				return EWndReply::Handled;

			case EWidgetReply::Ignored:
				break;
			}
		}

		return EWndReply::NotHandled;
	}
	EWndReply MWidgets::onKeyChar( U32 utf32Char )
	{
		for( IWidget &widget : getKeyWidgets() ) {
			const EWidgetReply reply = widget.onKeyChar( utf32Char );

			switch( reply ) {
			case EWidgetReply::Handled:
				return EWndReply::Handled;

			case EWidgetReply::Ignored:
				break;
			}
		}

		return EWndReply::NotHandled;
	}

	EWndReply MWidgets::onMousePress( EMouse mouse, U32 mods, const SIntVector2 &mousepos )
	{
		const SIntVector2 pos = gfx_getDefaultLayer()->globalToLocal( mousepos );

		IWidget *pWidget = m_pHoverWidget;
		while( pWidget != nullptr ) {
			if( pWidget->onMousePress( mouse, mods, pWidget->globalToLocal( pos ) ) == EWidgetReply::Handled ) {
				return EWndReply::Handled;
			}

			pWidget = pWidget->m_pParent;
		}

		return EWndReply::NotHandled;
	}
	EWndReply MWidgets::onMouseRelease( EMouse mouse, U32 mods, const SIntVector2 &mousepos )
	{
		const SIntVector2 pos = gfx_getDefaultLayer()->globalToLocal( mousepos );

		IWidget *pWidget = m_pHoverWidget;
		while( pWidget != nullptr ) {
			if( pWidget->onMouseRelease( mouse, mods, pWidget->globalToLocal( pos ) ) == EWidgetReply::Handled ) {
				return EWndReply::Handled;
			}

			pWidget = pWidget->m_pParent;
		}

		return EWndReply::NotHandled;
	}
	EWndReply MWidgets::onMouseWheel( U32 mods, F32 fDelta, const SIntVector2 &mousepos )
	{
		const SIntVector2 pos = gfx_getDefaultLayer()->globalToLocal( mousepos );

		IWidget *pWidget = m_pHoverWidget;
		while( pWidget != nullptr ) {
			if( pWidget->onMouseWheel( mods, fDelta, pWidget->globalToLocal( pos ) ) == EWidgetReply::Handled ) {
				return EWndReply::Handled;
			}

			pWidget = pWidget->m_pParent;
		}

		return EWndReply::NotHandled;
	}
	EWndReply MWidgets::onMouseMove( U32 mods, const SIntVector2 &mousepos )
	{
		const SIntVector2 pos = gfx_getDefaultLayer()->globalToLocal( mousepos );

		IWidget *pWidget = pick( pos );
		setHoverWidget( pWidget, pos, mods );

		while( pWidget != nullptr ) {
			const EWidgetReply reply = pWidget->onMouseMove( mods, pWidget->globalToLocal( pos ) );
			if( reply == EWidgetReply::Handled ) {
				return EWndReply::Handled;
			}

			pWidget = pWidget->m_pParent;
		}

		return EWndReply::NotHandled;
	}
	EWndReply MWidgets::onMouseLeave( U32 mods )
	{
		setHoverWidget( nullptr, SIntVector2(), mods );
		return EWndReply::NotHandled;
	}

	static Void notifyEnter_r( IWidget *pWidget, IWidget *pBreakWidget, const SIntVector2 &globPos, U32 mods )
	{
		if( pWidget == pBreakWidget ) {
			return;
		}

		if( pWidget->getParent() != nullptr ) {
			notifyEnter_r( pWidget->getParent(), pBreakWidget, globPos, mods );
		}

		pWidget->onMouseEnter( mods, pWidget->globalToLocal( globPos ) );
	}
	Void MWidgets::setHoverWidget( IWidget *pNewWidget, const SIntVector2 &globPos, U32 mods )
	{
		// Do nothing on no-op
		if( m_pHoverWidget == pNewWidget ) {
			return;
		}

		// Testing widget handle
		IWidget *pWidget;

		// Outer-loop if-statement to avoid costly inner-loop if-statement
		if( pNewWidget != nullptr ) {
			IWidget *const pNewParent = pNewWidget->m_pParent;

			// Leave all current widgets
			for( pWidget = m_pHoverWidget; pWidget != nullptr; pWidget = pWidget->m_pParent ) {
				if( pWidget == pNewParent || pWidget == pNewWidget ) {
					break;
				}

				pWidget->onMouseLeave( mods );
			}
		} else {
			// Leave all current widgets
			for( pWidget = m_pHoverWidget; pWidget != nullptr; pWidget = pWidget->m_pParent ) {
				pWidget->onMouseLeave( mods );
			}
		}

		/*

			A1		B1		C1

			 A2		 B2		 C2

			  A3	  B3	  C3

		*/

		// Set the new hover widget
		if( ( m_pHoverWidget = pNewWidget ) != nullptr ) {
			notifyEnter_r( m_pHoverWidget, pWidget, globPos, mods );
		}
	}
	
	EWndReply MWidgets::onKeyPress_f( OSWindow, EKey key, U32 uMods, Bool bIsRepeat )
	{
		return g_widgets->onKeyPress( key, uMods, bIsRepeat );
	}
	EWndReply MWidgets::onKeyRelease_f( OSWindow, EKey key, U32 uMods )
	{
		return g_widgets->onKeyRelease( key, uMods );
	}
	EWndReply MWidgets::onKeyChar_f( OSWindow, U32 utf32Char )
	{
		return g_widgets->onKeyChar( utf32Char );
	}

	EWndReply MWidgets::onMousePress_f( OSWindow, EMouse mouse, S32 x, S32 y, U32 uMods )
	{
		return g_widgets->onMousePress( mouse, uMods, SIntVector2( x, y ) );
	}
	EWndReply MWidgets::onMouseRelease_f( OSWindow, EMouse mouse, S32 x, S32 y, U32 uMods )
	{
		return g_widgets->onMouseRelease( mouse, uMods, SIntVector2( x, y ) );
	}
	EWndReply MWidgets::onMouseWheel_f( OSWindow, float fDelta, S32 x, S32 y, U32 uMods )
	{
		return g_widgets->onMouseWheel( uMods, fDelta, SIntVector2( x, y ) );
	}
	EWndReply MWidgets::onMouseMove_f( OSWindow, S32 x, S32 y, U32 uMods )
	{
		return g_widgets->onMouseMove( uMods, SIntVector2( x, y ) );
	}
	EWndReply MWidgets::onMouseExit_f( OSWindow, U32 uMods )
	{
		return g_widgets->onMouseLeave( uMods );
	}

	// ------------------------------------------------------------------ //

	DOLL_FUNC Void DOLL_API ux_init( OSWindow w )
	{
		g_widgets->install( w );
	}

	DOLL_FUNC Void DOLL_API ux_deleteAllWidgets()
	{
		g_widgets->deleteAllWidgets();
	}

	DOLL_FUNC Bool DOLL_API ux_visitWidget( FnWidgetVisitor pfnVisitor, Void *pData, IWidget *pParent )
	{
		return g_widgets->visit( pfnVisitor, pData, pParent );
	}

	DOLL_FUNC IWidget *DOLL_API ux_pick( const SIntVector2 &pos )
	{
		return g_widgets->pick( pos );
	}

}
