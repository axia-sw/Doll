#pragma once

#include "../Core/Defs.hpp"
#include "WaveFmt.hpp"

namespace doll
{

	class CSoundDevice;
	class CSoundMixer;
	class CSoundTrack;
	class CSoundClip;

	class ISoundFile;

	enum ESoundDeviceDefaultBits: U8
	{
		// Device not set as a default
		kSoundDevDefNone    = 0x00,

		// Device used for console apps by default
		kSoundDevDefConsole = 0x01,
		// Device used for multimedia apps (e.g., a movie player) by default
		kSoundDevDefMedia   = 0x02,
		// Device used for voice communication by default
		kSoundDevDefComm    = 0x04,
		// Device used for gaming apps by default
		kSoundDevDefGame    = 0x08,

		// Device used for all audio scenarios by default
		kSoundDevDefAll     = 0x0F,

		// Device has an invalid role assigned
		kSoundDevDefInvalid = 0x80
	};


	struct SSoundDeviceConf
	{
		// Sample rate (e.g., 44100Hz or 48000Hz), or 0 for system default
		U32 cSamplesHz;
		// Channels (See EChannelBits in "WaveFmt.hpp"), or 0 for system default
		U32 uChannelMask;
	};

	struct SSoundDeviceInfo
	{
		// Text-based identifier for the device
		Str textId;
		// Name of the device (suitable for display to the user)
		Str name;
		// Sample rate (e.g., 44100Hz or 48000Hz)
		U32 cSamplesHz;
		// Channels (See EChannelBits in "doll-snd-wavefmt.hpp")
		U32 uChannelMask;
		// Bits per sample (e.g., 16)
		U16 cBitsPerSample;
		// Role/defaults flags (See ESoundDeviceDefaultBits above)
		U8  uDefRoleMask;
	};

	struct SSoundSettings
	{
		// Volume level 0=silence, 1=full, 2=full(+1gain), 3=full(+2gain), etc
		F32 fVolume;

		// Pan (-1=left, 0=center, 1=right)
		F32 fPan;
	};

	struct SSoundBuffer
	{
		typedef TList<SSoundBuffer>           List;
		typedef TList<SSoundBuffer>::Iterator Iter;

		// Data for this sound buffer
		const Void *pBytes;
		// Number of bytes contained in this sound buffer
		UPtr        cBytes;
		// Number of samples contained once decoded
		UPtr        cSamples;
		// Offset in samples to the loop's starting point
		U32         uLoopSample;
		// Length of loop in samples (0 for the rest of the buffer)
		U32         cLoopSamples;
		// Number of loops (0 for "play once," and 0xFF for "infinite")
		U8          cLoops;
	};

	namespace detail
	{

		class ISoundDeviceHW;
		class ISoundMixerHW;
		class ISoundVoiceHW;

		enum ESoundVoiceHWFlags: U32
		{
			// Voice is considered music; allows system to replace or ignore if
			// user is playing music
			kSoundVoiceHWIsMusic = 1<<0,
			// Voice can be used with filter effects
			kSoundVoiceHWFilterable = 1<<1
		};
		enum ESoundVoiceHWStopFlags: U32
		{
			// Voice should stop immediately
			kSoundVoiceHWStopNow = 0,
			// Voice should stop, but allow effect output to continue
			kSoundVoiceHWStopWithTails = 1<<0
		};

		class ISoundHW
		{
		public:
			ISoundHW()
			{
			}
			virtual ~ISoundHW()
			{
			}

			virtual TArr<SSoundDeviceInfo> enumDevices() = 0;
			virtual ISoundDeviceHW *initDevice( UPtr uDeviceId, const SSoundDeviceConf *pConf ) = 0;
			virtual Void finiDevice( ISoundDeviceHW *pDevice ) = 0;

			virtual Void enableOutput() = 0;
			virtual Void disableOutput() = 0;

			virtual ISoundMixerHW *getMasterMixer( ISoundDeviceHW *pDevice ) = 0;
			virtual ISoundMixerHW *newMixer( ISoundDeviceHW *pDevice, ISoundMixerHW *pParentMixer, U32 uHierarchyLevel ) = 0;
			virtual Void deleteMixer( ISoundDeviceHW *pDevice, ISoundMixerHW *pMixer ) = 0;

			virtual ISoundVoiceHW *newVoice( ISoundDeviceHW *pDevice, ISoundMixerHW *pMixer, const SWaveFormat &wf, U32 uFlags ) = 0;
			virtual Void deleteVoice( ISoundDeviceHW *pDevice, ISoundVoiceHW *pVoice ) = 0;

			virtual Bool submitBuffer( ISoundDeviceHW *pDevice, ISoundVoiceHW *pVoice, const SSoundBuffer &buf ) = 0;

			virtual Void nextOperationSet() = 0;
			virtual Bool startVoice( ISoundDeviceHW *pDevice, ISoundVoiceHW *pVoice ) = 0;
			virtual Void stopVoice( ISoundDeviceHW *pDevice, ISoundVoiceHW *pVoice, U32 uFlags ) = 0;

			virtual Void setVoiceVolumes( ISoundDeviceHW *pDevice, ISoundVoiceHW *pVoice, U32 cChannels, const F32 *pVolumes ) = 0;
		};

	}


	/*

		SOUND DEVICE
		============
		Represents the platform's core sound interface (e.g., IXAudio2)

		Devices consist of a collection of mixers which are able to hold a
		series of tracks which can then be used to play back audio clips.

	*/
	class CSoundDevice
	{
	friend class CSoundMixer;
	friend class CSoundTrack;
	public:
		CSoundDevice( detail::ISoundDeviceHW *pHWDev );
		~CSoundDevice();

		CSoundMixer       &masterMixer();
		CSoundMixer const &masterMixer() const;

		CSoundMixer *newMixer( Str name );

		TArr<CSoundMixer*> mixers() const;

	private:
		detail::ISoundDeviceHW *m_pHWDevice;

		// Root mixers (index 0 = master mixer)
		TMutArr<CSoundMixer*>   m_pMixers;

		AX_DELETE_COPYFUNCS(CSoundDevice);
	};


	/*

		SOUND MIXER
		===========
		Grouping of related sound tracks. (e.g., "BGM," "SFX," "KOE")

		Mixers consist of a series of tracks which will receive audio buffers to
		be mixed together under the settings held here.

		Mixers can exist in a hierarchy.

	*/
	class CSoundMixer
	{
	friend class CSoundDevice;
	friend class CSoundTrack;
	public:
		CSoundMixer( CSoundDevice &device, detail::ISoundMixerHW *pHWMixer );
		~CSoundMixer();

		inline Str name() const { return m_name; }

		inline CSoundDevice       &device()       { return m_device; }
		inline CSoundDevice const &device() const { return m_device; }

		CSoundMixer       *parent();
		CSoundMixer const *parent() const;

		TArr<CSoundMixer *> submixers() const;

		Bool setTrackLimit( UPtr cTracks );
		UPtr getTrackLimit() const;

		CSoundTrack *findFreeTrack() const;

		TArr<CSoundTrack *> playingTracks() const;

		// Sets the wave format for this mixer, but must be done before any
		// tracks are allocated; defaults to the master mixer's format
		Void setWaveFormat( const SWaveFormat &wf );
		const SWaveFormat &getWaveFormat() const;

	private:
		detail::ISoundMixerHW *m_pHWMixer;

		CSoundDevice &         m_device;
		CSoundMixer *          m_pPrnt;
		TMutArr<CSoundMixer *> m_pMixers;
		TMutArr<CSoundTrack *> m_pTracks;
		TMutArr<CSoundTrack *> m_pLiveTracks;
		TMutArr<CSoundTrack *> m_pDeadTracks;

		MutStr                 m_name;

		SWaveFormat            m_wf;

		union
		{
			struct
			{
				// Disables all mixing if the user is playing their own music
				// and treats all tracks as music tracks
				Bool           bIsMusic : 1;
				// Pauses all tracks when a menu is activated (e.g., the pause
				// menu)
				Bool           bPauseOnMenu : 1;
				// Pauses all tracks when the app falls to the background
				Bool           bPauseOnInactive : 1;
			}                  bits;
			U32                uMask;
		}                      m_flags;

		AX_DELETE_COPYFUNCS(CSoundMixer);
	};


	/*

		SOUND TRACK
		===========
		Receives buffers to be mixed by a given mixer.

	*/
	class CSoundTrack
	{
	public:
		CSoundTrack( CSoundMixer &mixer, detail::ISoundVoiceHW *pHWVoice );
		~CSoundTrack();

		CSoundMixer       &mixer();
		CSoundMixer const &mixer() const;

		Bool start( CSoundClip &sound );
		Void stop();

	private:
		detail::ISoundVoiceHW *m_pHWVoice;

		CSoundMixer &          m_mixer;
		CSoundClip *           m_pClip;
		SSoundBuffer::Iter     m_curBuf;

		AX_DELETE_COPYFUNCS(CSoundTrack);
	};


	/*

		SOUND CLIP
		==========
		Collection of buffers and settings representing a sound.

	*/
	class CSoundClip
	{
	friend class CSoundTrack;
	public:
		CSoundClip();
		~CSoundClip();

		Bool setFormat( SWaveFormat const &fmt );
		SWaveFormat const &getFormat() const;

		Bool buffer( UPtr cBytes, const Void *pBytes, UPtr cSamples = 0, U8 cLoops = 0 );
		Bool buffer( const SSoundBuffer &buf );

		U32 playbackLengthInMilliseconds() const;
		U32 playbackLengthInSamples() const;
		U32 totalLengthInMilliseconds() const;
		U32 totalLengthInSamples() const;

		inline F64 playbackLengthInSeconds() const
		{
			return F64(playbackLengthInMilliseconds())/1000.0;
		}
		inline F64 totalLengthInSeconds() const
		{
			return F64(totalLengthInMilliseconds())/1000.0;
		}

	private:
		SWaveFormat        m_wf;
		SSoundBuffer::List m_buffers;
		U32                m_cTotalSamples;
		U32                m_cPlaybackSamples;

		AX_DELETE_COPYFUNCS(CSoundClip);
	};


	/*

		SOUND SYSTEM
		============
		Manages the whole sound system

	*/

	// Get a list of sound devices
	DOLL_FUNC Bool DOLL_API snd_enumDevices( TArr<SSoundDeviceInfo> &dst );
	inline TArr<SSoundDeviceInfo> DOLL_API snd_enumDevices()
	{
		TArr<SSoundDeviceInfo> r;
		return snd_enumDevices( r ), r;
	}
	// Initialize a given device
	//
	// Only one device may be initialized at a time.
	//
	// uDeviceId: Index corresponds to the index of the device information
	//            returned by snd_enumDevices()
	// pConf: Optional configuration settings for the device (if null or
	//        initialized to zeroes then device defaults are used)
	//
	// return: Non-null handle to the device on success, or null on failure
	DOLL_FUNC CSoundDevice *DOLL_API snd_init( UPtr uDeviceId = ~UPtr(0), const SSoundDeviceConf *pConf = nullptr );
	// Deinitialize the initialized device
	DOLL_FUNC Void DOLL_API snd_fini();
	// Retrieve the current device
	//
	// return: Non-null handle to the currently initialized device, or null if
	//         no device is presently initialized
	DOLL_FUNC CSoundDevice *DOLL_API snd_getDevice();
	// Do miscellaneous updates for the sound system
	DOLL_FUNC Void DOLL_API snd_sync();

}
