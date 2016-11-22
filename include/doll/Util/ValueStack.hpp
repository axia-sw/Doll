#pragma once

#include "../Core/Defs.hpp"
#include "CountDivs.hpp"

namespace doll
{

	namespace detail
	{

		typedef unsigned long long int ValueStoreType;

		template< typename TArrType >
		struct TValueStackMutArrAdapter
		{
			static inline Bool isEmpty( const TArrType &x )
			{
				return x.isEmpty();
			}
			static inline Bool append( TArrType &x, ValueStoreType v )
			{
				return x.append( v );
			}
			static inline Void removeLast( TArrType &x )
			{
				return x.removeLast();
			}
			static inline ValueStoreType &last( TArrType &x )
			{
				return x.last();
			}
			static inline const ValueStoreType &last( const TArrType &x )
			{
				return x.last();
			}
		};

	}

	template< unsigned tHighestValue, class TDynArr = TMutArr< detail::ValueStoreType > >
	class TValueStack
	{
	public:
		typedef detail::ValueStoreType ItemType;
		typedef unsigned               SizeType;
		typedef unsigned               ValueType;
		typedef TDynArr                DynArrType;

		static const unsigned kValueRange    = tHighestValue;
		static const unsigned kValuesPerItem = TCountDivs< kValueRange, ~ItemType( 0 ) >::count;

		inline TValueStack()
		: m_cValues( 0 )
		, m_mainValue( 0 )
		, m_extraValues()
		{
		}
		inline ~TValueStack()
		{
		}

		inline Bool push( ValueType value )
		{
			if( value >= kValueRange || m_cValues == ~SizeType( 0 ) ) {
				return false;
			}

			if( ( m_cValues + 1 )/kValuesPerItem > m_cValues/kValuesPerItem ) {
				if( !appendExtra( 0 ) ) {
					return false;
				}
			}

			ItemType &n = last();
			n *= kValueRange;
			n += value;

			++m_cValues;

			return true;
		}
		inline Bool pop( ValueType *pDstValue = nullptr )
		{
			if( !m_cValues ) {
				return false;
			}

			detail::ValueStoreType &n = last();

			if( pDstValue != nullptr ) {
				*pDstValue = n%kValueRange;
			}
			n /= kValueRange;

			if( ( m_cValues - 1 )/kValuesPerItem < m_cValues/kValuesPerItem ) {
				removeLastExtra();
			}

			--m_cValues;
			return true;
		}
		inline Bool pop( ValueType &dstValue )
		{
			return pop( &dstValue );
		}

		inline ValueType top() const
		{
			AX_ASSERT( isUsed() );

			return last()%kValueRange;
		}

		inline Bool isTop( ValueType x ) const
		{
			if( isEmpty() ) {
				return false;
			}

			return last()%kValueRange == x;
		}

		inline SizeType num() const
		{
			return m_cValues;
		}
		inline SizeType len() const
		{
			return m_cValues;
		}
		inline Bool isEmpty() const
		{
			return m_cValues == 0;
		}
		inline Bool isUsed() const
		{
			return m_cValues != 0;
		}

	private:
		typedef detail::TValueStackMutArrAdapter< TDynArr > MutArrAdapter;

		SizeType              m_cValues;
		ItemType              m_mainValue;
		DynArrType            m_extraValues;

		inline Bool isExtraEmpty() const
		{
			return MutArrAdapter::isEmpty( m_extraValues );
		}
		inline Bool appendExtra( detail::ValueStoreType v )
		{
			return MutArrAdapter::append( m_extraValues, v );
		}
		inline Void removeLastExtra()
		{
			MutArrAdapter::removeLast( m_extraValues );
		}
		inline detail::ValueStoreType &lastExtra()
		{
			return MutArrAdapter::last( m_extraValues );
		}
		inline detail::ValueStoreType lastExtra() const
		{
			return MutArrAdapter::last( m_extraValues );
		}

		inline detail::ValueStoreType &last()
		{
			return isExtraEmpty() ? m_mainValue : lastExtra();
		}
		inline detail::ValueStoreType last() const
		{
			return isExtraEmpty() ? m_mainValue : lastExtra();
		}
	};

}
