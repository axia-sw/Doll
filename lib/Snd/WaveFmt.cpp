#define DOLL_TRACE_FACILITY doll::kLog_SndFile

#include "doll/Snd/WaveFmt.hpp"

#include "doll/Core/Logger.hpp"

#include "doll/Util/ByteSwap.hpp"

namespace doll
{

#define CHECK(Expr_,Msg_)\
		do {\
			if( !( Expr_ ) ) {\
				g_ErrorLog(filename) += (Msg_);\
				return false;\
			}\
		} while(0)

#define F(...) (axspf(szFmtBuf,__VA_ARGS__),szFmtBuf)
#define INRANGE(V_,L_,H_) ((V_)>=(L_)&&(V_)<=(H_))

	Bool SWaveFormat::isValid( EWaveTag asTag, Str filename ) const
	{
		char szFmtBuf[ 512 ];

		switch( asTag ) {
		case kWaveTagPCM:
			// Bits per sample: 8, 16, 24, or 32
			CHECK( INRANGE( cSampleBits/8, 1, 4 )&&cSampleBits%8==0, F( "[PCM] Bits per sample must be 8, 16, 24, or 32; have %u", +cSampleBits ) );

			// Block alignment
			CHECK( uBlockAlign == cChannels*cSampleBits/8, F( "[PCM] Block alignment (%u) is not equal to channel count (%u) times bytes per sample (%u) (which is %u)", +uBlockAlign, +cChannels, cSampleBits/8, cChannels*cSampleBits/8 ) );

			// Average bytes per second
			CHECK( cAvgBytesHz == cSamplesHz*uBlockAlign, F( "[PCM] Average bytes per second (%u) is not equal to the samples per second (%u) times block alignment (%u) (which is %u)", +cAvgBytesHz, +cSamplesHz, +uBlockAlign ) );

			// Extensions
			if( tag != asTag ) {
				// Valid bits per sample
				const U16 cSampleValidBits = extra.ex.samples.cSampleValidBits;
				// Known valid bits
				static const U16 cKnownValidBits[] = { 0, 8, 16, 20, 24, 32 };

				// Check the valid bits per sample
				CHECK( isOneOf( cSampleValidBits, cKnownValidBits ), F("[PCM-Ex] Valid bits per sample (%u) must be 0, 8, 16, 20, 24, or 32", cSampleValidBits) );

				// Check that we're contained by the actual bits per sample
				CHECK( cSampleValidBits <= cSampleBits, F("[PCM-Ex] Valid bits per sample (%u) must not exceed the allocated bits per sample (%u)", cSampleValidBits, cSampleBits) );
			}

			break;

		case kWaveTagFloat:
			CHECK( false, "[Float] Format not yet supported" );
			break;

		case kWaveTagADPCM:
			CHECK( tag == asTag, "[ADPCM] Cannot be used within the extensible format" );

			CHECK( INRANGE( cChannels, 1, 2 ), F( "[ADPCM] Only mono and stereo are supported; have %u channels", cChannels ) );

			CHECK( cSampleBits == 4, F( "[ADPCM] Must use 4 bits per sample; have %u", cSampleBits ) );

			CHECK( cExtraBytes == 32, F( "[ADPCM] Must have 32 bytes for extra field; have %u", cExtraBytes ) );

			{
#define N (U16)(S16)-
				static const U16 coefs1[ 7 ] = { 256, 512, 0, 192, 240, 460, 392 };
				static const U16 coefs2[ 7 ] = { 0, N 256, 0, 64, 0, N 208, N 232 };
#undef N

				const SWaveADPCM &x = extra.adpcm;

				CHECK( x.cCoefficients == 7, F( "[ADPCM] Must have 7 coefficients; have %u", x.cCoefficients ) );

				for( U16 i = 0; i < 7; ++i ) {
					CHECK( x.coefs[i][0]==coefs1[i] && x.coefs[i][1]==coefs2[i], F("[ADPCM] Found nonstandard coefficients (%u,%u) where standard coefficients (%u,%u) were expected", x.coefs[i][0],x.coefs[i][1], coefs1[i],coefs2[i]) );
				}

				CHECK( INRANGE(x.cBlockSamples, SWaveADPCM::kMinBlockSamples, SWaveADPCM::kMaxBlockSamples), F( "[ADPCM] Must have between %u and %u block samples; have %u", SWaveADPCM::kMinBlockSamples, SWaveADPCM::kMaxBlockSamples, x.cBlockSamples ) );

				CHECK( cChannels != 1 || x.cBlockSamples % 2 == 0, F("[ADPCM] Mono encoding must have even number of samples per block; have %u", x.cBlockSamples) );

				const U32 cHeaderBytes = 7*cChannels;
				const U32 cFrameBits = 4*cChannels;
				const U32 cBlockPcmFrames = ( uBlockAlign - cHeaderBytes )*8/cFrameBits + 2;

				CHECK( x.cBlockSamples == cBlockPcmFrames, F("[ADPCM] Samples per block must be %u (channels=%u, block alignment=%u); is %u", cBlockPcmFrames, cChannels, uBlockAlign, x.cBlockSamples) );
			}

			break;

		case kWaveTagWMA2:
		case kWaveTagWMA3:
			CHECK( cSampleBits == 16, F("[xWMA] Bits per sample must be 16; is %u", cSampleBits) );
			CHECK( uBlockAlign != 0, "[xWMA] Block alignment must be non-zero" );
			CHECK( cAvgBytesHz != 0, "[xWMA] Average bytes per second must be non-zero" );

			break;

		case kWaveTagXMA2:
			CHECK( tag == asTag, "[XMA2] Cannot be used within the extensible format" );

			CHECK( uBlockAlign == cChannels*2, F("[XMA2] Block alignment must be equal to the number of channels (%u) times 2; is %u", cChannels, uBlockAlign) );
			CHECK( cSampleBits == 16, F("[XMA2] Bits per sample must be sixteen; is %u", cSampleBits) );
			CHECK( cExtraBytes == sizeof( SWaveXMA2 ), F("[XMA2] Extra data field must be %u bytes in length; is %u", sizeof(SWaveXMA2), cExtraBytes) );

			{
				const SWaveXMA2 &x = extra.xma2;

				static const U32 kMaxReadbuffer = 0x007FF800;

				CHECK( x.uEncVer >= 3, F("[XMA2] Encoder version must be 3 or higher; is %u", x.uEncVer) );
				CHECK( x.cBlocks != 0, F("[XMA2] Block count must be non-zero") );
				CHECK( INRANGE(x.cBlockBytes, 1, kMaxReadbuffer), F("[XMA2] Block size (0x%.8X) exceeds maximum read buffer size (0x%.8X)", x.cBlockBytes, kMaxReadbuffer) );

				if( x.uChannelMask != 0 ) {
					const U16 cXMA2Channels = countBits( x.uChannelMask );

					CHECK( cXMA2Channels == cChannels, F("[XMA2] %u channel%s specified in mask (0x%.8X), but header specifies %u channel%s", cXMA2Channels, cXMA2Channels==1?"":"s", x.uChannelMask, cChannels, cChannels==1?"":"s") );
				}

				const U16 cChkStreams = ( cChannels + 1 )/2;
				CHECK( x.cStreams == cChkStreams, F("[XMA2] Must have %u stream%s (channels=%u); have %u stream%s", cChkStreams, cChkStreams==1?"":"s", cChannels, x.cStreams, x.cStreams==1?"":"s") );

				CHECK( x.uPlaySample + x.cPlaySamples <= x.cEncodedSamples, F("[XMA2] Play region (%u + %u) goes beyond last sample (%u)", x.uPlaySample, x.cPlaySamples, x.cEncodedSamples) );
				CHECK( x.uLoopSample + x.cLoopSamples <= x.cEncodedSamples, F("[XMA2] Loop region (%u + %u) goes beyond last sample (%u)", x.uLoopSample, x.cLoopSamples, x.cEncodedSamples) );
			}

			break;

		default:
			CHECK( false, F("Invalid or unknown tag (0x%.4X)", asTag) );
		}

		return true;
	}

	Bool SWaveFormat::isValid( Str filename ) const
	{
		char szFmtBuf[512];

		// Check the channel count
		CHECK( INRANGE(cChannels,1,kMaxChannels), F("Must have between 1 and %u channels; have %u channels", +kMaxChannels, +cChannels) );

		// Check the sample rate
		CHECK( INRANGE(cSamplesHz,kMinSampleHz,kMaxSampleHz), F("Sample rate must be between %uHz and %uHz; rate is %uHz", kMinSampleHz, kMaxSampleHz, cSamplesHz) );

		// Spoofed check?
		if( tag == kWaveTagEx ) {
			// Extra bytes must match
			CHECK( cExtraBytes == sizeof( SWaveEx ), F("[WavEx] Extra bytes must be %u; is %u", sizeof(SWaveEx), cExtraBytes) );

			// Subformat
			const auto &f = extra.ex.subformat;

			// Check that the subformat is one we expect
			CHECK( f.isKnown(), F("[WavEx] Unknown subformat: {%.8X-%.4X-%.4X-%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X}", f.a, f.b[0],f.b[1], f.c[0],f.c[1],f.c[2],f.c[3],f.c[4],f.c[5],f.c[6],f.c[7]) );

			// Check that the number of channels specified matches
			if( extra.ex.uChannelMask != 0 ) {
				const U16 cExChannels = countBits( extra.ex.uChannelMask );
				CHECK( cExChannels == cChannels, F("[WavEx] Channel mask (0x%.8X) has %u channel%s, but header requires %u channel%s", extra.ex.uChannelMask, cExChannels, cExChannels==1?"":"s", cChannels, cChannels==1?"":"s") );
			}

			// Spoofed check (uses the tag from the subformat)
			return isValid( extra.ex.getTag(), filename );
		}

		// Normal check
		return isValid( tag, filename );
	}
#undef F
#undef CHECK

	SWaveFormat &SWaveFormat::loadSwapMe()
	{
		tag = (EWaveTag)getLE(U32(tag));

		cChannels   = getLE(cChannels);
		cSamplesHz  = getLE(cSamplesHz);
		cAvgBytesHz = getLE(cAvgBytesHz);
		uBlockAlign = getLE(uBlockAlign);
		cSampleBits = getLE(cSampleBits);

		// ### TODO ###
		return *this;
	}
	SWaveFormat &SWaveFormat::saveSwapMe()
	{
		tag = (EWaveTag)setLE(U32(tag));

		cChannels   = setLE(cChannels);
		cSamplesHz  = setLE(cSamplesHz);
		cAvgBytesHz = setLE(cAvgBytesHz);
		uBlockAlign = setLE(uBlockAlign);
		cSampleBits = setLE(cSampleBits);

		// ### TODO ###
		return *this;
	}

}

