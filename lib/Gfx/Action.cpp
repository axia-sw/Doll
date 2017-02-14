#define DOLL_TRACE_FACILITY doll::kLog_GfxAction

#include "doll/Gfx/Action.hpp"
#include "doll/Gfx/Sprite.hpp"

namespace doll
{

	ActiveAction::Glob &ActiveAction::Glob::get()
	{
		static Glob instance;
		return instance;
	}

	ActiveAction::Glob::Glob()
	: activeActions()
	{
	}
	ActiveAction::Glob::~Glob()
	{
	}

	static U64 CalculateStartTime( ActionRef::List &activeActionRefs )
	{
		const ActionRef *const pRef = activeActionRefs.tail();
		if( !pRef ) {
			return 0;
		}

		return pRef->getStartInMicroseconds() + pRef->getDurationInMicroseconds();
	}

	ActionRef::ActionRef( List &activeActionRefs, Action &action )
	: m_uStartTime( CalculateStartTime( activeActionRefs ) )
	, m_action( action )
	, m_phase( EPhase::Init )
	, m_activeActionLink( this )
	{
	}
	ActionRef::~ActionRef()
	{
	}
	ActionRef *ActionRef::update( RSprite &sprite, const SActionTiming &Timing )
	{
		if( m_phase != EPhase::Fini ) {
			if( m_phase == EPhase::Init ) {
				m_phase = EPhase::Step;

				m_action.start( sprite );
			}

			m_action.update( sprite, Timing );

			if( !Timing.isComplete() ) {
				return this;
			}

			m_phase = EPhase::Fini;
			m_action.complete( sprite );

			m_phase = EPhase::Init;
		}

		return m_activeActionLink.next();
	}

	ActiveAction::ActiveAction( RSprite &sprite )
	: m_refs()
	, m_pCurrentRef( nullptr )
	, m_sprite( sprite )
	, m_cRepeats( 0 )
	, m_uStartTime( CFrameCounter::guCurrentTick )
	, m_uPriorTime( 0 )
	, m_spriteLink( this )
	, m_globalLink( this )
	{
		G().activeActions.addTail( m_globalLink );
		sprite.activeActions.addTail( m_spriteLink );
	}
	ActiveAction::~ActiveAction()
	{
	}

	Void ActiveAction::addAction( Action &action )
	{
		ActionRef *const pRef = new ActionRef( m_refs, action );
		if( !AX_VERIFY_MEMORY( pRef ) ) {
			return;
		}

		if( !m_pCurrentRef ) {
			m_pCurrentRef = pRef;
		}
	}

	Void ActiveAction::update()
	{
		if( m_pCurrentRef != nullptr ) {
			SActionTiming Timing;

			const U64 uActiveElapsed = CFrameCounter::guCurrentTick - m_uStartTime;
			const U64 uLocalStart = m_pCurrentRef->getStartInMicroseconds();

			Timing.uPriorElapsedTimeMicroseconds = m_uPriorTime - uLocalStart;
			Timing.uElapsedTimeMicroseconds = uActiveElapsed - uLocalStart;
			Timing.uDurationMicroseconds = m_pCurrentRef->getDurationInMicroseconds();
			Timing.pFrameCounter = m_pCurrentRef->getFrameCounter();

			Timing.clamp();

			m_uPriorTime = uActiveElapsed;

			m_pCurrentRef = m_pCurrentRef->update( m_sprite, Timing );
			if( m_pCurrentRef != nullptr ) {
				return;
			}

			if( m_cRepeats > 0 ) {
				--m_cRepeats;
				m_pCurrentRef = m_refs.head();
				return;
			}
		}

		while( m_refs.head() != nullptr ) {
			delete m_refs.head();
		}

		m_spriteLink.unlink();
		m_globalLink.unlink();
		delete this;
	}
	Void ActiveAction::runAll()
	{
		ActiveAction *pAction = G().activeActions.head();
		ActiveAction *pNext;
		while( pAction != NULL ) {
			pNext = pAction->m_globalLink.next();

			pAction->update();
			pAction = pNext;
		}
	}

	Action::Action()
	: m_cRefs( 0 )
	, m_uDurationMicroseconds( 0 )
	, m_pFrameCounter( NULL )
	{
	}
	Action::~Action()
	{
	}
	Void Action::retain()
	{
		++m_cRefs;
	}
	Void Action::release()
	{
		if( --m_cRefs > 0 ) {
			return;
		}

		delete this;
	}
	Void Action::setFrameCounter( CFrameCounter *pCounter )
	{
		m_pFrameCounter = pCounter;
	}
	CFrameCounter *Action::getFrameCounter() const
	{
		return m_pFrameCounter;
	}

	U64 Action::getDurationInMicroseconds() const
	{
		return m_uDurationMicroseconds;
	}
	F64 Action::getDurationInSeconds() const
	{
		return microsecondsToSeconds( m_uDurationMicroseconds );
	}

	Void Action::setDurationInMicroseconds( U64 uMicroseconds )
	{
		m_uDurationMicroseconds = uMicroseconds;
	}
	Void Action::setDurationInSeconds( F64 fSeconds )
	{
		m_uDurationMicroseconds = secondsToMicroseconds( fSeconds );
	}

	ActiveAction *Action::create( RSprite &InoutSprite )
	{
		ActiveAction *const pActiveAction = new ActiveAction( InoutSprite );
		if( !AX_VERIFY_MEMORY( pActiveAction ) ) {
			return nullptr;
		}

		pActiveAction->addAction( *this );

		return pActiveAction;
	}
	Void Action::start( RSprite &InoutSprite )
	{
		( Void )InoutSprite;
	}
	Void Action::update( RSprite &InoutSprite, const SActionTiming &InTiming )
	{
		( Void )InoutSprite;
		( Void )InTiming;
	}
	Void Action::complete( RSprite &InoutSprite )
	{
		( Void )InoutSprite;
	}

	SequenceAction::SequenceAction()
	: m_actions()
	{
	}
	SequenceAction::~SequenceAction()
	{
	}

	Void SequenceAction::addAction( Action *pAction )
	{
		if( !AX_VERIFY_NOT_NULL( pAction ) ) {
			return;
		}

		if( !AX_VERIFY_MSG( m_actions.addTail( pAction ) != m_actions.end(), "Failed to add action to sequence" ) ) {
			return;
		}

		setDurationInMicroseconds( getDurationInMicroseconds() + pAction->getDurationInMicroseconds() );
	}
	ActiveAction *SequenceAction::create( RSprite &InoutSprite )
	{
		if( !AX_VERIFY_MSG( m_actions.isUsed(), "Cannot create sequence with zero actions" ) ) {
			return nullptr;
		}

		ActiveAction *const pActiveAction = new ActiveAction( InoutSprite );
		if( !AX_VERIFY_MEMORY( pActiveAction ) ) {
			return nullptr;
		}

		for( Action *&pAction : m_actions ) {
			pActiveAction->addAction( *pAction );
		}

		return pActiveAction;
	}

	GroupAction::GroupAction()
	{
	}
	GroupAction::~GroupAction()
	{
	}

	Void GroupAction::addAction( Action *pAction )
	{
		if( !AX_VERIFY_NOT_NULL( pAction ) ) {
			return;
		}

		if( !AX_VERIFY_MSG( m_actions.addTail( pAction ) != m_actions.end(), "Failed to add action to group" ) ) {
			return;
		}

		if( getDurationInMicroseconds() < pAction->getDurationInMicroseconds() ) {
			setDurationInMicroseconds( pAction->getDurationInMicroseconds() );
		}
	}
	Void GroupAction::update( RSprite &InoutSprite, const SActionTiming &InTiming )
	{
		for( auto &pAction : m_actions ) {
			SActionTiming NewTiming = InTiming;
			NewTiming.uDurationMicroseconds = pAction->getDurationInMicroseconds();
			if( !NewTiming.pFrameCounter ) {
				NewTiming.pFrameCounter = pAction->getFrameCounter();
			}

			pAction->update( InoutSprite, NewTiming );
		}
	}

	CallbackAction::CallbackAction()
	: Action()
	, m_pfnStart( nullptr )
	, m_pfnComplete( nullptr )
	, m_pfnUpdate( nullptr )
	, m_pCallbackData( nullptr )
	{
	}
	CallbackAction::~CallbackAction()
	{
	}

	Void CallbackAction::setStartCallback( FnStart pfnStart )
	{
		m_pfnStart = pfnStart;
	}
	Void CallbackAction::setCompleteCallback( FnComplete pfnComplete )
	{
		m_pfnComplete = pfnComplete;
	}
	Void CallbackAction::setUpdateCallback( FnUpdate pfnUpdate )
	{
		m_pfnUpdate = pfnUpdate;
	}
	Void CallbackAction::setCallbackData( Void *pCallbackData )
	{
		m_pCallbackData = pCallbackData;
	}

	CallbackAction::FnStart CallbackAction::getStartCallback() const
	{
		return m_pfnStart;
	}
	CallbackAction::FnComplete CallbackAction::getCompleteCallback() const
	{
		return m_pfnComplete;
	}
	CallbackAction::FnUpdate CallbackAction::getUpdateCallback() const
	{
		return m_pfnUpdate;
	}
	Void *CallbackAction::getCallbackData() const
	{
		return m_pCallbackData;
	}

	Void CallbackAction::start( RSprite &InoutSprite )
	{
		if( !m_pfnStart ) {
			return;
		}

		m_pfnStart( m_pCallbackData, &InoutSprite );
	}
	Void CallbackAction::update( RSprite &InoutSprite, const SActionTiming &InTiming )
	{
		if( !m_pfnUpdate ) {
			return;
		}

		m_pfnUpdate( m_pCallbackData, &InoutSprite, InTiming.getProgress(), InTiming.getDelta() );
	}
	Void CallbackAction::complete( RSprite &InoutSprite )
	{
		if( !m_pfnComplete ) {
			return;
		}

		m_pfnComplete( m_pCallbackData, &InoutSprite );
	}

	//----------------------------------------------------------------------------//

#define EXPECT_ACTION(pAction_)\
	if( !AX_VERIFY_MSG( pAction_ != nullptr, "Expected valid action" ) )
#define EXPECT_CBACTION(pCBAction_)\
	if( !AX_VERIFY_MSG( pCBAction_ != nullptr, "Expected valid callback action" ) )

	DOLL_FUNC CallbackAction *DOLL_API gfx_newCallbackAction()
	{
		CallbackAction *const pCBAction = new CallbackAction();
		if( !AX_VERIFY_MEMORY( pCBAction ) ) {
			return nullptr;
		}

		return pCBAction;
	}
	DOLL_FUNC Action *DOLL_API gfx_deleteAction( Action *pAction )
	{
		if( pAction != nullptr ) {
			pAction->release();
		}

		return nullptr;
	}
	DOLL_FUNC Void DOLL_API gfx_setActionCounter( Action *pAction, CFrameCounter *pCounter )
	{
		EXPECT_ACTION( pAction ) {
			return;
		}

		pAction->setFrameCounter( pCounter );
	}
	DOLL_FUNC CFrameCounter *DOLL_API gfx_getActionCounter( const Action *pAction )
	{
		EXPECT_ACTION( pAction ) {
			return nullptr;
		}

		return pAction->getFrameCounter();
	}
	DOLL_FUNC F64 DOLL_API gfx_getActionDuration( const Action *pAction )
	{
		EXPECT_ACTION( pAction ) {
			return 0.0;
		}

		return pAction->getDurationInSeconds();
	}
	DOLL_FUNC Void DOLL_API gfx_setActionDuration( Action *pAction, F64 fDuration )
	{
		EXPECT_ACTION( pAction ) {
			return;
		}

		pAction->setDurationInSeconds( fDuration );
	}
	DOLL_FUNC Void DOLL_API gfx_setCallbackActionStartFunction( CallbackAction *pCBAction, CallbackAction::FnStart pfnStart )
	{
		EXPECT_CBACTION( pCBAction ) {
			return;
		}

		pCBAction->setStartCallback( pfnStart );
	}
	DOLL_FUNC Void DOLL_API gfx_setCallbackActionCompleteFunction( CallbackAction *pCBAction, CallbackAction::FnComplete pfnComplete )
	{
		EXPECT_CBACTION( pCBAction ) {
			return;
		}

		pCBAction->setCompleteCallback( pfnComplete );
	}
	DOLL_FUNC Void DOLL_API gfx_setCallbackActionFunction( CallbackAction *pCBAction, CallbackAction::FnUpdate pfnUpdate )
	{
		EXPECT_CBACTION( pCBAction ) {
			return;
		}

		pCBAction->setUpdateCallback( pfnUpdate );
	}
	DOLL_FUNC Void DOLL_API gfx_setCallbackActionData( CallbackAction *pCBAction, Void *pCallbackData )
	{
		EXPECT_CBACTION( pCBAction ) {
			return;
		}

		pCBAction->setCallbackData( pCallbackData );
	}
	DOLL_FUNC CallbackAction::FnStart DOLL_API gfx_getCallbackActionStartFunction( const CallbackAction *pCBAction )
	{
		EXPECT_CBACTION( pCBAction ) {
			return nullptr;
		}

		return pCBAction->getStartCallback();
	}
	DOLL_FUNC CallbackAction::FnComplete DOLL_API gfx_getCallbackActionCompleteFunction( const CallbackAction *pCBAction )
	{
		EXPECT_CBACTION( pCBAction ) {
			return nullptr;
		}

		return pCBAction->getCompleteCallback();
	}
	DOLL_FUNC CallbackAction::FnUpdate DOLL_API gfx_getCallbackActionFunction( const CallbackAction *pCBAction )
	{
		EXPECT_CBACTION( pCBAction ) {
			return nullptr;
		}

		return pCBAction->getUpdateCallback();
	}
	DOLL_FUNC Void *DOLL_API gfx_getCallbackActionData( const CallbackAction *pCBAction )
	{
		EXPECT_CBACTION( pCBAction ) {
			return nullptr;
		}

		return pCBAction->getCallbackData();
	}

}
