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

#include "chains/chain_micfwd_send.h"
#include "chains/chain_scofwd_recv.h"
#include "chains/chain_micfwd_send_2mic.h"
#include "chains/chain_scofwd_recv_2mic.h"


/*! AEC REF message ID */
#define OPMSG_AEC_REFERENCE_ID_SAME_INPUT_OUTPUT_CLK_SOURCE 0x0008

/*! \brief Message AECREF operator that the back-end of the operator are coming
    from same clock source. This is for optimisation purpose and it's recommended
    to be enabled for use cases where speaker input and microphone output are
    synchronised (e.g. SCO and USB voice use cases). Note: Send/Resend this message
    when all microphone input/output and REFERENCE output are disconnected.
    \param aec_op The AEC Reference operator.
*/
static void appKymeraSetAecSameIOClockSource(Operator aec_op)
{
    uint16 msg[2];
    msg[0] = OPMSG_AEC_REFERENCE_ID_SAME_INPUT_OUTPUT_CLK_SOURCE;
    msg[1] = 1;
    PanicFalse(OperatorMessage(aec_op, msg, 2, NULL, 0));
}

static void appKymeraScoForwardingSetSwitchedPassthrough(switched_passthrough_states state)
{
    kymera_chain_handle_t sco_chain = appKymeraGetScoChain();

    Operator spc_op = (Operator)PanicZero(ChainGetOperatorByRole(sco_chain,
                                                                 OPR_SWITCHED_PASSTHROUGH_CONSUMER));
    appKymeraConfigureSpcMode(spc_op, state);
}

static void appKymeraMicForwardingSetSwitchedPassthrough(switched_passthrough_states state)
{
    kymera_chain_handle_t sco_chain = appKymeraGetScoChain();
    Operator spc_op = (Operator)PanicZero(ChainGetOperatorByRole(sco_chain,
                                                                 OPR_MICFWD_RECV_SPC));
    appKymeraConfigureSpcMode(spc_op, state);
}

void appKymeraScoForwardingPause(bool pause)
{
    DEBUG_LOGF("appKymeraSfwdForwardingPause, pause %u", pause);
    if (appKymeraGetState() == KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING ||
        appKymeraGetState() == KYMERA_STATE_SCOFWD_RX_ACTIVE)
    {
        appKymeraScoForwardingSetSwitchedPassthrough(pause ? CONSUMER_MODE : PASSTHROUGH_MODE);
    }
}

void appKymeraHandleInternalScoForwardingStartTx(Sink forwarding_sink)
{
    UNUSED(forwarding_sink);
    kymeraTaskData *theKymera = appGetKymera();
    kymera_chain_handle_t sco_chain = appKymeraGetScoChain();

    if (appKymeraGetState() != KYMERA_STATE_SCO_ACTIVE || !sco_chain)
    {
        DEBUG_LOGF("appKymeraHandleInternalScoStartForwardingTx, failed, state %d, chain %u",
                   appKymeraGetState(), sco_chain);
        return;
    }

    Source scofwd_ep_src = PanicNull(ChainGetOutput(sco_chain, EPR_SCOFWD_TX_OTA));

    DEBUG_LOGF("appKymeraHandleInternalScoStartForwardingTx, sink %p, source %p, state %d",
                forwarding_sink, scofwd_ep_src, appKymeraGetState());

    PanicNotZero(theKymera->lock);

    /* Tell SCO forwarding what the source of SCO frames is and enable the
     * passthrough to give it the SCO frames. */
    appScoFwdInitScoPacketising(scofwd_ep_src);
    appKymeraScoForwardingSetSwitchedPassthrough(PASSTHROUGH_MODE);

    /* Setup microphone forwarding if it's enabled */
    if (appConfigMicForwardingEnabled())
    {
        /* Tell SCO forwarding what the forwarded microphone data sink is */
        Sink micfwd_ep_snk = PanicNull(ChainGetInput(sco_chain, EPR_MICFWD_RX_OTA));
        appScoFwdNotifyIncomingMicSink(micfwd_ep_snk);

        /* Setup the SPC to use the currently selected microphone
         * Will inform the peer to enable/disable MIC fowrading once the
         * peer MIC path has been setup.
        */
        appKymeraSelectSpcSwitchInput(appKymeraGetMicSwitch(), theKymera->mic);

        /* Put the microphone receive switched passthrough consumer into passthrough mode */
        appKymeraMicForwardingSetSwitchedPassthrough(PASSTHROUGH_MODE);
    }

    /* All done, so update kymera state */
    appKymeraSetState(KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING);
}

bool appKymeraHandleInternalScoForwardingStopTx(void)
{
    DEBUG_LOGF("appKymeraHandleInternalScoStopForwardingTx, state %u", appKymeraGetState());

    /* Can't stop forwarding if it hasn't been started */
    if (appKymeraGetState() != KYMERA_STATE_SCO_ACTIVE_WITH_FORWARDING)
        return FALSE;

    /* Put switched passthrough consumer into consume mode, so that no SCO frames are sent*/
    appKymeraScoForwardingSetSwitchedPassthrough(CONSUMER_MODE);

    /* Put the microphone receive switched passthrough consumer into consume mode */
    if (appConfigMicForwardingEnabled())
        appKymeraMicForwardingSetSwitchedPassthrough(CONSUMER_MODE);

    /* All done, so update kymera state and return indicating success */
    appKymeraSetState(KYMERA_STATE_SCO_ACTIVE);
    return TRUE;
}

void appKymeraHandleInternalScoForwardingStartRx(const KYMERA_INTERNAL_SCOFWD_RX_START_T *start_req)
{
    kymeraTaskData *theKymera = appGetKymera();
    const uint16 rate = 16000;
    kymera_chain_handle_t sco_chain = appKymeraGetScoChain();
    const chain_config_t *config;
    bool enable_mic_fwd = start_req->enable_mic_fwd && appConfigMicForwardingEnabled();

    DEBUG_LOGF("appKymeraHandleInternalScoForwardingStartRx, start source 0x%x, rate %u, state %u, micfwd %d",
                start_req->link_source, rate, appKymeraGetState(), enable_mic_fwd);

    PanicNotZero(theKymera->lock);

    if (appKymeraGetState() == KYMERA_STATE_TONE_PLAYING)
    {
        /* If there is a tone still playing at this point,
         * it must be an interruptible tone, so cut it off */
        appKymeraTonePromptStop();
    }

    /* If we are not idle (a pre-requisite) and this message can be delayed,
       then re-send it. The normal situation is message delays when stopping
       A2DP/AV. That is calls were issued in the right order to stop A2DP then
       start SCO receive but the number of messages required for each were
       different, leading the 2nd action to complete 1st. */
    if (   start_req->pre_start_delay
        && appKymeraGetState() != KYMERA_STATE_IDLE)
    {
        DEBUG_LOG("appKymeraHandleInternalScoForwardingStartRx, re-queueing.");
        appKymeraScoFwdStartReceiveHelper(start_req->link_source, start_req->volume,
                                          enable_mic_fwd,
                                          start_req->pre_start_delay - 1);
        return;
    }

    /* Can't start voice chain if we're not idle */
    PanicFalse(appKymeraGetState() == KYMERA_STATE_IDLE);

    /* SCO chain must be destroyed if we get here */
    PanicNotNull(sco_chain);

    /* Create appropriate SCO chain */
    if (appConfigScoMic2() != NO_MIC)
        config = enable_mic_fwd ? &chain_micfwd_send_2mic_config : &chain_scofwd_recv_2mic_config;
    else
        config = enable_mic_fwd ? &chain_micfwd_send_config : &chain_scofwd_recv_config;

    theKymera->chainu.sco_handle = sco_chain = ChainCreate(config);

    /* Get microphone sources */
    Source mic_src_1a;
    Source mic_src_1b;
    appKymeraMicSetup(appConfigScoMic1(), &mic_src_1a, appConfigScoMic2(), &mic_src_1b, rate);

    /* Get speaker sink */
    Sink spk_snk = StreamAudioSink(appConfigLeftAudioHardware(), appConfigLeftAudioInstance(), appConfigLeftAudioChannel());
    SinkConfigure(spk_snk, STREAM_CODEC_OUTPUT_RATE, rate);

    /* Get sources and sinks for chain endpoints */
    Source spk_ep_src  = ChainGetOutput(sco_chain, EPR_SCO_SPEAKER);
    Source sco_ep_src  = ChainGetOutput(sco_chain, EPR_MICFWD_TX_OTA);
    Sink sco_ep_snk    = ChainGetInput(sco_chain, EPR_SCOFWD_RX_OTA);
    Sink mic_1a_ep_snk = ChainGetInput(sco_chain, EPR_SCO_MIC1);
    Sink mic_1b_ep_snk = (appConfigScoMic2() != NO_MIC) ? ChainGetInput(sco_chain, EPR_SCO_MIC2) : 0;

    appScoFwdNotifyIncomingSink(sco_ep_snk);
    if (enable_mic_fwd)
        appScoFwdInitMicPacketising(sco_ep_src);

    /* Set AEC REF sample rate */
    Operator aec_op = ChainGetOperatorByRole(sco_chain, OPR_SCO_AEC);
    OperatorsAecSetSampleRate(aec_op, rate, rate);
    OperatorsStandardSetTimeToPlayLatency(aec_op, SFWD_TTP_DELAY_US);
    appKymeraSetAecSameIOClockSource(aec_op);

    /* Set async WBS decoder bitpool and buffer size */
    Operator awbs_op = ChainGetOperatorByRole(sco_chain, OPR_SCOFWD_RECV);
    OperatorsAwbsSetBitpoolValue(awbs_op, SFWD_MSBC_BITPOOL, TRUE);
    OperatorsStandardSetBufferSizeWithFormat(awbs_op, SFWD_RECV_CHAIN_BUFFER_SIZE, operator_data_format_pcm);

    Operator spc_op = ChainGetOperatorByRole(sco_chain, OPR_SWITCHED_PASSTHROUGH_CONSUMER);
    if (spc_op)
        appKymeraConfigureScoSpcDataFormat(spc_op, ADF_GENERIC_ENCODED);
    
    /* Set async WBS encoder bitpool if microphone forwarding is enabled */
    if (enable_mic_fwd)
    {
        Operator micwbs_op = ChainGetOperatorByRole(sco_chain, OPR_MICFWD_SEND);
        OperatorsAwbsSetBitpoolValue(micwbs_op, SFWD_MSBC_BITPOOL, FALSE);
    }

    /*! \todo Before updating from Products, this was not muting */
    appKymeraConfigureOutputChainOperators(sco_chain, rate, KICK_PERIOD_VOICE, 0, 0);
    appKymeraSetOperatorUcids(TRUE, FALSE);

    /* Connect microphones to chain microphone endpoints */
    StreamConnect(mic_src_1a, mic_1a_ep_snk);
    if (appConfigScoMic2() != NO_MIC)
        StreamConnect(mic_src_1b, mic_1b_ep_snk);

    /* Connect chain speaker endpoint to speaker */
    StreamConnect(spk_ep_src, spk_snk);

    /* Connect chain */
    ChainConnect(sco_chain);

    /* Enable external amplifier if required */
    appKymeraExternalAmpControl(TRUE);

    /* Start chain */
    if (ChainStartAttempt(sco_chain))
    {
        if (enable_mic_fwd)
        {
            /* Default to not forwarding the MIC. The primary will tell us when to start/stop using
               SFWD_OTA_MSG_MICFWD_START and SFWD_OTA_MSG_MICFWD_STOP mesages */

            DEBUG_LOG("appKymeraHandleInternalScoForwardingStartRx, not forwarding MIC data");
            appKymeraScoForwardingSetSwitchedPassthrough(CONSUMER_MODE);
        }        

        /* Move to SCO active state */
        appKymeraSetState(KYMERA_STATE_SCOFWD_RX_ACTIVE);
        theKymera->output_rate = rate;

        appKymeraHandleInternalScoSetVolume(start_req->volume);
    }
    else
    {
        /*! \todo Handle failure gracefully */
        DEBUG_LOG("appKymeraHandleInternalScoForwardingStartRx, chain failed to start");
        Panic();
    }
}

void appKymeraHandleInternalScoForwardingStopRx(void)
{
    kymeraTaskData *theKymera = appGetKymera();
    DEBUG_LOGF("appKymeraHandleInternalScoForwardingStopRx, state %u", appKymeraGetState());

    PanicNotZero(theKymera->lock);

    /* Should be in SCO forwarding active receive state */
    PanicFalse(appKymeraGetState() == KYMERA_STATE_SCOFWD_RX_ACTIVE);

    /* Get current SCO chain */
    kymera_chain_handle_t sco_chain = PanicNull(appKymeraGetScoChain());

    Sink mic_1a_ep_snk = ChainGetInput(sco_chain, EPR_SCO_MIC1);
    Sink mic_1b_ep_snk = (appConfigScoMic2() != NO_MIC) ? ChainGetInput(sco_chain, EPR_SCO_MIC2) : 0;
    Source spk_ep_src  = ChainGetOutput(sco_chain, EPR_SCO_SPEAKER);

    /* A tone still playing at this point must be interruptable */
    appKymeraTonePromptStop();

    /* Stop chains */
    ChainStop(sco_chain);

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


