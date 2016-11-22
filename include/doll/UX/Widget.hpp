#pragma once

#include "../Core/Defs.hpp"

#include "../Types/IntVector2.hpp"
#include "../Types/Rect.hpp"

#include "../Gfx/Layer.hpp"

#include "../OS/Key.hpp"
#include "../OS/Window.hpp"

namespace doll
{

	class MWidgets;
	class IWidgetVisitor;
	class IWidget;
	class WBaseWidget;

	struct SWidgetKeyIterator;

	//! How a widget replies to an event
	enum class EWidgetReply
	{
		//! We did nothing with this event
		Ignored,

		//! Handled the event
		Handled
	};

	//! The result of "visiting" a widget
	enum class EWidgetVisitResult: U32
	{
		//! Do not visit anymore widgets
		Break,
		//! Continue to the next sibling widget
		Continue,
		//! Recurse into this widget's subwidgets
		Recurse
	};

	//! Callback for visiting widgets
	typedef EWidgetVisitResult( DOLL_API *FnWidgetVisitor )( IWidget *pWidget, IWidget *pParent, Void *pVisitData );

	namespace detail
	{
		DOLL_FUNC EWidgetVisitResult DOLL_API ux__cxx_widgetVisitor_f( IWidget *, IWidget *, Void * );
	}

	//! Interface for visiting widgets
	class IWidgetVisitor
	{
	friend class MWidgets;
	public:
		//! Constructor
		IWidgetVisitor() {}
		//! Destructor
		virtual ~IWidgetVisitor() {}

		//! Visit a single widget
		virtual EWidgetVisitResult visit( IWidget *pWidget, IWidget *pParent ) = 0;
	};

	namespace detail
	{

		//! \internal
		DOLL_FUNC Void *DOLL_API ux__allocWidget( UPtr cBytes );
		//! \internal
		DOLL_FUNC NullPtr DOLL_API ux__freeWidget( Void * );

	}

	//! \internal
	class MWidgets
	{
	friend class IWidget;
	public:
		static MWidgets &get();

		Void install( OSWindow );
		Void frame();

		inline RLayer *getLayer() const
		{
			return m_pWidgetLayer;
		}

		struct SWidgetKeyRange
		{
			TIntrList< IWidget > &widgets;

			SWidgetKeyRange( MWidgets &widgets )
			: widgets( widgets.m_widgets )
			{
			}
			SWidgetKeyRange( const SWidgetKeyRange & ) = default;
			~SWidgetKeyRange() = default;

			SWidgetKeyIterator begin();
			SWidgetKeyIterator end();
		};

		template< typename T >
		T *newWidget();
		template< typename T, typename... TArgs >
		T *newWidget( TArgs... args );

		NullPtr deleteWidget( IWidget *pWidget );

		Void deleteAllWidgets();

		Bool visit( FnWidgetVisitor pfnVisitor, Void *pData = nullptr, IWidget *pParent = nullptr ) const;
		template< typename T >
		inline Bool visit( FnWidgetVisitor pfnVisitor, T &data, IWidget *pParent = nullptr ) const
		{
			return visit( pfnVisitor, reinterpret_cast< Void * >( &data ), pParent );
		}
		inline Bool visit( IWidgetVisitor *pVisitor, IWidget *pParent = nullptr ) const
		{
			return visit( &detail::ux__cxx_widgetVisitor_f, reinterpret_cast< Void * >( pVisitor ), pParent );
		}

		IWidget *pick( const SIntVector2 &pos ) const;

		inline SWidgetKeyRange getKeyWidgets()
		{
			return SWidgetKeyRange( *this );
		}

		EWndReply onKeyPress( EKey, U32 mods, Bool isRepeat );
		EWndReply onKeyRelease( EKey, U32 mods );
		EWndReply onKeyChar( U32 utf32Char );

		EWndReply onMousePress( EMouse, U32 mods, const SIntVector2 &pos );
		EWndReply onMouseRelease( EMouse, U32 mods, const SIntVector2 &pos );
		EWndReply onMouseMove( U32 mods, const SIntVector2 &pos );
		EWndReply onMouseWheel( U32 mods, F32 fDelta, const SIntVector2 &pos );
		EWndReply onMouseLeave( U32 mods );

		//! \internal
		Void *mem__allocWidget( UPtr cBytes );
		//! \internal
		NullPtr mem__freeWidget( Void * );

	private:
		TIntrList<IWidget> m_widgets;
		TIntrList<IWidget> m_refreshWidgets;
		IWidget *          m_pHoverWidget;
		RLayer *           m_pWidgetLayer;

		MWidgets();
		~MWidgets();

		Void setHoverWidget( IWidget *pHoverWidget, const SIntVector2 &globPos, U32 mods );

		static EWndReply onKeyPress_f( OSWindow, EKey, U32, Bool );
		static EWndReply onKeyRelease_f( OSWindow, EKey, U32 );
		static EWndReply onKeyChar_f( OSWindow, U32 );

		static EWndReply onMousePress_f( OSWindow, EMouse, S32, S32, U32 );
		static EWndReply onMouseRelease_f( OSWindow, EMouse, S32, S32, U32 );
		static EWndReply onMouseWheel_f( OSWindow, float, S32, S32, U32 );
		static EWndReply onMouseMove_f( OSWindow, S32, S32, U32 );
		static EWndReply onMouseExit_f( OSWindow, U32 );
	};
#ifdef DOLL__BUILD
	static TManager<MWidgets> g_widgets;
#endif

#ifdef DOLL__BUILD
# define DOLL__UX_WIDGETS MWidgets::get()
#else
# define DOLL__UX_WIDGETS doll__ux_getWidgets()
#endif

	//! \internal
	DOLL_FUNC MWidgets *DOLL_API doll__ux_getWidgets_ptr();
	inline MWidgets &DOLL_API doll__ux_getWidgets() {
		return *doll__ux_getWidgets_ptr();
	}

	//! Widget interface
	class IWidget
	{
	friend class MWidgets;
	friend class WBaseWidget;
	friend struct SWidgetKeyIterator;
	public:
		//! Change the shape (rectangle) of a widget, relative to its parent
		virtual Void setShape( const SRect & ) = 0;
		//! Retrieve the current shape (rectangle) of the widget
		inline const SRect &getShape() const
		{
			return m_shape;
		}
		//! Retrieve the size of the widget
		inline SIntVector2 getSize() const
		{
			return m_shape.size();
		}
		//! Retrieve the position of the widget (relative to its parent)
		inline SIntVector2 getPosition() const
		{
			return m_shape.origin();
		}

		//! Set the position of the widget (relative to its parent)
		inline Void setPosition( const SIntVector2 &pos )
		{
			setShape( m_shape.positioned( pos ) );
			updateLayerPosition();
		}
		//! Set the size of the widget
		inline Void setSize( const SIntVector2 &res )
		{
			setShape( m_shape.resized( res ) );
			if( m_pLayer != nullptr ) {
				m_pLayer->setSize( res );
			}
		}

		//! Retrieve the widget's parent widget (or `nullptr` if no parent)
		inline IWidget *getParent() const
		{
			return m_pParent;
		}
		//! Retrieve the widget's child widgets
		inline const TIntrList<IWidget> &getChildren() const
		{
			return m_children;
		}

		//! Determine whether the widget is ultimately a subwidget of `pAncestor`
		inline Bool isDescendantOf( const IWidget *pAncestor ) const
		{
			if( !pAncestor ) {
				return true;
			}

			const IWidget *pTest = m_pParent;
			while( pTest != nullptr ) {
				if( pTest == pAncestor ) {
					return true;
				}

				pTest = pTest->m_pParent;
			}

			return false;
		}
		//! Change the parent of the widget (`nullptr` means "root")
		inline Void setParent( IWidget *pParent )
		{
			AX_ASSERT( pParent != this );

			if( pParent != nullptr && pParent->isDescendantOf( this ) ) {
				pParent->setParent( m_pParent );
			}

			onParentChanging( pParent );

			IWidget *const pOldParent = m_pParent;

			TIntrList<IWidget> &newSiblings = pParent != nullptr ? pParent->m_children : DOLL__UX_WIDGETS.m_widgets;

			m_pParent = pParent;
			newSiblings.addTail( m_siblings );

			if( m_pLayer != nullptr ) {
				updateLayerParent();
				updateLayerPosition();
			}

			onParentChanged( pOldParent );
		}

		//! Initialize the layer
		inline Bool initLayer()
		{
			if( m_pLayer != nullptr ) {
				return true;
			}

			m_pLayer = gfx_newLayer();
			if( !AX_VERIFY_MEMORY( m_pLayer ) ) {
				m_pLayer = nullptr;
				return false;
			}

			gfx_disableLayerAutoclear( m_pLayer );
			gfx_setLayerLayout( m_pLayer, 0 );

			updateLayerParent();
			updateLayerPosition();

			gfx_setLayerSizeVec( m_pLayer, m_shape.size() );

			return true;
		}
		//! Destroy the layer
		inline Void finiLayer()
		{
			if( !m_pLayer ) {
				return;
			}

			m_pLayer = gfx_deleteLayer( m_pLayer );
		}
		//! Get the layer
		inline RLayer *getLayer() const
		{
			return m_pLayer;
		}

		//! Convert a local position to a global position
		inline SIntVector2 localToGlobal( const SIntVector2 &off = SIntVector2() ) const
		{
			SIntVector2 r = off;
			for( const IWidget *pSuper = this; pSuper != nullptr; pSuper = pSuper->m_pParent ) {
				r += pSuper->getPosition();
			}
			return r;
		}
		//! Convert a global position to a local position
		inline SIntVector2 globalToLocal( const SIntVector2 &pos = SIntVector2() ) const
		{
			SIntVector2 r = pos;
			for( const IWidget *pSuper = this; pSuper != nullptr; pSuper = pSuper->m_pParent ) {
				r -= pSuper->getPosition();
			}
			return r;
		}

		//! Invalidate the widget (forcing it to draw again)
		inline Void invalidate()
		{
			DOLL__UX_WIDGETS.m_refreshWidgets.addTail( m_refreshLink );
		}
		//! Validate the widget (marking that it no longer needs to be drawn)
		inline Void validate()
		{
			m_refreshLink.unlink();
		}

		//! Determine whether a given offset intersects the widget
		virtual Bool pick( const SIntVector2 &localOffset ) const = 0;

		//! Handle a key press event
		virtual EWidgetReply onKeyPress( EKey, U32 mods, Bool isRepeat ) = 0;
		//! Handle a key release event
		virtual EWidgetReply onKeyRelease( EKey, U32 mods ) = 0;
		//! Handle a character input event
		virtual EWidgetReply onKeyChar( U32 utf32Char ) = 0;

		//! Handle a mouse press event
		virtual EWidgetReply onMousePress( EMouse, U32 mods, const SIntVector2 &pos ) = 0;
		//! Handle a mouse release event
		virtual EWidgetReply onMouseRelease( EMouse, U32 mods, const SIntVector2 &pos ) = 0;
		//! Handle a mouse enter event (the mouse now intersects with the widget)
		virtual Void onMouseEnter( U32 mods, const SIntVector2 &pos ) = 0;
		//! Handle a mouse leave event (the mouse no longer intersects with the widget)
		virtual Void onMouseLeave( U32 mods ) = 0;
		//! Handle a mouse wheel movement event
		virtual EWidgetReply onMouseWheel( U32 mods, F32 fDelta, const SIntVector2 &pos ) = 0;
		//! Handle a mouse movement event (the mouse moved within the widget)
		virtual EWidgetReply onMouseMove( U32 mods, const SIntVector2 &pos ) = 0;

		//! Handle a "parent changing" event (occurs before the parent is changed)
		virtual Void onParentChanging( IWidget *pNewParent ) = 0;
		//! Handle a "parent changed" event (occurs after the parent has been changed)
		virtual Void onParentChanged( IWidget *pOldParent ) = 0;

		//! Draw the widget
		virtual Void onDraw() = 0;
		
	protected:
		//! Constructor
		IWidget()
		: m_shape()
		, m_pParent( nullptr )
		, m_siblings( this )
		, m_children()
		, m_refreshLink( this )
		, m_pLayer( nullptr )
		{
			DOLL__UX_WIDGETS.m_refreshWidgets.addTail( m_refreshLink );
		}
		//! Destructor
		virtual ~IWidget()
		{
			MWidgets &mgr = DOLL__UX_WIDGETS;

			while( m_children.isUsed() ) {
				mgr.deleteWidget( m_children.head() );
			}
		}

	private:
		SRect              m_shape;
		IWidget *          m_pParent;
		TIntrLink<IWidget> m_siblings;
		TIntrList<IWidget> m_children;
		TIntrLink<IWidget> m_refreshLink;
		RLayer *           m_pLayer;

		inline Void updateLayerParent()
		{
			if( !m_pLayer ) {
				return;
			}

			RLayer *const pPrntLayer = m_pParent != nullptr ? m_pParent->m_pLayer : nullptr;
			gfx_setLayerParent( m_pLayer, pPrntLayer != nullptr ? pPrntLayer : DOLL__UX_WIDGETS.getLayer() );
		}
		inline Void updateLayerPosition()
		{
			if( !m_pLayer ) {
				return;
			}

			const SRect parentShape( gfx_getLayerParentScreenShape( m_pLayer ) );
			const SIntVector2 globalPos( parentShape.origin() + m_shape.origin() );
			
			gfx_setLayerPositionVec( m_pLayer, gfx_layerScreenToClient( m_pLayer, globalPos ) );
		}
	};

	//! Base widget implementation, suitable for subclassing
	class WBaseWidget: public IWidget
	{
	public:
		//! Constructor
		WBaseWidget(): IWidget() {}
		//! Destructor
		virtual ~WBaseWidget() {}

		virtual Void setShape( const SRect &shape ) override
		{
			if( m_shape != shape ) {
				invalidate();
			}

			m_shape = shape;
		}
		virtual Bool pick( const SIntVector2 &localOffset ) const override
		{
			const SRect &shape = getShape();

			if( localOffset.min() < 0 || localOffset.x >= shape.resX() || localOffset.y >= shape.resY() ) {
				return false;
			}

			return true;
		}

		virtual EWidgetReply onKeyPress( EKey, U32, Bool ) override { return EWidgetReply::Ignored; }
		virtual EWidgetReply onKeyRelease( EKey, U32 ) override { return EWidgetReply::Ignored; }
		virtual EWidgetReply onKeyChar( U32 ) override { return EWidgetReply::Ignored; }

		virtual EWidgetReply onMousePress( EMouse, U32, const SIntVector2& ) override { return EWidgetReply::Ignored; }
		virtual EWidgetReply onMouseRelease( EMouse, U32, const SIntVector2& ) override { return EWidgetReply::Ignored; }
		virtual Void onMouseEnter( U32, const SIntVector2& ) override {}
		virtual Void onMouseLeave( U32 ) override {}
		virtual EWidgetReply onMouseWheel( U32, F32, const SIntVector2 & ) override { return EWidgetReply::Ignored; }
		virtual EWidgetReply onMouseMove( U32, const SIntVector2& ) override { return EWidgetReply::Ignored; }

		virtual Void onParentChanging( IWidget * ) override {}
		virtual Void onParentChanged( IWidget * ) override {}

		virtual Void onDraw() override {}
	};

	template< typename T >
	inline T *MWidgets::newWidget()
	{
		static_assert( TIsBaseOf< IWidget, T >::value, "Non-widget class passed to `newWidget`" );

		T *const p = reinterpret_cast< T * >( detail::ux__allocWidget( sizeof( T ) ) );
		if( !p ) {
			return nullptr;
		}

		new( ( Void * )p, ax::detail::SPlcmntNw() ) T();

		IWidget *const pWidget = static_cast< IWidget * >( p );

		m_widgets.addTail( pWidget->m_siblings );
		return p;
	}
	template< typename T, typename... TArgs >
	inline T *MWidgets::newWidget( TArgs... args )
	{
		static_assert( TIsBaseOf< IWidget, T >::value, "Non-widget class passed to `newWidget`" );

		Void *const p = detail::ux__allocWidget( sizeof( T ) );
		if( !p ) {
			return nullptr;
		}

		new( p, ax::detail::SPlcmntNw() ) T( args... );

		IWidget *const pWidget = reinterpret_cast< IWidget * >( p );

		m_widgets.addTail( pWidget->m_siblings );
		return ( T * )pWidget;
	}

	inline NullPtr MWidgets::deleteWidget( IWidget *pWidget )
	{
		pWidget->~IWidget();
		detail::ux__freeWidget( pWidget );

		return nullptr;
	}

	struct SWidgetKeyIterator
	{
		IWidget *pWidget = nullptr;

		SWidgetKeyIterator( IWidget *pWidget = nullptr )
		: pWidget( pWidget )
		{
		}
		SWidgetKeyIterator( const SWidgetKeyIterator & ) = default;
		SWidgetKeyIterator( NullPtr )
		: pWidget( nullptr )
		{
		}
		~SWidgetKeyIterator() = default;

		/*

		`	A1		B1		C1
		`	 A2		 B2	 C2a C2b
		`	  A3	  B3	   C3

		*/

		SWidgetKeyIterator &advance()
		{
			AX_ASSERT_NOT_NULL( pWidget );

			if( !pWidget->m_siblings.prevLink() ) {
				pWidget = pWidget->getParent();
				return *this;
			}

			pWidget = pWidget->m_siblings.prev();
			AX_STATIC_ASSUME( pWidget != nullptr ); // if check above handles it

			while( pWidget->m_children.isUsed() ) {
				pWidget = pWidget->m_children.tail();
			}

			return *this;
		}
		SWidgetKeyIterator &retreat()
		{
			AX_ASSERT_NOT_NULL( pWidget );

			if( pWidget->m_children.isEmpty() ) {
				pWidget = pWidget->m_siblings.next();
				return *this;
			}

			pWidget = pWidget->m_children.head();

			return *this;
		}

		SWidgetKeyIterator &operator++()
		{
			return advance();
		}
		SWidgetKeyIterator operator++( int )
		{
			SWidgetKeyIterator x( *this );
			advance();
			return x;
		}

		SWidgetKeyIterator &operator--()
		{
			return retreat();
		}
		SWidgetKeyIterator operator--( int )
		{
			SWidgetKeyIterator x( *this );
			retreat();
			return x;
		}

		Bool operator==( const SWidgetKeyIterator &i ) const
		{
			return pWidget == i.pWidget;
		}
		Bool operator!=( const SWidgetKeyIterator &i ) const
		{
			return pWidget != i.pWidget;
		}
		Bool operator!() const
		{
			return !pWidget;
		}
		operator Bool() const
		{
			return pWidget != nullptr;
		}

		IWidget *operator->()
		{
			AX_ASSERT_NOT_NULL( pWidget );
			return pWidget;
		}
		const IWidget *operator->() const
		{
			AX_ASSERT_NOT_NULL( pWidget );
			return pWidget;
		}

		IWidget &operator*()
		{
			AX_ASSERT_NOT_NULL( pWidget );
			return *pWidget;
		}
		const IWidget &operator*() const
		{
			AX_ASSERT_NOT_NULL( pWidget );
			return *pWidget;
		}
	};

	inline SWidgetKeyIterator MWidgets::SWidgetKeyRange::begin()
	{
		IWidget *pWidget = nullptr;
		TIntrList<IWidget> *pList = &widgets;

		while( pList->isUsed() ) {
			AX_ASSERT_NOT_NULL( pList->tailLink() );
			AX_ASSERT_NOT_NULL( pList->tailLink()->node() );

			pWidget = pList->tail();
			pList = &pWidget->m_children;
		}

		return pWidget;
	}
	inline SWidgetKeyIterator MWidgets::SWidgetKeyRange::end()
	{
		return nullptr;
	}

	// ------------------------------------------------------------------ //

	DOLL_FUNC Void DOLL_API ux_init( OSWindow );
	inline RLayer *DOLL_API ux_getLayer()
	{
		return DOLL__UX_WIDGETS.getLayer();
	}

	template< typename T >
	inline T *DOLL_API ux_newWidget()
	{
		return DOLL__UX_WIDGETS.newWidget< T >();
	}
	template< typename T, typename... TArgs >
	inline T *DOLL_API ux_newWidget( TArgs... args )
	{
		return DOLL__UX_WIDGETS.newWidget< T >( args... );
	}

	inline NullPtr DOLL_API ux_deleteWidget( IWidget *pWidget )
	{
		return DOLL__UX_WIDGETS.deleteWidget( pWidget );
	}

	DOLL_FUNC Void DOLL_API ux_deleteAllWidgets();

	DOLL_FUNC Bool DOLL_API ux_visitWidget( FnWidgetVisitor pfnVisitor, Void *pData = nullptr, IWidget *pParent = nullptr );
	template< typename T >
	inline Bool DOLL_API ux_visitWidget( FnWidgetVisitor pfnVisitor, T &data, IWidget *pParent = nullptr )
	{
		return ux_visitWidget( pfnVisitor, reinterpret_cast< Void * >( &data ), pParent );
	}
	inline Bool DOLL_API ux_visitWidget( IWidgetVisitor *pVisitor, IWidget *pParent = nullptr )
	{
		return ux_visitWidget( &detail::ux__cxx_widgetVisitor_f, reinterpret_cast< Void * >( pVisitor ), pParent );
	}

	DOLL_FUNC IWidget *DOLL_API ux_pick( const SIntVector2 &pos );
	inline IWidget *DOLL_API ux_pick( S32 x, S32 y )
	{
		return ux_pick( SIntVector2( x, y ) );
	}

	inline MWidgets::SWidgetKeyRange DOLL_API ux_getKeyWidgets()
	{
		return DOLL__UX_WIDGETS.getKeyWidgets();
	}

}
