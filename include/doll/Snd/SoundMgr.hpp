#pragma once

#include "SoundCore.hpp"

namespace doll
{

	/*
	===========================================================================

		SOUND SOURCE INTERFACE
		
		Used by the sound manager to buffer data on the I/O thread and submit
		data to the sound system as needed.

		The buffered data may come from a file, a network, synthesized, etc.
		i.e., There is no limitation on the source of the data. However,
		because the callback occurs on the I/O thread, the sound source itself
		should try to be as fast as possible to avoid glitches.

		Prefixes are used for each method name, which indicates the thread the
		function will be called on. These are as follows:

			PREFIX  THREAD
			mt_     Multiple threads (any thread)
			io_     I/O thread
			ss_     Sound system thread (implementation dependent)

	===========================================================================
	*/

	class ISoundSource
	{
	public:
		ISoundSource(): m_cRefs(1) {}
		virtual ~ISoundSource() {}

		virtual ISoundSource *grab()
		{
			return ++m_cRefs, this;
		}
		virtual NullPtr drop()
		{
			if( --m_cRefs == 0 ) {
				delete this;
			}

			return nullptr;
		}

		// Construct the wave format that this source will be using
		//
		// return: `true` upon successfully filling `wf` with valid data.
		//         `false` if the source is invalid
		virtual Bool mt_prepare( SWaveFormat &wf ) = 0;

		// Perform a read in the IO thread
		//
		// This should be mindful of multiple IO requests and thus only buffer
		// what it can within a short amount of time.
		//
		// Buffering 4MB[FIXME:calculate properly] worth of data per call is
		// probably optimal. Try not to exceed 80 milliseconds of execution
		// time.
		//
		// return: `true` if no error occurred and data is still available.
		//         `false` if an error occurred or no more data is available.
		virtual Bool io_read() = 0;

		// Fill a buffer of sound data
		//
		// The data pointer marked should *NOT* be freed or overwritten until
		// a corresponding `ss_unlockBuffer()` call. The sound system will
		// create an internal copy of it.
		//
		// NOTE: The `io_read()` implementation should be mindful of a buffer's
		//       "locked" status.
		//
		// Until `ss_unlockBuffer()` is called, there will be no further calls
		// to `ss_lockBuffer()`.
		//
		// return: `true` if a buffer was available and `buf` represents it.
		//         `false` if no buffer was available.
		virtual Bool ss_lockBuffer( SSoundBuffer &buf ) = 0;
		// Mark the previous retrieved buffer as available again
		//
		// This will be called *once* per `ss_lockBuffer()` call. This call will
		// match the prior `ss_lockBuffer()` call.
		virtual Void ss_unlockBuffer() = 0;

	private:
		U32 m_cRefs;

		AX_DELETE_COPYFUNCS(ISoundSource);
	};

	//! \brief Retrieve the master mixing interface.
	//
	// If the given sound device is `nullptr` then the default is used.
	DOLL_FUNC CSoundMixer *DOLL_API snd_getMasterMixer( const CSoundDevice * = nullptr );
	//! \brief Create a new mixer.
	DOLL_FUNC CSoundMixer *DOLL_API snd_newMixer( const Str &name = Str(), CSoundDevice * = nullptr );
	//! \brief Delete a mixer.
	DOLL_FUNC NullPtr DOLL_API snd_deleteMixer( CSoundMixer * );
	//! \brief Enumerate all the current (root level) mixers.
	DOLL_FUNC Bool DOLL_API snd_getMixers( TArr<CSoundMixer *> &dstMixers, const CSoundDevice * = nullptr );
	inline TArr<CSoundMixer *> DOLL_API snd_getMixers( const CSoundDevice *pDevice = nullptr )
	{
		TArr<CSoundMixer *> r;
		return snd_getMixers( r, pDevice ), r;
	}

	//! \brief Retrieve the name of the given mixer.
	DOLL_FUNC Bool DOLL_API snd_getMixerName( Str &dstName, const CSoundMixer * );
	inline Str DOLL_API snd_getMixerName( const CSoundMixer *pMixer )
	{
		Str r;
		return snd_getMixerName( r, pMixer ), r;
	}
	//! \brief Retrieve the device the given mixer is a part of.
	DOLL_FUNC CSoundDevice *DOLL_API snd_getMixerDevice( const CSoundMixer * );
	//! \brief Retrieve the parent mixer of the given mixer.
	DOLL_FUNC CSoundMixer *DOLL_API snd_getMixerParent( const CSoundMixer * );
	//! \brief Retrieve the submixers of the given mixer.
	DOLL_FUNC Bool DOLL_API snd_getSubmixers( TArr<CSoundMixer *> &dstMixers, const CSoundMixer * );
	inline TArr<CSoundMixer * > DOLL_API snd_getSubmixers( const CSoundMixer *pMixer )
	{
		TArr<CSoundMixer *> r;
		return snd_getSubmixers( r, pMixer ), r;
	}
	//! \brief Set the track limit for the given mixer.
	DOLL_FUNC Bool DOLL_API snd_setTrackLimit( CSoundMixer *, UPtr cTracks );
	//! \brief Retrieve the track limit of the given mixer.
	DOLL_FUNC UPtr DOLL_API snd_getTrackLimit( const CSoundMixer * );
	//! \brief Find an unused track in the given mixer.
	DOLL_FUNC CSoundTrack *DOLL_API snd_findFreeTrack( const CSoundMixer * );
	//! \brief Retrieve the list of currently playing tracks for the given mixer.
	DOLL_FUNC Bool DOLL_API snd_getPlayingTracks( TArr<CSoundTrack *> &dstTracks, const CSoundMixer * );
	inline TArr<CSoundTrack *> DOLL_API snd_getPlayingTracks( const CSoundMixer *pMixer )
	{
		TArr<CSoundTrack *> r;
		return snd_getPlayingTracks( r, pMixer ), r;
	}
	//! \brief Set the wave format for the given mixer.
	//!
	//! This must be done before any tracks have been allocated to the mixer.
	//! The default is the master mixer's format.
	DOLL_FUNC Bool DOLL_API snd_setMixerWaveFormat( CSoundMixer *, const SWaveFormat & );
	//! \brief Retrieves the wave format used by the given mixer.
	DOLL_FUNC const SWaveFormat *DOLL_API snd_getMixerWaveFormat_ptr( const CSoundMixer * );
	inline const SWaveFormat &DOLL_API snd_getMixerWaveFormat( const CSoundMixer *mixer ) {
		return *snd_getMixerWaveFormat_ptr( mixer );
	}

	//! \brief Retrieve the mixer of a given sound track.
	DOLL_FUNC CSoundMixer *DOLL_API snd_getTrackMixer( const CSoundTrack * );
	//! \brief Start playing a sound on the given track.
	DOLL_FUNC Bool DOLL_API snd_startTrack( CSoundTrack *, CSoundClip * );
	//! \brief Stop playback for the given track.
	DOLL_FUNC Void DOLL_API snd_stopTrack( CSoundTrack * );

	//! \brief Create a sound clip.
	DOLL_FUNC CSoundClip *DOLL_API snd_newClip();
	//! \brief Destroy a sound clip.
	DOLL_FUNC NullPtr DOLL_API snd_deleteClip( CSoundClip * );

	//! \brief Set the format for the given sound clip.
	DOLL_FUNC Bool DOLL_API snd_setClipWaveFormat( CSoundClip *, const SWaveFormat & );
	//! \brief Retrieve the format used by the given sound clip.
	DOLL_FUNC const SWaveFormat *DOLL_API snd_getClipWaveFormat_ptr( const CSoundClip * );
	inline const SWaveFormat &DOLL_API snd_getClipWaveFormat( const CSoundClip *clip ) {
		return *snd_getClipWaveFormat_ptr( clip );
	}
	//! \brief Buffer data into the given sound clip.
	DOLL_FUNC Bool DOLL_API snd_buffer( CSoundClip *, UPtr cBytes, const Void *pBytes, UPtr cSamples = 0, U8 cLoops = 0 );
	//! \brief Buffer data into the given sound clip using a sound buffer structure.
	DOLL_FUNC Bool DOLL_API snd_bufferEx( CSoundClip *, const SSoundBuffer & );
	//! \brief Retrieve the playback length of the given sound in milliseconds.
	DOLL_FUNC U32 DOLL_API snd_getPlaybackMilliseconds( const CSoundClip * );
	//! \brief Retrieve the playback length of the given sound in samples.
	DOLL_FUNC U32 DOLL_API snd_getPlaybackSamples( const CSoundClip * );
	//! \brief Retrieve the total playback length of the given sound in milliseconds.
	DOLL_FUNC U32 DOLL_API snd_getTotalMilliseconds( const CSoundClip * );
	//! \brief Retrieve the total playback length of the given sound in samples.
	DOLL_FUNC U32 DOLL_API snd_getTotalSamples( const CSoundClip * );
	//! \brief Retrieve the playback length of the given sound in seconds.
	inline F64 DOLL_API snd_getPlaybackSeconds( const CSoundClip *p )
	{
		return F64( snd_getPlaybackMilliseconds(p) )/1000.0;
	}
	//! \brief Retrieve the total playback length of the given sound in seconds.
	inline F64 DOLL_API snd_getTotalSeconds( const CSoundClip *p )
	{
		return F64( snd_getTotalMilliseconds( p ) )/1000.0;
	}

	//! \brief When to play the BGM track. (See `snd_playBGM`.)
	enum class EPlayBGM
	{
		//! Whenever it finishes loading is when it should be played.
		Whenever,
		//! Load it immediately, stopping everything else, then play it.
		Now
	};

	//! \brief Play a background music track on loop, replacing the prior BGM.
	DOLL_FUNC Bool DOLL_API snd_playBGM( const Str &filename, EPlayBGM = EPlayBGM::Whenever );

}
