#define DOLL_TRACE_FACILITY doll::kLog_SndCore
#include "../BuildSettings.hpp"

#include "doll/Snd/SoundCore.hpp"

#include "doll/Core/Defs.hpp"
#include "doll/Core/Logger.hpp"

#include "doll/Math/Math.hpp"

#define DOLL_SND_IMPL_XAUDIO2 0

#if DOLL_DX_AVAILABLE || 1 // HACK
# undef  DOLL_SND_IMPL_XAUDIO2
# define DOLL_SND_IMPL_XAUDIO2 1
#endif

#if DOLL_SND_IMPL_XAUDIO2
# include "doll/Snd/API-XA2.hpp"
#else
# error Unknown sound implementation.
#endif

#include "doll/Snd/SoundMgr.hpp"

namespace doll
{

	struct SSoundGlob
	{
		detail::ISoundHW *pHW;
		CSoundDevice *    pDev;

		SSoundGlob()
		: pHW( nullptr )
		, pDev( nullptr )
		{
		}
		~SSoundGlob()
		{
			fini();
		}

		Bool init()
		{
			if( pHW != nullptr ) {
				return true;
			}

#if DOLL_SND_IMPL_XAUDIO2
			if( !( pHW = snd_xa2_initHW() ) ) {
				return false;
			}
#else
# error SSoundGlob::init() not implemented
#endif

			return true;
		}
		Void fini()
		{
			delete pDev;
			pDev = nullptr;

			delete pHW;
			pHW = nullptr;
		}
	};
	static SSoundGlob g_sound;

	DOLL_FUNC Bool DOLL_API snd_enumDevices( TArr<SSoundDeviceInfo> &dst )
	{
		if( !g_sound.init() ) {
			dst = TArr<SSoundDeviceInfo>();
			return false;
		}

		AX_ASSERT_NOT_NULL( g_sound.pHW );

		dst = g_sound.pHW->enumDevices();
		return true;
	}
	DOLL_FUNC CSoundDevice *DOLL_API snd_init( UPtr uDeviceId, const SSoundDeviceConf *pConf )
	{
		if( !g_sound.init() ) {
			return nullptr;
		}

		AX_ASSERT_NOT_NULL( g_sound.pHW );

		detail::ISoundDeviceHW *const pSndDevHW = g_sound.pHW->initDevice( uDeviceId, pConf );
		if( !AX_VERIFY_NOT_NULL( pSndDevHW ) ) {
			return nullptr;
		}

		CSoundDevice *const pSndDev = new CSoundDevice( pSndDevHW );
		if( !AX_VERIFY_MEMORY( pSndDev ) ) {
			return nullptr;
		}

		delete g_sound.pDev;
		g_sound.pDev = pSndDev;

		return pSndDev;
	}
	DOLL_FUNC Void DOLL_API snd_fini()
	{
		if( g_sound.pHW != nullptr ) {
			snd_playBGM( Str(), EPlayBGM::Now );
		}

		g_sound.fini();
	}
	DOLL_FUNC CSoundDevice *DOLL_API snd_getDevice()
	{
		return g_sound.pDev;
	}
	DOLL_FUNC Void DOLL_API snd_sync()
	{
		if( !g_sound.pHW ) {
			return;
		}

		g_sound.pHW->nextOperationSet();
	}




	/*

		SOUND DEVICE

	*/

	CSoundDevice::CSoundDevice( detail::ISoundDeviceHW *pHWDev )
	: m_pHWDevice( pHWDev )
	, m_pMixers()
	{
		AX_ASSERT_NOT_NULL( pHWDev );
		AX_ASSERT_NOT_NULL( g_sound.pHW );

		detail::ISoundMixerHW *const pMixerHW = g_sound.pHW->getMasterMixer( pHWDev );
		if( !pMixerHW ) {
			return;
		}

		CSoundMixer *const pMasterMixer = new CSoundMixer( *this, pMixerHW );
		if( !AX_VERIFY_MEMORY( pMasterMixer ) ) {
			return;
		}

		m_pMixers.append( pMasterMixer );
	}
	CSoundDevice::~CSoundDevice()
	{
		AX_ASSERT_NOT_NULL( g_sound.pHW );
		AX_ASSERT_NOT_NULL( m_pHWDevice );

		while( m_pMixers.isUsed() ) {
			delete m_pMixers.last();
		}

		g_sound.pHW->finiDevice( m_pHWDevice );
	}

	CSoundMixer &CSoundDevice::masterMixer()
	{
		AX_ASSERT( m_pMixers.isUsed() );
		AX_ASSERT_NOT_NULL( m_pMixers.first() );

		return *m_pMixers.first();
	}
	CSoundMixer const &CSoundDevice::masterMixer() const
	{
		AX_ASSERT( m_pMixers.isUsed() );
		AX_ASSERT_NOT_NULL( m_pMixers.first() );

		return *m_pMixers.first();
	}

	CSoundMixer *CSoundDevice::newMixer( Str name )
	{
		AX_ASSERT_NOT_NULL( g_sound.pHW );
		AX_ASSERT_NOT_NULL( m_pHWDevice );
		AX_ASSERT( m_pMixers.isUsed() );

		if( !AX_VERIFY_MEMORY( m_pMixers.reserve( m_pMixers.num() + 1 ) ) ) {
			return nullptr;
		}

		detail::ISoundMixerHW *const pRootMixerHW = g_sound.pHW->newMixer( m_pHWDevice, nullptr, 0 );
		if( !AX_VERIFY_NOT_NULL( pRootMixerHW ) ) {
			return nullptr;
		}

		CSoundMixer *const pRootMixer = new CSoundMixer( *this, pRootMixerHW );
		if( !AX_VERIFY_MEMORY( pRootMixer ) ) {
			g_sound.pHW->deleteMixer( m_pHWDevice, pRootMixerHW );
			return nullptr;
		}

		pRootMixer->m_name.assign( name );

		( Void )m_pMixers.append( pRootMixer );
		return pRootMixer;
	}
	TArr<CSoundMixer*> CSoundDevice::mixers() const
	{
		AX_ASSERT( m_pMixers.isUsed() );
		return TArr<CSoundMixer*>(m_pMixers).skip();
	}




	/*

		SOUND MIXER

	*/

	CSoundMixer::CSoundMixer( CSoundDevice &device, detail::ISoundMixerHW *pHWMixer )
	: m_pHWMixer( pHWMixer )
	, m_device( device )
	, m_pPrnt( nullptr )
	, m_pMixers()
	, m_pTracks()
	, m_pLiveTracks()
	, m_pDeadTracks()
	, m_name()
	, m_wf()
	{
		AX_ASSERT_NOT_NULL( pHWMixer );

		m_flags.uMask = 0;
	}
	CSoundMixer::~CSoundMixer()
	{
		AX_ASSERT_NOT_NULL( g_sound.pHW );
		AX_ASSERT_NOT_NULL( m_device.m_pHWDevice );

		// Remove reference to self from parent's list
		{
			TMutArr<CSoundMixer*> &prntMixers = m_pPrnt != nullptr ? m_pPrnt->m_pMixers : m_device.m_pMixers;
			for( UPtr j = prntMixers.num(); j > 0; --j ) {
				const UPtr i = j - 1;
				if( prntMixers[ i ] != this ) {
					continue;
				}

				prntMixers.remove( i );
				break;
			}
		}

		while( m_pMixers.isUsed() ) {
			delete m_pMixers.last();
		}

		while( m_pTracks.isUsed() ) {
			delete m_pTracks.last();
		}

		g_sound.pHW->deleteMixer( m_device.m_pHWDevice, m_pHWMixer );
	}

	CSoundMixer *CSoundMixer::parent()
	{
		return m_pPrnt;
	}
	CSoundMixer const *CSoundMixer::parent() const
	{
		return m_pPrnt;
	}

	TArr<CSoundMixer *> CSoundMixer::submixers() const
	{
		return m_pMixers;
	}

	Bool CSoundMixer::setTrackLimit( UPtr cTracks )
	{
		if( m_pLiveTracks.isUsed() ) {
			char szBuf[512];
			const UPtr n = m_pLiveTracks.num();
			DOLL_ERROR_LOG += (axspf(szBuf,"Cannot change track limit of mixer \"%.*s\" because %zu track%s %s still playing",m_name.len(),m_name.get(),n,n==1?"":"s",n==1?"is":"are"),szBuf);
			return false;
		}

		m_pDeadTracks.clear();

		bool r = true;

		r = r && AX_VERIFY_MEMORY( m_pTracks.reserve( cTracks ) );
		r = r && AX_VERIFY_MEMORY( m_pLiveTracks.reserve( cTracks ) );
		r = r && AX_VERIFY_MEMORY( m_pDeadTracks.reserve( cTracks ) );

		if( !r ) {
			char szBuf[512];
			DOLL_ERROR_LOG += (axspf(szBuf,"Failed to change track limit of mixer \"%.*s\" to %zu track%s",m_name.len(),m_name.get(),cTracks,cTracks==1?"":"s"),szBuf);
			return false;
		}

		U32 uHWFlags = 0;
		if( m_flags.bits.bIsMusic ) {
			uHWFlags |= detail::kSoundVoiceHWIsMusic;
		}
		// ### TODO ### detail::kSoundVoiceHWFilterable

		for( UPtr i = m_pTracks.num(); i < cTracks; ++i ) {
			detail::ISoundVoiceHW *const pVoiceHW = g_sound.pHW->newVoice( m_device.m_pHWDevice, m_pHWMixer, m_wf, uHWFlags );
			if( !AX_VERIFY_NOT_NULL( pVoiceHW ) ) {
				r = false;
				break;
			}

			CSoundTrack *const pTrack = new CSoundTrack( *this, pVoiceHW );
			if( !AX_VERIFY_MEMORY( pTrack ) ) {
				g_sound.pHW->deleteVoice( m_device.m_pHWDevice, pVoiceHW );
				r = false;
				break;
			}

			m_pTracks.append( pTrack );
			m_pDeadTracks.append( pTrack );
		}

		if( !r ) {
			char szBuf[512];
			DOLL_ERROR_LOG += (axspf(szBuf,"Failed to create all tracks for mixer \"%.*s\": have=%zu; want=%zu",m_name.len(),m_name.get(),m_pTracks.num(),cTracks),szBuf);
			return false;
		}

		return true;
	}
	UPtr CSoundMixer::getTrackLimit() const
	{
		return m_pTracks.num();
	}

	CSoundTrack *CSoundMixer::findFreeTrack() const
	{
		AX_ASSERT_MSG( m_pTracks.isUsed(), "Track limit not set" );

		if( m_pDeadTracks.isEmpty() ) {
			return nullptr;
		}

		return m_pDeadTracks.first();
	}

	TArr<CSoundTrack *> CSoundMixer::playingTracks() const
	{
		return m_pLiveTracks;
	}

	Void CSoundMixer::setWaveFormat( const SWaveFormat &wf )
	{
		AX_ASSERT_MSG( m_pTracks.isEmpty(), "Wave format must be set prior to setting track limit" );

		m_wf = wf;
	}
	const SWaveFormat &CSoundMixer::getWaveFormat() const
	{
		return m_wf;
	}




	/*

		SOUND TRACK

	*/

	static inline Void removeTrack( TMutArr<CSoundTrack*> &arr, CSoundTrack *p )
	{
		const UPtr n = arr.num();
		for( UPtr i = 0; i < n; ++i ) {
			if( arr[ i ] == p ) {
				arr.remove( i );
				return;
			}
		}
	}

	CSoundTrack::CSoundTrack( CSoundMixer &mixer, detail::ISoundVoiceHW *pHWVoice )
	: m_pHWVoice( pHWVoice )
	, m_mixer( mixer )
	, m_pClip( nullptr )
	, m_curBuf()
	{
		AX_ASSERT_NOT_NULL( pHWVoice );
	}
	CSoundTrack::~CSoundTrack()
	{
		AX_ASSERT_NOT_NULL( g_sound.pHW );

		removeTrack( m_mixer.m_pTracks, this );
		removeTrack( m_mixer.m_pLiveTracks, this );
		removeTrack( m_mixer.m_pDeadTracks, this );

		g_sound.pHW->deleteVoice( m_mixer.m_device.m_pHWDevice, m_pHWVoice );
	}

	CSoundMixer &CSoundTrack::mixer()
	{
		return m_mixer;
	}
	CSoundMixer const &CSoundTrack::mixer() const
	{
		return m_mixer;
	}

	Bool CSoundTrack::start( CSoundClip &sound )
	{
		AX_ASSERT_NOT_NULL( g_sound.pHW );
		AX_ASSERT_NOT_NULL( m_pHWVoice );
		AX_ASSERT_NOT_NULL( m_mixer.m_device.m_pHWDevice );

		AX_ASSERT_IS_NULL( m_pClip );
		AX_ASSERT( sound.m_buffers.isUsed() );

		m_pClip = &sound;
		m_curBuf = sound.m_buffers.begin();
		AX_ASSERT( m_curBuf != sound.m_buffers.end() );

		if( !AX_VERIFY( g_sound.pHW->submitBuffer( m_mixer.m_device.m_pHWDevice, m_pHWVoice, *m_curBuf ) ) ) {
			return false;
		}

		if( !AX_VERIFY( g_sound.pHW->startVoice( m_mixer.m_device.m_pHWDevice, m_pHWVoice ) ) ) {
			return false;
		}

		return true;
	}
	Void CSoundTrack::stop()
	{
		AX_ASSERT_NOT_NULL( g_sound.pHW );
		AX_ASSERT_NOT_NULL( m_pHWVoice );
		AX_ASSERT_NOT_NULL( m_mixer.m_device.m_pHWDevice );

		m_pClip = nullptr;

		g_sound.pHW->stopVoice( m_mixer.m_device.m_pHWDevice, m_pHWVoice, detail::kSoundVoiceHWStopNow );
	}




	/*

		SOUND CLIP

	*/

	CSoundClip::CSoundClip()
	: m_wf()
	, m_buffers()
	, m_cTotalSamples( 0 )
	, m_cPlaybackSamples( 0 )
	{
	}
	CSoundClip::~CSoundClip()
	{
	}

	Bool CSoundClip::setFormat( SWaveFormat const &fmt )
	{
		AX_ASSERT_MSG( m_buffers.isEmpty(), "Cannot change format after buffers are submitted" );

		if( !AX_VERIFY( fmt.isValid() ) ) {
			return false;
		}

		m_wf = fmt;
		return true;
	}
	SWaveFormat const &CSoundClip::getFormat() const
	{
		return m_wf;
	}

	Bool CSoundClip::buffer( UPtr cBytes, const Void *pBytes, UPtr cSamples, U8 cLoops )
	{
		AX_ASSERT_NOT_NULL( pBytes );
		AX_ASSERT( cBytes > 0 );

		if( !pBytes || !cBytes ) {
			return false;
		}

		SSoundBuffer buf;

		buf.pBytes = pBytes;
		buf.cBytes = cBytes;
		buf.cSamples = cSamples;
		buf.uLoopSample = 0;
		buf.cLoopSamples = 0;
		buf.cLoops = cLoops;

		return buffer( buf );
	}
	Bool CSoundClip::buffer( const SSoundBuffer &buf )
	{
		AX_ASSERT_NOT_NULL( buf.pBytes );
		AX_ASSERT( buf.cBytes > 0 );

		auto bufiter = m_buffers.addTail( buf );
		if( !AX_VERIFY_MEMORY( bufiter != m_buffers.end() ) ) {
			return false;
		}

		//const UPtr cSamples = buf.cSamples == 0 ? buf.cBytes/m_wf.uBlockAlign : buf.cSamples;
		if( buf.cSamples == 0 ) {
			AX_ASSERT_MSG( m_wf.getTag() == kWaveTagPCM, "Must use PCM for automatic sample calculation" );
			bufiter->cSamples = buf.cBytes/m_wf.uBlockAlign;
		}

		AX_ASSERT( m_cTotalSamples + bufiter->cSamples > m_cTotalSamples );
		m_cTotalSamples += (U32)bufiter->cSamples;

		const U32 cLoopSamples = buf.cLoopSamples != 0 ? buf.cLoopSamples : (U32)bufiter->cSamples;
		if( m_cPlaybackSamples + buf.cLoopSamples*buf.cLoops <= m_cPlaybackSamples ) {
			m_cPlaybackSamples = ~0U;
		} else {
			m_cPlaybackSamples += U32( bufiter->cSamples + cLoopSamples*buf.cLoops );
		}

		return true;
	}

	U32 CSoundClip::playbackLengthInMilliseconds() const
	{
		return U32( U64(m_cPlaybackSamples)*1000/m_wf.cSamplesHz );
	}
	U32 CSoundClip::playbackLengthInSamples() const
	{
		return m_cPlaybackSamples;
	}

	U32 CSoundClip::totalLengthInMilliseconds() const
	{
		return U32( U64(m_cTotalSamples)*1000/m_wf.cSamplesHz );
	}
	U32 CSoundClip::totalLengthInSamples() const
	{
		return m_cTotalSamples;
	}

}
