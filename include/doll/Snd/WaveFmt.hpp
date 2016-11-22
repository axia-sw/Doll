#pragma once

#include "../Core/Defs.hpp"

namespace doll
{

	enum EWaveTag: U16
	{
		// Pulse Code Modulation (standard uncompressed)
		kWaveTagPCM   = 0x0001,
		// Adaptive Differential PCM
		kWaveTagADPCM = 0x0002,
		// IEEE-754 32-bit floating-point (uncompressed)
		kWaveTagFloat = 0x0003,
		// Windows Media Audio
		kWaveTagWMA2  = 0x0161,
		// Windows Media Audio Pro
		kWaveTagWMA3  = 0x0162,
		// XMA2
		kWaveTagXMA2  = 0x0166,
		// Extensible
		kWaveTagEx    = 0xFFFE
	};
	enum EChannelBits: U32
	{
		kChannelFrontLeft        = 0x000001,
		kChannelFrontRight       = 0x000002,
		kChannelFrontCenter      = 0x000004,
		kChannelLowFrequency     = 0x000008,
		kChannelBackLeft         = 0x000010,
		kChannelBackRight        = 0x000020,
		kChannelFrontLeftCenter  = 0x000040,
		kChannelFrontRightCenter = 0x000080,
		kChannelBackCenter       = 0x000100,
		kChannelSideLeft         = 0x000200,
		kChannelSideRight        = 0x000400,
		kChannelTopCenter        = 0x000800,
		kChannelTopFrontLeft     = 0x001000,
		kChannelTopFrontCenter   = 0x002000,
		kChannelTopFrontRight    = 0x004000,
		kChannelTopBackLeft      = 0x008000,
		kChannelTopBackCenter    = 0x010000,
		kChannelTopBackRight     = 0x020000,

		kChannelsMono            = kChannelFrontCenter,
		kChannelsStereo          = kChannelFrontLeft | kChannelFrontRight,
		kChannels2Point1         = kChannelsStereo | kChannelLowFrequency,
		kChannelsSurround        = kChannelsStereo | kChannelFrontCenter | kChannelBackCenter,
		kChannelsQuad            = kChannelsStereo | kChannelBackLeft | kChannelBackRight,
		kChannels5               = kChannelsQuad | kChannelFrontCenter,
		kChannels6               = kChannels5 | kChannelBackCenter,
		kChannels7               = kChannels5 | kChannelFrontLeftCenter | kChannelFrontRightCenter,
		kChannels8               = kChannels6 | kChannelSideLeft | kChannelSideRight,
		kChannels4Point1         = kChannelsQuad | kChannelLowFrequency,
		kChannels5Point1         = kChannels4Point1 | kChannelFrontCenter,
		kChannels6Point1         = kChannels6 | kChannelLowFrequency,
		kChannels7Point1         = kChannels5Point1 | kChannelFrontLeftCenter | kChannelFrontRightCenter,
		kChannels8Point1         = kChannels8 | kChannelLowFrequency,
		kChannels5Point1Surround = kChannels2Point1 | kChannelFrontCenter | kChannelSideLeft | kChannelSideRight,
		kChannels7Point1Surround = kChannels5Point1Surround | kChannelBackLeft | kChannelBackRight
	};

#pragma pack(push, 1)

	struct SWaveBase
	{
		static const U16 kMaxChannels = 64;

		static const U32 kMinSampleHz = 1000;
		static const U32 kMaxSampleHz = 200000;

		EWaveTag tag;
		U16      cChannels;
		U32      cSamplesHz;
		U32      cAvgBytesHz;
		U16      uBlockAlign;
		U16      cSampleBits;
	};

	struct SWaveADPCM
	{
		static const U16 kMinBlockSamples = 4;
		static const U16 kMaxBlockSamples = 64000;

		static const U16 kNumCoefficients = 7;

		U16 cBlockSamples;
		U16 cCoefficients;
		S16 coefs[ 7 ][ 2 ];
	};
	struct SWaveXMA2
	{
		U16 cStreams;
		U32 uChannelMask;
		U32 cEncodedSamples;
		U32 cBlockBytes;
		U32 uPlaySample;
		U32 cPlaySamples;
		U32 uLoopSample;
		U32 cLoopSamples;
		U8  cLoops;
		U8  uEncVer;
		U16 cBlocks;
	};

	struct SWaveEx
	{
		union
		{
			U16 cSampleValidBits;
			U16 cBlockSamples;
		}       samples;
		U32     uChannelMask;
		struct SGUID
		{
			U32 a;
			U16 b[ 2 ];
			U8  c[ 8 ];

			inline Bool isKnown() const
			{
				return
					b[0] == 0x0000 &&
					b[1] == 0x0010 &&
					c[0] == 0x80 && c[1] == 0x00 &&
					c[2] == 0x00 && c[3] == 0xAA &&
					c[4] == 0x00 && c[5] == 0x38 &&
					c[6] == 0x9B && c[7] == 0x71;
			}
		}       subformat;

		inline EWaveTag getTag() const
		{
			return ( EWaveTag )subformat.a;
		}
	};


	struct SWaveFormat: public SWaveBase
	{
		U16            cExtraBytes;
		union
		{
			SWaveADPCM adpcm;
			SWaveXMA2  xma2;
			SWaveEx    ex;
		}              extra;

		// Determine whether this is a valid wave format
		//
		// filename: Optionally specifies the filename to use for logging
		Bool isValid( Str filename = Str() ) const;
		// Perform an in-place swap to native byte order (after load)
		SWaveFormat &loadSwapMe();
		// Perform an in-place swap to file byte order (before save)
		SWaveFormat &saveSwapMe();

		inline EWaveTag getTag() const
		{
			return tag == kWaveTagEx ? extra.ex.getTag() : tag;
		}

	private:
		Bool isValid( EWaveTag asTag, Str filename ) const;
	};

#pragma pack(pop)

}
