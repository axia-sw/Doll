#pragma once

#include "../Core/Defs.hpp"
#include "../Core/Memory.hpp"
#include "../Core/MemoryTags.hpp"
#include "../Math/Math.hpp"
#include "Texture.hpp"
#include "API-GL.hpp"

#ifndef DOLL_HAS_SPRITE_ACTIONS
# define DOLL_HAS_SPRITE_ACTIONS 1
#endif

namespace doll
{

	class RSprite;
	class RSpriteGroup;
	class MSprites;

	class ActiveAction; // in "doll-gfx-action.hpp"

	struct SSpriteTransform
	{
		Vec2f translation;
		F32   rotation;
		Vec2f scale;

		inline SSpriteTransform()
		: translation()
		, rotation( 0 )
		, scale( 1 )
		{
		}
		inline SSpriteTransform( const Vec2f &translation, F32 rotation, const Vec2f &scale )
		: translation( translation )
		, rotation( wrap360( rotation ) )
		, scale( scale )
		{
		}
		inline SSpriteTransform( const SSpriteTransform &rhs )
		: translation( rhs.translation )
		, rotation( rhs.rotation )
		, scale( rhs.scale )
		{
		}

		inline Void move( const Vec2f &distance )
		{
			const F32 s = sin( rotation );
			const F32 c = cos( rotation );

			translation += distance*Vec2f( s, c );
		}
		inline Void turn( F32 delta )
		{
			rotation = wrap360( rotation + delta );
		}

		inline SSpriteTransform invert() const
		{
			SSpriteTransform inv;

			const F32 invAngle = wrap360( -rotation );
			const Vec2f rot( sin( invAngle ), cos( invAngle ) );

			inv.translation.x = -( translation.x*rot.y - translation.y*rot.x );
			inv.translation.y = -( translation.x*rot.x + translation.y*rot.y );
			inv.rotation = invAngle;
			inv.scale.x = 1.0f/scale.x;
			inv.scale.y = 1.0f/scale.y;

			return inv;
		}

		inline SSpriteTransform operator+( const SSpriteTransform &rhs ) const
		{
			return ( *this )*rhs;
		}
		inline SSpriteTransform operator*( const SSpriteTransform &rhs ) const
		{
			return
				SSpriteTransform
				(
					translation + rhs.translation,
					wrap360( rotation + rhs.rotation ),
					scale*rhs.scale
				);
		}

		inline Vec2f operator*( const Vec2f &rhs ) const
		{
			return translation + doll::rotate( rhs*scale, rotation );
		}
		inline Vec2f rotate( const Vec2f &rhs ) const
		{
			return doll::rotate( rhs, rotation );
		}

		inline SSpriteTransform &operator=( const SSpriteTransform &rhs )
		{
			translation = rhs.translation;
			rotation = rhs.rotation;
			scale = rhs.scale;

			return *this;
		}
		inline SSpriteTransform &operator+=( const SSpriteTransform &rhs )
		{
			*this = *this + rhs;
			return *this;
		}

		static SSpriteTransform identity;
	};
	struct SSpriteFrame
	{
		SSpriteTransform transform;

		SPixelRect sourceRect;
		U32 diffuse[ 4 ];
	};

	/*
	===============================================================================

		SPRITE
		Represents a 2D image on the screen

	===============================================================================
	*/

	class RSprite: public TPoolObject< RSprite, kTag_Sprite >
	{
	friend class RSpriteGroup;
	friend class ActiveAction;
	public:
		enum class EAnimationMode
		{
			PlayOnce,
			Repeat,
			DestroyAtEnd
		};

		RSprite();
		~RSprite();

		inline Void move( const Vec2f &distance )
		{
			baseTransform.move( distance );
		}
		inline Void turn( F32 delta )
		{
			baseTransform.turn( delta );
		}
		inline Void setPosition( const Vec2f &position )
		{
			baseTransform.translation = position;
		}
		inline Void setRotation( F32 angle )
		{
			baseTransform.rotation = angle;
		}
		inline Void setScale( const Vec2f &scale )
		{
			baseTransform.scale = scale;
		}
		inline const Vec2f &getPosition() const
		{
			return baseTransform.translation;
		}
		inline F32 getRotation() const
		{
			return baseTransform.rotation;
		}
		inline const Vec2f &getScale() const
		{
			return baseTransform.scale;
		}

		Void moveFrame( const Vec2f &distance );
		Void turnFrame( F32 delta );
		Void setFramePosition( const Vec2f &position );
		Void setFrameRotation( F32 angle );
		Void setFrameScale( const Vec2f &scale );
		Void setFrameSourceRectangle( const SPixelRect &sourceRect );
		Void setFrameCornerDiffuses( U32 diffuseTL, U32 diffuseTR, U32 diffuseBL, U32 diffuseBR );
		inline Void setFrameDiffuse( U32 diffuse )
		{
			setFrameCornerDiffuses( diffuse, diffuse, diffuse, diffuse );
		}

		Void deleteAllFrames();

		Void enableAutoupdate();
		Void disableAutoupdate();
		Bool isAutoupdateEnabled() const;

		SSpriteFrame *addDefaultFrame();
		inline SSpriteFrame *duplicateCurrentFrame()
		{
			if( !AX_VERIFY_MSG( numFrames>0, "No current frame" ) ) {
				return nullptr;
			}
			AX_ASSERT_MSG( frames!=nullptr, "Invalid internal state" );

			const SSpriteFrame *const currentFrame = &frames[ numFrames ];
			SSpriteFrame *const frame = addDefaultFrame();
			if( !AX_VERIFY_MSG( frame!=nullptr, "Failed to addDefaultFrame" ) ) {
				return nullptr;
			}

			*frame = *currentFrame;
			return frame;
		}

		inline Void setCurrentFrame( U32 frameIndex )
		{
			if( !numFrames ) {
				return;
			}

			if( frameIndex >= numFrames ) {
				frameIndex = numFrames - 1;
			}

			curFrame = frameIndex;
		}

		inline const Vec2f &getFramePosition() const
		{
			const SSpriteFrame *const frame = getFramePointer();
			if( !frame ) {
				static const Vec2f zero( 0, 0 );
				return zero;
			}

			return frame->transform.translation;
		}
		inline F32 getFrameRotation() const
		{
			const SSpriteFrame *const frame = getFramePointer();
			if( !frame ) {
				return 0;
			}

			return frame->transform.rotation;
		}
		inline const Vec2f &getFrameScale() const
		{
			const SSpriteFrame *const frame = getFramePointer();
			if( !frame ) {
				static const Vec2f one( 1, 1 );
				return one;
			}

			return frame->transform.scale;
		}
		inline const SPixelRect getFrameSourceRectangle() const
		{
			const SSpriteFrame *const frame = getFramePointer();
			if( !frame ) {
				static const SPixelRect zero = { { 0, 0 }, { 0, 0 } };
				return zero;
			}

			return frame->sourceRect;
		}
		inline U32 getFrameCornerDiffuse( size_t index ) const
		{
			const SSpriteFrame *const frame = getFramePointer();
			if( !frame ) {
				return 0x00000000;
			}

			if( !AX_VERIFY_MSG( index<4, "Index out of range" ) ) {
				return 0x00000000;
			}

			return frame->diffuse[ index ];
		}
		inline U32 getFrameDiffuse() const {
			return getFrameCornerDiffuse( 0 );
		}

		inline Void setTexture( RTexture *newTexture )
		{
			setTexture_raw( newTexture );
			if( !newTexture ) {
				return;
			}

			const SPixelVec2 &resolution = newTexture->getResolution();
			for( size_t i=0; i<numFrames; ++i ) {
				frames[ i ].sourceRect.res.x = resolution.x;
				frames[ i ].sourceRect.res.y = resolution.y;
			}
		}
		inline Void setTexture_raw( RTexture *newTexture )
		{
			texture = newTexture;
		}
		inline RTexture *getTexture()
		{
			return texture;
		}
		inline const RTexture *getTexture_const() const
		{
			return texture;
		}
		inline const RTexture *getTexture() const
		{
			return texture;
		}

		inline U32 getFrameCount() const
		{
			return numFrames;
		}
		inline U32 getCurrentFrame() const
		{
			return curFrame;
		}

		inline Void setFramesPerSecond( F64 fps )
		{
			microsecPerFrame = ( U32 )( 1000000.0/fps );
		}
		inline F64 getFramesPerSecond() const
		{
			return 1000000.0/( F64 )( microsecPerFrame );
		}

		inline SSpriteFrame *getFramePointer()
		{
			if( !frames ) {
				return nullptr;
			}

			return &frames[ curFrame ];
		}
		inline const SSpriteFrame *getFramePointer_const() const
		{
			if( !frames ) {
				return nullptr;
			}

			return &frames[ curFrame ];
		}
		inline const SSpriteFrame *getFramePointer() const
		{
			return getFramePointer_const();
		}
	
		inline SSpriteFrame *getFramePointerByIndex( size_t index )
		{
			return const_cast< SSpriteFrame * >( getFramePointerByIndex_const(
				index ) );
		}
		inline const SSpriteFrame *getFramePointerByIndex( size_t index ) const
		{
			return getFramePointerByIndex_const( index );
		}
		inline const SSpriteFrame *getFramePointerByIndex_const( size_t index ) const
		{
			if( !frames || index >= numFrames ) {
				return nullptr;
			}

			return &frames[ index ];
		}

		inline const SSpriteTransform &getFrameTransform_const() const
		{
			const SSpriteFrame *frame = getFramePointer_const();
			if( !frame ) {
				return SSpriteTransform::identity;
			}
		
			if( isAnimationPlaying() ) {
				return currentInterpolatedFrame.transform;
			}

			return frame->transform;
		}
		inline const SSpriteTransform &getFrameTransform() const
		{
			return getFrameTransform_const();
		}

		inline const SSpriteTransform &getBaseTransform_const() const
		{
			return baseTransform;
		}
		inline const SSpriteTransform &getBaseTransform() const
		{
			return baseTransform;
		}

		inline Void playAnimation( U32 beginFrame, U32 endFrame, EAnimationMode mode )
		{
			loopFrameRange[ 0 ] = beginFrame;
			loopFrameRange[ 1 ] = endFrame;

			loopStartTime = microseconds();
			curFrame = beginFrame;

			animMode = mode;
		}
		inline Void stopAnimation()
		{
			loopFrameRange[ 0 ] = curFrame;
			loopFrameRange[ 1 ] = curFrame;
		}
		// returns whether the sprite should exist or not
		Bool updateAnimation();
		Void interpolateAnimationFrames( U32 originalFrameIndex, U32 currentFrameIndex, F32 frameTime );
		inline Bool isAnimationPlaying() const
		{
			return ( loopFrameRange[ 1 ] - loopFrameRange[ 0 ] ) != 0;
		}
		inline Bool isAnimationLooping() const
		{
			return animMode==EAnimationMode::Repeat;
		}

		Void setGroup( RSpriteGroup *newGroup );
		inline RSpriteGroup *getGroup()
		{
			return grp_parent;
		}
		inline const RSpriteGroup *getGroup_const() const
		{
			return grp_parent;
		}
		inline const RSpriteGroup *getGroup() const
		{
			return grp_parent;
		}

		/*
			XXX: This is terribly inefficient.
		*/
		SSpriteTransform getGlobalTransform() const;

		Void bind( RSprite *master );
		inline Void bindTranslation( RSprite *master )
		{
			bind( master );
			bind_flags.bits.translation = true;
			bind_flags.bits.rotation = false;
			bind_flags.bits.scale = false;
		}
		inline Void bindSelectively( RSprite *master, Bool bindTranslation, Bool bindRotation, Bool bindScale )
		{
			bind( master );
			bind_flags.bits.translation = bindTranslation;
			bind_flags.bits.rotation = bindRotation;
			bind_flags.bits.scale = bindScale;
		}
		inline RSprite *getBindMaster()
		{
			return bind_master;
		}
		inline const RSprite *getBindMaster_const() const
		{
			return bind_master;
		}
		inline const RSprite *getBindMaster() const
		{
			return bind_master;
		}
		inline Bool isBoundToTranslation() const
		{
			return bind_flags.bits.translation;
		}
		inline Bool isBoundToRotation() const
		{
			return bind_flags.bits.rotation;
		}
		inline Bool isBoundToScale() const
		{
			return bind_flags.bits.scale;
		}

		inline Void setOffset( const Vec2f &newOffset )
		{
			offset = newOffset;
		}
		inline const Vec2f &getOffset() const
		{
			return offset;
		}

		inline Void flipHorizontal()
		{
			flip[ 0 ] ^= true;
		}
		inline Void flipVertical()
		{
			flip[ 1 ] ^= true;
		}

		inline Void setMirrorHorizontal( Bool mirror )
		{
			flip[ 0 ] = mirror;
		}
		inline Void setMirrorVertical( Bool mirror )
		{
			flip[ 1 ] = mirror;
		}
		inline Bool getMirrorHorizontal() const
		{
			return flip[ 0 ];
		}
		inline Bool getMirrorVertical() const
		{
			return flip[ 1 ];
		}

		inline Vec2f getSize() const
		{
			if( !frames ) {
				return Vec2f( 0, 0 );
			}

			AX_ASSERT_MSG( curFrame<numFrames, "Invalid frame index" );
			return Vec2f( frames[ curFrame ].sourceRect.res.x,
				frames[ curFrame ].sourceRect.res.y )*getScale();
		}

		inline Bool isVisible() const {
			return visible;
		}
		inline Void setVisible( Bool visibility ) {
			visible = visibility;
		}

	protected:
		RSpriteGroup *grp_parent;
		TIntrLink<RSprite> grp_spriteLink;
		TIntrLink<RSprite> grp_updateLink;

		RSprite *bind_master;
		TIntrList<RSprite> bind_list;
		TIntrLink<RSprite> bind_link;
		union {
			struct {
				Bool translation:1;
				Bool rotation:1;
				Bool scale:1;
			} bits;
			U32 mask;
		} bind_flags;

		Bool flip[ 2 ];
	
#if DOLL_HAS_SPRITE_ACTIONS
		TIntrList< ActiveAction > activeActions;
#endif

	private:
		RTexture *texture;

		//
		//	TODO: Use CArray<> for this.
		//
		U32 numFrames;
		U32 maxFrames;
		SSpriteFrame *frames;
		SSpriteFrame currentInterpolatedFrame;
		SSpriteTransform baseTransform;

		Vec2f offset;

		U32 microsecPerFrame;
		U32 curFrame;

		static U32 defaultMicrosecPerFrame;

		U32 loopFrameRange[ 2 ];
		U64 loopStartTime;
		EAnimationMode animMode;

		Bool visible;
	};

	/*
	===============================================================================

		SPRITE GROUPS
		Batches a set of sprites together to be drawn efficiently as well as to
		support shared settings

	===============================================================================
	*/

	class RSpriteGroup
	{
	friend class MSprites;
	friend class RSprite;
	public:
		RSpriteGroup();
		~RSpriteGroup();

		Void render_gl( S32 w, S32 h );
		Void update();

		RSprite *newSprite();
		RSprite *loadAnimSprite( RTexture *texture, S32 cellResX, S32 cellResY,
			S32 startFrame, S32 numFrames, S32 offX, S32 offY, S32 padX, S32 padY );

		inline Void setVisible( Bool newVisibility )
		{
			flags.bits.visible = newVisibility;
		}
		inline Bool isVisible() const
		{
			return flags.bits.visible;
		}

		inline Void enableScissor( const SPixelRect &newScissorRect )
		{
			flags.bits.scissor = true;
			scissorRect = newScissorRect;
		}
		inline Void disableScissor()
		{
			flags.bits.scissor = false;
		}
		inline Bool isScissorEnabled() const
		{
			return flags.bits.scissor;
		}
		inline const SPixelRect &getScissorRectangle() const
		{
			return scissorRect;
		}

		inline Void setAutoupdates( Bool newAutoupdate )
		{
			flags.bits.update = newAutoupdate;
		}
		inline Bool areAutoupdatesEnabled() const
		{
			return flags.bits.update;
		}

		inline Void setTranslation( const Vec2f &translation )
		{
			camera.translation = translation;
		}
		inline Void setRotation( F32 rotation )
		{
			camera.rotation = rotation;
		}
		inline const Vec2f &getTranslation() const
		{
			return camera.translation;
		}
		inline F32 getRotation() const
		{
			return camera.rotation;
		}

		inline Void enableAutofollow( const RSprite *follow )
		{
			if( !AX_VERIFY_MSG( follow!=nullptr, "Invalid follow sprite" ) ) {
				return;
			}

			camera.follow = follow;
		}
		inline Void disableAutofollow()
		{
			camera.follow = nullptr;
		}
		inline Bool isAutofollowEnabled() const
		{
			return camera.follow != nullptr;
		}

		inline Void setAutofollowMinimumDistance( const Vec2f &dist )
		{
			camera.minimumDistance = dist;
		}
		inline Void setAutofollowMaximumDistance( const Vec2f &dist )
		{
			camera.maximumDistance = dist;
		}
		inline const Vec2f &getAutofollowMinimumDistance() const
		{
			return camera.minimumDistance;
		}
		inline const Vec2f &getAutofollowMaximumDistance() const
		{
			return camera.maximumDistance;
		}

		inline Void setAutofollowSpeed( const Vec2f &speed )
		{
			camera.followSpeed = speed;
		}
		inline const Vec2f &getAutofollowSpeed() const
		{
			return camera.followSpeed;
		}

		inline Void enableVirtualResolution()
		{
			virtualResolutionEnabled = true;
		}
		inline Void disableVirtualResolution()
		{
			virtualResolutionEnabled = false;
		}
		inline Bool isVirtualResolutionEnabled() const
		{
			return virtualResolutionEnabled;
		}
		inline Void setVirtualResolution( const Vec2f &res )
		{
			virtualResolution = res;
		}
		inline const Vec2f &getVirtualResolution() const
		{
			return virtualResolution;
		}
		inline Void setVirtualOrigin( const Vec2f &org )
		{
			virtualOrigin = org;
		}
		inline const Vec2f &getVirtualOrigin() const
		{
			return virtualOrigin;
		}

	protected:
		union {
			struct {
				Bool visible:1;
				Bool scissor:1;
				Bool update:1;
			} bits;
			U32 mask;
		} flags;
		SPixelRect scissorRect;

		TIntrLink<RSpriteGroup> mgr_spriteGroupLink;
		TIntrList<RSprite> grp_spriteList;
		TIntrList<RSprite> grp_updateList;

	private:
		struct SCamera {
			Vec2f translation;
			F32 rotation;

			const RSprite *follow;
			Vec2f minimumDistance;
			Vec2f maximumDistance;
			Vec2f followSpeed;
		} camera;

		Bool virtualResolutionEnabled;
		Vec2f virtualResolution;
		Vec2f virtualOrigin;
	};

	/*
	===============================================================================

		SPRITE MANAGER
		Core API for the sprites

	===============================================================================
	*/
	class MSprites {
	friend class RSpriteGroup;
	public:
		static MSprites instance;

		MSprites();
		~MSprites();

		Bool init_gl();
		Void fini_gl();

		Void render_gl( CGfxFrame *pFrame );
		Void update();

		inline RSpriteGroup *getDefaultSpriteGroup()
		{
			return defaultSpriteGroup;
		}
		inline const RSpriteGroup *getDefaultSpriteGroup_const() const
		{
			return defaultSpriteGroup;
		}
		inline const RSpriteGroup *getDefaultSpriteGroup() const
		{
			return getDefaultSpriteGroup_const();
		}

		RSpriteGroup *newSpriteGroup();

	protected:
		TIntrList< RSpriteGroup > mgr_spriteGroupList;

	private:
		RSpriteGroup *defaultSpriteGroup;
	};
	extern MSprites &g_spriteMgr;

	// ------------------------------------------------------------------ //

	DOLL_FUNC RSprite *DOLL_API gfx_newSprite();
	DOLL_FUNC RSprite *DOLL_API gfx_newSpriteInGroup( RSpriteGroup *group );
	DOLL_FUNC RSprite *DOLL_API gfx_deleteSprite( RSprite *spr );
	DOLL_FUNC RSprite *DOLL_API gfx_loadAnimSprite( U16 img, S32 cellResX, S32 cellResY, S32 startFrame, S32 numFrames, S32 offX, S32 offY, S32 padX, S32 padY );
	DOLL_FUNC RSprite *DOLL_API gfx_loadAnimSpriteInGroup( RSpriteGroup *group, RTexture *texture, S32 cellResX, S32 cellResY, S32 startFrame, S32 numFrames, S32 offX, S32 offY, S32 padX, S32 padY );

	DOLL_FUNC RSpriteGroup *DOLL_API gfx_newSpriteGroup();
	DOLL_FUNC RSpriteGroup *DOLL_API gfx_deleteSpriteGroup( RSpriteGroup *group );
	DOLL_FUNC RSpriteGroup *DOLL_API gfx_getDefaultSpriteGroup();
	DOLL_FUNC Void DOLL_API gfx_showSpriteGroup( RSpriteGroup *group );
	DOLL_FUNC Void DOLL_API gfx_hideSpriteGroup( RSpriteGroup *group );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteGroupVisible( const RSpriteGroup *group );
	DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupScissor( RSpriteGroup *group, S32 posX, S32 posY, S32 resX, S32 resY );
	DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupScissor( RSpriteGroup *group );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteGroupScissorEnabled( const RSpriteGroup *group );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorPositionX( const RSpriteGroup *group );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorPositionY( const RSpriteGroup *group );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorResolutionX( const RSpriteGroup *group );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupScissorResolutionY( const RSpriteGroup *group );

	DOLL_FUNC Void DOLL_API gfx_moveSprite( RSprite *sprite, F32 x, F32 y );
	DOLL_FUNC Void DOLL_API gfx_turnSprite( RSprite *sprite, F32 theta );
	DOLL_FUNC Void DOLL_API gfx_setSpritePosition( RSprite *sprite, F32 x, F32 y );
	DOLL_FUNC Void DOLL_API gfx_setSpriteRotation( RSprite *sprite, F32 angle );
	DOLL_FUNC Void DOLL_API gfx_setSpriteScale( RSprite *sprite, F32 x, F32 y );
	DOLL_FUNC Void DOLL_API gfx_moveSpriteFrame( RSprite *sprite, F32 x, F32 y );
	DOLL_FUNC Void DOLL_API gfx_turnSpriteFrame( RSprite *sprite, F32 theta );
	DOLL_FUNC Void DOLL_API gfx_setSpriteFramePosition( RSprite *sprite, F32 x, F32 y );
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameRotation( RSprite *sprite, F32 angle );
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameScale( RSprite *sprite, F32 x, F32 y );
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameSourceRectangle( RSprite *sprite, S32 posX, S32 posY, S32 resX, S32 resY );
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameCornerColors( RSprite *sprite, U32 colorTL, U32 colorTR, U32 colorBL, U32 colorBR );
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameCornerAlphas( RSprite *sprite, U32 alphaTL, U32 alphaTR, U32 alphaBL, U32 alphaBR );
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameColor( RSprite *sprite, U32 color );
	DOLL_FUNC Void DOLL_API gfx_setSpriteFrameAlpha( RSprite *sprite, U32 alpha );
	DOLL_FUNC Void DOLL_API gfx_deleteAllSpriteFrames( RSprite *sprite );

	DOLL_FUNC S32 DOLL_API gfx_addSpriteFrame( RSprite *sprite );
	DOLL_FUNC SSpriteFrame *DOLL_API gfx_duplicateCurrentSpriteFrame( RSprite *sprite );

	DOLL_FUNC S32 DOLL_API gfx_getSpritePosition( const RSprite *sprite, F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpritePositionX( const RSprite *sprite );
	DOLL_FUNC F32 DOLL_API gfx_getSpritePositionY( const RSprite *sprite );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteRotation( const RSprite *sprite );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteScale( const RSprite *sprite, F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteScaleX( const RSprite *sprite );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteScaleY( const RSprite *sprite );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteFramePosition( const RSprite *sprite, F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteFramePositionX( const RSprite *sprite );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteFramePositionY( const RSprite *sprite );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteFrameRotation( const RSprite *sprite );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteFrameScale( const RSprite *sprite, F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteFrameScaleX( const RSprite *sprite );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteFrameScaleY( const RSprite *sprite );
	DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameCornerColor( const RSprite *sprite, U32 index );
	DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameCornerAlpha( const RSprite *sprite, U32 index );
	DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameColor( const RSprite *sprite );
	DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameAlpha( const RSprite *sprite );

	DOLL_FUNC Void DOLL_API gfx_setSpriteTexture( RSprite *sprite, U32 textureId );
	DOLL_FUNC U32 DOLL_API gfx_getSpriteTexture( const RSprite *sprite );

	DOLL_FUNC U32 DOLL_API gfx_getSpriteFrameCount( const RSprite *sprite );
	DOLL_FUNC Void DOLL_API gfx_setSpriteCurrentFrame( RSprite *sprite, U32 frameIndex );
	DOLL_FUNC U32 DOLL_API gfx_getSpriteCurrentFrame( const RSprite *sprite );

	DOLL_FUNC Void DOLL_API gfx_setSpriteFramesPerSecond( RSprite *sprite, F64 fps );
	DOLL_FUNC F64 DOLL_API gfx_getSpriteFramesPerSecond( const RSprite *sprite );

	DOLL_FUNC Void DOLL_API gfx_playSpriteAnimation( RSprite *sprite, U32 beginFrame, U32 endFrame );
	DOLL_FUNC Void DOLL_API gfx_loopSpriteAnimation( RSprite *sprite, U32 beginFrame, U32 endFrame );
	DOLL_FUNC Void DOLL_API gfx_playSpriteAnimationWithDeath( RSprite *sprite, U32 beginFrame, U32 endFrame );
	DOLL_FUNC Void DOLL_API gfx_stopSpriteAnimation( RSprite *sprite );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteAnimationPlaying( const RSprite *sprite );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteAnimationLooping( const RSprite *sprite );

	DOLL_FUNC Void DOLL_API gfx_setSpriteGroup( RSprite *sprite, RSpriteGroup *group );
	DOLL_FUNC RSpriteGroup *DOLL_API gfx_getSpriteGroup( RSprite *sprite );

	DOLL_FUNC Void DOLL_API gfx_bindSprite( RSprite *sprite, RSprite *master );
	DOLL_FUNC Void DOLL_API gfx_bindSpriteTranslation( RSprite *sprite, RSprite *master );
	DOLL_FUNC Void DOLL_API gfx_selectivelyBindSprite( RSprite *sprite, RSprite *master, S32 bindTranslation, S32 bindRotation, S32 bindScale );
	DOLL_FUNC Void DOLL_API gfx_unbindSprite( RSprite *sprite );
	DOLL_FUNC RSprite *DOLL_API gfx_getSpriteBindMaster( RSprite *sprite );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteBoundToTranslation( const RSprite *sprite );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteBoundToRotation( const RSprite *sprite );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteBoundToScale( const RSprite *sprite );

	DOLL_FUNC Void DOLL_API gfx_setSpriteOffset( RSprite *sprite, F32 x, F32 y );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteOffset( const RSprite *sprite, F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteOffsetX( const RSprite *sprite );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteOffsetY( const RSprite *sprite );

	DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupAutoupdating( RSpriteGroup *group );
	DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupAutoupdating( RSpriteGroup *group );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteGroupAutoupdatingEnabled( const RSpriteGroup *group );

	DOLL_FUNC Void DOLL_API gfx_enableSpriteAutoupdating( RSprite *sprite );
	DOLL_FUNC Void DOLL_API gfx_disableSpriteAutoupdating( RSprite *sprite );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteAutoupdatingEnabled( const RSprite *sprite );

	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupPosition( RSpriteGroup *group, F32 x, F32 y );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupPosition( const RSpriteGroup *group, F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupPositionX( const RSpriteGroup *group );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupPositionY( const RSpriteGroup *group );

	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupRotation( RSpriteGroup *group, F32 rotation );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupRotation( const RSpriteGroup *group );

	DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupFollow( RSpriteGroup *group, const RSprite *follow );
	DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupFollow( RSpriteGroup *group );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteGroupFollowEnabled( const RSpriteGroup *group );

	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupFollowMinimumDistance( RSpriteGroup *group, F32 x, F32 y );
	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupFollowMaximumDistance( RSpriteGroup *group, F32 x, F32 y );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupFollowMinimumDistance( const RSpriteGroup *group, F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMinimumDistanceX( const RSpriteGroup *group );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMinimumDistanceY( const RSpriteGroup *group );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupFollowMaximumDistance( const RSpriteGroup *group, F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMaximumDistanceX( const RSpriteGroup *group );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowMaximumDistanceY( const RSpriteGroup *group );

	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupFollowSpeed( RSpriteGroup *group, F32 x, F32 y );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupFollowSpeed( const RSpriteGroup *group, F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowSpeedX( const RSpriteGroup *group );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupFollowSpeedY( const RSpriteGroup *group );
	DOLL_FUNC Void DOLL_API gfx_flipSpriteHorizontal( RSprite *sprite );
	DOLL_FUNC Void DOLL_API gfx_flipSpriteVertical( RSprite *sprite );
	DOLL_FUNC Void DOLL_API gfx_setSpriteMirrorHorizontal( RSprite *sprite, Bool mirror );
	DOLL_FUNC Void DOLL_API gfx_setSpriteMirrorVertical( RSprite *sprite, Bool mirror );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteMirrorHorizontal( const RSprite *sprite );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteMirrorVertical( const RSprite *sprite );

	DOLL_FUNC S32 DOLL_API gfx_getSpriteSize( const RSprite *sprite, F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteSizeX( const RSprite *sprite );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteSizeY( const RSprite *sprite );

	DOLL_FUNC Void DOLL_API gfx_enableSpriteGroupVirtualResolution( RSpriteGroup *group );
	DOLL_FUNC Void DOLL_API gfx_disableSpriteGroupVirtualResolution( RSpriteGroup *group );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteGroupVirtualResolutionEnabled( const RSpriteGroup *group );
	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupVirtualResolution( RSpriteGroup *group,
													  F32 x, F32 y );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupVirtualResolution( const RSpriteGroup *group,
													 F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualResolutionX( const RSpriteGroup *group );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualResolutionY( const RSpriteGroup *group );
	DOLL_FUNC Void DOLL_API gfx_setSpriteGroupVirtualOrigin( RSpriteGroup *group, F32 x, F32 y );
	DOLL_FUNC S32 DOLL_API gfx_getSpriteGroupVirtualOrigin( const RSpriteGroup *group, F32 *x, F32 *y );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualOriginX( const RSpriteGroup *group );
	DOLL_FUNC F32 DOLL_API gfx_getSpriteGroupVirtualOriginY( const RSpriteGroup *group );

	DOLL_FUNC Void DOLL_API gfx_setSpriteVisible( RSprite *sprite, S32 visible );
	DOLL_FUNC S32 DOLL_API gfx_isSpriteVisible( const RSprite *sprite );

}
