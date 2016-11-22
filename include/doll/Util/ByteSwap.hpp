#pragma once

#include "../Core/Defs.hpp"

namespace doll
{

	static inline U16 bswap( U16 x )
	{
		return (x&0xFF)<<8 | x>>8;
	}

	static inline U32 bswap( U32 x )
	{
		return (x&0xFF)<<24 | (x&0xFF00)<<8 | ((x>>8)&0xFF00) | ((x>>24)&0xFF);
	}

	static inline U64 bswap( U64 x )
	{
		return
			U64(bswap(U32(x&0xFFFFFFFF)))<<32 |
			U64(bswap(U32(x>>32)));
	}

	static inline U16 getBE( U16 x )
	{
		const U8 *const p = ( const U8 * )&x;
		return U16(p[0])<<8 | U16(p[1]);
	}
	static inline U32 getBE( U32 x )
	{
		const U8 *const p = ( const U8 * )&x;
		return U32(p[0])<<24 | U32(p[1])<<16 | U32(p[2])<<8 | U32(p[3]);
	}
	static inline U64 getBE( U64 x )
	{
		const U8 *const p = ( const U8 * )&x;
		return
			U64(p[0])<<56 | U64(p[1])<<48 | U64(p[2])<<40 | U64(p[3])<<32 |
			U64(p[4])<<24 | U64(p[5])<<16 | U64(p[6])<< 8 | U64(p[7]);
	}

	static inline U16 setBE( U16 x )
	{
		union { U8 a[2]; U16 v; } t;
		t.a[0] = U8(x&0xFF);
		t.a[1] = U8(x>>8);
		return t.v;
	}
	static inline U32 setBE( U32 x )
	{
		union { U8 a[4]; U32 v; } t;
		t.a[0] = U8((x>> 0)&0xFF);
		t.a[1] = U8((x>> 8)&0xFF);
		t.a[2] = U8((x>>16)&0xFF);
		t.a[3] = U8((x>>24)&0xFF);
		return t.v;
	}
	static inline U64 setBE( U64 x )
	{
		union { U8 a[8]; U64 v; } t;
		t.a[0] = U8((x>> 0)&0xFF);
		t.a[1] = U8((x>> 8)&0xFF);
		t.a[2] = U8((x>>16)&0xFF);
		t.a[3] = U8((x>>24)&0xFF);
		t.a[4] = U8((x>>32)&0xFF);
		t.a[5] = U8((x>>40)&0xFF);
		t.a[6] = U8((x>>48)&0xFF);
		t.a[7] = U8((x>>56)&0xFF);
		return t.v;
	}

	static inline U16 getLE( U16 x )
	{
		const U8 *const p = ( const U8 * )&x;
		return U16(p[1])<<8 | U16(p[0]);
	}
	static inline U32 getLE( U32 x )
	{
		const U8 *const p = ( const U8 * )&x;
		return U32(p[3])<<24 | U32(p[2])<<16 | U32(p[1])<<8 | U32(p[0]);
	}
	static inline U64 getLE( U64 x )
	{
		const U8 *const p = ( const U8 * )&x;
		return
			U64(p[7])<<56 | U64(p[6])<<48 | U64(p[5])<<40 | U64(p[4])<<32 |
			U64(p[3])<<24 | U64(p[2])<<16 | U64(p[1])<< 8 | U64(p[0]);
	}

	static inline U16 setLE( U16 x )
	{
		union { U8 a[2]; U16 v; } t;
		t.a[0] = U8(x>>8);
		t.a[1] = U8(x&0xFF);
		return t.v;
	}
	static inline U32 setLE( U32 x )
	{
		union { U8 a[4]; U32 v; } t;
		t.a[0] = U8((x>>24)&0xFF);
		t.a[1] = U8((x>>16)&0xFF);
		t.a[2] = U8((x>> 8)&0xFF);
		t.a[3] = U8((x>> 0)&0xFF);
		return t.v;
	}
	static inline U64 setLE( U64 x )
	{
		union { U8 a[8]; U64 v; } t;
		t.a[0] = U8((x>>56)&0xFF);
		t.a[1] = U8((x>>48)&0xFF);
		t.a[2] = U8((x>>40)&0xFF);
		t.a[3] = U8((x>>32)&0xFF);
		t.a[4] = U8((x>>24)&0xFF);
		t.a[5] = U8((x>>16)&0xFF);
		t.a[6] = U8((x>> 8)&0xFF);
		t.a[7] = U8((x>> 0)&0xFF);
		return t.v;
	}

	static inline S16 getBE( S16 x ) { return getBE( U16( x ) ); }
	static inline S32 getBE( S32 x ) { return getBE( U32( x ) ); }
	static inline S64 getBE( S64 x ) { return getBE( U64( x ) ); }
	static inline S16 setBE( S16 x ) { return setBE( U16( x ) ); }
	static inline S32 setBE( S32 x ) { return setBE( U32( x ) ); }
	static inline S64 setBE( S64 x ) { return setBE( U64( x ) ); }

	static inline S16 getLE( S16 x ) { return getLE( U16( x ) ); }
	static inline S32 getLE( S32 x ) { return getLE( U32( x ) ); }
	static inline S64 getLE( S64 x ) { return getLE( U64( x ) ); }
	static inline S16 setLE( S16 x ) { return setLE( U16( x ) ); }
	static inline S32 setLE( S32 x ) { return setLE( U32( x ) ); }
	static inline S64 setLE( S64 x ) { return setLE( U64( x ) ); }

	static inline U8 getBE( U8 x ) { return x; }
	static inline S8 getBE( S8 x ) { return x; }
	static inline U8 setBE( U8 x ) { return x; }
	static inline S8 setBE( S8 x ) { return x; }

	static inline U8 getLE( U8 x ) { return x; }
	static inline S8 getLE( S8 x ) { return x; }
	static inline U8 setLE( U8 x ) { return x; }
	static inline S8 setLE( S8 x ) { return x; }

}
