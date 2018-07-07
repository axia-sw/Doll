#define DOLL_TRACE_FACILITY doll::kLog_GfxAPIDrv

#include "doll/Core/Defs.hpp"
#include "doll/Gfx/APIs.def.hpp"
#if DOLL_GFX_OPENGL_ENABLED

#include "doll/Gfx/API-GL.hpp"

#include "doll/Core/Logger.hpp"
#include "doll/Math/Matrix.hpp"
#include "doll/OS/OpenGL.hpp"

#include <GL/glew.h>

#if DOLL__USE_GLFW
# include "doll/Core/Engine.hpp"
# include <GLFW/glfw3.h>
#endif


#ifndef GL_TEXTURE_MAX_ANISOTROPY
# ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#  error Neither GL_TEXTURE_MAX_ANISOTROPY nor GL_TEXTURE_MAX_ANISOTROPY_EXT are defined.
# endif
# define GL_TEXTURE_MAX_ANISOTROPY GL_TEXTURE_MAX_ANISOTROPY_EXT
#endif

#ifndef GL_MIRROR
# define GL_MIRROR GL_MIRROR_CLAMP_EXT
#endif

namespace doll
{

	template<typename T>
	constexpr T *objectToPointer( GLuint obj )
	{
		return reinterpret_cast< T * >( static_cast< UPtr >( obj ) );
	}
	template<typename T>
	constexpr GLuint pointerToObject( T *ptr )
	{
		return static_cast< GLuint >( reinterpret_cast< UPtr >( ptr ) );
	}

	struct GLBindPoint {
		GLuint location;
		UPtr   nameIndex;
	};
	struct GLProgram {
		GLuint               programObject;
		TSmallArr<MutStr,20> stringTable;

		struct {
			TSmallArr<GLBindPoint, 4> samplers;
			TSmallArr<GLBindPoint, 4> textures;
			TSmallArr<GLBindPoint, 4> ubuffers;
		} bindPoints;
	};

	class CGfxAPIProvider_GL: public IGfxAPIProvider {
	public:
		virtual Void drop() override {
		}

		virtual Bool is( const Str &name ) const override {
			return
				name.caseCmp( "gl" ) ||
				name.caseCmp( "ogl" ) ||
				name.caseCmp( "opengl" );
		}
		virtual Str getName() const override {
			return Str( "gl" );
		}
		virtual Str getDescription() const override {
			return Str( "OpenGL" );
		}
		virtual IGfxAPI *initAPI( OSWindow wnd, const SGfxInitDesc &desc ) override {
			return CGfxAPI_GL::init( wnd, desc, *this );
		}
		virtual Void finiAPI( IGfxAPI *pAPI ) override {
			delete pAPI;
		}
	};
	static CGfxAPIProvider_GL openGLGfxAPIProvider_;
	IGfxAPIProvider &openGLGfxAPIProvider = openGLGfxAPIProvider_;
	
	void gl_CheckError_( const char *file, int line )
	{
		const char *err;

		switch( glGetError() ) {
		case GL_NO_ERROR: return;
		case GL_INVALID_ENUM: err = "GL_INVALID_ENUM"; break;
		case GL_INVALID_VALUE: err = "GL_INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: err = "GL_INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW: err = "GL_STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW: err = "GL_STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY: err = "GL_OUT_OF_MEMORY"; break;
		default: err = "(unknown)"; break;
		}

		g_ErrorLog( file, line ) += axf( "glGetError() = %s", err );
	}
#define CHECKGL() gl_CheckError_(__FILE__,__LINE__)

	static GLuint getGLUint( GLenum pname )
	{
		GLuint x = 0;
		glGetIntegerv( pname, ( GLint * )&x );
		return x;
	}
	static GLuint getGLBufUint( GLenum target, GLenum pname )
	{
		GLuint x = 0;
		glGetBufferParameteriv( target, pname, ( GLint * )&x );
		return x;
	}

	static S32 fixYPos( S32 y )
	{
		return S32( gfx_r_resY() ) - y;
	}

	static GLenum compTyToGLTy( EVectorType compTy )
	{
		switch( compTy ) {
		case kVectorTypeS8:        break;
		case kVectorTypeS16:       break;
		case kVectorTypeS32:       break;
		case kVectorTypeU8:        return GL_UNSIGNED_BYTE;
		case kVectorTypeU16:       return GL_UNSIGNED_SHORT;
		case kVectorTypeU32:       return GL_UNSIGNED_INT;
		case kVectorTypeF32:       return GL_FLOAT;
		case kVectorTypeF32_SNorm: break;
		case kVectorTypeF32_UNorm: break;
		}

		return GL_INVALID_ENUM;
	}
	GLenum getTopologyGL( ETopology mode )
	{
		switch( mode )
		{
		case kTopologyPointList:     return GL_POINTS;
		case kTopologyLineList:      return GL_LINES;
		case kTopologyLineStrip:     return GL_LINE_STRIP;
		case kTopologyTriangleList:  return GL_TRIANGLES;
		case kTopologyTriangleStrip: return GL_TRIANGLE_STRIP;
		case kTopologyTriangleFan:   return GL_TRIANGLE_FAN;
		}

		AX_ASSERT_MSG( false, "Unhandled topology" );
		return GL_POINTS;
	}

	static GLenum getBufferUsage( EBufferPerformance perf, EBufferPurpose purpose )
	{
		GLuint base = 0;

		switch( perf )
		{
		case kBufferPerfStream:  base = GL_STREAM_DRAW;  break;
		case kBufferPerfStatic:  base = GL_STATIC_DRAW;  break;
		case kBufferPerfDynamic: base = GL_DYNAMIC_DRAW; break;
		}

		if( !base ) {
			DOLL_ERROR_LOG += "Invalid buffer performance mode";
			return 0;
		}

		GLuint bias = ~0U;
		switch( purpose ) {
		case kBufferPurposeDraw: bias = 0; break;
		case kBufferPurposeRead: bias = 1; break;
		case kBufferPurposeCopy: bias = 2; break;
		}

		if( bias == ~0U ) {
			DOLL_ERROR_LOG += "Invalid buffer purpose setting";
			return 0;
		}

		return ( GLenum )( base + bias );
	}

	CGfxAPI_GL::CGfxAPI_GL( IGfxAPIProvider &provider, SGLContext *pCtx )
	: IGfxAPI( provider )
#if DOLL__USE_GLFW
	, m_pCtx( g_core.view.window )
#else
	, m_pCtx( pCtx )
#endif
	{
#if DOLL__USE_GLFW
		((void)pCtx);
		AX_ASSERT_IS_NULL( pCtx );
#else
		AX_ASSERT_NOT_NULL( pCtx );
#endif
	}
	CGfxAPI_GL::~CGfxAPI_GL()
	{
	}

	EGfxAPI CGfxAPI_GL::getAPI() const
	{
		return kGfxAPIOpenGL;
	}
	TArr<EShaderFormat> CGfxAPI_GL::getSupportedShaderFormats() const
	{
		static const EShaderFormat formats[] = {
			kShaderFormatGLSL
		};
		return TArr<EShaderFormat>(formats);
	}
	TArr<EShaderStage> CGfxAPI_GL::getSupportedShaderStages() const
	{
		static const EShaderStage shaderStages[] = {
			kShaderStageVertex,
			kShaderStagePixel
		};

		return TArr<EShaderStage>( shaderStages );
	}

	Void CGfxAPI_GL::setDefaultState( const Mat4f &proj )
	{
#if DOLL__USE_GLFW
		glfwMakeContextCurrent( m_pCtx );
#else
		os_setActiveGL( *m_pCtx );
#endif

		glEnable( GL_BLEND );
		//glBlendEquation( GL_FUNC_ADD );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

		glEnable( GL_ALPHA_TEST );
		glAlphaFunc( GL_GEQUAL, 1.0f/255.0f );

		glDisable( GL_COLOR_MATERIAL );
		glDisable( GL_LIGHTING );
		glDisable( GL_CULL_FACE );

		glDisable( GL_DEPTH_TEST );
		glDisable( GL_SCISSOR_TEST );
		glDisable( GL_STENCIL_TEST );

		glDisable( GL_TEXTURE_1D );
		glDisable( GL_TEXTURE_2D );

		glBindTexture( GL_TEXTURE_2D, 0 );

		glMatrixMode( GL_PROJECTION );
		glLoadMatrixf( proj.ptr() );

		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();

		glViewport( 0, 0, gfx_r_resX(), gfx_r_resY() );
	}

	Void CGfxAPI_GL::resize( U32 uResX, U32 uResY )
	{
		// Don't have any buffers or textures to resize... yet

		( Void )uResX;
		( Void )uResY;
	}
	Void CGfxAPI_GL::getSize( U32 &uResX, U32 &uResY )
	{
#if DOLL__USE_GLFW
		int w, h;
		glfwGetWindowSize( m_pCtx, &w, &h );
		uResX = U32( w );
		uResY = U32( h );
#else
		RECT rc = { 0, 0, 0, 0 };
		GetClientRect( m_pCtx->hWnd, &rc );
		uResX = (U32)rc.right;
		uResY = (U32)rc.bottom;
#endif
	}

	Void CGfxAPI_GL::wsiPresent()
	{
#if DOLL__USE_GLFW
		glfwSwapBuffers( m_pCtx );
#else
		SwapBuffers( m_pCtx->hDC );
#endif
	}

	static GLenum getMipFilter( ETextureFilter f, EMipmapMode m )
	{
		switch( m ) {
		case kMipmapNone:
			switch( f ) {
			case kTexFilterNearest: return GL_NEAREST;
			case kTexFilterLinear:  return GL_LINEAR;
			}
			break;

		case kMipmapNearest:
			switch( f ) {
			case kTexFilterNearest: return GL_NEAREST_MIPMAP_NEAREST;
			case kTexFilterLinear:  return GL_LINEAR_MIPMAP_NEAREST;
			}
			break;

		case kMipmapLinear:
			switch( f ) {
			case kTexFilterNearest: return GL_NEAREST_MIPMAP_LINEAR;
			case kTexFilterLinear:  return GL_LINEAR_MIPMAP_LINEAR;
			}
			break;
		}

		AX_UNREACHABLE();
		return 0;
	}
	static GLenum getWrapMode( ETextureWrap w )
	{
		switch( w ) {
		case kTexWrapRepeat: return GL_REPEAT;
		case kTexWrapMirror: return GL_MIRROR;
		case kTexWrapClamp:  return GL_CLAMP_TO_EDGE;
		case kTexWrapBorder: return GL_CLAMP_TO_BORDER;
		}

		AX_UNREACHABLE();
		return 0;
	}

	static Bool isBorderWhite( ETextureBorder b )
	{
		return b == kTexBorderTransparentWhite || b == kTexBorderOpaqueWhite;
	}
	static Bool isBorderOpaque( ETextureBorder b )
	{
		return b == kTexBorderOpaqueBlack || b == kTexBorderOpaqueWhite;
	}

	static void fillBorderColor( float( &borderColor )[ 4 ], ETextureBorder border ) {
		const float c = isBorderWhite( border ) ? 1.0f : 0.0f;
		const float a = isBorderOpaque( border ) ? 1.0f : 0.0f;

		borderColor[0] = c;
		borderColor[1] = c;
		borderColor[2] = c;
		borderColor[3] = a;
	}

	static GLenum getCompareOp( EGfxCompareOp op )
	{
		switch( op )
		{
		case kGfxCmpNever:          return GL_NEVER;
		case kGfxCmpLess:           return GL_LESS;
		case kGfxCmpEqual:          return GL_EQUAL;
		case kGfxCmpLessOrEqual:    return GL_LEQUAL;
		case kGfxCmpGreater:        return GL_GREATER;
		case kGfxCmpNotEqual:       return GL_NOTEQUAL;
		case kGfxCmpGreaterOrEqual: return GL_GEQUAL;
		case kGfxCmpAlways:         return GL_ALWAYS;
		}

		AX_UNREACHABLE();
		return 0;
	}

	static GLuint genSamplerObject( const SGfxSamplerDesc &desc )
	{
		GLuint ss;
		glGenSamplers( 1, &ss );
		if( !ss || !glIsSampler( ss ) ) {
			return 0;
		}

		glSamplerParameteri( ss, GL_TEXTURE_MAG_FILTER, getMipFilter( desc.magFilter, desc.mipmapMode ) );
		glSamplerParameteri( ss, GL_TEXTURE_MIN_FILTER, getMipFilter( desc.minFilter, desc.mipmapMode ) );
		CHECKGL();

		glSamplerParameteri( ss, GL_TEXTURE_WRAP_S, getWrapMode( desc.wrapU ) );
		glSamplerParameteri( ss, GL_TEXTURE_WRAP_T, getWrapMode( desc.wrapV ) );
		glSamplerParameteri( ss, GL_TEXTURE_WRAP_R, getWrapMode( desc.wrapW ) );
		CHECKGL();

		glSamplerParameterf( ss, GL_TEXTURE_LOD_BIAS, desc.mipLodBias );
		glSamplerParameterf( ss, GL_TEXTURE_MIN_LOD, desc.minLod );
		glSamplerParameterf( ss, GL_TEXTURE_MAX_LOD, desc.maxLod );
		CHECKGL();

		float borderColor[4];
		fillBorderColor( borderColor, desc.borderColor );
		glSamplerParameterfv( ss, GL_TEXTURE_BORDER_COLOR, &borderColor[0] );
		CHECKGL();

		if( desc.anisotropyEnable ) {
			glSamplerParameterf( ss, GL_TEXTURE_MAX_ANISOTROPY, desc.maxAnisotropy );
			CHECKGL();
		}

		if( desc.compareEnable ) {
			glSamplerParameteri( ss, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
			glSamplerParameteri( ss, GL_TEXTURE_COMPARE_FUNC, getCompareOp( desc.compareOp ) );
			CHECKGL();
		}

		return ss;
	}

	IGfxAPISampler *CGfxAPI_GL::createSampler( const SGfxSamplerDesc &desc )
	{
		( ( Void )desc );

		GLuint ss;

		if( !( ss = genSamplerObject( desc ) ) ) {
			return nullptr;
		}

		return objectToPointer<IGfxAPISampler>( ss );
	}
	Void CGfxAPI_GL::destroySampler( IGfxAPISampler *pSampler )
	{
		if( !pSampler ) {
			return;
		}

		GLuint ss = pointerToObject( pSampler );
		AX_ASSERT_MSG( objectToPointer< IGfxAPISampler >( ss ) != pSampler, "Invalid pointer" );
		AX_ASSERT_MSG( glIsSampler( ss ), "Invalid sampler state" );

		glDeleteSamplers( 1, &ss );
		CHECKGL();
	}

	IGfxAPITexture *CGfxAPI_GL::createTexture( ETextureFormat fmt, U16 resX, U16 resY, const U8 *pData )
	{
		TMutArr<U8> emptyData;
		GLuint texh;

		AX_ASSERT( resX > 0 );
		AX_ASSERT( resY > 0 );

		glGenTextures( 1, &texh );
		if( !texh ) {
			DOLL_ERROR_LOG += "Failed to generate GL texture number";
			return nullptr;
		}

		const GLvoid *p = ( const GLvoid * )pData;
		if( !p ) {
			if( !AX_VERIFY_MEMORY( emptyData.resize( UPtr( resX )*UPtr( resY )*4 ) ) ) {
				glDeleteTextures( 1, &texh );
				return nullptr;
			}

			p = ( const GLvoid * )emptyData.pointer();
		}

		const GLuint oldtex = getGLUint( GL_TEXTURE_BINDING_2D );

		glGetError();

		GLenum srctexfmt = ( GLenum )0;
		GLenum dsttexfmt = ( GLenum )0;
		GLenum texfmttyp = ( GLenum )0;
		switch( fmt )
		{
		case kTexFmtRGBA8:
			srctexfmt = GL_BGRA;
			dsttexfmt = GL_RGBA;
			texfmttyp = GL_UNSIGNED_BYTE;
			break;

		case kTexFmtRGB8:
			srctexfmt = GL_BGR;
			dsttexfmt = GL_RGB;
			texfmttyp = GL_UNSIGNED_BYTE;
			break;
		}

		glBindTexture( GL_TEXTURE_2D, texh );
		CHECKGL();
		glTexImage2D( GL_TEXTURE_2D, 0, dsttexfmt, resX, resY, 0, srctexfmt, texfmttyp, p );
		CHECKGL();

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		CHECKGL();

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		CHECKGL();

		const UPtr texret = glGetError() == 0 ? UPtr( texh ) : 0;

		glBindTexture( GL_TEXTURE_2D, oldtex );
		CHECKGL();

		return (IGfxAPITexture*)texret;
	}
	Void CGfxAPI_GL::destroyTexture( IGfxAPITexture *tex )
	{
		AX_ASSERT( UPtr(tex) <= 0x7FFFFFFF );
		const GLuint texh = GLuint( UPtr(tex) );

		glDeleteTextures( 1, &texh );
	}

	IGfxAPIVLayout *CGfxAPI_GL::createLayout( const SGfxLayout &desc )
	{
		return (IGfxAPIVLayout*)&desc;
	}
	Void CGfxAPI_GL::destroyLayout( IGfxAPIVLayout *pLayout )
	{
		if( m_pCurrLayout != (SGfxLayout*)pLayout ) {
			return;
		}

		m_pCurrLayout = nullptr;
		m_layoutVBuf  = ~0U;
	}

	IGfxAPIVBuffer *CGfxAPI_GL::createVBuffer( UPtr cBytes, const Void *pData, EBufferPerformance perf, EBufferPurpose purpose )
	{
		const GLenum usage = getBufferUsage( perf, purpose );
		if( !usage ) {
			return nullptr;
		}

		GLuint vbo = 0;
		glGenBuffers( 1, &vbo );
		if( !vbo ) {
			return nullptr;
		}

		glGetError();

		glBindBuffer( GL_ARRAY_BUFFER, vbo );
		CHECKGL();
		glBufferData( GL_ARRAY_BUFFER, cBytes, pData, usage );
		CHECKGL();

		if( glGetError() != 0 ) {
			DOLL_ERROR_LOG += "Vertex buffer data write failed.";

			glDeleteBuffers( 1, &vbo );
			return nullptr;
		}

		return objectToPointer< IGfxAPIVBuffer >( vbo );
	}
	IGfxAPIIBuffer *CGfxAPI_GL::createIBuffer( UPtr cBytes, const Void *pData, EBufferPerformance perf, EBufferPurpose purpose )
	{
		const GLenum usage = getBufferUsage( perf, purpose );
		if( !usage ) {
			return nullptr;
		}

		GLuint ebo = 0;
		glGenBuffers( 1, &ebo );
		if( !ebo ) {
			return nullptr;
		}

		glGetError();

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
		CHECKGL();
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, cBytes, pData, usage );
		CHECKGL();

		if( glGetError() != 0 ) {
			DOLL_ERROR_LOG += "Index buffer data write failed.";

			glDeleteBuffers( 1, &ebo );
			return nullptr;
		}

		return objectToPointer< IGfxAPIIBuffer >( ebo );
	}
	IGfxAPIUBuffer *CGfxAPI_GL::createUBuffer( UPtr cBytes, const Void *pData, EBufferPerformance perf, EBufferPurpose purpose )
	{
		const GLenum usage = getBufferUsage( perf, purpose );
		if( !usage ) {
			return nullptr;
		}

		GLuint ubo = 0;
		glGenBuffers( 1, &ubo );
		if( !ubo ) {
			return nullptr;
		}

		glGetError();

		glBindBuffer( GL_UNIFORM_BUFFER, ubo );
		CHECKGL();
		glBufferData( GL_UNIFORM_BUFFER, cBytes, pData, usage );
		CHECKGL();

		if( glGetError() != 0 ) {
			DOLL_ERROR_LOG += "Uniform buffer data write failed.";

			glDeleteBuffers( 1, &ubo );
			return nullptr;
		}

		return objectToPointer< IGfxAPIUBuffer >( ubo );
	}

	static Void destroyBufferGL( GLuint buf )
	{
		if( !buf ) {
			return;
		}

		glDeleteBuffers( 1, &buf );
	}
	Void CGfxAPI_GL::destroyVBuffer( IGfxAPIVBuffer *vb )
	{
		const GLuint vbo = pointerToObject( vb );

		if( m_layoutVBuf == vbo ) {
			m_layoutVBuf = ~0U;
		}

		destroyBufferGL( vbo );
	}
	Void CGfxAPI_GL::destroyIBuffer( IGfxAPIIBuffer *ib )
	{
		destroyBufferGL( pointerToObject( ib ) );
	}
	Void CGfxAPI_GL::destroyUBuffer( IGfxAPIUBuffer *ub )
	{
		destroyBufferGL( pointerToObject( ub ) );
	}

	IGfxAPIShader *CGfxAPI_GL::createShader( Str filename, EShaderFormat fmt, EShaderStage stage, UPtr cBytes, const Void *pData, IGfxDiagnostic *pDiag )
	{
		if( fmt != kShaderFormatGLSL ) {
			if( pDiag != nullptr ) {
				pDiag->error( filename, 0, 0, "Expected GLSL format." );
			} else {
				g_ErrorLog( filename ) += "Expected GLSL format.";
			}

			return nullptr;
		}

		return nullptr;
	}
	IGfxAPIProgram *CGfxAPI_GL::createProgram( TArr<IGfxAPIShader> shaders, IGfxDiagnostic *pDiag )
	{
		((Void)shaders);
		((Void)pDiag);

		AX_ASSERT_MSG( false, "CGfxAPI_GL::createProgram() not yet implemented" );

		return nullptr;
	}
	Void CGfxAPI_GL::destroyShader( IGfxAPIShader *pShader )
	{
		if( !pShader ) {
			return;
		}

		AX_ASSERT_MSG( false, "CGfxAPI_GL::destroyShader() not yet implemented" );
	}
	Void CGfxAPI_GL::destroyProgram( IGfxAPIProgram *pProgram )
	{
		if( !pProgram ) {
			return;
		}

		AX_ASSERT_MSG( false, "CGfxAPI_GL::destroyProgram() not yet implemented" );
	}
	Bool CGfxAPI_GL::setCacheDirectory( Str basePath )
	{
		((Void)basePath);

		AX_ASSERT_MSG( false, "CGfxAPI_GL::setCacheDirectory() not yet implemented" );

		return false;
	}
	Str CGfxAPI_GL::getCacheDirectory() const
	{
		AX_ASSERT_MSG( false, "CGfxAPI_GL::getCacheDirectory() not yet implemented" );
		return Str();
	}
	Void CGfxAPI_GL::invalidateShaderCache()
	{
		AX_ASSERT_MSG( false, "CGfxAPI_GL::invalidateShaderCache() not yet implemented" );
	}

	Void CGfxAPI_GL::vsSetProjectionMatrix( const F32 *matrix )
	{
		glMatrixMode( GL_PROJECTION );
		glLoadMatrixf( matrix );
	}
	Void CGfxAPI_GL::vsSetModelViewMatrix( const F32 *matrix )
	{
		glMatrixMode( GL_MODELVIEW );
		glLoadMatrixf( matrix );
	}

	Void CGfxAPI_GL::psoSetScissorEnable( Bool enable )
	{
		if( enable ) {
			glEnable( GL_SCISSOR_TEST );
		} else {
			glDisable( GL_SCISSOR_TEST );
		}
		CHECKGL();
	}
	Void CGfxAPI_GL::psoSetTextureEnable( Bool enable )
	{
		if( enable ) {
			glEnable( GL_TEXTURE_2D );
		} else {
			glDisable( GL_TEXTURE_2D );
		}
		CHECKGL();
	}

	static GLenum blendOpToGL( EBlendOp x )
	{
#define UNSUPPORTED_(X_) \
	case X_:\
		do {\
			static Bool bDisplayed = false;\
			if( bDisplayed ) {\
				break;\
			}\
			\
			bDisplayed = true;\
			g_ErrorLog += "GL: `EBlendOp::" #X_ "` is unsupported.";\
		} while( false );\
		break

		switch( x ) {
		case kBlendAdd:
			return GL_FUNC_ADD;
		case kBlendSub:
			return GL_FUNC_SUBTRACT;
		case kBlendRevSub:
			return GL_FUNC_REVERSE_SUBTRACT;

		UNSUPPORTED_(kBlendMin);
		UNSUPPORTED_(kBlendMax);
		UNSUPPORTED_(kBlendLogicalClear);
		UNSUPPORTED_(kBlendLogicalSet);
		UNSUPPORTED_(kBlendLogicalCopy);
		UNSUPPORTED_(kBlendLogicalCopyInverted);
		UNSUPPORTED_(kBlendLogicalNop);
		UNSUPPORTED_(kBlendLogicalInvert);
		UNSUPPORTED_(kBlendLogicalAnd);
		UNSUPPORTED_(kBlendLogicalNand);
		UNSUPPORTED_(kBlendLogicalOr);
		UNSUPPORTED_(kBlendLogicalNor);
		UNSUPPORTED_(kBlendLogicalXor);
		UNSUPPORTED_(kBlendLogicalEquiv);
		UNSUPPORTED_(kBlendLogicalAndReverse);
		UNSUPPORTED_(kBlendLogicalAndInverted);
		UNSUPPORTED_(kBlendLogicalOrReverse);
		UNSUPPORTED_(kBlendLogicalOrInverted);
		}

		return GLenum( 0 );

#undef UNSUPPORTED_
	}
	static GLenum blendFactorToGL( EBlendFactor x )
	{
#define UNSUPPORTED_(X_) \
	case X_:\
		do {\
			static Bool bDisplayed = false;\
			if( bDisplayed ) {\
				break;\
			}\
			\
			bDisplayed = true;\
			g_ErrorLog += "GL: `EBlendFactor::" #X_ "` is unsupported.";\
		} while( false );\
		break

		switch( x ) {
		case kBlendZero:
			return GL_ZERO;
		case kBlendOne:
			return GL_ONE;
		case kBlendSrcColor:
			return GL_SRC_COLOR;
		case kBlendSrcAlpha:
			return GL_SRC_ALPHA;
		case kBlendDstColor:
			return GL_DST_COLOR;
		case kBlendDstAlpha:
			return GL_DST_ALPHA;
		case kBlendInvSrcColor:
			return GL_ONE_MINUS_SRC_COLOR;
		case kBlendInvSrcAlpha:
			return GL_ONE_MINUS_SRC_ALPHA;
		case kBlendInvDstColor:
			return GL_ONE_MINUS_DST_COLOR;
		case kBlendInvDstAlpha:
			return GL_ONE_MINUS_DST_ALPHA;
		}

		return GLenum( 0 );

#undef UNSUPPORTED_
	}
	Void CGfxAPI_GL::psoSetBlend( EBlendOp op, EBlendFactor colA, EBlendFactor colB, EBlendFactor alphaA, EBlendFactor alphaB )
	{
		const GLenum gop = blendOpToGL( op );
		const GLenum gca = blendFactorToGL( colA );
		const GLenum gcb = blendFactorToGL( colB );
		const GLenum gaa = blendFactorToGL( alphaA );
		const GLenum gab = blendFactorToGL( alphaB );

		glBlendEquation( gop );
		CHECKGL();

		if( gca != gaa || gcb != gab ) {
			glBlendFuncSeparate( gca, gcb, gaa, gab );
		} else {
			glBlendFunc( gca, gcb );
		}
		CHECKGL();
	}

	Void CGfxAPI_GL::rsSetScissor( S32 posX, S32 posY, U32 resX, U32 resY )
	{
		glScissor( posX, fixYPos( posY ) - resY, resX, resY );
		CHECKGL();
	}
	Void CGfxAPI_GL::rsSetViewport( S32 posX, S32 posY, U32 resX, U32 resY )
	{
		glViewport( posX, fixYPos( posY ) - resY, resX, resY );
		CHECKGL();
	}

	static Void DOLL_API applyLayoutPointers( SGfxLayout *p, UPtr base )
	{
		glDisableClientState( GL_VERTEX_ARRAY );
		glDisableClientState( GL_NORMAL_ARRAY );
		glDisableClientState( GL_COLOR_ARRAY );
		glDisableClientState( GL_SECONDARY_COLOR_ARRAY );
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		CHECKGL();

		const GLsizei stride = ( GLsizei )p->stride;
		U8 uTexStage = 0;
		for( UPtr i = 0; i < p->cElements; ++i ) {
			const SGfxLayoutElement &q = p->elements[ i ];

			const GLvoid *const ptr = ( const GLvoid * )( base + q.uOffset );

			const GLint size = q.cComps;
			const GLenum type = compTyToGLTy( q.compTy );

			switch( q.type ) {
			case kGfxLayoutElementVertex:
				glEnableClientState( GL_VERTEX_ARRAY );
				glVertexPointer( size, type, stride, ptr );
				CHECKGL();
				break;

			case kGfxLayoutElementNormal:
				glEnableClientState( GL_NORMAL_ARRAY );
				glNormalPointer( type, stride, ptr );
				CHECKGL();
				break;

			case kGfxLayoutElementColor:
				glEnableClientState( GL_COLOR_ARRAY );
				glColorPointer( size, type, stride, ptr );
				CHECKGL();
				break;

			case kGfxLayoutElementTexCoord:
				glActiveTexture( GL_TEXTURE0 + uTexStage );
				CHECKGL();

				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
				glTexCoordPointer( size, type, stride, ptr );
				CHECKGL();

				++uTexStage;
				break;
			}
		}
	}
	Void CGfxAPI_GL::applyLayout()
	{
		AX_ASSERT_NOT_NULL( m_pCurrLayout );

		const GLuint vbo = getGLUint( GL_ARRAY_BUFFER_BINDING );
		AX_ASSERT( vbo != 0 );

		if( m_layoutVBuf != vbo ) {
			m_layoutVBuf = vbo;

			applyLayoutPointers( ( SGfxLayout * )m_pCurrLayout, 0 );
		}
	}

	Void CGfxAPI_GL::iaSetLayout( IGfxAPIVLayout *pLayout )
	{
		if( m_pCurrLayout == (SGfxLayout*)pLayout ) {
			return;
		}

		m_pCurrLayout = (SGfxLayout*)pLayout;
		m_layoutVBuf  = ~0U;
	}

	Void CGfxAPI_GL::tsBindTexture( IGfxAPITexture *tex, U32 uStage )
	{
		glActiveTexture( GL_TEXTURE0 + uStage );
		glBindTexture( GL_TEXTURE_2D, ( GLuint )( UPtr )tex );
		CHECKGL();
	}
	Void CGfxAPI_GL::tsBindSampler( IGfxAPISampler *pSampler, U32 uStage )
	{
		( ( Void )pSampler );
		( ( Void )uStage );
	}
	Void CGfxAPI_GL::iaBindVBuffer( IGfxAPIVBuffer *vb )
	{
		AX_ASSERT_NOT_NULL( vb );

		glBindBuffer( GL_ARRAY_BUFFER, (GLint)(UPtr)vb );
		CHECKGL();
	}
	Void CGfxAPI_GL::iaBindIBuffer( IGfxAPIIBuffer *ib )
	{
		AX_ASSERT_NOT_NULL( ib );

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, (GLint)(UPtr)ib );
		CHECKGL();
	}

	Void CGfxAPI_GL::plBindProgram( IGfxAPIProgram *pProgram )
	{
		((Void)pProgram);

		AX_ASSERT_MSG( false, "CGfxAPI_GL::plBindProgram() not yet implemented" );
	}
	Void CGfxAPI_GL::plUnbindProgram()
	{
		AX_ASSERT_MSG( false, "CGfxAPI_GL::plUnbindProgram() not yet implemented" );
	}
	Void CGfxAPI_GL::cmdUpdateProgramBindings( const SGfxBinding &binding )
	{
		((Void)binding);

		AX_ASSERT_MSG( false, "CGfxAPI_GL::cmdUpdateProgramBindings() not yet implemented" );
	}

	Void CGfxAPI_GL::cmdClearRect( S32 posX, S32 posY, U32 resX, U32 resY, U32 value )
	{
		GLint box[ 4 ];
		glGetIntegerv( GL_SCISSOR_BOX, box );

		const Bool enabled = glIsEnabled( GL_SCISSOR_TEST ) != 0;

		glEnable( GL_SCISSOR_TEST );
		gfx_r_setScissor( posX, posY, resX, resY );

		glClearColor
		(
			F32( DOLL_COLOR_R( value ) )/255.0f,
			F32( DOLL_COLOR_G( value ) )/255.0f,
			F32( DOLL_COLOR_B( value ) )/255.0f,
			F32( DOLL_COLOR_A( value ) )/255.0f
		);
		glClear( GL_COLOR_BUFFER_BIT );

		glScissor( box[0], box[1], box[2], box[3] );
		CHECKGL();
		if( !enabled ) {
			glDisable( GL_SCISSOR_TEST );
		}
	}
	Void CGfxAPI_GL::cmdUpdateTexture( IGfxAPITexture *tex, U16 posX, U16 posY, U16 resX, U16 resY, const U8 *pData )
	{
		AX_ASSERT( resX > 0 );
		AX_ASSERT( resY > 0 );
		AX_ASSERT_NOT_NULL( pData );

		g_DebugLog += axf( "Updating texture %zu at (%u,%u -> %u,%u) with %ux%u data (%p)", UPtr(tex), posX, posY, posX+resX, posY+resY, resX, resY, pData );

		const GLuint oldtex = getGLUint( GL_TEXTURE_BINDING_2D );

		// FIXME: Use actual texture format, not GL_BGRA+GL_UNSIGNED_BYTE

		glBindTexture( GL_TEXTURE_2D, ( GLuint )( UPtr )tex );
		CHECKGL();
		glTexSubImage2D( GL_TEXTURE_2D, 0, ( GLint )posX, ( GLint )posY, resX, resY, GL_BGRA, GL_UNSIGNED_BYTE, ( const GLvoid * )pData );
		CHECKGL();

		glBindTexture( GL_TEXTURE_2D, oldtex );
		CHECKGL();
	}

	static Bool updateBufferGL( GLenum target, GLenum targetBinding, UPtr vbuffer, UPtr offset, UPtr size, const void *pData )
	{
		const GLuint oldvbo = getGLUint( targetBinding );

		glGetError();

		glBindBuffer( target, ( GLuint )vbuffer );
		if( glGetError() != 0 ) {
			return false;
		}

		const GLuint vboSize = getGLBufUint( target, GL_BUFFER_SIZE );
		const GLenum vboUsage = getGLBufUint( target, GL_BUFFER_USAGE );

		if( offset + size > vboSize ) {
			glBindBuffer( target, oldvbo );
			return false;
		}

		if( offset + size == vboSize ) {
			glBufferData( target, ( GLsizeiptr )size, pData, vboUsage );
		} else {
			AX_ASSERT_NOT_NULL( pData );
			glBufferSubData( target, ( GLintptr )offset, ( GLsizeiptr )size, pData );
		}

		const Bool bResult = glGetError() == 0;

		glBindBuffer( target, oldvbo );
		return bResult;
	}
	Void CGfxAPI_GL::cmdWriteVBuffer( IGfxAPIVBuffer *vb, UPtr offset, UPtr size, const Void *pData )
	{
		( Void )updateBufferGL( GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, pointerToObject(vb), offset, size, pData );
	}
	Void CGfxAPI_GL::cmdWriteIBuffer( IGfxAPIIBuffer *ib, UPtr offset, UPtr size, const Void *pData )
	{
		( Void )updateBufferGL( GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING, pointerToObject(ib), offset, size, pData );
	}
	Void CGfxAPI_GL::cmdWriteUBuffer( IGfxAPIUBuffer *ub, UPtr offset, UPtr size, const Void *pData )
	{
		( Void )updateBufferGL( GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING, pointerToObject(ub), offset, size, pData );
	}

	static Bool readBufferGL( GLenum target, GLenum targetBinding, UPtr buffer, UPtr offset, UPtr size, void *pData )
	{
		AX_ASSERT( buffer != 0 );
		AX_ASSERT_NOT_NULL( pData );
		AX_ASSERT( size > 0 );

		const GLuint oldbuf = getGLUint( targetBinding );

		glGetError();

		glBindBuffer( target, ( GLuint )buffer );
		glGetBufferSubData( target, ( GLintptr )offset, ( GLsizeiptr )size, pData );

		const Bool bResult = glGetError() == 0;

		glBindBuffer( target, oldbuf );
		return bResult;
	}
	Void CGfxAPI_GL::cmdReadVBuffer( IGfxAPIVBuffer *vb, UPtr offset, UPtr size, Void *pData )
	{
		( Void )readBufferGL( GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, pointerToObject(vb), offset, size, pData );
	}
	Void CGfxAPI_GL::cmdReadIBuffer( IGfxAPIIBuffer *ib, UPtr offset, UPtr size, Void *pData )
	{
		( Void )readBufferGL( GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING, pointerToObject(ib), offset, size, pData );
	}
	Void CGfxAPI_GL::cmdReadUBuffer( IGfxAPIUBuffer *ub, UPtr offset, UPtr size, Void *pData )
	{
		( Void )readBufferGL( GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_BINDING, pointerToObject(ub), offset, size, pData );
	}

	Void CGfxAPI_GL::cmdDraw( ETopology mode, U32 cVerts, U32 uOffset )
	{
		AX_ASSERT( getGLUint( GL_ARRAY_BUFFER_BINDING ) != 0 );
		AX_ASSERT_NOT_NULL( m_pCurrLayout );

		applyLayout();

		glDrawArrays( getTopologyGL( mode ), ( GLint )uOffset, ( GLsizei )cVerts );
		CHECKGL();
	}
	Void CGfxAPI_GL::cmdDrawIndexed( ETopology mode, U32 cIndices, U32 uOffset, U32 uBias )
	{
		AX_ASSERT( getGLUint( GL_ARRAY_BUFFER_BINDING ) != 0 );
		AX_ASSERT( getGLUint( GL_ELEMENT_ARRAY_BUFFER_BINDING ) != 0 );
		AX_ASSERT_NOT_NULL( m_pCurrLayout );

		applyLayout();

		const GLenum topology = getTopologyGL( mode );

		if( uBias > 0 ) {
			glDrawElementsBaseVertex( topology, ( GLsizei )cIndices, GL_UNSIGNED_SHORT, ( const GLvoid * )( UPtr )uOffset, ( GLint )uBias );
			CHECKGL();
		} else {
			glDrawElements( topology, ( GLsizei )cIndices, GL_UNSIGNED_SHORT, ( const GLvoid * )( UPtr )uOffset );
			CHECKGL();
		}
	}

	static void errorBox( OSWindow wnd, const char *message, const char *title = "Error" )
	{
		AX_ASSERT_NOT_NULL( message );
		AX_ASSERT_NOT_NULL( title );

#ifdef _WIN32
		MessageBoxA( HWND(wnd), message, title, MB_ICONERROR | MB_OK );
#endif

		g_ErrorLog[ kLog_GfxAPIDrv ] += axf( "GL: %s: %s", title, message );
	}

	DOLL_FUNC CGfxAPI_GL *DOLL_API gfx__api_init_gl( OSWindow wnd, const SGfxInitDesc &desc, IGfxAPIProvider &provider )
	{
		DOLL_TRACE( axf( "wnd=%p, desc=%p", (void*)wnd, (void*)&desc ) );
#if DOLL__USE_GLFW
		((void)desc);
#else
		SGLContext *const pCtx = os_initGL( wnd, SGLInitInfo().setDepthStencil( 24, 8 ).setFullscreen( desc.windowing != kGfxScreenModeWindowed, desc.vsync ) );
		if( !AX_VERIFY_MSG( pCtx != nullptr, "Failed to initialize OpenGL context" ) ) {
			axpf("f\n");
			return nullptr;
		}
#endif

		DOLL_DEBUG_LOG += axf( "GL version: %s", glGetString( GL_VERSION ) );
		DOLL_DEBUG_LOG += axf( "GL vendor: %s", glGetString( GL_VENDOR ) );
		DOLL_DEBUG_LOG += axf( "GL renderer: %s", glGetString( GL_RENDERER ) );

		DOLL_TRACE( "Trying to initialize GLEW..." );
		GLenum glewErr;
		if( ( glewErr = glewInit() ) != GLEW_OK ) {
#if !DOLL__USE_GLFW
			os_finiGL( pCtx );
#endif

			errorBox( wnd, "GLEW initialization failed." );
			return nullptr;
		}

		int glversion = 0;
		{
			int glver[2] = { 0, 0 };
			const char *const glstr = (const char *)glGetString( GL_VERSION );
			const char *const str = glstr != nullptr ? glstr : "0.0";

			static const size_t dstn = AX_COUNTOF(glver);
			size_t dsti = 0;
			const char *p;
			for( p = str; *p != '\0'; ++p ) {
				if( *p >= '0' && *p <= '9' ) {
					glver[dsti] *= 10;
					glver[dsti] += int( *p - '0' );
					continue;
				}

				if( ++dsti == dstn ) {
					break;
				}
			}

			if( glver[1] > 9 ) {
				glver[1] = 9;
			}

			glversion = glver[0]*100 + glver[1]*10 + 0;
			DOLL_TRACE( axf( "GL version: %i.%i (%d)", glver[0], glver[1], glversion ) );
		}

		DOLL_TRACE( "Checking for modern GL support" );
		if( glversion < 330 ) {
#if !DOLL__USE_GLFW
			os_finiGL( pCtx );
#endif

			const char *const pszErrorMsg =
				"OpenGL version is too low.\n\n"
				"Your video driver is reporting a version of OpenGL below 3.3.\n"
				"It is likely that you did not update your video drivers directly from your GPU vendor. (AMD, NVIDIA, or Intel.)\n\n"
				"Try updating your drivers and restarting if you have not already done so.\n\n"
				"Other possibilities include:\n"
#ifdef _WIN32
				" * You're in safe mode. (How? We check for that before getting here!)\n"
#endif
				" * You're actually running this with a GPU from 2005 or earlier. (Why!?)\n"
				" * You have transcended the physical world and are now software. Welcome to the Wired, Lain.";
			const char *const pszErrorCap = "Error - OpenGL version is lower than 3.3";

			errorBox( wnd, pszErrorMsg, pszErrorCap );
			return nullptr;
		}

		/*

			FIXME: We're kind of being overly paranoid by checking these
			-      extensions. Just seeing "version 3.3" confirms their
			-      presence for the most part.

		*/

		DOLL_TRACE( "Checking OpenGL extension availability..." );
		static const char *const pszExtNames[] = {
			"GL_ARB_vertex_buffer_object",
			"GL_ARB_vertex_array_object",
			"GL_ARB_vertex_program",
			"GL_ARB_fragment_program",
			"GL_ARB_framebuffer_object",
			"GL_ARB_shader_objects",
			"GL_ARB_sampler_objects",
			"GL_ARB_texture_storage",
			"GL_ARB_texture_swizzle",
			"GL_EXT_bgra"
		};
		static const UPtr cExtNames = sizeof( pszExtNames )/sizeof( pszExtNames[ 0 ] );
		bool bExtsAvailable[ cExtNames ];

		bool bAllAvailable = true;
#ifndef __APPLE__
		for( UPtr i = 0; i < cExtNames; ++i ) {
			bExtsAvailable[ i ] = !!glewIsSupported( pszExtNames[ i ] );
			bAllAvailable &= bExtsAvailable[ i ];
		}
#else
# if 0
		GLint cGLExts = 0;
		glGetIntegerv( GL_NUM_EXTENSIONS, &cGLExts );

		UPtr cExtsFound = 0;
		for( GLint extIdx = 0; extIdx < cGLExts; ++extIdx ) {
			const Str extName = ( const char * )glGetStringi( GL_EXTENSIONS, GLuint(extIdx) );
			axpf( "    %0.3i: \"%.*s\"\n", extIdx, extName.lenInt(), extName.get() );
			for( UPtr i = 0; i < cExtNames; ++i ) {
				if( bExtsAvailable[ i ] ) {
					continue;
				}
				if( extName.caseCmp( pszExtNames[ i ] ) ) {
					bExtsAvailable[ i ] = true;
					++cExtsFound;
					break;
				}
			}
		}
		bAllAvailable = cExtsFound == cExtNames;
# else
		/* On all supported versions of macOS, everything we want is available */
		for( UPtr i = 0; i < cExtNames; ++i ) {
			bExtsAvailable[ i ] = true;
		}
		bAllAvailable = true;
# endif
#endif

		if( !bAllAvailable ) {
#if !DOLL__USE_GLFW
			os_finiGL( pCtx );
#endif

			char szBuf[ 4096 ] = { '\0' };

			axstr_cat( szBuf, "Error. One or more required OpenGL extensions are missing:\n\n" );
			for( UPtr i = 0; i < cExtNames; ++i ) {
				if( bExtsAvailable[ i ] ) {
					continue;
				}

				axstr_cat( szBuf, axf( " * %s\n", pszExtNames[ i ] ) );
			}
			axstr_cat( szBuf, "\nTry updating your video driver or upgrading to a newer GPU." );

			DOLL_TRACE( axf( "Extensions: %s", (const char *)glewGetString(GL_EXTENSIONS) ) );

			errorBox( wnd, szBuf, "Error - OpenGL extensions missing" );
			return nullptr;
		}

		DOLL_TRACE( "Creating CGfxAPI_GL object..." );
#if DOLL__USE_GLFW
		SGLContext *const pCtx = nullptr;
#endif

		CGfxAPI_GL *const pGLAPI = new CGfxAPI_GL( provider, pCtx );
		if( !AX_VERIFY_MEMORY( pGLAPI ) ) {
#if !DOLL__USE_GLFW
			os_finiGL( pCtx );
#endif
			errorBox( wnd, "Insufficient memory" );
			return nullptr;
		}

		DOLL_TRACE( "Done!" );
		return pGLAPI;
	}

}

#endif
