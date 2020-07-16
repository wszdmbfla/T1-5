/*!
\copyright  Copyright (c) 2017-2018  Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    Part of 6.3.2
\file
\brief      Kymera SCO
*/

#include <vmal.h>
#include <packetiser_helper.h>

#include "av_headset_kymera_private.h"

#include "chains/chain_sco_nb.h"
#include "chains/chain_sco_wb.h"
#include "chains/chain_micfwd_wb.h"
#include "chains/chain_micfwd_nb.h"
#include "chains/chain_scofwd_wb.h"
#include "chains/chain_scofwd_nb.h"

#include "chains/chain_sco_nb_2mic.h"
#include "chains/chain_sco_wb_2mic.h"
#include "chains/chain_micfwd_wb_2mic.h"
#include "chains/chain_micfwd_nb_2mic.h"
#include "chains/chain_scofwd_wb_2mic.h"
#include "chains/chain_scofwd_nb_2mic.h"


#define AWBSDEC_SET_BITPOOL_VALUE    0x0003
#define AWBSENC_SET_BITPOOL_VALUE    0x0001

#define AEC_TX_BUFFER_SIZE_MS 15

#define SCOFWD_BASIC_PASS_BUFFER_SIZE 512

typedef struct set_bitpool_msg_s
{
    uint16 id;
    uint16 bitpool;
}set_bitpool_msg_t;


void OperatorsAwbsSetBitpoolValue(Operator op, uint16 bitpool, bool decoder)
{
    set_bitpool_msg_t bitpool_msg;
    bitpool_msg.id = decoder ? AWBSDEC_SET_BITPOOL_VALUE : AWBSENC_SET_BITPOOL_VALUE;
    bitpool_msg.bitpool = (uint16)(bitpool);

    PanicFalse(VmalOperatorMessage(op, &bitpool_msg, SIZEOF_OPERATOR_MESSAGE(bitpool_msg), NULL, 0));
}


kymera_chain_handle_t appKymeraGetScoChain(void)
{
    kymeraTaskData *theKymera = appGetKymera();
    if (theKymera)
    {
        return theKymera->chainu.sco_handle;
    }
    return (kymera_chain_handle_t)NULL;
}

/* Set the SPC switch to a specific input 
    Inputs are renumbered 1,2 etc.
    0 will equal consume
 */
void appKymeraSelectSpcSwitchInput(Operator op, micSelection input)
{
    uint16 msg[2];
    msg[0] = 5 /* Temporary as OPMSG_SPC_ID_SELECT_PASSTHROUGH not avaioable */;
    msg[1] = input;
    PanicFalse(OperatorMessage(op, msg, 2, NULL, 0));
}

Operator appKymeraGetMicSwitch(void)
{
    kymera_chain_handle_t chain = appKymeraGetScoChain();
    
    if (chain)
    {
        return ChainGetOperatorByRole(chain, OPR_MICFWD_SPC_SWITCH);
    }
    return INVALID_OPERATOR;
}

void appKymeraSwitchSelectMic(micSelection mic)
{
    kymeraTaskData *theKymera = appGetKymera();
    
    /* always remember currently selected mic */
    theKymera->mic = mic;

    if (appKymeraGetState() == KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING)
    {
        Operator spc_switch = appKymeraGetMicSwitch();
        if (spc_switch)
        {
            DEBUG_LOGF("appKymeraSwitchSelectMic %u", mic);
            /* Tell the peer to start or stop sending MIC data */
            appScoFwdMicForwardingEnable(mic == MIC_SELECTION_REMOTE);
            appKymeraSelectSpcSwitchInput(spc_switch, mic);
        }
        else
        {
            DEBUG_LOG("appKymeraSwitchSelectMic failed to get OPR_MICFWD_SPC_SWITCH");
        }
    }
    else
    {
        DEBUG_LOGF("appKymeraSwitchSelectMic invalid state to switch mic %u", appKymeraGetState());
    }
}


/*  \brief Set an operator's terminal buffer sizes.
    \param op The operator.
    \param rate The sample rate is Hz.
    \param buffer_size_ms The amount of audio the buffer should contain in ms.
    \param input_terminals The input terminals for which the buffer size should be set (bitmask).
    \param output_terminals The output terminals for which the buffer size should be set (bitmask).
*/
static void appKymeraSetTerminalBufferSize(Operator op, uint32 rate, uint32 buffer_size_ms,
                                           uint16 input_terminals, uint16 output_terminals)
{
    uint16 msg[4];
    msg[0] = OPMSG_COMMON_ID_SET_TERMINAL_BUFFER_SIZE;
    msg[1] = (rate * buffer_size_ms) / 1000;
    msg[2] = input_terminals;
    msg[3] = output_terminals;
    OperatorMessage(op, msg, ARRAY_DIM(msg), NULL, 0);
}

static const bool sco_forwarding_supported = appConfigScoForwardingEnabled();


/* SCO chain selection is wrapped in this function.
   
   This code relies on the NB and WB chains being basically 
   "the same" other than operators.
 */
static kymera_chain_handle_t appKymeraScoCreateChain(hfp_wbs_codec_mask codec,
                                                     bool use_sco_fwd)
{
    kymeraTaskData *theKymera = appGetKymera();
    const chain_config_t *chain_config = NULL;

    /* Select narrowband or wideband chain depending on CODEC */
    bool is_wideband = (codec == hfp_wbs_codec_mask_msbc);

    if (appConfigScoMic2() == NO_MIC)
    {
        DEBUG_LOGF("appKymeraCreateScoChain, 1 Mic. SCO Forwarding : supported=%d, requested=%d",
                                                    sco_forwarding_supported, use_sco_fwd);
        if (use_sco_fwd)
        {
            if (appConfigMicForwardingEnabled())
                chain_config = is_wideband ? &chain_micfwd_wb_config : &chain_micfwd_nb_config;
            else
                chain_config = is_wideband ? &chain_scofwd_wb_config : &chain_scofwd_nb_config;
        }
        else
        {
            chain_config = is_wideband ? &chain_sco_wb_config : &chain_sco_nb_config;
        }
    }
    else
    {
        DEBUG_LOGF("appKymeraCreateScoChain, 2 Mic. SCO Forwarding : supported=%d. requested=%d",
                                                    sco_forwarding_supported, use_sco_fwd);
        if (use_sco_fwd)
        {
            if (appConfigMicForwardingEnabled())
                chain_config = is_wideband ? &chain_micfwd_wb_2mic_config : &chain_micfwd_nb_2mic_config;
            else
                chain_config = is_wideband ? &chain_scofwd_wb_2mic_config : &chain_scofwd_nb_2mic_config;
        }
        else
        {
            chain_config = is_wideband ? &chain_sco_wb_2mic_config : &chain_sco_nb_2mic_config;
        }
    }

    return theKymera->chainu.sco_handle = ChainCreate(chain_config);
}


static void appKymeraScoConfigureForwardingChain(kymera_chain_handle_t sco_chain, bool is_narrowband)
{
    const uint16 rate = is_narrowband ? 8000 : 16000;

    /* Set AEC REF sample rate */
    Operator aec_op = ChainGetOperatorByRole(sco_chain, OPR_SCO_AEC);
    OperatorsAecSetSampleRate(aec_op, rate, rate);

    /* Set AEC REF input terminal buffer size */
    appKymeraSetTerminalBufferSize(aec_op, rate, AEC_TX_BUFFER_SIZE_MS, 1 << 0, 0);

    /* Confiure SBC encoder bitpool for forwarded audio */
    Operator awbs_op = PanicZero(ChainGetOperatorByRole(sco_chain, OPR_SCOFWD_SEND));
    OperatorsAwbsSetBitpoolValue(awbs_op, SFWD_MSBC_BITPOOL, FALSE);

    /* Configure upsampler from 8K to 16K for narrowband, or passthough from wideband */
    if (is_narrowband)
    {
        Operator upsampler_op = PanicZero(ChainGetOperatorByRole(sco_chain, OPR_SCO_UP_SAMPLE));
        OperatorsResamplerSetConversionRate(upsampler_op, 8000, 16000);
    }
    else
    {
        /* Wideband chains add a basic passthrough in place of the resampler.
           This is currently required to avoid issues when the splitter
           is connected directly to the encoder. */
        Operator basic_pass = PanicZero(ChainGetOperatorByRole(sco_chain, OPR_SCOFWD_BASIC_PASS));
        OperatorsStandardSetBufferSizeWithFormat(basic_pass, SCOFWD_BASIC_PASS_BUFFER_SIZE, operator_data_format_pcm);
    }

    if (appConfigMicForwardingEnabled())
    {
        /* setup the mic recv spc for encoded data */
        Operator spc_op;
        PanicFalse(GET_OP_FROM_CHAIN(spc_op, sco_chain, OPR_MICFWD_RECV_SPC));
        appKymeraConfigureSpcDataFormat(spc_op, ADF_16_BIT_WITH_METADATA);

        Operator mic_recv = ChainGetOperatorByRole(sco_chain,OPR_MICFWD_RECV);
        OperatorsAwbsSetBitpoolValue(mic_recv, SFWD_MSBC_BITPOOL, TRUE);
        OperatorsStandardSetBufferSizeWithFormat(mic_recv, SFWD_MICFWD_RECV_CHAIN_BUFFER_SIZE, operator_data_format_pcm);

        Operator mic_switch = ChainGetOperatorByRole(sco_chain, OPR_MICFWD_SPC_SWITCH);
        appKymeraConfigureSpcDataFormat(mic_switch, ADF_PCM);

        /* resample the incoming mic data to the SCO sample rate if necessary */
        if (is_narrowband)
        {
            Operator downsampler_op;
            PanicFalse(GET_OP_FROM_CHAIN(downsampler_op, sco_chain, OPR_MIC_DOWN_SAMPLE));
            OperatorsResamplerSetConversionRate(downsampler_op, 16000, 8000);
        }
    }

    Operator splitter_op;
    PanicFalse(GET_OP_FROM_CHAIN(splitter_op, sco_chain, OPR_SCOFWD_SPLITTER));
    OperatorsConfigureSplitter(splitter_op, SFWD_SEND_CHAIN_BUFFER_SIZE, TRUE, operator_data_format_pcm);

    /* Configure passthrough for encoded data so we can connect. */
    Operator switch_op;
    PanicFalse(GET_OP_FROM_CHAIN(switch_op, sco_chain, OPR_SWITCHED_PASSTHROUGH_CONSUMER));
    appKymeraConfigureScoSpcDataFormat(switch_op,  ADF_GENERIC_ENCODED);

    Operator sco_op = PanicZero(ChainGetOperatorByRole(sco_chain, OPR_SCO_RECEIVE));
    OperatorsStandardSetTimeToPlayLatency(sco_op, SFWD_TTP_DELAY_US);
}


static void appKymeraScoConfigureChain(kymera_chain_handle_t sco_chain, bool is_narrowband, uint8 wesco)
{
    const uint16 rate = is_narrowband ? 8000 : 16000;

    /* Set AEC REF sample rate */
    Operator aec_op = ChainGetOperatorByRole(sco_chain, OPR_SCO_AEC);
    OperatorsAecSetSampleRate(aec_op, rate, rate);

    /* Set AEC REF input terminal buffer size */
    appKymeraSetTerminalBufferSize(aec_op, rate, AEC_TX_BUFFER_SIZE_MS, 1 << 0, 0);

    /*! \todo Need to decide ahead of time if we need any latency.
        Simple enough to do if we are legacy or not. Less clear if
        legacy but no peer connection */
    /* Enable Time To Play if supported */
    if (appConfigScoChainTTP(wesco) != 0)
    {
        Operator sco_op = PanicZero(ChainGetOperatorByRole(sco_chain, OPR_SCO_RECEIVE));
        OperatorsStandardSetTimeToPlayLatency(sco_op, appConfigScoChainTTP(wesco));

        /*! \todo AEC Gate is V2 silicon, downloadable has native TTP support*/
        OperatorsAecEnableTtpGate(aec_op, TRUE, 50, TRUE);
    }
}



void appKymeraHandleInternalScoStart(Sink sco_snk, hfp_wbs_codec_mask codec,
                                     uint8 wesco, uint16 volume, bool use_sco_fwd)
{
    kymeraTaskData *theKymera = appGetKymera();
    const uint16 rate = (codec == hfp_wbs_codec_mask_msbc) ? 16000 : 8000;
    const bool is_narrowband = (rate == 8000);

    DEBUG_LOGF("appKymeraHandleInternalScoStart, sink 0x%x, rate %u, wesco %u, state %u", sco_snk, rate, wesco, appKymeraGetState());

    if (appKymeraGetState() == KYMERA_STATE_TONE_PLAYING)
    {
        /* If there is a tone still playing at this point,
         * it must be an interruptible tone, so cut it off */
        appKymeraTonePromptStop();
    }

    /* Can't start voice chain if we're not idle */
    PanicFalse(appKymeraGetState() == KYMERA_STATE_IDLE);

    /* SCO chain must be destroyed if we get here */
    PanicNotNull(appKymeraGetScoChain());

    /* Move to SCO active state now, what ever happens we end up in this state
      (even if it's temporary) */
    appKymeraSetState(KYMERA_STATE_SCO_ACTIVE);

    /* Configure DSP power mode appropriately for CVC */
    appKymeraConfigureDspPowerMode(FALSE);

    /* Create appropriate SCO chain */
    kymera_chain_handle_t sco_chain = appKymeraScoCreateChain(codec, use_sco_fwd);

    /* Get microphone sources */
    Source mic_src_1a;
    Source mic_src_1b;
    appKymeraMicSetup(appConfigScoMic1(), &mic_src_1a, appConfigScoMic2(), &mic_src_1b, rate);

    /* Get speaker sink */
    Sink spk_snk = StreamAudioSink(appConfigLeftAudioHardware(), appConfigLeftAudioInstance(), appConfigLeftAudioChannel());
    SinkConfigure(spk_snk, STREAM_CODEC_OUTPUT_RATE, rate);

    /* Get SCO source from SCO sink */
    Source sco_src = StreamSourceFromSink(sco_snk);

    /* Get sources and sinks for chain endpoints */
    Source spk_ep_src  = ChainGetOutput(sco_chain, EPR_SCO_SPEAKER);
    Source sco_ep_src  = ChainGetOutput(sco_chain, EPR_SCO_TO_AIR);
    Sink sco_ep_snk    = ChainGetInput(sco_chain, EPR_SCO_FROM_AIR);
    Sink mic_1a_ep_snk = ChainGetInput(sco_chain, EPR_SCO_MIC1);
    Sink mic_1b_ep_snk = (appConfigScoMic2() != NO_MIC) ? ChainGetInput(sco_chain, EPR_SCO_MIC2) : 0;

    /* Configure chain specific operators */
    if (use_sco_fwd)
        appKymeraScoConfigureForwardingChain(sco_chain, is_narrowband);
    else
        appKymeraScoConfigureChain(sco_chain, is_narrowband, wesco);

    appKymeraConfigureOutputChainOperators(sco_chain, rate, KICK_PERIOD_VOICE, 0, 0);
    appKymeraSetOperatorUcids(TRUE, is_narrowband);

    /* Connect SCO to chain SCO endpoints */
    StreamConnect(sco_ep_src, sco_snk);
    StreamConnect(sco_src, sco_ep_snk);

    /* Connect microphones to chain microphone endpoints */
    StreamConnect(mic_src_1a, mic_1a_ep_snk);
    if (appConfigScoMic2() != NO_MIC)
        StreamConnect(mic_src_1b, mic_1b_ep_snk);

    /* Connect chain speaker endpoint to speaker */
    StreamConnect(spk_ep_src, spk_snk);

    /* Connect chain */
    ChainConnect(sco_chain);

    if (use_sco_fwd && appConfigMicForwardingEnabled())
    {
        /* Chain connection sets the switch into consume mode.
           Select the local Microphone */
        appKymeraSelectSpcSwitchInput(appKymeraGetMicSwitch(), MIC_SELECTION_LOCAL);
    }

    /* Enable external amplifier if required */
    appKymeraExternalAmpControl(TRUE);

    /* The chain can fail to start if the SCO source disconnects whilst kymera
    is queuing the SCO start request or starting the chain. If the attempt fails,
    ChainStartAttempt will stop (but not destroy) any operators it started in the chain. */
    if (ChainStartAttempt(sco_chain))
    {
        theKymera->output_rate = rate;
        appKymeraHandleInternalScoSetVolume(volume);
    }
    else
    {
        DEBUG_LOG("appKymeraHandleInternalScoStart could not start chain");
        /* Stop/destroy the chain, returning state to KYMERA_STATE_IDLE.
        This needs to be done here, since between the failed attempt to start
        and the subsequent stop (when appKymeraScoStop() is called), a tone
        may need to be played - it would not be possible to play a tone in a
        stopped SCO chain. The state needs to be KYMERA_STATE_SCO_ACTIVE for
        appKymeraHandleInternalScoStop() to stop/destroy the chain. */
        appKymeraHandleInternalScoStop();
    }
}

void appKymeraHandleInternalScoStop(void)
{
    kymeraTaskData *theKymera = appGetKymera();
    DEBUG_LOGF("appKymeraHandleInternalScoStop, state %u", appKymeraGetState());

    if (appKymeraGetState() == KYMERA_STATE_IDLE)
    {
        /* Attempting to stop a SCO chain when IDLE. This happens when the user
        calls appKymeraScoStop() following a failed attempt to start the SCO
        chain - see ChainStartAttempt() in appKymeraHandleInternalScoStart().
        In this case, there is nothing to do, since the failed start attempt
        cleans up by calling this function in state KYMERA_STATE_SCO_ACTIVE */
        DEBUG_LOG("appKymeraHandleInternalScoStop, not stopping - already idle");
        return;
    }

    /* Stop forwarding first */
    if (appKymeraGetState() == KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING)
        PanicFalse(appKymeraHandleInternalScoForwardingStopTx());

    /* Should be in SCO active state */
    PanicFalse(appKymeraGetState() == KYMERA_STATE_SCO_ACTIVE);

    /* Get current SCO chain */
    kymera_chain_handle_t sco_chain = PanicNull(appKymeraGetScoChain());

    Source spk_ep_src  = ChainGetOutput(sco_chain, EPR_SCO_SPEAKER);
    Source sco_ep_src  = ChainGetOutput(sco_chain, EPR_SCO_TO_AIR);
    Sink sco_ep_snk    = ChainGetInput(sco_chain, EPR_SCO_FROM_AIR);
    Sink mic_1a_ep_snk = ChainGetInput(sco_chain, EPR_SCO_MIC1);
    Sink mic_1b_ep_snk = (appConfigScoMic2() != NO_MIC) ? ChainGetInput(sco_chain, EPR_SCO_MIC2) : 0;

    /* A tone still playing at this point must be interruptable */
    appKymeraTonePromptStop();

    /* Stop chains */
    ChainStop(sco_chain);

    /* Disconnect SCO from chain SCO endpoints */
    StreamDisconnect(sco_ep_src, NULL);
    StreamDisconnect(NULL, sco_ep_snk);

    /* Disconnect microphones from chain microphone endpoints */
    StreamDisconnect(NULL, mic_1a_ep_snk);
    if (appConfigScoMic2() != NO_MIC)
        StreamDisconnect(NULL, mic_1b_ep_snk);

    /* Disconnect chain speaker endpoint to speaker */
    StreamDisconnect(spk_ep_src, NULL);

    /* Close microphone sources */
    appKymeraMicCleanup(appConfigScoMic1(), appConfigScoMic2());

    /* Destroy chains */
    ChainDestroy(sco_chain);
    theKymera->chainu.sco_handle = sco_chain = NULL;

    /* Disable external amplifier if required */
    if (!appKymeraAncIsEnabled())
        appKymeraExternalAmpControl(FALSE);

    /* Update state variables */
    appKymeraSetState(KYMERA_STATE_IDLE);
    theKymera->output_rate = 0;
}

void appKymeraHandleInternalScoSetVolume(uint8 volume)
{
    kymera_chain_handle_t scoChain = appKymeraGetScoChain();

    DEBUG_LOGF("appKymeraHandleInternalScoSetVolume, vol %u", volume);

    switch (appGetKymera()->state)
    {
        case KYMERA_STATE_SCO_ACTIVE:
        case KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING:
        case KYMERA_STATE_SCOFWD_RX_ACTIVE:
        {
            uint16 volume_scaled = ((uint16)volume * VOLUME_MAX) / 15;
            appKymeraSetMainVolume(scoChain, volume_scaled);
        }
        break;
        default:
            break;
    }
}

void appKymeraHandleInternalScoMicMute(bool mute)
{
    DEBUG_LOGF("appKymeraHandleInternalScoMicMute, mute %u", mute);

    switch (appGetKymera()->state)
    {
        case KYMERA_STATE_SCO_ACTIVE:
        {
            Operator aec_op;

            if (GET_OP_FROM_CHAIN(aec_op, appKymeraGetScoChain(), OPR_SCO_AEC))
            {
                OperatorsAecMuteMicOutput(aec_op, mute);
            }
        }
        break;

        default:
            break;
    }
}

uint8 appKymeraScoVoiceQuality(void)
{
    uint8 quality = appConfigVoiceQualityWorst();

    if (appConfigVoiceQualityMeasurementEnabled())
    {
        Operator cvc_send_op;
        if (GET_OP_FROM_CHAIN(cvc_send_op, appKymeraGetScoChain(), OPR_CVC_SEND))
        {
            uint16 rx_msg[2], tx_msg = OPMSG_COMMON_GET_VOICE_QUALITY;
            PanicFalse(OperatorMessage(cvc_send_op, &tx_msg, 1, rx_msg, 2));
            quality = MIN(appConfigVoiceQualityBest() , rx_msg[1]);
            quality = MAX(appConfigVoiceQualityWorst(), quality);
        }
    }
    else
    {
        quality = appConfigVoiceQualityWhenDisabled();
    }

    DEBUG_LOGF("appKymeraScoVoiceQuality %u", quality);

    return quality;
}

