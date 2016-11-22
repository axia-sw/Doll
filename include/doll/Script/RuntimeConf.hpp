#pragma once

#include "../Core/Defs.hpp"

namespace doll { namespace script {

	enum class EPlatform
	{
		// Doll's 64-bit virtual machine
		DollVM64,

		// Default platform to use
		Default = DollVM64
	};

	inline Bool isPlatform64Bit( EPlatform p )
	{
		switch( p ) {
		case EPlatform::DollVM64:
			return true;
		}

		return false;
	}

	class RuntimeConfiguration
	{
	public:
		RuntimeConfiguration( EPlatform plat = EPlatform::Default )
		: m_defaultAlignment( getPlatformDefaultAlignment( plat ) )
		, m_pointerWidth( getPlatformPointerWidth( plat ) )
		, m_registerWidth( getPlatformRegisterWidth( plat ) )
		{
		}

		U32 getDefaultAlignment() const
		{
			return m_defaultAlignment;
		}
		U32 getPointerWidth() const
		{
			return m_pointerWidth;
		}
		U32 getRegisterWidth() const
		{
			return m_registerWidth;
		}

	private:
		U32 m_defaultAlignment;
		U32 m_pointerWidth;
		U32 m_registerWidth;

		static constexpr U32 getPlatformDefaultAlignment( EPlatform plat )
		{
			return
				(
					plat == EPlatform::DollVM64 ? 16 :
					8
				);
		}
		static constexpr U32 getPlatformPointerWidth( EPlatform plat )
		{
			return
				(
					plat == EPlatform::DollVM64 ? 64 :
					32
				);
		}
		static constexpr U32 getPlatformRegisterWidth( EPlatform plat )
		{
			return
				(
					plat == EPlatform::DollVM64 ? 64 :
					32
				);
		}
	};

}}
