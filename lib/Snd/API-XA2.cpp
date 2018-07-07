#define DOLL_TRACE_FACILITY doll::kLog_SndAPI
#include "../BuildSettings.hpp"

#include "doll/Snd/API-XA2.hpp"

#ifdef _WIN32

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:28218)
#endif
#include <XAudio2.h>
#ifdef _MSC_VER
# pragma warning(pop)
#endif

#include "doll/Util/SafeDX.hpp"

// ### HACK ### - I need to grab the debug version of x64 xaudio2.7
#define DOLL_XAUDIO2_DEBUG_ENABLED 0

#ifndef DOLL_XAUDIO2_DEBUG_ENABLED
# if AX_DEBUG_ENABLED
#  define DOLL_XAUDIO2_DEBUG_ENABLED 1
# else
#  define DOLL_XAUDIO2_DEBUG_ENABLED 0
# endif
#endif

#if DOLL_XAUDIO2_DEBUG_ENABLED
# define DOLL_XAUDIO2_FLAGS XAUDIO2_DEBUG_ENGINE
#else
# define DOLL_XAUDIO2_FLAGS 0
#endif

namespace doll
{

	namespace detail
	{

		template< typename T >
		class TFakedBase
		{
		public:
			typedef T Type;

			inline T *operator->()
			{
				return reinterpret_cast<T*>(this);
			}
			inline const T *operator->() const
			{
				return reinterpret_cast<const T*>(this);
			}
		};

		class ISoundDeviceHW: public TFakedBase<IXAudio2MasteringVoice>
		{
		};
		class ISoundMixerHW: public TFakedBase<IXAudio2SubmixVoice>
		{
		};
		class ISoundVoiceHW: public TFakedBase<IXAudio2SourceVoice>
		{
		};

	}

	template< typename T >
	inline typename T::Type *xa2ptr( T *p )
	{
		return reinterpret_cast<typename T::Type *>( p );
	}

	using detail::ISoundDeviceHW;
	using detail::ISoundMixerHW;
	using detail::ISoundVoiceHW;

	class CSoundHW_XA2: public virtual detail::ISoundHW
	{
	public:
		CSoundHW_XA2()
		: ISoundHW()
		, m_pEngine( nullptr )
		, m_devices()
		, m_uOperationSet( 1 )
		, m_bNeedNewOpSet( false )
		{
		}
		virtual ~CSoundHW_XA2()
		{
			fini();
		}

		Bool init()
		{
			AX_ASSERT_IS_NULL( m_pEngine );

			const UINT32 uCreateFlags = DOLL_XAUDIO2_FLAGS;

			DOLL_SAFE_DX( XAudio2Create( &m_pEngine, uCreateFlags ) ) {
				fini();
				return false;
			}

#if DOLL_XAUDIO2_DEBUG_ENABLED
			XAUDIO2_DEBUG_CONFIGURATION xdc;

			xdc.TraceMask       = XAUDIO2_LOG_WARNINGS;
			xdc.BreakMask       = XAUDIO2_LOG_ERRORS;
			xdc.LogThreadID     = TRUE;
			xdc.LogFileline     = TRUE;
			xdc.LogFunctionName = TRUE;
			xdc.LogTiming       = TRUE;

			m_pEngine->SetDebugConfiguration( &xdc );
#endif

			return true;
		}
		Void fini()
		{
			m_devices.purge();

			if( m_pEngine != nullptr ) {
				m_pEngine->Release();
				m_pEngine = nullptr;
			}
		}

		Bool tryEnumDevices()
		{
			AX_ASSERT_NOT_NULL( m_pEngine );

			UINT32 cDevices = 0;
			DOLL_SAFE_DX( m_pEngine->GetDeviceCount( &cDevices ) ) {
				return false;
			}

			m_devices.clear();
			m_devStrs.clear();

			if( !AX_VERIFY_MEMORY( m_devices.resize( cDevices ) ) ) {
				return false;
			}
			if( !AX_VERIFY_MEMORY( m_devStrs.resize( cDevices*2 ) ) ) {
				return false;
			}

			for( UINT32 uDevice = 0; uDevice < cDevices; ++uDevice ) {
				XAUDIO2_DEVICE_DETAILS dev;
				DOLL_SAFE_DX( m_pEngine->GetDeviceDetails( uDevice, &dev ) ) {
					m_devices.clear();
					return false;
				}

				m_devStrs[ uDevice*2 + 0 ] = MutStr::fromWStr( dev.DeviceID );
				m_devStrs[ uDevice*2 + 1 ] = MutStr::fromWStr( dev.DisplayName );

				if( !AX_VERIFY_MEMORY( m_devStrs[ uDevice*2 + 0 ].isUsed() ) ) {
					m_devices.clear();
					return false;
				}
				if( !AX_VERIFY_MEMORY( m_devStrs[ uDevice*2 + 1 ].isUsed() ) ) {
					m_devices.clear();
					return false;
				}

				SSoundDeviceInfo &dst = m_devices[ uDevice ];

				dst.textId         = m_devStrs[ uDevice*2 + 0 ];
				dst.name           = m_devStrs[ uDevice*2 + 1 ];
				dst.uDefRoleMask   = U8( dev.Role&0x8F );
				
				dst.cSamplesHz     = dev.OutputFormat.Format.nSamplesPerSec;
				dst.uChannelMask   = dev.OutputFormat.dwChannelMask;
				dst.cBitsPerSample = dev.OutputFormat.Format.wBitsPerSample;
			}

			return true;
		}

		virtual TArr<SSoundDeviceInfo> enumDevices() override
		{
			if( !tryEnumDevices() ) {
				return TArr<SSoundDeviceInfo>();
			}

			return m_devices;
		}
		virtual ISoundDeviceHW *initDevice( UPtr uDeviceId, const SSoundDeviceConf *pConf ) override
		{
			AX_ASSERT_NOT_NULL( m_pEngine );

			if( uDeviceId == ~UPtr( 0 ) ) {
				uDeviceId = 0;

				if( m_devices.isEmpty() && !tryEnumDevices() ) {
					DOLL_WARNING_LOG += "Failed to enumerate sound devices; defaulting to first device";
				}

				char szBuf[256];
				for( UPtr i = 0; i < m_devices.num(); ++i ) {
					if( ~m_devices[ i ].uDefRoleMask & kSoundDevDefGame ) {
						continue;
					}

					const Str &devid = m_devices[ i ].textId;
					const Str &name = m_devices[ i ].name;

					uDeviceId = i;
					DOLL_VERBOSE_LOG += (axspf(szBuf, "Defaulted to sound device #%zu \"%.*s\" (%.*s)", i + 1, name.len(), name.get(), devid.len(), devid.get()), szBuf);

					break;
				}
			} else if( uDeviceId >= m_devices.num() && uDeviceId > 0 ) {
				char szBuf[ 128 ];
				DOLL_WARNING_LOG += (axspf(szBuf, "Sound device #%zu given but only %zu available", uDeviceId + 1, m_devices.num()), szBuf);

				uDeviceId = 0;
			}

			UINT32 cInputChannels = XAUDIO2_DEFAULT_CHANNELS;
			UINT32 cInputSamplesHz = XAUDIO2_DEFAULT_SAMPLERATE;

			if( pConf != nullptr ) {
				if( pConf->cSamplesHz != 0 ) {
					cInputSamplesHz = pConf->cSamplesHz;
				}

				if( pConf->uChannelMask != 0 ) {
					cInputChannels = U32( countBits( pConf->uChannelMask ) );
				}
			}

			IXAudio2MasteringVoice *pMaster = nullptr;
			DOLL_SAFE_DX( m_pEngine->CreateMasteringVoice( &pMaster, cInputChannels, cInputSamplesHz, 0, UINT32(uDeviceId), nullptr ) ) {
				return nullptr;
			}

			return reinterpret_cast<ISoundDeviceHW *>( pMaster );
		}
		virtual Void finiDevice( ISoundDeviceHW *pDevice ) override
		{
			AX_ASSERT_NOT_NULL( m_pEngine );

			if( !pDevice ) {
				return;
			}

			xa2ptr(pDevice)->DestroyVoice();
		}

		virtual Void enableOutput() override
		{
			AX_ASSERT_NOT_NULL( m_pEngine );
			m_pEngine->StartEngine();
		}
		virtual Void disableOutput() override
		{
			AX_ASSERT_NOT_NULL( m_pEngine );
			m_pEngine->StopEngine();
		}

		virtual ISoundMixerHW *getMasterMixer( ISoundDeviceHW *pDevice ) override
		{
			AX_ASSERT_NOT_NULL( m_pEngine );
			AX_ASSERT_NOT_NULL( pDevice );
			return reinterpret_cast< ISoundMixerHW * >( pDevice );
		}
		virtual ISoundMixerHW *newMixer( ISoundDeviceHW *pDevice, ISoundMixerHW *pParentMixer, U32 uHierarchyLevel ) override
		{
			AX_ASSERT_NOT_NULL( m_pEngine );
			AX_ASSERT_NOT_NULL( pDevice );

			XAUDIO2_VOICE_DETAILS masterVoice;
			xa2ptr(pDevice)->GetVoiceDetails( &masterVoice );

			XAUDIO2_SEND_DESCRIPTOR sendDescs[] = { { 0, ( IXAudio2Voice * )pParentMixer } };
			XAUDIO2_VOICE_SENDS sends = { (UINT32)arraySize(sendDescs), &sendDescs[0] };

			const XAUDIO2_VOICE_SENDS *const pSendList = pParentMixer != nullptr ? &sends : nullptr;

			IXAudio2SubmixVoice *pSubmix = nullptr;
			DOLL_SAFE_DX( m_pEngine->CreateSubmixVoice( &pSubmix, masterVoice.InputChannels, masterVoice.InputSampleRate, 0, 10 + uHierarchyLevel, pSendList, nullptr ) ) {
				return nullptr;
			}

			return reinterpret_cast< ISoundMixerHW * >( pSubmix );
		}
		virtual Void deleteMixer( ISoundDeviceHW *pDevice, ISoundMixerHW *pMixer ) override
		{
			AX_ASSERT_NOT_NULL( m_pEngine );
			AX_ASSERT_NOT_NULL( pDevice );

			if( !pMixer || reinterpret_cast<ISoundMixerHW*>(pDevice) == pMixer ) {
				return;
			}

			xa2ptr(pMixer)->DestroyVoice();
		}

		virtual ISoundVoiceHW *newVoice( ISoundDeviceHW *pDevice, ISoundMixerHW *pMixer, const SWaveFormat &wf, U32 uFlags ) override
		{
			( Void )pDevice;

			AX_ASSERT_NOT_NULL( m_pEngine );
			AX_ASSERT_NOT_NULL( pDevice );

			UINT32 uXA2Flags = 0;

			if( takeFlag( uFlags, detail::kSoundVoiceHWIsMusic ) ) {
				uXA2Flags |= XAUDIO2_VOICE_MUSIC;
			}
			if( takeFlag( uFlags, detail::kSoundVoiceHWFilterable ) ) {
				uXA2Flags |= XAUDIO2_VOICE_USEFILTER;
			}

			if( uFlags != 0 ) {
				// ### TODO ### Warn about unknown or unhandled flags
			}

			float fMaxFreqRatio = XAUDIO2_DEFAULT_FREQ_RATIO;

			IXAudio2VoiceCallback *pCallback = nullptr;

			XAUDIO2_SEND_DESCRIPTOR sendDescs[] = { { 0, ( IXAudio2Voice * )pMixer } };
			XAUDIO2_VOICE_SENDS sends = { (UINT32)arraySize( sendDescs ), &sendDescs[ 0 ] };

			const XAUDIO2_VOICE_SENDS *const pSendList = pMixer != nullptr ? &sends : nullptr;

			IXAudio2SourceVoice *pVoice = nullptr;
			DOLL_SAFE_DX( m_pEngine->CreateSourceVoice( &pVoice, ( const WAVEFORMATEX * )&wf, uXA2Flags, fMaxFreqRatio, pCallback, pSendList, nullptr ) ) {
				return nullptr;
			}

			return reinterpret_cast<ISoundVoiceHW *>( pVoice );
		}
		virtual Void deleteVoice( ISoundDeviceHW *pDevice, ISoundVoiceHW *pVoice ) override
		{
			( Void )pDevice;

			AX_ASSERT_NOT_NULL( m_pEngine );
			AX_ASSERT_NOT_NULL( pDevice );

			xa2ptr(pVoice)->DestroyVoice();
		}

		virtual Bool submitBuffer( ISoundDeviceHW *pDevice, ISoundVoiceHW *pVoice, const SSoundBuffer &buf ) override
		{
			( Void )pDevice;

			AX_ASSERT_NOT_NULL( m_pEngine );
			AX_ASSERT_NOT_NULL( pDevice );
			AX_ASSERT_NOT_NULL( pVoice );
			
			AX_ASSERT_NOT_NULL( buf.pBytes );
			AX_ASSERT( buf.cBytes > 0 );
			AX_ASSERT( buf.cSamples > 0 );
			AX_ASSERT( buf.cBytes <= 0x7FFFFFFF );
			AX_ASSERT( buf.cSamples <= 0x7FFFFFFF );

			XAUDIO2_BUFFER xabuf;

			xabuf.Flags = 0;
			xabuf.AudioBytes = (UINT32)buf.cBytes;
			xabuf.pAudioData = ( const BYTE * )buf.pBytes;
			xabuf.PlayBegin = 0;
			xabuf.PlayLength = (UINT32)buf.cSamples;
			xabuf.LoopBegin = buf.uLoopSample;
			xabuf.LoopLength = buf.cLoopSamples;
			xabuf.LoopCount = buf.cLoops;
			xabuf.pContext = nullptr;

			DOLL_SAFE_DX( xa2ptr(pVoice)->SubmitSourceBuffer( &xabuf ) ) {
				return false;
			}

			return true;
		}

		virtual Void nextOperationSet() override
		{
			if( m_bNeedNewOpSet ) {
				m_bNeedNewOpSet = false;
				DOLL_SAFE_DX( m_pEngine->CommitChanges( atomicInc( &m_uOperationSet, Mem::Relaxed() ) ) ) {
					// nothing to do...
				}
			}
		}
		virtual Bool startVoice( ISoundDeviceHW *pDevice, ISoundVoiceHW *pVoice ) override
		{
			AX_ASSERT_NOT_NULL( m_pEngine );
			AX_ASSERT_NOT_NULL( pDevice );
			AX_ASSERT_NOT_NULL( pVoice );

			( void )pDevice;

			DOLL_SAFE_DX( xa2ptr(pVoice)->Start( 0, m_uOperationSet ) ) {
				return false;
			}

			m_bNeedNewOpSet = true;
			return true;
		}
		virtual Void stopVoice( ISoundDeviceHW *pDevice, ISoundVoiceHW *pVoice, U32 uFlags ) override
		{
			AX_ASSERT_NOT_NULL( m_pEngine );
			AX_ASSERT_NOT_NULL( pDevice );
			AX_ASSERT_NOT_NULL( pVoice );

			( void )pDevice;

			UINT32 uXA2Flags = 0;
			if( uFlags & detail::kSoundVoiceHWStopWithTails ) {
				uXA2Flags |= XAUDIO2_PLAY_TAILS;
			}

			xa2ptr(pVoice)->Stop( uXA2Flags, XAUDIO2_COMMIT_NOW );
		}

		virtual Void setVoiceVolumes( ISoundDeviceHW *pDevice, ISoundVoiceHW *pVoice, U32 cChannels,  const F32 *pVolumes ) override
		{
			AX_ASSERT_NOT_NULL( m_pEngine );
			AX_ASSERT_NOT_NULL( pDevice );
			AX_ASSERT_NOT_NULL( pVoice );

			( void )pDevice;

			xa2ptr(pVoice)->SetChannelVolumes( cChannels, pVolumes, m_uOperationSet );
			m_bNeedNewOpSet = true;
		}

	private:
		IXAudio2 *                m_pEngine;

		TMutArr<SSoundDeviceInfo> m_devices;
		TMutArr<MutStr>           m_devStrs;

		U32                       m_uOperationSet;
		Bool                      m_bNeedNewOpSet;
	};

	DOLL_FUNC detail::ISoundHW *DOLL_API snd_xa2_initHW()
	{
		CSoundHW_XA2 *const pSndHW = new CSoundHW_XA2();
		if( !AX_VERIFY_MEMORY( pSndHW ) ) {
			return nullptr;
		}

		if( !AX_VERIFY( pSndHW->init() ) ) {
			delete pSndHW;
			return nullptr;
		}

		return pSndHW;
	}

}

#else

namespace doll
{

	DOLL_FUNC detail::ISoundHW *DOLL_API snd_xa2_initHW()
	{
		return nullptr;
	}

}

#endif
