#pragma once

#include "../Core/Defs.hpp"

namespace doll
{

	enum ETopology
	{
		kTopologyPointList,
		kTopologyLineList,
		kTopologyLineStrip,
		kTopologyTriangleList,
		kTopologyTriangleStrip,
		kTopologyTriangleFan
	};

	enum
	{
		kVF_XYZ        = 0x00000000,
		kVF_XYZW       = 0x00000001,
		kVF_XY         = 0x00000002,
		kVF_DiffuseBit = 0x00000004,
		kVF_Tex0       = 0x00000000,
		kVF_Tex1       = 0x10000000,
		kVF_Tex2       = 0x20000000,
		kVF_Tex3       = 0x30000000,
		kVF_Tex4       = 0x40000000,
		kVF_Tex5       = 0x50000000,
		kVF_Tex6       = 0x60000000,
		kVF_Tex7       = 0x70000000,
		kVF_Tex8       = 0x80000000,

		kVFMask_Pos    = 0x00000003,
		kVFShft_Pos    = 0,

		kVFMask_Tex    = 0xF0000000,
		kVFShft_Tex    = 28
	};

	struct SVertex2D
	{
		static const U32 kFmtPos = kVF_XYZW;

		F32 x, y, z, w;
		U32 diffuse;
		F32 u, v;
	};
	struct SVertex2DSprite
	{
		static const U32 kFmtPos = kVF_XY;

		F32 x, y;
		U32 diffuse;
		F32 u, v;
	};

}
