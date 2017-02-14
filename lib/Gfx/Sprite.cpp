#define DOLL_TRACE_FACILITY doll::kLog_GfxSprite

#include "doll/Gfx/Sprite.hpp"
#include "doll/Gfx/Action.hpp"
#include "doll/Gfx/Vertex.hpp"
#include "doll/Gfx/Layer.hpp"
#include "doll/Gfx/API-GL.hpp"
#include "doll/Math/Math.hpp"

#include <GL/gl.h>

namespace doll
{

	MSprites MSprites::instance;
	MSprites &g_spriteMgr = MSprites::instance;

	SSpriteTransform SSpriteTransform::identity;

	/*
	===============================================================================

		SPRITE MANAGER

	===============================================================================
	*/
	MSprites::MSprites()
	: defaultSpriteGroup( nullptr )
	{
	}
	MSprites::~MSprites()
	{
		fini_gl();
	}

	Bool MSprites::init_gl()
	{
		if( defaultSpriteGroup != nullptr ) {
			return true;
		}

		defaultSpriteGroup = newSpriteGroup();

		if( !AX_VERIFY_MSG( defaultSpriteGroup != nullptr, "newSpriteGroup() failed" ) ) {
			return false;
		}

		return true;
	}
	Void MSprites::fini_gl()
	{
	}

	Void MSprites::render_gl( CGfxFrame *pFrame )
	{
		// early exit if nothing to render
		if( mgr_spriteGroupList.isEmpty() ) {
			return;
		}

		const U32 w = pFrame->getResX();
		const U32 h = pFrame->getResY();

		pFrame->setDefaultState();

#if 0
		hr = device->SetFVF( SVertex2DSprite::FVF_DEFAULT );
		AX_ASSERT_MSG( SUCCEEDED( hr ), "Invalid D3D op" );
#endif

		for( RSpriteGroup *group = mgr_spriteGroupList.head(); group != nullptr; group = group->mgr_spriteGroupLink.next() ) {
			group->render_gl( w, h );
		}
	}

	Void MSprites::update()
	{
		for( RSpriteGroup *group = mgr_spriteGroupList.head(); group != nullptr; group = group->mgr_spriteGroupLink.next() ) {
			group->update();
		}
	}

	RSpriteGroup *MSprites::newSpriteGroup()
	{
		RSpriteGroup *group = new RSpriteGroup();
		if( !AX_VERIFY_MSG( group!=nullptr, "Out of memory" ) ) {
			return nullptr;
		}

		RLayer *const pDefLayer = g_layerMgr->getDefaultLayer();
		AX_ASSERT_NOT_NULL( pDefLayer );

		pDefLayer->addPostrenderSpriteGroup( *group );

		return group;
	}

	/*
	===============================================================================

		SPRITE GROUP

	===============================================================================
	*/
	RSpriteGroup::RSpriteGroup()
	: mgr_spriteGroupLink( this )
	, grp_spriteList()
	{
		flags.bits.visible = true;
		flags.bits.scissor = false;
		flags.bits.update = true;

		scissorRect.off.x = 0;
		scissorRect.off.y = 0;
		scissorRect.res.x = 0;
		scissorRect.res.y = 0;

		g_spriteMgr.mgr_spriteGroupList.addTail( mgr_spriteGroupLink );

		camera.translation = 0;
		camera.rotation = 0;
		camera.follow = nullptr;
		camera.followSpeed =  1;
		camera.minimumDistance = 0;
		camera.maximumDistance = 0;

		virtualResolutionEnabled = false;
		virtualResolution = Vec2f( 640, 480 );
	}
	RSpriteGroup::~RSpriteGroup()
	{
	}

	static inline Void _applyCamera( F32 x, F32 y, F32 angle, F32 orgX, F32 orgY, F32 resX, F32 resY )
	{
		Mat4f proj;
		proj.loadOrthoProj( orgX, orgX + resX, orgY + resY, orgY, 0, 1000 );

		gfx_r_loadProjection( proj.ptr() );

		// FIXME: This doesn't actually work
		const F32 c = cos( degrees( 360 - wrap360( angle ) ) );
		const F32 s = sin( degrees( 360 - wrap360( angle ) ) );
		const F32 z = -s;
		const F32 p = -( x*c + y*z );
		const F32 q = -( x*s + y*c );

		const float viewTransform[] = {
			c, z, 0, 0,
			s, c, 0, 0,
			0, 0, 1, 0,
			p, q, 0, 1
		};

		// HACK: Uncomment once the above above is correct
		//gfx_r_loadModelView( viewTransform );
	}
	Void RSpriteGroup::render_gl( S32 w, S32 h )
	{
		// early exit if nothing to render
		if( grp_spriteList.isEmpty() ) {
			return;
		}

		STextureRect texRect;

		// apply any sort of clipping
		if( isScissorEnabled() ) {
			SRect rect;
			rect.positionMe( SIntVector2( scissorRect.off.x, scissorRect.off.y ) );
			rect.resizeMe( SIntVector2( scissorRect.res.x, scissorRect.res.y ) );

			// if x is <= 0 or > w
			if( ( U32 )( rect.x2 - 1 ) >= ( U32 )w ) {
				rect.x2 = w;
			}
			// if y is <= 0 or > h
			if( ( U32 )( rect.y2 - 1 ) >= ( U32 )h ) {
				rect.y2 = h;
			}

			rect.x2 += rect.x1;
			rect.y2 += rect.y1;

			gfx_r_enableScissor();
			gfx_r_setScissor( rect.posX(), rect.posY(), rect.resX(), rect.resY() );
		} else {
			gfx_r_disableScissor();
		}

		// apply the view transform
		const F32 org[ 2 ] = {
			virtualResolutionEnabled ? virtualOrigin.x : 0.0f,
			virtualResolutionEnabled ? virtualOrigin.y : 0.0f
		};
		const F32 res[ 2 ] = {
			virtualResolutionEnabled ? virtualResolution.x : ( F32 )w,
			virtualResolutionEnabled ? virtualResolution.y : ( F32 )h
		};
		_applyCamera( camera.translation.x, camera.translation.y, camera.rotation, org[ 0 ], org[ 1 ], res[ 0 ], res[ 1 ] );

		renderPrims.reset();
		renderPrims.setPrimitiveType( kTopologyTriangleList );

		SVertex2DSprite quad[ 4 ];

		// render each sprite
		for( RSprite *sprite = grp_spriteList.head(); sprite != nullptr; sprite = sprite->grp_spriteLink.next() ) {
			AX_ASSERT_MSG( sprite->frames != nullptr, "Invalid sprite" );
			AX_ASSERT_MSG( sprite->curFrame < sprite->numFrames, "Invalid sprite" );

			if( !sprite->isVisible() ) {
				continue;
			}

			const RTexture *texture = sprite->getTexture();
			if( !texture ) {
				continue;
			}

			const SSpriteTransform xform = sprite->getGlobalTransform();
			const SSpriteFrame &frame = sprite->frames[ sprite->curFrame ];

			const SPixelRect srcRect = texture->getAtlasRectangle();
			const SPixelRect subsrcRect = {
				{
					( U16 )( srcRect.off.x + frame.sourceRect.off.x ),
					( U16 )( srcRect.off.y + frame.sourceRect.off.y ),
				},
				{
					( U16 )frame.sourceRect.res.x,
					( U16 )frame.sourceRect.res.y
				}
			};
			texture->translateCoordinates( texRect, subsrcRect );
			const F32 dx = texture->getUnitResX();
			const F32 dy = texture->getUnitResY();
			//const F32 x = xform.translation.x;
			//const F32 y = xform.translation.y;
			const F32 x1 = sprite->getOffset().x;
			const F32 y1 = sprite->getOffset().y;
			const F32 x2 = x1 + xform.scale.x*( F32 )( subsrcRect.res.x + 0 );
			const F32 y2 = y1 + xform.scale.y*( F32 )( subsrcRect.res.y + 0 );
			const F32 u1 = texRect.pos[ 0 ];
			const F32 v1 = texRect.pos[ 1 ];
			const F32 u2 = texRect.res[ 0 ] + u1;
			const F32 v2 = texRect.res[ 1 ] + v1;
			const F32 s1 = sprite->flip[ 0 ] ? u2 - dx : u1;
			const F32 s2 = sprite->flip[ 0 ] ? u1 + dx : u2;
			const F32 t1 = sprite->flip[ 1 ] ? v2 - dy : v1;
			const F32 t2 = sprite->flip[ 1 ] ? v1 + dy : v2;
			const U32 d1 = sprite->getFrameCornerDiffuse( 0 );
			const U32 d2 = sprite->getFrameCornerDiffuse( 1 );
			const U32 d3 = sprite->getFrameCornerDiffuse( 2 );
			const U32 d4 = sprite->getFrameCornerDiffuse( 3 );
			const Vec2f tl = ( xform.translation + rotate( Vec2f( x1, y1 ), xform.rotation ) ).snap();
			const Vec2f tr = ( xform.translation + rotate( Vec2f( x2, y1 ), xform.rotation ) ).snap();
			const Vec2f bl = ( xform.translation + rotate( Vec2f( x1, y2 ), xform.rotation ) ).snap();
			const Vec2f br = ( xform.translation + rotate( Vec2f( x2, y2 ), xform.rotation ) ).snap();
#define SETQ(I_,X_,Y_,D_,U_,V_)\
	do {\
		quad[I_].x=X_;\
		quad[I_].y=Y_;\
		/*quad[I_].z=1;*/\
		quad[I_].diffuse=D_;\
		quad[I_].u=U_;\
		quad[I_].v=V_;\
	} while(0)
#define SENDQ(I_)\
	do {\
		renderPrims.color( quad[I_].diffuse );\
		renderPrims.texcoord2f( quad[I_].u, quad[I_].v );\
		renderPrims.vertex2f( quad[I_].x, quad[I_].y );\
	} while(0)
#define Q_TL 0
#define Q_TR 1
#define Q_BL 2
#define Q_BR 3

			const UPtr texh = texture->getBackingTexture();
			renderPrims.setTexture( texh );

			SETQ( Q_TL, tl.x, tl.y, d3, s1, t2 );
			SETQ( Q_TR, tr.x, tr.y, d2, s2, t2 );
			SETQ( Q_BL, bl.x, bl.y, d1, s1, t1 );
			SETQ( Q_BR, br.x, br.y, d4, s2, t1 );

			SENDQ( Q_BL );
			SENDQ( Q_TL );
			SENDQ( Q_TR );

			SENDQ( Q_BL );
			SENDQ( Q_TR );
			SENDQ( Q_BR );

#undef Q_BR
#undef Q_BL
#undef Q_TR
#undef Q_TL
#undef SENDQ
#undef SETQ

#if 0
			const F32 c = cos( xform.rotation );
			const F32 s = sin( xform.rotation );
			const F32 z = -s;
			const D3DMATRIX modelTransform = {
				c, s, 0, 0,
				z, c, 0, 0,
				0, 0, 1, 0,
				x, y, 0, 1
			};
#endif

#if 0
			hr = device->SetTransform( D3DTS_WORLD, &modelTransform );
			AX_ASSERT_MSG( SUCCEEDED( hr ), "Invalid D3D op" );
#endif
		}

		renderPrims.submit();
	}
	Void RSpriteGroup::update()
	{
		if( !flags.bits.update ) {
			return;
		}

		RSprite *spriteToDelete = nullptr;

		for( RSprite *sprite = grp_updateList.head(); sprite != nullptr; sprite = sprite->grp_updateLink.next() ) {
			if( spriteToDelete != nullptr ) {
				delete spriteToDelete;
				spriteToDelete = nullptr;
			}

			const Bool keep = sprite->updateAnimation();
			if( keep == true ) {
				continue;
			}

			spriteToDelete = sprite;
		}

		if( spriteToDelete != nullptr ) {
			delete spriteToDelete;
			spriteToDelete = nullptr;
		}
	}

	RSprite *RSpriteGroup::newSprite()
	{
		RSprite *spr = new RSprite();
		if( !AX_VERIFY_MSG( spr != nullptr, "Out of memory" ) ) {
			return nullptr;
		}

		SSpriteFrame *frame = spr->addDefaultFrame();
		if( !AX_VERIFY_MSG( frame != nullptr, "Out of memory" ) ) {
			delete spr;
			return nullptr;
		}

		spr->setGroup( this );

		return spr;
	}
	RSprite *RSpriteGroup::loadAnimSprite( RTexture *texture, S32 cellResX, S32 cellResY, S32 startFrame, S32 numFrames, S32 offX, S32 offY, S32 padX, S32 padY )
	{
		if( !AX_VERIFY_NOT_NULL( texture ) ) {
			return nullptr;
		}
		if( !AX_VERIFY_MSG( cellResX > 0, "Invalid cell resolution on x-axis" ) ) {
			return nullptr;
		}
		if( !AX_VERIFY_MSG( cellResY > 0, "Invalid cell resolution on y-axis" ) ) {
			return nullptr;
		}
		if( !AX_VERIFY_MSG( offX >= 0, "Negative offset is invalid" ) ) {
			return nullptr;
		}
		if( !AX_VERIFY_MSG( offY >= 0, "Negative offset is invalid" ) ) {
			return nullptr;
		}

		//
		//	TODO: Support negative startFrame (for beginning from end) and negative
		//	-     numFrames for "all frames after startFrame except the last 'n'"
		//
		if( !AX_VERIFY_MSG( startFrame >= 0, "Expected valid starting frame" ) ) {
			return nullptr;
		}
		if( !AX_VERIFY_MSG( numFrames >= 0, "Expected valid frame count" ) ) {
			return nullptr;
		}

		const SPixelVec2 texOff = { ( U16 )offX, ( U16 )offY };
		const SPixelVec2 texResFull = texture->getResolution();
		const SPixelVec2 texRes = {
			( U16 )( texResFull.x - texOff.x ),
			( U16 )( texResFull.y - texOff.y )
		};
		const SPixelVec2 texCell = {
			( U16 )( cellResX + padX ),
			( U16 )( cellResY + padY )
		};
		const SPixelVec2 numCells = {
			( U16 )( texRes.x/texCell.x ),
			( U16 )( texRes.y/texCell.y )
		};

		const unsigned startingFrame = ( unsigned )startFrame;
		const unsigned maxFrames = numCells.x*numCells.y;

		if( !AX_VERIFY_MSG( startingFrame < maxFrames, "Starting index is too high" ) ) {
			return nullptr;
		}

		if( !numFrames ) {
			numFrames = maxFrames - startFrame;
		}

		RSprite *const spr = newSprite();
		if( !AX_VERIFY_MEMORY( spr ) ) {
			return nullptr;
		}

		spr->setTexture( texture );

		for( S32 i = startFrame; i < startFrame + numFrames; ++i ) {
			const unsigned cellX = texOff.x + ( i%numCells.x )*texCell.x;
			const unsigned cellY = texOff.y + ( i/numCells.y )*texCell.y;

			SSpriteFrame *frame = nullptr;
			if( i == startFrame ) {
				frame = spr->getFramePointerByIndex( 0 );
			} else {
				frame = spr->addDefaultFrame();
			}
			if( !AX_VERIFY_MEMORY( frame ) ) {
				delete spr;
				return nullptr;
			}

			frame->sourceRect.off.x = ( U16 )cellX;
			frame->sourceRect.off.y = ( U16 )cellY;
			frame->sourceRect.res.x = cellResX;
			frame->sourceRect.res.y = cellResY;
		}

		spr->enableAutoupdate();

		return spr;
	}

	/*
	===============================================================================

		SPRITES

	===============================================================================
	*/
	RSprite::RSprite()
	: grp_parent( nullptr )
	, grp_spriteLink()
	, grp_updateLink()
	, bind_master( nullptr )
	, bind_list()
	, bind_link( this )
	, bind_flags({})
	, flip()
#if DOLL_HAS_SPRITE_ACTIONS
	, activeActions()
#endif
	, texture( nullptr )
	, numFrames( 0 )
	, maxFrames( 0 )
	, frames( nullptr )
	, currentInterpolatedFrame()
	, baseTransform( SSpriteTransform::identity )
	, offset()
	, microsecPerFrame( 16666 )
	, curFrame( 0 )
	, loopFrameRange()
	, loopStartTime( 0 )
	, animMode( EAnimationMode::PlayOnce )
	, visible( true )
	{
		loopFrameRange[ 0 ] = 0;
		loopFrameRange[ 1 ] = 0;

		grp_spriteLink.setNode( this );
		grp_updateLink.setNode( this );

		bind_flags.mask = 0;

		flip[ 0 ] = false;
		flip[ 1 ] = false;
	}
	RSprite::~RSprite()
	{
	#if 0
		char buf[ 512 ];

		sprintf_s( buf, "~RSprite:%p", ( Void * )this );
		WriteConsoleLine( buf );
	#endif

		deleteAllFrames();
	}

	Void RSprite::moveFrame( const Vec2f &distance )
	{
		getFramePointer()->transform.move( distance );
	}
	Void RSprite::turnFrame( F32 delta )
	{
		getFramePointer()->transform.turn( delta );
	}
	Void RSprite::setFramePosition( const Vec2f &position )
	{
		getFramePointer()->transform.translation = position;
	}
	Void RSprite::setFrameRotation( F32 angle )
	{
		getFramePointer()->transform.rotation = wrap360( angle );
	}
	Void RSprite::setFrameScale( const Vec2f &scale )
	{
		getFramePointer()->transform.scale = scale;
	}
	Void RSprite::setFrameSourceRectangle( const SPixelRect &sourceRect )
	{
		getFramePointer()->sourceRect = sourceRect;
	}
	static inline U32 _swapRB( U32 color )
	{
		U32 stripped = color & 0xFF00FF00;
		U32 r = ( color & 0x00FF0000 ) >> 16;
		U32 b = ( color & 0x000000FF ) >> 0;

		return stripped | ( b << 16 ) | ( r << 0 );
	}
	Void RSprite::setFrameCornerDiffuses( U32 diffuseTL, U32 diffuseTR, U32 diffuseBL, U32 diffuseBR )
	{
		SSpriteFrame *const frame = getFramePointer();

		frame->diffuse[ 0 ] = _swapRB( diffuseTL );
		frame->diffuse[ 1 ] = _swapRB( diffuseTR );
		frame->diffuse[ 2 ] = _swapRB( diffuseBL );
		frame->diffuse[ 3 ] = _swapRB( diffuseBR );
	}

	Void RSprite::deleteAllFrames()
	{
		curFrame = 0;
		stopAnimation();

		delete [] frames;
		frames = nullptr;

		numFrames = 0;
		maxFrames = 0;
	}

	SSpriteFrame *RSprite::addDefaultFrame()
	{
		static const SPixelVec2 pixelVector2_zero = { 0, 0 };
		if( numFrames==maxFrames ) {
			SSpriteFrame *p = new SSpriteFrame[ maxFrames + 8 ];
			if( !AX_VERIFY_MSG( p!=nullptr, "Out of memory" ) ) {
				return nullptr;
			}

			if( frames!=nullptr ) {
				memcpy( &p[ 0 ], &frames[ 0 ], sizeof( *p )*maxFrames );
			}

			maxFrames += 8;
			frames = p;
		}

		SSpriteFrame &frame = frames[ numFrames++ ];
		const SPixelVec2 resolution = ( texture!=nullptr ) ?
			texture->getResolution() : pixelVector2_zero;

		frame.transform.translation = 0;
		frame.transform.rotation = 0;
		frame.transform.scale = 1;
		frame.sourceRect.off.x = 0;
		frame.sourceRect.off.y = 0;
		frame.sourceRect.res.x = resolution.x;
		frame.sourceRect.res.y = resolution.y;
		frame.diffuse[ 0 ] = 0xFFFFFFFF;
		frame.diffuse[ 1 ] = 0xFFFFFFFF;
		frame.diffuse[ 2 ] = 0xFFFFFFFF;
		frame.diffuse[ 3 ] = 0xFFFFFFFF;

		return &frame;
	}

	Void RSprite::setGroup( RSpriteGroup *newGroup )
	{
		if( !newGroup ) {
			newGroup = g_spriteMgr.getDefaultSpriteGroup();
		}

		grp_spriteLink.unlink();
		newGroup->grp_spriteList.addTail( grp_spriteLink );
		grp_parent = newGroup;
	}

	Bool RSprite::updateAnimation()
	{
		// how far along are we?
		const U64 elapsedMicrosecs = microseconds() - loopStartTime;
		const F64 elapsedTime = microsecondsToSeconds( elapsedMicrosecs );
		const U32 microsecSinceStart = ( U32 )( 1000000.0*elapsedTime );
		const U32 framesSinceStart = microsecSinceStart/microsecPerFrame;
		const U32 frameAfterCurrent = framesSinceStart + 1;
		const F64 beginFrameTime = F64( framesSinceStart*microsecPerFrame );
		//const F64 nextFrameTime = F64( frameAfterCurrent*microsecPerFrame );
		const F64 elapsedFrameTime = microsecSinceStart - beginFrameTime;
		const F64 frameTime = elapsedFrameTime/F64( microsecPerFrame );
		const U32 rangeSize = ( loopFrameRange[ 1 ] - loopFrameRange[ 0 ] ) + 1;
		const U32 currentRangedFrame = framesSinceStart%rangeSize;
		const U32 nextRangedFrame = frameAfterCurrent%rangeSize;

		// is an animation playing? (animations have at least one frame)
		if( !rangeSize ) {
			// keep the sprite alive still
			return true;
		}

		// determine what the current frame would be for looping
		//const U32 framesWithinRange = framesSinceStart%rangeSize;
		U32 nextFrameIndex = loopFrameRange[ 0 ] + nextRangedFrame;
		U32 currentFrameIndex = loopFrameRange[ 0 ] + currentRangedFrame;

		// has the animation ended?
		if( framesSinceStart>rangeSize ) {
			if( animMode==EAnimationMode::DestroyAtEnd ) {
				// kill the sprite (let the handling system know)
				return false;
			}

			if( animMode!=EAnimationMode::Repeat ) {
				// if we're not looping, we should clamp to the last frame
				currentFrameIndex = loopFrameRange[ 0 ] + rangeSize - 1;
				nextFrameIndex = currentFrameIndex;
			}
		}

		// interpolate between each frame
		interpolateAnimationFrames( currentFrameIndex, nextFrameIndex,
			F32( frameTime ) );

		// don't forget to tell the system which frame to use
		curFrame = currentFrameIndex;

		// everything was successful and the sprite should not be deleted
		return true;
	}
	static inline U32 _interpolateColor( U32 colorA, U32 colorB, F32 t )
	{
		const F64 srcA[ 4 ] = {
			F64( ( colorA&0xFF000000 )>>24 ),
			F64( ( colorA&0x00FF0000 )>>16 ),
			F64( ( colorA&0x0000FF00 )>>8 ),
			F64( ( colorA&0x000000FF )>>0 )
		};
		const F64 srcB[ 4 ] = {
			F64( ( colorB&0xFF000000 )>>24 ),
			F64( ( colorB&0x00FF0000 )>>16 ),
			F64( ( colorB&0x0000FF00 )>>8 ),
			F64( ( colorB&0x000000FF )>>0 )
		};

		const F64 dst[ 4 ] = {
			lerp( srcA[ 0 ], srcB[ 0 ], t ),
			lerp( srcA[ 1 ], srcB[ 1 ], t ),
			lerp( srcA[ 2 ], srcB[ 2 ], t ),
			lerp( srcA[ 3 ], srcB[ 3 ], t ),
		};

		const U32 dstBytes[ 4 ] = {
			U32( dst[ 0 ]*255.0 ),
			U32( dst[ 1 ]*255.0 ),
			U32( dst[ 2 ]*255.0 ),
			U32( dst[ 3 ]*255.0 )
		};

		return ( dstBytes[ 0 ]<<24 )|( dstBytes[ 1 ]<<16 )|( dstBytes[ 2 ]<<8 )|( dstBytes[ 3 ]<<0 );
	}
	Void RSprite::interpolateAnimationFrames( U32 originalFrameIndex, U32 currentFrameIndex, F32 frameTime )
	{
		AX_ASSERT_MSG( originalFrameIndex < numFrames, "Index out of range" );
		AX_ASSERT_MSG( currentFrameIndex < numFrames, "Index out of range" );

		// retrieve each frame
		SSpriteFrame &dstFrame = currentInterpolatedFrame;
		const SSpriteFrame &srcFrameA = frames[ originalFrameIndex ];
		const SSpriteFrame &srcFrameB = frames[ currentFrameIndex ];

		// interpolate between each transform
		SSpriteTransform &dstTransform = dstFrame.transform;
		const SSpriteTransform &srcTransformA = srcFrameA.transform;
		const SSpriteTransform &srcTransformB = srcFrameB.transform;

		dstTransform.translation = lerp( srcTransformA.translation, srcTransformB.translation, frameTime );
		dstTransform.rotation = slerp( srcTransformA.rotation, srcTransformB.rotation, frameTime );
		dstTransform.scale = lerp( srcTransformA.scale, srcTransformB.scale, frameTime );

		// interpolate the colors
		for( size_t i = 0; i < 4; ++i ) {
			dstFrame.diffuse[ i ] = _interpolateColor( srcFrameA.diffuse[ i ], srcFrameB.diffuse[ i ], frameTime );
		}

		// copy over the current frame's texture coordinates
		dstFrame.sourceRect = srcFrameA.sourceRect;
	}

	SSpriteTransform RSprite::getGlobalTransform() const
	{
		const SSpriteTransform &frameTransform = getFrameTransform_const();
		const SSpriteTransform transform = baseTransform + frameTransform;

		if( !bind_master ) {
			return transform;
		}

		SSpriteTransform globalTransform = bind_master->getGlobalTransform();
		if( !bind_flags.bits.translation ) {
			globalTransform.translation = 0;
		}
		if( !bind_flags.bits.rotation ) {
			globalTransform.rotation = 0;
		}
		if( !bind_flags.bits.scale ) {
			globalTransform.scale = 1;
		}

		globalTransform.translation += rotate( transform.translation, globalTransform.rotation );
		globalTransform.rotation += transform.rotation;
		globalTransform.scale *= transform.scale;

		return globalTransform;
	}
	Void RSprite::bind( RSprite *master )
	{
		if( bind_master != nullptr ) {
			bind_master->bind_list.unlink( bind_link );
			bind_master = nullptr;

			bind_flags.bits.translation = false;
			bind_flags.bits.rotation = false;
			bind_flags.bits.scale = false;
		}

		if( !master ) {
			return;
		}

		master->bind_list.addTail( bind_link );
		bind_master = master;

		bind_flags.bits.translation = true;
		bind_flags.bits.rotation = true;
		bind_flags.bits.scale = true;
	}

	Void RSprite::enableAutoupdate()
	{
		if( !grp_updateLink.list() ) {
			return;
		}

		AX_ASSERT_NOT_NULL( grp_parent );
		grp_parent->grp_updateList.addTail( grp_updateLink );
	}
	Void RSprite::disableAutoupdate()
	{
		if( !grp_updateLink.list() ) {
			return;
		}
	
		AX_ASSERT_NOT_NULL( grp_parent );
		grp_parent->grp_updateList.unlink( grp_updateLink );
	}
	Bool RSprite::isAutoupdateEnabled() const
	{
		return grp_updateLink.list() != nullptr;
	}

	/*
	===============================================================================

		EASY SPRITES INTERFACE

	===============================================================================
	*/

	DOLL_FUNC RSprite *DOLL_API gfx_newSprite()
	{
		return g_spriteMgr.getDefaultSpriteGroup()->newSprite();
	}
	DOLL_FUNC RSprite *DOLL_API gfx_newSpriteInGroup( RSpriteGroup *group )
	{
		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup();
		}

		return group->newSprite();
	}
	DOLL_FUNC RSprite *DOLL_API gfx_deleteSprite( RSprite *spr )
	{
		delete spr;
		return nullptr;
	}
	DOLL_FUNC RSprite *DOLL_API gfx_loadAnimSprite( U16 img, S32 cellResX, S32 cellResY, S32 startFrame, S32 numFrames, S32 offX, S32 offY, S32 padX, S32 padY )
	{
		return g_spriteMgr.getDefaultSpriteGroup()->loadAnimSprite( g_textureMgr.getTextureById( img ), cellResX, cellResY, startFrame, numFrames, offX, offY, padX, padY );
	}
	DOLL_FUNC RSprite *DOLL_API gfx_loadAnimSpriteInGroup( RSpriteGroup *group, RTexture *texture, S32 cellResX, S32 cellResY, S32 startFrame, S32 numFrames, S32 offX, S32 offY, S32 padX, S32 padY )
	{
		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup();
		}

		return group->loadAnimSprite( texture, cellResX, cellResY, startFrame,
			numFrames, offX, offY, padX, padY );
	}

	DOLL_FUNC RSpriteGroup *DOLL_API gfx_newSpriteGroup()
	{
		return g_spriteMgr.newSpriteGroup();
	}
	DOLL_FUNC RSpriteGroup *DOLL_API gfx_deleteSpriteGroup( RSpriteGroup *group )
	{
		delete group;
		return nullptr;
	}
	DOLL_FUNC RSpriteGroup *DOLL_API gfx_getDefaultSpriteGroup()
	{
		return g_spriteMgr.getDefaultSpriteGroup();
	}
	DOLL_FUNC Void DOLL_API gfx_showSpriteGroup( RSpriteGroup *group )
	{
		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup();
		}

		group->setVisible( true );
	}
	DOLL_FUNC Void DOLL_API gfx_hideSpriteGroup( RSpriteGroup *group )
	{
		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup();
		}

		group->setVisible( false );
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteGroupVisible( const RSpriteGroup *group )
	{
		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup_const();
		}

		return +group->isVisible();
	}
	DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupScissor( RSpriteGroup *group, S32 posX, S32 posY, S32 resX, S32 resY )
	{
		const SPixelRect rect = {
			{ ( U16 )posX, ( U16 )posY },
			{ ( U16 )resX, ( U16 )resY }
		};

		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup();
		}

		group->enableScissor( rect );
	}
	DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupScissor( RSpriteGroup *group )
	{
		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup();
		}

		group->disableScissor();
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteGroupScissorEnabled( const RSpriteGroup *group )
	{
		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup_const();
		}

		return +group->isScissorEnabled();
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorPositionX( const RSpriteGroup *group )
	{
		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup_const();
		}

		return group->getScissorRectangle().off.x;
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorPositionY( const RSpriteGroup *group )
	{
		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup_const();
		}

		return group->getScissorRectangle().off.y;
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorResolutionX( const RSpriteGroup *group )
	{
		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup_const();
		}

		return group->getScissorRectangle().res.x;
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorResolutionY( const RSpriteGroup *group )
	{
		if( !group ) {
			group = g_spriteMgr.getDefaultSpriteGroup_const();
		}

		return group->getScissorRectangle().res.y;
	}

#define EXPECT_SPRITE( sprite )\
	if( !AX_VERIFY_MSG( sprite != nullptr, "Invalid sprite" ) )
#define EXPECT_SPRITE_GROUP( group )\
	if( !AX_VERIFY_MSG( group != nullptr, "Invalid sprite group" ) )

	DOLL_FUNC Void DOLL_API gfx_moveSprite( RSprite *sprite, F32 x, F32 y )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->move( Vec2f( x, y ) );
	}
	DOLL_FUNC Void DOLL_API gfx_turnSprite( RSprite *sprite, F32 theta )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->turn( theta );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpritePosition( RSprite *sprite, F32 x, F32 y )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setPosition( Vec2f( x, y ) );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteRotation( RSprite *sprite, F32 angle )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setRotation( angle );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteScale( RSprite *sprite, F32 x, F32 y )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setScale( Vec2f( x, y ) );
	}
	DOLL_FUNC Void DOLL_API gfx_moveSpriteFrame( RSprite *sprite, F32 x, F32 y )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->moveFrame( Vec2f( x, y ) );
	}
	DOLL_FUNC Void DOLL_API gfx_turnSpriteFrame( RSprite *sprite, F32 theta )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->turnFrame( theta );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteFramePosition( RSprite *sprite, F32 x, F32 y )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setFramePosition( Vec2f( x, y ) );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameRotation( RSprite *sprite, F32 angle )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setFrameRotation( angle );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameScale( RSprite *sprite, F32 x, F32 y )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setFrameScale( Vec2f( x, y ) );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameSourceRectangle( RSprite *sprite, S32 posX, S32 posY, S32 resX, S32 resY )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		const SPixelRect rect = {
			{ ( U16 )posX, ( U16 )posY },
			{ ( U16 )resX, ( U16 )resY }
		};
		sprite->setFrameSourceRectangle( rect );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameCornerColors( RSprite *sprite, U32 colorTL, U32 colorTR, U32 colorBL, U32 colorBR )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		U32 diffuseTL = sprite->getFrameCornerDiffuse( 0 ) & 0xFF000000;
		U32 diffuseTR = sprite->getFrameCornerDiffuse( 1 ) & 0xFF000000;
		U32 diffuseBL = sprite->getFrameCornerDiffuse( 2 ) & 0xFF000000;
		U32 diffuseBR = sprite->getFrameCornerDiffuse( 3 ) & 0xFF000000;

		diffuseTL |= ( colorTL & 0x00FFFFFF );
		diffuseTR |= ( colorTR & 0x00FFFFFF );
		diffuseBL |= ( colorBL & 0x00FFFFFF );
		diffuseBR |= ( colorBR & 0x00FFFFFF );

		sprite->setFrameCornerDiffuses( diffuseTL, diffuseTR, diffuseBL, diffuseBR );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameCornerAlphas( RSprite *sprite, U32 alphaTL, U32 alphaTR, U32 alphaBL, U32 alphaBR )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		U32 diffuseTL = sprite->getFrameCornerDiffuse( 0 ) & 0x00FFFFFF;
		U32 diffuseTR = sprite->getFrameCornerDiffuse( 1 ) & 0x00FFFFFF;
		U32 diffuseBL = sprite->getFrameCornerDiffuse( 2 ) & 0x00FFFFFF;
		U32 diffuseBR = sprite->getFrameCornerDiffuse( 3 ) & 0x00FFFFFF;
	
		diffuseTL |= ( alphaTL & 0x000000FF )<<24;
		diffuseTR |= ( alphaTR & 0x000000FF )<<24;
		diffuseBL |= ( alphaBL & 0x000000FF )<<24;
		diffuseBR |= ( alphaBR & 0x000000FF )<<24;

		sprite->setFrameCornerDiffuses( diffuseTL, diffuseTR, diffuseBL, diffuseBR );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameColor( RSprite *sprite, U32 color )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		U32 diffuse = sprite->getFrameDiffuse() & 0xFF000000;
		diffuse |= ( color & 0x00FFFFFF );

		sprite->setFrameDiffuse( diffuse );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameAlpha( RSprite *sprite, U32 alpha )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		U32 diffuse = sprite->getFrameDiffuse() & 0x00FFFFFF;
		diffuse |= ( alpha & 0x000000FF )<<24;

		sprite->setFrameDiffuse( diffuse );
	}
	DOLL_FUNC Void DOLL_API gfx_deleteAllSpriteFrames( RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->deleteAllFrames();
	}

	DOLL_FUNC S32 DOLL_API gfx_addSpriteFrame( RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		if( !sprite->addDefaultFrame() ) {
			return 0;
		}

		return ( S32 )sprite->getFrameCount();
	}
	DOLL_FUNC SSpriteFrame *DOLL_API gfx_duplicateCurrentSpriteFrame( RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		if( !sprite->duplicateCurrentFrame() ) {
			return 0;
		}

		return sprite->duplicateCurrentFrame();
	}

	DOLL_FUNC S32 DOLL_API gfx_getSpritePosition( const RSprite *sprite, F32 *x, F32 *y )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		const Vec2f &pos = sprite->getPosition();

		if( x!=nullptr ) {
			*x = pos.x;
		}
		if( y!=nullptr ) {
			*y = pos.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpritePositionX( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getPosition().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpritePositionY( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getPosition().y;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteRotation( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getRotation();
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteScale( const RSprite *sprite, F32 *x, F32 *y )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		const Vec2f &scale = sprite->getScale();

		if( x!=nullptr ) {
			*x = scale.x;
		}
		if( y!=nullptr ) {
			*y = scale.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteScaleX( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getScale().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteScaleY( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getScale().y;
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteFramePosition( const RSprite *sprite, F32 *x, F32 *y )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		const Vec2f &pos = sprite->getFramePosition();

		if( x!=nullptr ) {
			*x = pos.x;
		}
		if( y!=nullptr ) {
			*y = pos.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteFramePositionX( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getFramePosition().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteFramePositionY( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getFramePosition().y;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteFrameRotation( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getFrameRotation();
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteFrameScale( const RSprite *sprite, F32 *x, F32 *y )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		const Vec2f &scale = sprite->getFrameScale();

		if( x!=nullptr ) {
			*x = scale.x;
		}
		if( y!=nullptr ) {
			*y = scale.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteFrameScaleX( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getFrameScale().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteFrameScaleY( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getFrameScale().y;
	}
	DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameCornerColor( const RSprite *sprite, U32 index )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getFrameCornerDiffuse( index ) & 0x00FFFFFF;
	}
	DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameCornerAlpha( const RSprite *sprite, U32 index )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return ( sprite->getFrameCornerDiffuse( index ) & 0xFF000000 )>>24;
	}
	DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameColor( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getFrameDiffuse() & 0x00FFFFFF;
	}
	DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameAlpha( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return ( sprite->getFrameDiffuse() & 0xFF000000 )>>24;
	}

	DOLL_FUNC Void DOLL_API gfx_setSpriteTexture( RSprite *sprite, U32 textureId )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setTexture( g_textureMgr.getTextureById( ( U16 )textureId ) );
	}
	DOLL_FUNC U32 DOLL_API gfx_getSpriteTexture( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		const RTexture *texture = sprite->getTexture_const();
		if( !texture ) {
			return 0;
		}

		return ( U32 )texture->getIdentifier();
	}

	DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameCount( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return ( U32 )sprite->getFrameCount();
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteCurrentFrame( RSprite *sprite, U32 frameIndex )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setCurrentFrame( frameIndex );
	}
	DOLL_FUNC U32 DOLL_API gfx_getSpriteCurrentFrame( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return ( U32 )sprite->getCurrentFrame();
	}

	DOLL_FUNC Void DOLL_API gfx_setSpriteFramesPerSecond( RSprite *sprite, F64 fps )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setFramesPerSecond( fps );
	}
	DOLL_FUNC F64 DOLL_API gfx_getSpriteFramesPerSecond( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getFramesPerSecond();
	}

	DOLL_FUNC Void DOLL_API gfx_playSpriteAnimation( RSprite *sprite, U32 beginFrame, U32 endFrame ) {
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->playAnimation( ( U32 )beginFrame, ( U32 )endFrame, RSprite::EAnimationMode::PlayOnce );
	}
	DOLL_FUNC Void DOLL_API gfx_loopSpriteAnimation( RSprite *sprite, U32 beginFrame, U32 endFrame )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->playAnimation( ( U32 )beginFrame, ( U32 )endFrame, RSprite::EAnimationMode::Repeat );
	}
	DOLL_FUNC Void DOLL_API gfx_playSpriteAnimationWithDeath( RSprite *sprite, U32 beginFrame, U32 endFrame )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->playAnimation( ( U32 )beginFrame, ( U32 )endFrame, RSprite::EAnimationMode::DestroyAtEnd );
	}
	DOLL_FUNC Void DOLL_API gfx_stopSpriteAnimation( RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->stopAnimation();
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteAnimationPlaying( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return +sprite->isAnimationPlaying();
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteAnimationLooping( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return +sprite->isAnimationLooping();
	}

	DOLL_FUNC Void DOLL_API gfx_setSpriteGroup( RSprite *sprite, RSpriteGroup *group )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setGroup( group );
	}
	DOLL_FUNC RSpriteGroup *DOLL_API gfx_getSpriteGroup( RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return nullptr;
		}

		return sprite->getGroup();
	}

	DOLL_FUNC Void DOLL_API gfx_bindSprite( RSprite *sprite, RSprite *master )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}
		EXPECT_SPRITE( master ) {
			return;
		}

		sprite->bind( master );
	}
	DOLL_FUNC Void DOLL_API gfx_bindSpriteTranslation( RSprite *sprite, RSprite *master )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}
		EXPECT_SPRITE( master ) {
			return;
		}

		sprite->bindTranslation( master );
	}
	DOLL_FUNC Void DOLL_API gfx_selectivelyBindSprite( RSprite *sprite, RSprite *master, S32 bindTranslation, S32 bindRotation, S32 bindScale )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}
		EXPECT_SPRITE( master ) {
			return;
		}

		sprite->bindSelectively( master, !!bindTranslation, !!bindRotation, !!bindScale );
	}
	DOLL_FUNC Void DOLL_API gfx_unbindSprite( RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->bind( nullptr );
	}
	DOLL_FUNC RSprite *DOLL_API gfx_getSpriteBindMaster( RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return nullptr;
		}

		return sprite->getBindMaster();
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteBoundToTranslation( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return +sprite->isBoundToTranslation();
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteBoundToRotation( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return +sprite->isBoundToRotation();
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteBoundToScale( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return +sprite->isBoundToScale();
	}

	DOLL_FUNC Void DOLL_API gfx_setSpriteOffset( RSprite *sprite, F32 x, F32 y )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setOffset( Vec2f( x, y ) );
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteOffset( const RSprite *sprite, F32 *x, F32 *y )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		const Vec2f &off = sprite->getOffset();

		if( x != nullptr ) {
			*x = off.x;
		}
		if( y != nullptr ) {
			*y = off.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteOffsetX( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getOffset().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteOffsetY( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getOffset().y;
	}

	DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupAutoupdating( RSpriteGroup *group )
	{
		if( !AX_VERIFY_MSG( group != nullptr, "Group is invalid" ) ) {
			return;
		}

		group->setAutoupdates( true );
	}
	DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupAutoupdating( RSpriteGroup *group )
	{
		if( !AX_VERIFY_MSG( group != nullptr, "Group is invalid" ) ) {
			return;
		}

		group->setAutoupdates( false );
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteGroupAutoupdatingEnabled( const RSpriteGroup *group )
	{
		if( !AX_VERIFY_MSG( group != nullptr, "Group is invalid" ) ) {
			return 0;
		}

		return +group->areAutoupdatesEnabled();
	}

	DOLL_FUNC Void DOLL_API gfx_enableSpriteAutoupdating( RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->enableAutoupdate();
	}
	DOLL_FUNC Void DOLL_API gfx_disableSpriteAutoupdating( RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->disableAutoupdate();
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteAutoupdatingEnabled( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return +sprite->isAutoupdateEnabled();
	}

	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupPosition( RSpriteGroup *group, F32 x, F32 y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return;
		}

		group->setTranslation( Vec2f( x, y ) );
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupPosition( const RSpriteGroup *group, F32 *x, F32 *y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		const Vec2f &pos = group->getTranslation();

		if( x != nullptr ) {
			*x = pos.x;
		}
		if( y != nullptr ) {
			*y = pos.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupPositionX( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getTranslation().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupPositionY( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getTranslation().y;
	}

	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupRotation( RSpriteGroup *group, F32 rotation )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return;
		}

		group->setRotation( rotation );
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupRotation( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getRotation();
	}

	DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupFollow( RSpriteGroup *group, const RSprite *follow )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return;
		}
		EXPECT_SPRITE( follow ) {
			return;
		}

		group->enableAutofollow( follow );
	}
	DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupFollow( RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return;
		}

		group->disableAutofollow();
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteGroupFollowEnabled( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->isAutofollowEnabled();
	}

	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupFollowMinimumDistance( RSpriteGroup *group, F32 x, F32 y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return;
		}

		group->setAutofollowMinimumDistance( Vec2f( x, y ) );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupFollowMaximumDistance( RSpriteGroup *group, F32 x, F32 y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return;
		}

		group->setAutofollowMaximumDistance( Vec2f( x, y ) );
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupFollowMinimumDistance( const RSpriteGroup *group, F32 *x, F32 *y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		const Vec2f &dist = group->getAutofollowMinimumDistance();

		if( x != nullptr ) {
			*x = dist.x;
		}
		if( y != nullptr ) {
			*y = dist.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMinimumDistanceX( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getAutofollowMinimumDistance().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMinimumDistanceY( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getAutofollowMinimumDistance().y;
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupFollowMaximumDistance( const RSpriteGroup *group, F32 *x, F32 *y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		const Vec2f &dist = group->getAutofollowMaximumDistance();

		if( x != nullptr ) {
			*x = dist.x;
		}
		if( y != nullptr ) {
			*y = dist.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMaximumDistanceX( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getAutofollowMaximumDistance().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMaximumDistanceY( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getAutofollowMaximumDistance().y;
	}

	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupFollowSpeed( RSpriteGroup *group, F32 x, F32 y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return;
		}

		group->setAutofollowSpeed( Vec2f( x, y ) );
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupFollowSpeed( const RSpriteGroup *group, F32 *x, F32 *y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		const Vec2f &speed = group->getAutofollowSpeed();

		if( x != nullptr ) {
			*x = speed.x;
		}
		if( y != nullptr ) {
			*y = speed.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowSpeedX( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getAutofollowSpeed().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowSpeedY( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getAutofollowSpeed().y;
	}

	DOLL_FUNC Void DOLL_API gfx_flipSpriteHorizontal( RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->flipHorizontal();
	}
	DOLL_FUNC Void DOLL_API gfx_flipSpriteVertical( RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->flipVertical();
	}

	DOLL_FUNC Void DOLL_API gfx_setSpriteMirrorHorizontal( RSprite *sprite, Bool mirror )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setMirrorHorizontal( mirror );
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteMirrorVertical( RSprite *sprite, Bool mirror )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setMirrorVertical( mirror );
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteMirrorHorizontal( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return +sprite->getMirrorHorizontal();
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteMirrorVertical( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return +sprite->getMirrorVertical();
	}

	DOLL_FUNC S32 DOLL_API gfx_getSpriteSize( const RSprite *sprite, F32 *x, F32 *y )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		Vec2f size = sprite->getSize();

		if( x != nullptr ) {
			*x = size.x;
		}
		if( y != nullptr ) {
			*y = size.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteSizeX( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getSize().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteSizeY( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return sprite->getSize().y;
	}

	DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupVirtualResolution( RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return;
		}

		group->enableVirtualResolution();
	}
	DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupVirtualResolution( RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return;
		}

		group->disableVirtualResolution();
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteGroupVirtualResolutionEnabled( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return +group->isVirtualResolutionEnabled();
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupVirtualResolution( RSpriteGroup *group, F32 x, F32 y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return;
		}

		group->setVirtualResolution( Vec2f( x, y ) );
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupVirtualResolution( const RSpriteGroup *group, F32 *x, F32 *y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		const Vec2f &res = group->getVirtualResolution();

		if( x != nullptr ) {
			*x = res.x;
		}
		if( y != nullptr ) {
			*y = res.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualResolutionX( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getVirtualResolution().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualResolutionY( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getVirtualResolution().y;
	}
	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupVirtualOrigin( RSpriteGroup *group, F32 x, F32 y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return;
		}

		group->setVirtualOrigin( Vec2f( x, y ) );
	}
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupVirtualOrigin( const RSpriteGroup *group, F32 *x, F32 *y )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		const Vec2f &org = group->getVirtualOrigin();

		if( x != nullptr ) {
			*x = org.x;
		}
		if( y != nullptr ) {
			*y = org.y;
		}

		return 1;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualOriginX( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getVirtualOrigin().x;
	}
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualOriginY( const RSpriteGroup *group )
	{
		EXPECT_SPRITE_GROUP( group ) {
			return 0;
		}

		return group->getVirtualOrigin().y;
	}

	DOLL_FUNC Void DOLL_API gfx_setSpriteVisible( RSprite *sprite, S32 visible )
	{
		EXPECT_SPRITE( sprite ) {
			return;
		}

		sprite->setVisible( !!visible );
	}
	DOLL_FUNC S32 DOLL_API gfx_isSpriteVisible( const RSprite *sprite )
	{
		EXPECT_SPRITE( sprite ) {
			return 0;
		}

		return +sprite->isVisible();
	}

}
