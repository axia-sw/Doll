#include "doll/Snd/ChannelUtil.hpp"

#include "doll/Math/Basic.hpp"

namespace doll
{

	DOLL_FUNC U32 DOLL_API snd_stringToChannelMask( const Str &src )
	{
		Str s( src );

		s.trim();

		const unsigned char *const e = ( const unsigned char * )s.getEnd();

		U32 uChannelMask = 0;
		while( s.isUsed() ) {
			const unsigned char *p = ( const unsigned char * )s.get();
			while( p < e && *p > ' ' ) {
				++p;
			}

			const Str t( s.get(), ( const char * )e );
			s = s.skip( t.len() ).skipWhitespace();

			const F32 f = abs( ( F32 )t.toFloat() );
			if( f >= 1.0f ) {
				const U32 n = ( U32 )f;
				const U32 w = ( ( U32 )( f*10 ) )%10;

				const U32 channelMasks[ 10 ] = {
					kChannelsMono,
					kChannelsMono, kChannelsStereo, kChannelsSurround, kChannelsQuad, kChannels5,
					kChannels6, kChannels7, kChannels8, kChannels8
				};

				uChannelMask |= channelMasks[ n%10 ];
				if( w >= 1 ) {
					uChannelMask |= kChannelLowFrequency;
				}

				continue;
			}

			if( t.caseCmp( "L" ) || t.caseCmp( "left" ) || t.caseCmp( "frontLeft" ) ) {
				uChannelMask |= kChannelFrontLeft;
				continue;
			}

			if( t.caseCmp( "R" ) || t.caseCmp( "right" ) || t.caseCmp( "frontRight" ) ) {
				uChannelMask |= kChannelFrontRight;
				continue;
			}

			if( t.caseCmp( "C" ) || t.caseCmp( "center" ) || t.caseCmp( "frontCenter" ) ) {
				uChannelMask |= kChannelFrontCenter;
				continue;
			}

			if( t.caseCmp( "S" ) || t.caseCmp( "sub" ) || t.caseCmp( "subwoofer" ) || t.caseCmp( "lowFrequency" ) || t.caseCmp( "lowFreq" ) || t.caseCmp( "lfq" ) ) {
				uChannelMask |= kChannelLowFrequency;
				continue;
			}

			if( t.caseCmp( "Lb" ) || t.caseCmp( "backLeft" ) ) {
				uChannelMask |= kChannelBackLeft;
				continue;
			}

			if( t.caseCmp( "Rb" ) || t.caseCmp( "backRight" ) ) {
				uChannelMask |= kChannelBackRight;
				continue;
			}

			if( t.caseCmp( "Lf" ) || t.caseCmp( "leftFront" ) || t.caseCmp( "leftFrontCenter" ) || t.caseCmp( "leftOfCenter" ) || t.caseCmp( "frontLeftCenter" ) ) {
				uChannelMask |= kChannelFrontLeftCenter;
				continue;
			}
			if( t.caseCmp( "Rf" ) || t.caseCmp( "rightFront" ) || t.caseCmp( "rightFrontCenter" ) || t.caseCmp( "rightOfCenter" ) || t.caseCmp( "frontRightCenter" ) ) {
				uChannelMask |= kChannelFrontRightCenter;
				continue;
			}

			if( t.caseCmp( "Cb" ) || t.caseCmp( "centerBack" ) || t.caseCmp( "backCenter" ) ) {
				uChannelMask |= kChannelBackCenter;
				continue;
			}

			if( t.caseCmp( "Ls" ) || t.caseCmp( "leftSide" ) || t.caseCmp( "sideLeft" ) ) {
				uChannelMask |= kChannelSideLeft;
				continue;
			}

			if( t.caseCmp( "Rs" ) || t.caseCmp( "rightSide" ) || t.caseCmp( "sideRight" ) ) {
				uChannelMask |= kChannelSideRight;
				continue;
			}

			if( t.caseCmp( "Lt" ) || t.caseCmp( "leftTop" ) || t.caseCmp( "topLeft" ) || t.caseCmp( "frontLeftTop" ) || t.caseCmp( "topFrontLeft" ) || t.caseCmp( "topLeftFront" ) ) {
				uChannelMask |= kChannelTopFrontLeft;
				continue;
			}
			if( t.caseCmp( "Rt" ) || t.caseCmp( "rightTop" ) || t.caseCmp( "topRight" ) || t.caseCmp( "frontRightTop" ) || t.caseCmp( "topFrontRight" ) || t.caseCmp( "topRightFront" ) ) {
				uChannelMask |= kChannelTopFrontRight;
				continue;
			}
			if( t.caseCmp( "Ctf" ) || t.caseCmp( "centerTopFront" ) || t.caseCmp( "centerFrontTop" ) || t.caseCmp( "topFrontCenter" ) || t.caseCmp( "frontTopCenter" ) ) {
				uChannelMask |= kChannelTopFrontCenter;
				continue;
			}
			if( t.caseCmp( "Ct" ) || t.caseCmp( "centerTop" ) || t.caseCmp( "topCenter" ) ) {
				uChannelMask |= kChannelTopCenter;
				continue;
			}

			if( t.caseCmp( "Ltb" ) || t.caseCmp( "leftTopBack" ) || t.caseCmp( "topLeftBack" ) || t.caseCmp( "backLeftTop" ) || t.caseCmp( "backTopLeft" ) || t.caseCmp( "topBackLeft" ) ) {
				uChannelMask |= kChannelTopBackLeft;
				continue;
			}
			if( t.caseCmp( "Rtb" ) || t.caseCmp( "rightTopBack" ) || t.caseCmp( "topRightBack" ) || t.caseCmp( "backRightTop" ) || t.caseCmp( "backTopRight" ) || t.caseCmp( "topBackRight" ) ) {
				uChannelMask |= kChannelTopBackRight;
				continue;
			}
			if( t.caseCmp( "Ctb" ) || t.caseCmp( "centerTopBack" ) || t.caseCmp( "topCenterBack" ) || t.caseCmp( "backCenterTop" ) || t.caseCmp( "backTopCenter" ) || t.caseCmp( "topBackCenter" ) ) {
				uChannelMask |= kChannelTopBackCenter;
				continue;
			}

			// FIXME: Issue warning for unknown channel token
		}

		return uChannelMask;
	}
	DOLL_FUNC UPtr DOLL_API snd_channelMaskToString( char *pszDst, UPtr cDstMax, U32 uChannelMask )
	{
		// FIXME: Give this value a name
		if( !takeFlag( uChannelMask, 0x80000000U ) ) {
			switch( uChannelMask ) {
			case kChannels8Point1: return axstr_cpy( pszDst, cDstMax, "8.1" );
			case kChannels7Point1: return axstr_cpy( pszDst, cDstMax, "7.1" );
			case kChannels6Point1: return axstr_cpy( pszDst, cDstMax, "6.1" );
			case kChannels5Point1: return axstr_cpy( pszDst, cDstMax, "5.1" );
			case kChannels4Point1: return axstr_cpy( pszDst, cDstMax, "4.1" );
			case kChannels2Point1: return axstr_cpy( pszDst, cDstMax, "2.1" );

			case kChannels8:       return axstr_cpy( pszDst, cDstMax, "8.0" );
			case kChannels7:       return axstr_cpy( pszDst, cDstMax, "7.0" );
			case kChannels6:       return axstr_cpy( pszDst, cDstMax, "6.0" );
			case kChannels5:       return axstr_cpy( pszDst, cDstMax, "5.0" );
			case kChannelsQuad:    return axstr_cpy( pszDst, cDstMax, "quad" );
			case kChannelsSurround:return axstr_cpy( pszDst, cDstMax, "surround" );
			case kChannelsStereo:  return axstr_cpy( pszDst, cDstMax, "stereo" );
			case kChannelsMono:    return axstr_cpy( pszDst, cDstMax, "mono" );

			case 0:                return axstr_cpy( pszDst, cDstMax, "default" );

			default:
				break;
			}
		}

		static const char *const pszChannels[] = {
			"L", "R", "C", "S", "Lb", "Rb", "Lf", "Rf", "Cb", "Ls", "Rs",
			"Lt", "Rt", "Ctf", "Ct", "Ltb", "Rtb", "Ctb"
		};
		static const U32 cChannels = ( U32 )arraySize( pszChannels );

		const UPtr cOrgMax = cDstMax;

		Bool bAddSpace = false;
		for( U32 i = 0; i < cChannels; ++i ) {
			if( !takeFlag( uChannelMask, 1UL<<i ) ) {
				continue;
			}

			if( bAddSpace ) {
				axstr_stream( &pszDst, &cDstMax, " " );
			} else {
				bAddSpace = true;
			}

			axstr_stream( &pszDst, &cDstMax, pszChannels[ i ] );
		}

		return cOrgMax - cDstMax;
	}

}
