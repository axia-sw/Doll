#define DOLL_TRACE_FACILITY doll::kLog_SndMgr
#include "../BuildSettings.hpp"

#include "doll/Snd/SoundMgr.hpp"
#include "doll/Snd/SoundCore.hpp"
#include "doll/Snd/WaveFile.hpp"

#include "doll/Core/Logger.hpp"
#include "doll/Core/Engine.hpp"

#include "doll/IO/AsyncIO.hpp"

// FIXME: Mend all functions that explicitly rely on these.
#include "doll/Gfx/RenderCommands.hpp"
#include "doll/Gfx/Layer.hpp"
#include "doll/Front/Frontend.hpp"

namespace doll
{

	inline Bool getDevice( CSoundDevice *&pDevice )
	{
		return pDevice != nullptr || ( pDevice = snd_getDevice() ) != nullptr;
	}
	inline Bool getDevice( const CSoundDevice *&pDevice )
	{
		return pDevice != nullptr || ( pDevice = snd_getDevice() ) != nullptr;
	}

	DOLL_FUNC CSoundMixer *DOLL_API snd_getMasterMixer( const CSoundDevice *pDevice )
	{
		if( !getDevice( pDevice ) ) {
			return nullptr;
		}

		return const_cast< CSoundMixer * >( &pDevice->masterMixer() );
	}
	DOLL_FUNC CSoundMixer *DOLL_API snd_newMixer( const Str &name, CSoundDevice *pDevice )
	{
		if( !getDevice( pDevice ) ) {
			return nullptr;
		}

		return pDevice->newMixer( name );
	}
	DOLL_FUNC NullPtr DOLL_API snd_deleteMixer( CSoundMixer *pMixer )
	{
		delete pMixer;
		return nullptr;
	}
	DOLL_FUNC Bool DOLL_API snd_getMixers( TArr<CSoundMixer *> &dstMixers, const CSoundDevice *pDevice )
	{
		if( !getDevice( pDevice ) ) {
			return false;
		}

		dstMixers = pDevice->mixers();
		return true;
	}

	inline Bool getMixer( CSoundMixer *&pMixer )
	{
		return pMixer != nullptr || ( pMixer = snd_getMasterMixer() ) != nullptr;
	}
	inline Bool getMixer( const CSoundMixer *&pMixer )
	{
		return pMixer != nullptr || ( pMixer = snd_getMasterMixer() ) != nullptr;
	}

	DOLL_FUNC Bool DOLL_API snd_getMixerName( Str &dstName, const CSoundMixer *pMixer )
	{
		if( !getMixer( pMixer ) ) {
			dstName = Str();
			return false;
		}

		dstName = pMixer->name();
		return true;
	}
	DOLL_FUNC CSoundDevice *DOLL_API snd_getMixerDevice( const CSoundMixer *pMixer )
	{
		if( !getMixer( pMixer ) ) {
			return nullptr;
		}

		return const_cast< CSoundDevice * >( &pMixer->device() );
	}
	DOLL_FUNC CSoundMixer *DOLL_API snd_getMixerParent( const CSoundMixer *pMixer )
	{
		if( !getMixer( pMixer ) ) {
			return nullptr;
		}

		return const_cast< CSoundMixer * >( pMixer->parent() );
	}
	DOLL_FUNC Bool DOLL_API snd_getSubmixers( TArr<CSoundMixer *> &dstMixers, const CSoundMixer *pMixer )
	{
		if( !getMixer( pMixer ) ) {
			dstMixers = TArr<CSoundMixer *>();
			return false;
		}

		dstMixers = pMixer->submixers();
		return true;
	}
	DOLL_FUNC Bool DOLL_API snd_setTrackLimit( CSoundMixer *pMixer, UPtr cTracks )
	{
		if( !getMixer( pMixer ) ) {
			return false;
		}

		return pMixer->setTrackLimit( cTracks );
	}
	DOLL_FUNC UPtr DOLL_API snd_getTrackLimit( const CSoundMixer *pMixer )
	{
		if( !getMixer( pMixer ) ) {
			return 0;
		}

		return pMixer->getTrackLimit();
	}
	DOLL_FUNC CSoundTrack *DOLL_API snd_findFreeTrack( const CSoundMixer *pMixer )
	{
		if( !getMixer( pMixer ) ) {
			return nullptr;
		}

		return pMixer->findFreeTrack();
	}
	DOLL_FUNC Bool DOLL_API snd_getPlayingTracks( TArr<CSoundTrack *> &dstTracks, const CSoundMixer *pMixer )
	{
		if( !getMixer( pMixer ) ) {
			dstTracks = TArr<CSoundTrack *>();
			return false;
		}

		dstTracks = pMixer->playingTracks();
		return true;
	}
	DOLL_FUNC Bool DOLL_API snd_setMixerWaveFormat( CSoundMixer *pMixer, const SWaveFormat &wf )
	{
		if( !getMixer( pMixer ) || !wf.isValid() ) {
			return false;
		}

		pMixer->setWaveFormat( wf );
		return true;
	}
	DOLL_FUNC const SWaveFormat *DOLL_API snd_getMixerWaveFormat_ptr( const CSoundMixer *pMixer )
	{
		if( !getMixer( pMixer ) ) {
			static const SWaveFormat wf = SWaveFormat();
			return &wf;
		}

		return &pMixer->getWaveFormat();
	}

	DOLL_FUNC CSoundMixer *DOLL_API snd_getTrackMixer( const CSoundTrack *pTrack )
	{
		AX_ASSERT_NOT_NULL( pTrack );

		return const_cast< CSoundMixer * >( &pTrack->mixer() );
	}
	DOLL_FUNC Bool DOLL_API snd_startTrack( CSoundTrack *pTrack, CSoundClip *pClip )
	{
		AX_ASSERT_NOT_NULL( pTrack );
		AX_ASSERT_NOT_NULL( pClip );

		return pTrack->start( *pClip );
	}
	DOLL_FUNC Void DOLL_API snd_stopTrack( CSoundTrack *pTrack )
	{
		AX_ASSERT_NOT_NULL( pTrack );
		pTrack->stop();
	}

	DOLL_FUNC CSoundClip *DOLL_API snd_newClip()
	{
		CSoundClip *const pClip = new CSoundClip();
		if( !AX_VERIFY_MEMORY( pClip ) ) {
			return nullptr;
		}

		return pClip;
	}
	DOLL_FUNC NullPtr DOLL_API snd_deleteClip( CSoundClip *pClip )
	{
		delete pClip;
		return nullptr;
	}

	DOLL_FUNC Bool DOLL_API snd_setClipWaveFormat( CSoundClip *pClip, const SWaveFormat &wf )
	{
		AX_ASSERT_NOT_NULL( pClip );
		return pClip->setFormat( wf );
	}
	DOLL_FUNC const SWaveFormat *DOLL_API snd_getClipWaveFormat_ptr( const CSoundClip *pClip )
	{
		AX_ASSERT_NOT_NULL( pClip );
		return &pClip->getFormat();
	}
	DOLL_FUNC Bool DOLL_API snd_buffer( CSoundClip *pClip, UPtr cBytes, const Void *pBytes, UPtr cSamples, U8 cLoops )
	{
		AX_ASSERT_NOT_NULL( pClip );
		return pClip->buffer( cBytes, pBytes, cSamples, cLoops );
	}
	DOLL_FUNC Bool DOLL_API snd_bufferEx( CSoundClip *pClip, const SSoundBuffer &buf )
	{
		AX_ASSERT_NOT_NULL( pClip );
		return pClip->buffer( buf );
	}
	DOLL_FUNC U32 DOLL_API snd_getPlaybackMilliseconds( const CSoundClip *pClip )
	{
		AX_ASSERT_NOT_NULL( pClip );
		return pClip->playbackLengthInMilliseconds();
	}
	DOLL_FUNC U32 DOLL_API snd_getPlaybackSamples( const CSoundClip *pClip )
	{
		AX_ASSERT_NOT_NULL( pClip );
		return pClip->playbackLengthInSamples();
	}
	DOLL_FUNC U32 DOLL_API snd_getTotalMilliseconds( const CSoundClip *pClip )
	{
		AX_ASSERT_NOT_NULL( pClip );
		return pClip->totalLengthInMilliseconds();
	}
	DOLL_FUNC U32 DOLL_API snd_getTotalSamples( const CSoundClip *pClip )
	{
		AX_ASSERT_NOT_NULL( pClip );
		return pClip->totalLengthInSamples();
	}

	//--------------------------------------------------------------------//

	static Void drawProgress( S32 x1, S32 y1, S32 x2, S32 y2, F64 progress )
	{
		gfx_ink( DOLL_RGB( 0xDD, 0xDD, 0xDD ) );
		gfx_outline( x1, y1, x2, y2 );

		const F64 range = ( x2 - x1 - 4 + 1 )*F64( saturate( F32( progress ) ) );

		gfx_ink( DOLL_RGB( 0xEE, 0xEE, 0xEE ) );
		gfx_box( x1 + 2, y1 + 2, x1 + 2 + S32( range ), y2 - 1 );
	}
	static Bool waitAsync( CAsyncOp *pOp )
	{
		AX_ASSERT_NOT_NULL( pOp );

		const UPtr cReqBytes = async_size( pOp );
		if( !cReqBytes ) {
			return true;
		}

		do {
			// Check if the operation is done
			const EAsyncStatus status = async_status( pOp );
			if( status != EAsyncStatus::Pending ) {
				if( status == EAsyncStatus::Success ) {
					return true;
				}

				break;
			}

			// Default rendering layer
			RLayer *const pDefLayer = gfx_getDefaultLayer();

			// Get the size of the layer (the area we're drawing into)
			const S32 resX = gfx_getLayerSizeX( pDefLayer );
			const S32 resY = gfx_getLayerSizeY( pDefLayer );

			// Clear the rendering queue
			gfx_clearQueue();

			// Loading background gradient
			gfx_vgradBox( 0, 0, resX, resY, DOLL_RGB( 0x11, 0x22, 0x44 ), DOLL_RGB( 0x22, 0x44, 0xAA ) );

			// Progress bar
			const F64 fProgress = !pOp ? 1.0 : F64( async_tell( pOp ) )/F64( cReqBytes );
			drawProgress( resX/2 - resX/4, resY/2 - 10, resX/2 + resX/4, resY/2 + 10, fProgress );
		} while( doll_sync() );

		return false;
	}

	DOLL_FUNC Bool DOLL_API snd_playBGM( const Str &filename, EPlayBGM when )
	{
		// WAV file (must persist, so using static storage)
		static CWaveFile wav;
		// BGM clip (must persist)
		static CSoundClip *pBGMClip = nullptr;

		// FIXME: Implement `EPlayBGM::Whenever` support.
		if( when == EPlayBGM::Whenever ) {
			DOLL_WARNING_LOG += "`EPlayBGM::Whenever` is not yet implemented. Using `EPlayBGM::Now`.";
			when = EPlayBGM::Now;
		}

		g_VerboseLog += "Clearing BGM resources...";

		// Stop playback if possible
		if( g_core.sound.pBGMMix != nullptr ) {
			TArr<CSoundTrack *> pTracks = snd_getPlayingTracks( g_core.sound.pBGMMix );
			for( CSoundTrack *pTrack : pTracks ) {
				AX_ASSERT_NOT_NULL( pTrack );
				snd_stopTrack( pTrack );
			}
		}

		// Stop loading an async op if we've got one
		if( g_core.sound.pBGMOp != nullptr ) {
			g_core.sound.pBGMOp = async_close( g_core.sound.pBGMOp );
		}

		// Get rid of the previous BGM clip
		pBGMClip = snd_deleteClip( pBGMClip );
		// Destroy the mixer
		g_core.sound.pBGMMix = snd_deleteMixer( g_core.sound.pBGMMix );
		// Clear up the wave file
		wav.fini();

		// Exit if we were just used to stop BGM playback
		if( filename.isEmpty() ) {
			return true;
		}

		g_VerboseLog( filename ) += "Preparing BGM...";

		// Do nothing if BGM playback (or playback in general) is not enabled
		if( ~g_core.sound.uFlags & ( kCoreSndF_Enabled | kCoreSndF_AllowBGM ) ) {
			g_DebugLog += axf( "~0x%.8X (0x%.8X) & 0x%.8X == 0x%.8X", g_core.sound.uFlags, ~g_core.sound.uFlags, kCoreSndF_Enabled | kCoreSndF_AllowBGM, ~g_core.sound.uFlags & ( kCoreSndF_Enabled | kCoreSndF_AllowBGM ) );
			return false;
		}

		// Current sound device
		CSoundDevice *const pDev = snd_getDevice();
		if( !pDev ) {
			// It's *NOT* an error if the device can't be found
			g_DebugLog += "No sound device.";
			return false;
		}

		// Recreate the mixer
		if( !( g_core.sound.pBGMMix = snd_newMixer( "BGM", pDev ) ) ) {
			g_ErrorLog += "Failed to create BGM mixer.";
			return false;
		}

		// BGM clip
		if( !( pBGMClip = snd_newClip() ) ) {
			g_ErrorLog += "Failed to create sound clip.";
			return false;
		}

		// Attempt initializing the file
		if( !wav.init( filename ) ) {
			g_ErrorLog += "Failed to initialize WAV.";
			return false;
		}

		// Begin asynchronously loading the data
		CAsyncOp *const pAsyncOp = wav.loadDataAsync();
		if( !pAsyncOp ) {
			g_ErrorLog += "Failed to create async-op for WAV data chunk load.";
			wav.fini();
			g_core.sound.pBGMMix = snd_deleteMixer( g_core.sound.pBGMMix );
			return false;
		}

		// Wave format used by the file
		const SWaveFormat &wf = wav.getFormat();

		Bool r = true;

		// Apply the wave format
		r = r && snd_setMixerWaveFormat( g_core.sound.pBGMMix, wf );
		r = r && snd_setClipWaveFormat( pBGMClip, wf );

		// Set the track limit for the BGM mixer
		r = r && snd_setTrackLimit( g_core.sound.pBGMMix, 1 );

		// BGM track
		CSoundTrack *const pBGMTrack = snd_findFreeTrack( g_core.sound.pBGMMix );
		if( !pBGMTrack ) {
			g_ErrorLog += "Failed to find free track.";
			return false;
		}

		// Show that the music is loading
		r = r && waitAsync( pAsyncOp );
		// Buffer in the finished music
		r = r && snd_buffer( pBGMClip, wav.getDataSize(), wav.getData(), 0, 0xFF );

		// Show the length of the file in seconds (ignoring loop regions)
		if( r ) {
			const F64 fTotalSeconds = snd_getTotalSeconds( pBGMClip );
			const U32 cMinutes = snd_getTotalMilliseconds( pBGMClip )/60000;
			const U32 cSeconds = ( snd_getTotalMilliseconds( pBGMClip )%60000 + 500 )/1000;
			g_DebugLog += axf( "Seconds: %.2f (%um:%.2us)", fTotalSeconds, cMinutes, cSeconds );
		}

		// Start playback
		r = r && snd_startTrack( pBGMTrack, pBGMClip );

		// Close the async operation
		async_close( pAsyncOp );

		// Done
		return r;
	}

}
