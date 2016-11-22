#pragma once

#include "../Core/Defs.hpp"
#include "../Util/Counter.hpp"

namespace doll
{

	class RSprite;

	class Action;
	class ActiveAction;

	/*
	===============================================================================

		ACTIONS
		-------
		Actions run on sprites for a specified duration of time. They are called
		automatically by the sprite for updates.

	===============================================================================
	*/

	// Timing information used for updating an action
	struct SActionTiming
	{
		U64            uPriorElapsedTimeMicroseconds;
		U64            uElapsedTimeMicroseconds;
		U64            uDurationMicroseconds;
		CFrameCounter *pFrameCounter;

		inline Void clamp()
		{
			if( uElapsedTimeMicroseconds > uDurationMicroseconds ) {
				uElapsedTimeMicroseconds = uDurationMicroseconds;
			}
		}

		inline F64 getProgress() const
		{
			const F64 fProgress = F64( uElapsedTimeMicroseconds )/F64( uDurationMicroseconds );
			if( pFrameCounter != nullptr ) {
				return pFrameCounter->getValue()*fProgress;
			}

			return fProgress;
		}
		inline U64 getDeltaMicroseconds() const
		{
			return uElapsedTimeMicroseconds - uPriorElapsedTimeMicroseconds;
		}
		inline F64 getDelta() const
		{
			return microsecondsToSeconds( getDeltaMicroseconds() );
		}
		inline Bool isComplete() const
		{
			return uElapsedTimeMicroseconds >= uDurationMicroseconds;
		}
	};

	// An action
	class Action
	{
	public:
		Action();

		Void retain();
		Void release();

		Void setFrameCounter( CFrameCounter *pCounter );
		CFrameCounter *getFrameCounter() const;

		U64 getDurationInMicroseconds() const;
		F64 getDurationInSeconds() const;

		Void setDurationInMicroseconds( U64 uMicroseconds );
		Void setDurationInSeconds( F64 fSeconds );

		// Create the action
		//
		// The default implementation provided creates an action from this action.
		// This can be overridden if that wouldn't work for the solution. (This is
		// done for SequenceActions.)
		virtual ActiveAction *create( RSprite & );

		// Start the action
		virtual Void start( RSprite & );
		// Update the current animation on the given sprite
		virtual Void update( RSprite &, const SActionTiming & );
		// Complete the action
		virtual Void complete( RSprite & );

	protected:
		virtual ~Action();

	private:
		U32            m_cRefs;
		U64            m_uDurationMicroseconds;
		CFrameCounter *m_pFrameCounter;
	};
	// Reference to an action (part of an active action)
	class ActionRef: public TPoolObject< ActionRef, kTag_Sprite >
	{
	public:
		typedef TIntrList< ActionRef > List;
		typedef TIntrLink< ActionRef > Link;

		enum class EPhase
		{
			Init,
			Step,
			Fini
		};

		ActionRef( List &activeActionRefs, Action &action );
		~ActionRef();

		// Update the referenced action on the given sprite with the provided
		// timing information.
		//
		// Returns a pointer to the next action to update in the list.
		ActionRef *update( RSprite &, const SActionTiming & );

		// Retrieve the starting time relative to the other actions in the sequence
		inline U64 getStartInMicroseconds() const
		{
			return m_uStartTime;
		}
		// Retrieve the duration of the referenced action
		inline U64 getDurationInMicroseconds() const
		{
			return m_action.getDurationInMicroseconds();
		}

		// Retrieve the action's frame counter
		inline CFrameCounter *getFrameCounter() const
		{
			return m_action.getFrameCounter();
		}

	protected:
		const U64 m_uStartTime;
		U32       m_cRepeats;
		Action &  m_action;
		EPhase    m_phase;
		Link      m_activeActionLink;
	};
	// An active action (to run on/for a given sprite)
	class ActiveAction: public TPoolObject< ActiveAction, kTag_Sprite >
	{
	public:
		typedef TIntrLink< ActiveAction > Link;
		typedef TIntrList< ActiveAction > List;

		static const U32 kRepeatForever = 0xFFFFFFFF;

		static Void runAll();

		ActiveAction( RSprite &sprite );
		~ActiveAction();

		Void addAction( Action &action );

		Void update();

		inline Void setRepeatCount( U32 cRepeats )
		{
			m_cRepeats = cRepeats;
		}
		inline Void setRepeatForever()
		{
			m_cRepeats = kRepeatForever;
		}
		inline U32 getRepeatCount() const
		{
			return m_cRepeats;
		}

	private:
		struct Glob
		{
			TIntrList< ActiveAction > activeActions;

			static Glob &get();

		private:
			Glob();
			~Glob();
		};

		ActionRef::List m_refs;
		ActionRef *     m_pCurrentRef;
		RSprite &       m_sprite;
		U32             m_cRepeats;
		U64             m_uStartTime;
		U64             m_uPriorTime;

		Link            m_spriteLink;
		Link            m_globalLink;

		inline static Glob &G()
		{
			return Glob::get();
		}
	};

	// Sequence action
	//
	// Runs all other actions in list sequentially
	class SequenceAction: public Action
	{
	public:
		SequenceAction();
		virtual ~SequenceAction();

		Void addAction( Action *pAction );
	
		virtual ActiveAction *create( RSprite & ) AX_OVERRIDE;

	private:
		TList< Action * > m_actions;
	};
	// Group action
	//
	// Runs all other actions in list at once
	class GroupAction: public Action
	{
	public:
		GroupAction();
		virtual ~GroupAction();

		Void addAction( Action *pAction );

		virtual Void update( RSprite &, const SActionTiming & ) AX_OVERRIDE;

	private:
		TList< Action * > m_actions;
	};

	// Callback action
	//
	// Runs user-provided functions
	class CallbackAction: public Action
	{
	public:
		typedef Void( DOLL_API *FnStart )( Void *pCallbackData, RSprite *pSprite );
		typedef Void( DOLL_API *FnComplete )( Void *pCallbackData, RSprite *pSprite );
		typedef Void( DOLL_API *FnUpdate )( Void *pCallbackData, RSprite *pSprite, F64 fProgress, F64 fDelta );

		CallbackAction();
		virtual ~CallbackAction();

		Void setStartCallback( FnStart pfnStart );
		Void setCompleteCallback( FnComplete pfnComplete );
		Void setUpdateCallback( FnUpdate pfnUpdate );
		Void setCallbackData( Void *pCallbackData );

		FnStart getStartCallback() const;
		FnComplete getCompleteCallback() const;
		FnUpdate getUpdateCallback() const;
		Void *getCallbackData() const;
	
		virtual Void start( RSprite & ) AX_OVERRIDE;
		virtual Void update( RSprite &, const SActionTiming & ) AX_OVERRIDE;
		virtual Void complete( RSprite & ) AX_OVERRIDE;

	private:
		FnStart    m_pfnStart;
		FnComplete m_pfnComplete;
		FnUpdate   m_pfnUpdate;
		Void *     m_pCallbackData;
	};

	//----------------------------------------------------------------------------//

	DOLL_FUNC CallbackAction *DOLL_API gfx_newCallbackAction();
	DOLL_FUNC Action *DOLL_API gfx_deleteAction( Action *pAction );
	DOLL_FUNC Void DOLL_API gfx_setActionCounter( Action *pAction, CFrameCounter *pCounter );
	DOLL_FUNC CFrameCounter *DOLL_API gfx_getActionCounter( const Action *pAction );
	DOLL_FUNC F64 DOLL_API gfx_getActionDuration( const Action *pAction );
	DOLL_FUNC Void DOLL_API gfx_setActionDuration( Action *pAction, F64 fDuration );
	DOLL_FUNC Void DOLL_API gfx_setCallbackActionStartFunction( CallbackAction *pCBAction, CallbackAction::FnStart pfnStart );
	DOLL_FUNC Void DOLL_API gfx_setCallbackActionCompleteFunction( CallbackAction *pCBAction, CallbackAction::FnComplete pfnComplete );
	DOLL_FUNC Void DOLL_API gfx_setCallbackActionFunction( CallbackAction *pCBAction, CallbackAction::FnUpdate pfnUpdate );
	DOLL_FUNC Void DOLL_API gfx_setCallbackActionData( CallbackAction *pCBAction, Void *pCallbackData );
	DOLL_FUNC CallbackAction::FnStart DOLL_API gfx_getCallbackActionStartFunction( const CallbackAction *pCBAction );
	DOLL_FUNC CallbackAction::FnComplete DOLL_API gfx_getCallbackActionCompleteFunction( const CallbackAction *pCBAction );
	DOLL_FUNC CallbackAction::FnUpdate DOLL_API gfx_getCallbackActionFunction( const CallbackAction *pCBAction );
	DOLL_FUNC Void *DOLL_API gfx_getCallbackActionData( const CallbackAction *pCBAction );

}
