#pragma once

#include "../Core/Defs.hpp"

namespace doll
{

	typedef Bool( DOLL_API *FnStateInit )( Void *pStateData, Void *pUserData );
	typedef Void( DOLL_API *FnStateFini )( Void *pStateData, Void *pUserData );
	typedef Bool( DOLL_API *FnStateStep )( Void *pStateData, Void *pUserData );

	struct SState
	{
		FnStateInit pfnInit = nullptr;
		FnStateFini pfnFini = nullptr;
		FnStateStep pfnStep = nullptr;
		Void *      pData = nullptr;

		SState() = default;
		SState( const SState & ) = default;
		~SState() = default;

		SState &operator=( const SState & ) = default;
	};

	namespace detail
	{

		DOLL_FUNC Bool DOLL_API initIState( Void *, Void * );
		DOLL_FUNC Void DOLL_API finiIState( Void *, Void * );
		DOLL_FUNC Bool DOLL_API stepIState( Void *, Void * );

	}

	class IState
	{
	public:
		IState() {}
		virtual ~IState() {}

		virtual Bool init( Void *pUserData ) = 0;
		virtual Void fini( Void *pUserData ) = 0;
		virtual Bool step( Void *pUserData ) = 0;
	};

	namespace detail
	{

		class FuncMgr
		{
		public:
			inline FuncMgr()
			: m_states()
			{
			}
			inline ~FuncMgr()
			{
				popAll();
			}

			FuncMgr( const FuncMgr & ) = delete;
			FuncMgr &operator=( const FuncMgr & ) = delete;

			inline Void popAll( Void *pUserData = nullptr )
			{
				while( pop( pUserData ) ) {
					((void)0);
				}
			}
			inline Bool pop( Void *pUserData = nullptr )
			{
				if( m_states.isEmpty() ) {
					return false;
				}

				const SState state = m_states.popLast();
				if( state.pfnFini != nullptr ) {
					state.pfnFini( state.pData, pUserData );
				}

				return true;
			}

			inline Bool push( const SState &state, Void *pUserData = nullptr )
			{
				if( !m_states.append( state ) ) {
					return false;
				}

				if( state.pfnInit != nullptr && !state.pfnInit( state.pData, pUserData ) ) {
					m_states.removeLast();
					return false;
				}

				return true;
			}
			inline Bool push( IState *pState, Void *pUserData )
			{
				SState state;

				state.pfnInit = &detail::initIState;
				state.pfnFini = &detail::finiIState;
				state.pfnStep = &detail::stepIState;
				state.pData   = reinterpret_cast< Void * >( pState );

				return push( state, pUserData );
			}

			template< typename T >
			inline Void popAll( T *pUserData )
			{
				popAll( reinterpret_cast< Void * >( pUserData ) );
			}
			template< typename T >
			inline Bool pop( T *pUserData )
			{
				return pop( reinterpret_cast< Void * >( pUserData ) );
			}
			template< typename T >
			inline Bool push( const SState &state, T *pUserData )
			{
				return push( state, reinterpret_cast< Void * >( pUserData ) );
			}
			template< typename T >
			inline Bool push( IState *pState, T *pUserData )
			{
				return push( pState, pUserData );
			}

			inline TArr< SState > getStates() const
			{
				return m_states;
			}

		private:
			TMutArr< SState > m_states;
		};

	}

	class StateMgr: public detail::FuncMgr
	{
	public:
		inline StateMgr()
		: FuncMgr()
		{
		}
		inline ~StateMgr()
		{
		}

		StateMgr( const StateMgr & ) = delete;
		StateMgr &operator=( const StateMgr & ) = delete;

		inline Bool step( Void *pUserData = nullptr )
		{
			const TArr< SState > states = getStates();
			if( states.isEmpty() ) {
				return false;
			}

			const SState &state = states.last();
			if( state.pfnStep != nullptr && !state.pfnStep( state.pData, pUserData ) ) {
				return false;
			}

			return true;
		}
		template< typename T >
		inline Bool step( T *pUserData )
		{
			return step( reinterpret_cast< Void * >( pUserData ) );
		}
	};

	class ProcessMgr: public detail::FuncMgr
	{
	public:
		inline ProcessMgr()
		: FuncMgr()
		{
		}
		inline ~ProcessMgr()
		{
		}

		ProcessMgr( const ProcessMgr & ) = delete;
		ProcessMgr &operator=( const ProcessMgr & ) = delete;

		inline Bool step( Void *pUserData = nullptr )
		{
			for( const SState &state : getStates() ) {
				if( state.pfnStep != nullptr && !state.pfnStep( state.pData, pUserData ) ) {
					return false;
				}
			}

			return true;
		}
	};

}
