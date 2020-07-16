/*!
\copyright  Copyright (c) 2008 - 2018 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    Part of 6.3.2
\file       av_headset_ui.c
\brief      Application User Interface
*/

#include <panic.h>
#include <ps.h>
#include <boot.h>
#include <input_event_manager.h>

#include "av_headset.h"
#include "av_headset_ui.h"
#include "av_headset_sm.h"
#include "av_headset_hfp.h"
#include "av_headset_power.h"
#include "av_headset_log.h"

#if defined(INCLUDE_EQ)
#include "av_headset_eq.h"
#endif

/*! \brief Bit set in lock to indicate UI event in progress */
#define APP_LOCK_PROMPT_UI                           0x0001

/*! Include the correct button header based on the number of buttons available to the UI */
#if defined(HAVE_9_BUTTONS)
#include "9_buttons.h"
#elif defined(HAVE_6_BUTTONS)
#include "6_buttons.h"
#elif defined(HAVE_1_BUTTON)
#include "1_button.h"
#else
#error "No buttons define found"
#endif

#ifdef HAVE_LTR2668
#include "peripherals/ltr2668.h"
#endif

#if defined(INCLUDE_TAP_SENSOR)  
#include "av_headset_tap_sensor.h"
#endif
#include "av_headset_scofwd.h"

#define EQ_MODE_TONE


/*! User interface internal messasges */
enum ui_internal_messages
{
    /*! Message sent later when a prompt is played. Until this message is delivered
        repeat prompts will not be played */
    UI_INTERNAL_CLEAR_LAST_PROMPT,
    UI_INTERNAL_CLEAR_DUT,    
    UI_INTERNAL_GETVOLTAGE,
    UI_INTERNAL_READ_PS_HOLD,
    UI_INTERNAL_ALLOW_ENTRY_DUT,    
    UI_INTERNAL_DUT_REBOOT_TIMEOUT,
#if defined(INCLUDE_TAP_SENSOR)  
    UI_INTERNAL_TAP_DOUBLE_CLICK,
#endif
#if defined(TWS_PEER_SYNC_POWEROFF)
    UI_INTERNAL_PEER_SYNC_POWEROFF,
#endif
#if defined(HAVE_LP_POWERON)
    UI_INTERNAL_FORBID_POWEROFF,
#endif
    UI_INTERNAL_NORMAL_DELAY_POWEROFF,  //  used for play completed "power off "
    UI_INTERNAL_FORBID_FACTORY_RESET,
#if defined(UI_LP_8S_AND_DOUBLECLICK_FACTORYRESET)
    UI_INTERNAL_ALLOW_FACTORY_RESET,
#endif
#if defined(EQ_MODE_TONE)
    UI_INTERNAL_DELAY_EQ_TONE,
#endif
    UI_INTERNAL_PEER_FORCED_PAIRING,
    UI_INTERNAL_PEER_FORCED_PAIRING_TIMEOUT,
    UI_INTERNAL_POWERON_CONNECT_HANDSET,
    UI_INTERNAL_VOICE_ACTIVE_TIMEOUT,
    UI_USER_SINGLE_KEY,
    UI_USER_DOUBLE_KEY,
    UI_USER_TRIPLE_KEY
};

/*! At the end of every tone, add a short rest to make sure tone mxing in the DSP doens't truncate the tone */
#define RINGTONE_STOP  RINGTONE_NOTE(REST, HEMIDEMISEMIQUAVER), RINGTONE_END

#define UI_DUT_TIMEOUT              D_SEC(1.5)
#define UI_GETVOLTAGE_TIMEOUT       D_SEC(60)
#define UI_FACTROY_TIMEOUT          D_SEC(2)

#if defined(MEDIA_AUTO_RESUME_PLAY)
extern bool g_peer_master_in_case_flag;
#endif
/*!@{ \name Definition of LEDs, and basic colour combinations

    The basic handling for LEDs is similar, whether there are
    3 separate LEDs, a tri-color LED, or just a single LED.
 */

#if (appConfigNumberOfLeds() == 3)
#define LED_0_STATE  (1 << 0)
#define LED_1_STATE  (1 << 1)
#define LED_2_STATE  (1 << 2)
#elif (appConfigNumberOfLeds() == 2)
/* We only have 2 LED so map all control to the same LED */
#define LED_0_STATE  (1 << 0)
#define LED_1_STATE  (1 << 1)
#define LED_2_STATE  (1 << 1)
#else
/* We only have 1 LED so map all control to the same LED */
#define LED_0_STATE  (1 << 0)
#define LED_1_STATE  (1 << 0)
#define LED_2_STATE  (1 << 0)
#endif

#define LED_RED         (LED_0_STATE)
#define LED_WHITE       (LED_2_STATE)
#define LED_BOTH        (LED_RED | LED_WHITE)

extern uint8 earbudVersion[];
extern uint8 earbudProName[];

#if defined(INCLUDE_EQ)
uint16 appui_get_eq_mode(void)
{
    uiTaskData *theUi = appGetUi();
    return theUi->eq_mode;
}
#endif
/*!@} */

/*! \brief An LED filter used for battery low

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_battery_low(uint16 led_state)
{
    return (led_state) ? LED_RED : 0;
}

/*! \brief An LED filter used for low charging level

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_low(uint16 led_state)
{
    UNUSED(led_state);
    return LED_RED;
}

/*! \brief An LED filter used for charging level OK

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_ok(uint16 led_state)
{
    UNUSED(led_state);
    //DEBUG_LOGF("app_led_filter_charging_ok:%d", led_state);
    return LED_RED;
}

uint16 app_led_filter_enter_DUT(uint16 led_state)
{
    UNUSED(led_state);
    return LED_WHITE;
}

uint16 app_led_filter_enter_factory_reset(uint16 led_state)
{
    UNUSED(led_state);
    return LED_BOTH;
}

bool appUiIsinFactoryReset(void)
{
    uiTaskData *theUi = appGetUi();
    return theUi->is_factoryReset;
}

/*! \brief An LED filter used for charging complete 

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_complete(uint16 led_state)
{
    UNUSED(led_state);
    return LED_RED;
}

/*! \cond led_patterns_well_named
    No need to document these. The public interface is
    from public functions such as appUiPowerOn()
 */

const ledPattern app_led_pattern_power_on[] = 
{
    LED_LOCK,
    LED_ON(LED_WHITE),   LED_WAIT(800),  LED_OFF(LED_WHITE),   LED_WAIT(200),
    LED_REPEAT(1, 1),
    LED_UNLOCK,
    LED_END
};

const ledPattern app_led_pattern_power_off[] = 
{
    LED_LOCK,
    LED_ON(LED_RED),  LED_WAIT(5000),
    LED_UNLOCK,
    LED_END
};

const ledPattern app_led_pattern_out_of_case[] = 
{
    LED_LOCK,
    LED_ON(LED_WHITE),   LED_WAIT(500),  LED_OFF(LED_WHITE),   LED_WAIT(200),
    LED_REPEAT(1, 1),
    LED_UNLOCK,
    LED_END
};

const ledPattern app_led_pattern_notify[] = 
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(50), LED_OFF(LED_WHITE), LED_WAIT(100),
    LED_UNLOCK,
    LED_END
};

const ledPattern app_led_red_pattern_notify[] = 
{
    LED_LOCK,
    LED_ON(LED_RED), LED_WAIT(50), LED_OFF(LED_RED), LED_WAIT(100),
    LED_UNLOCK,
    LED_END
};

const ledPattern app_led_pattern_dut[] = 
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(800), LED_OFF(LED_WHITE), LED_WAIT(2000),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
    LED_END
};

const ledPattern app_led_pattern_error[] = 
{
    LED_LOCK,
    LED_ON(LED_RED), LED_WAIT(100), LED_OFF(LED_RED), LED_WAIT(100),
    LED_REPEAT(1, 2),
    LED_UNLOCK,
    LED_END
};

const ledPattern app_led_pattern_idle[] = 
{
    LED_SYNC(2000),
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(200), LED_OFF(LED_WHITE), LED_WAIT(200),
    LED_ON(LED_WHITE), LED_WAIT(200), LED_OFF(LED_WHITE), LED_WAIT(2400),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const ledPattern app_led_pattern_idle_connected[] = 
{
    LED_SYNC(1000),
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(100), LED_OFF(LED_WHITE),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const ledPattern app_led_pattern_pairing[] = 
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_OFF(LED_RED),LED_WAIT(350),
    LED_ON(LED_RED), LED_OFF(LED_WHITE), LED_WAIT(350),
    LED_UNLOCK,
    LED_REPEAT(0, 0)
};

const ledPattern app_led_pattern_pairing_deleted[] = 
{
    LED_LOCK,
    LED_ON(LED_RED), LED_WAIT(100), LED_OFF(LED_RED), LED_WAIT(100),
    LED_REPEAT(1, 2),
    LED_UNLOCK,
    LED_END
};

const ledPattern app_led_pattern_pre_peer[] =
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(200), LED_OFF(LED_WHITE), LED_WAIT(200),
    LED_REPEAT(1, 2),
    LED_UNLOCK,
    LED_END
};

const ledPattern app_led_pattern_peer_pairing[] =
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(100), LED_OFF(LED_WHITE), LED_WAIT(100),
    LED_UNLOCK,
    LED_REPEAT(0, 0)
};

const ledPattern app_led_pattern_sco_s[] =
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(50), LED_OFF(LED_WHITE), LED_WAIT(100),
    LED_REPEAT(1, 1),
    LED_WAIT(5000),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const ledPattern app_led_pattern_sco_m[] =
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(50), LED_OFF(LED_WHITE), LED_WAIT(5000),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

#ifdef INCLUDE_AV
const ledPattern app_led_pattern_streaming_s[] =
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(100), LED_OFF(LED_WHITE), LED_WAIT(6900),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const ledPattern app_led_pattern_streaming_m[] =
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(100), LED_OFF(LED_WHITE), LED_WAIT(6900),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

/* a2dp of audio is playing*/
const ledPattern app_led_pattern_streaming_aptx_s[] =
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(100), LED_OFF(LED_WHITE), LED_WAIT(6900),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

const ledPattern app_led_pattern_streaming_aptx_m[] =
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(100), LED_OFF(LED_WHITE), LED_WAIT(6900),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

#endif

const ledPattern app_led_pattern_battery_low[] = 
{
    LED_LOCK,
    LED_ON(LED_WHITE), LED_WAIT(100), LED_OFF(LED_WHITE), LED_WAIT(100),
    LED_REPEAT(1, 1),
    LED_WAIT(8000),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};

/*! \cond constant_well_named_tones 
    No Need to document these tones. Their access through functions such as
    appUiIdleActive() is the public interface.
 */
 
const ringtone_note app_tone_button[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_button_siri[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_button_factory_reset[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(A7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(C7, SEMIQUAVER),
    RINGTONE_NOTE(B7, SEMIQUAVER),
    RINGTONE_STOP
};

#ifdef INCLUDE_AV
const ringtone_note app_tone_av_connect[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_disconnect[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_remote_control[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_connected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_NOTE(A6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_disconnected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A6,  SEMIQUAVER),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_link_loss[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};
#endif

const ringtone_note app_tone_hfp_connect[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_connected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_NOTE(A6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_disconnected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A6,  SEMIQUAVER),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_link_loss[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};
        
const ringtone_note app_tone_hfp_sco_connected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(AS5, DEMISEMIQUAVER),
    RINGTONE_NOTE(DS6, DEMISEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_sco_disconnected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(DS6, DEMISEMIQUAVER),
    RINGTONE_NOTE(AS5, DEMISEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_mute_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_sco_unencrypted_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_ring[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),
    RINGTONE_NOTE(REST, SEMIQUAVER),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_ring_caller_id[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_voice_dial[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_voice_dial_disable[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_answer[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_hangup[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_mute_active[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(CS7, SEMIQUAVER),
    RINGTONE_NOTE(DS7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_mute_inactive[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(DS7, SEMIQUAVER),
    RINGTONE_NOTE(CS7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_talk_long_press[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_pairing[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_paired[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A6, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_pairing_deleted[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(A6, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_volume[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_volume_limit[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_error[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_battery_empty[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B6, SEMIQUAVER),
    RINGTONE_NOTE(B6, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_power_on[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_power_off[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_paging_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_peer_pairing[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_peer_pairing_error[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_peer_pairing_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_STOP
};


#if defined (EQ_MODE_TONE)
const ringtone_note app_tone_eq_mode0[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};
const ringtone_note app_tone_eq_mode1[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};
const ringtone_note app_tone_eq_mode2[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};
const ringtone_note app_tone_eq_mode3[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};
const ringtone_note app_tone_eq_mode4[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};
#endif

#if defined(PEER_FORCE_PAIRING)
/* L &R earbud  Longpress 5S ,earbud forced to pairing status*/
static uiForcedPairing ui_fpairing_status={FALSE,FALSE};
static void uiForcedpairingclean(void)
{
    ui_fpairing_status.peer = FALSE;
    ui_fpairing_status.self = FALSE;
}

void uiPeerReadytoForcedPairing(void)
{
    MessageCancelAll(appGetUiTask(), UI_INTERNAL_PEER_FORCED_PAIRING);
    MessageSend(appGetUiTask(), UI_INTERNAL_PEER_FORCED_PAIRING, NULL);
}
#endif

/*! \endcond constant_well_named_tones */

/*! \brief Play tone.
    \param tone The tone to play.
    \param interruptible If TRUE, always play to completion, if FALSE, the tone may be
    interrupted before completion.
    \param queueable If TRUE, tone can be queued behind already playing tone, if FALSE, the tone will
    only play if no other tone playing or queued.
    \param client_lock If not NULL, bits set in client_lock_mask will be cleared
    in client_lock when the tone finishes - either on completion, when interrupted,
    or if the tone is not played at all, because the UI is not currently playing tones.
    \param client_lock_mask A mask of bits to clear in the client_lock.
*/
void appUiPlayToneCore(const ringtone_note *tone, bool interruptible, bool queueable,
                       uint16 *client_lock, uint16 client_lock_mask)
{
#ifndef INCLUDE_TONES
    UNUSED(tone);
    UNUSED(interruptible);
    UNUSED(queueable);
#else
    /* Only play tone if it can be heard */
    if (PHY_STATE_IN_EAR == appPhyStateGetState())
    {
        if (queueable || !appKymeraIsTonePlaying())
            appKymeraTonePlay(tone, interruptible, client_lock, client_lock_mask);
    }
    else
#endif
    {
        if (client_lock)
        {
            *client_lock &= ~client_lock_mask;
        }
    }
}

/*! \brief Play prompt.
    \param prompt The prompt to play.
    \param interruptible If TRUE, always play to completion, if FALSE, the prompt may be
    interrupted before completion.
    \param client_lock If not NULL, bits set in client_lock_mask will be cleared
    in client_lock when the prompt finishes - either on completion, when interrupted,
    or if the prompt is not played at all, because the UI is not currently playing prompts.
    \param client_lock_mask A mask of bits to clear in the client_lock.
*/
void appUiPlayPromptCore(voicePromptName prompt, bool interruptible, bool queueable,
                         uint16 *client_lock, uint16 client_lock_mask)
{
#ifndef INCLUDE_PROMPTS
    UNUSED(prompt);
    UNUSED(interruptible);
    UNUSED(queueable);
#else
    uiTaskData *theUi = appGetUi();
    PanicFalse(prompt < NUMBER_OF_PROMPTS);
    /* Only play prompt if it can be heard */
    if ((PHY_STATE_IN_CASE != appPhyStateGetState()) && (prompt != theUi->prompt_last) &&
        (queueable || !appKymeraIsTonePlaying()))
    {
        const promptConfig *config = appConfigGetPromptConfig(prompt);
        FILE_INDEX *index = theUi->prompt_file_indexes + prompt;
        if (*index == FILE_NONE)
        {
            const char* name = config->filename;
            *index = FileFind(FILE_ROOT, name, strlen(name));
            /* Prompt not found */
            PanicFalse(*index != FILE_NONE);
        }
        appKymeraPromptPlay(*index, config->format, config->rate,
                            interruptible, client_lock, client_lock_mask);

        if (appConfigPromptNoRepeatDelay())
        {
            MessageCancelFirst(&theUi->task, UI_INTERNAL_CLEAR_LAST_PROMPT);
            MessageSendLater(&theUi->task, UI_INTERNAL_CLEAR_LAST_PROMPT, NULL,
                             appConfigPromptNoRepeatDelay());
            theUi->prompt_last = prompt;
        }
    }
    else
#endif
    {
        if (client_lock)
        {
            *client_lock &= ~client_lock_mask;
        }
    }
}

/*! \brief Report a generic error on LEDs and play tone */
void appUiError(void)
{
    appUiPlayTone(app_tone_error);
    appLedSetPattern(app_led_pattern_error, LED_PRI_EVENT);
}

/*! \brief Play HFP error tone and set LED error pattern.
    \param silent If TRUE the error is not presented on the UI.
*/
void appUiHfpError(bool silent)
{
    if (!silent)
    {
        appUiPlayTone(app_tone_error);
        appLedSetPattern(app_led_pattern_error, LED_PRI_EVENT);
    }
}

/*! \brief Play AV error tone and set LED error pattern.
    \param silent If TRUE the error is not presented on the UI.
*/
void appUiAvError(bool silent)
{
    if (!silent)
    {
        appUiPlayTone(app_tone_error);
        appLedSetPattern(app_led_pattern_error, LED_PRI_EVENT);
    }
}


/*! \brief Play power on prompt and LED pattern */
void appUiPowerOn(void)
{
    /* Enable LEDs */
    appLedEnable(TRUE);
#if defined(HAVE_LP_POWERON)
    MessageSendLater(appGetUiTask(), UI_INTERNAL_FORBID_POWEROFF, NULL, D_SEC(5));
    SetPoweroffReason(1);
#endif
    appLedSetPattern(app_led_pattern_power_on, LED_PRI_EVENT);
    appUiPlayPrompt(PROMPT_POWER_ON);
    // start the timer
    MessageSendLater(appGetUiTask(), UI_INTERNAL_GETVOLTAGE, NULL, UI_GETVOLTAGE_TIMEOUT);

    MessageSendLater(appGetUiTask(), UI_INTERNAL_POWERON_CONNECT_HANDSET, NULL, D_SEC(5));
}

/*! \brief Play power off prompt and LED pattern.
    \param lock The caller's lock, may be NULL.
    \param lock_mask Set bits in lock_mask will be cleared in lock when the UI completes.
 */
void appUiPowerOff(uint16 *lock, uint16 lock_mask)
{
   // appLedSetPattern(app_led_pattern_power_off, LED_PRI_EVENT);

    /* time out to sleep shoud play prompt, use put to shutdown don't play*/
    if(2 == appGetPower()->shutdown_reson){
        appUiPlayPromptClearLock(PROMPT_POWER_OFF, lock, lock_mask);
    }
    else if (lock){
        *lock &= ~lock_mask;
    }

    /* Disable LEDs */
    appLedEnable(FALSE);
    // stop the timer
    MessageCancelAll(appGetUiTask(), UI_INTERNAL_GETVOLTAGE);
}

/*! \brief Prepare UI for sleep.
    \note If in future, this function is modified to play a tone, it should
    be modified to resemble #appUiPowerOff, so the caller's lock is cleared when
    the tone is completed. */
void appUiSleep(void)
{
    appLedSetPattern(app_led_pattern_power_off, LED_PRI_EVENT);
    appLedEnable(FALSE);
}

void appUiSetDUT(bool b){
    appGetUi()->is_dut = b;
}

void appUiAllowEnterDUT(void)
{
   MessageCancelAll(appGetUiTask(), UI_INTERNAL_ALLOW_ENTRY_DUT);
   MessageSendLater(appGetUiTask(), UI_INTERNAL_ALLOW_ENTRY_DUT, NULL, D_SEC(5));
}
static void appUiEnterDUT(void)
{
    DEBUG_LOG("appUiEnterDUT");
    appUiSetDUT(TRUE);
    appUiPlayPrompt(PROMPT_DUT);
    appLedCancelFilter(1);
    appLedSetFilter(app_led_filter_enter_DUT, 1);
    MessageCancelAll(appGetUiTask(), UI_INTERNAL_DUT_REBOOT_TIMEOUT);
    MessageSendLater(appGetUiTask(), UI_INTERNAL_DUT_REBOOT_TIMEOUT, NULL, D_SEC(5));
}

void appUiDUTReboot(void)
{
    appSetRebootDUTMode(1);
    BootSetMode(1);
}

void appAvVolumeUpEx(void)
{
    DEBUG_LOG("appAvVolumeUpEx");
    
    if (appHfpIsScoActive())
     {
     	 appHfpVolumeStart(1);
     	 appHfpVolumeStop(appConfigGetHfpVolumeStep());
     }
    else if (appScoFwdIsReceiving())
     {
       appScoFwdVolumeStart(1);
       appScoFwdVolumeStop(appConfigGetHfpVolumeStep());
     }
#ifdef INCLUDE_AV
    else if (appAvIsStreaming())
     {
     	 appAvVolumeStart(8);
     	 appAvVolumeStop(appConfigGetAvVolumeStep());
     }
#endif
    else if (appHfpIsConnected())
     {
     	 appHfpVolumeStart(1);
     	 appHfpVolumeStop(appConfigGetHfpVolumeStep());
     }
    else if (appScoFwdIsConnected())
     {
       appScoFwdVolumeStart(1);
       appScoFwdVolumeStop(appConfigGetHfpVolumeStep());
     }

}

/*! \brief Send volume down command
*/
void appAvVolumeDownEx(void)
{
    DEBUG_LOG("appAvVolumeDownEx");
    
    if (appHfpIsScoActive())
     {
       appHfpVolumeStart(-1);
       appHfpVolumeStop(-appConfigGetHfpVolumeStep());
     }
    else if (appScoFwdIsReceiving())
     {
       appScoFwdVolumeStart(-1);
       appScoFwdVolumeStop(-appConfigGetHfpVolumeStep());
     }
#ifdef INCLUDE_AV
    else if (appAvIsStreaming())
     {
     	 appAvVolumeStart(-8);
       appAvVolumeStop(-appConfigGetAvVolumeStep());
     }
#endif
    else if (appHfpIsConnected())
     {
       appHfpVolumeStart(-1);
       appHfpVolumeStop(-appConfigGetHfpVolumeStep());
     }
    else if (appScoFwdIsConnected())
     {
     	 appScoFwdVolumeStart(-1);
     	 appScoFwdVolumeStop(-appConfigGetHfpVolumeStep());
     }
}
/*! \brief Message Handler

    This function is the main message handler for the UI module, all user button
    presses are handled by this function.

    NOTE - only a single button config is currently defined for both earbuds.
    The following defines could be used to split config amongst the buttons on
    two earbuds.

        APP_RIGHT_CONFIG
        APP_SINGLE_CONFIG
        APP_LEFT_CONFIG
*/ 
static uint16 appKeypressCnt = 0;

static void multiClickCheck(void)
{
   uiTaskData *theUi = appGetUi();
   MessageCancelFirst(&theUi->task, UI_USER_SINGLE_KEY);
   MessageCancelFirst(&theUi->task, UI_USER_DOUBLE_KEY);
   MessageCancelFirst(&theUi->task, UI_USER_TRIPLE_KEY);

   appKeypressCnt++;
   if(appKeypressCnt == 2)
   {
     MessageSendLater(&theUi->task,UI_USER_DOUBLE_KEY,NULL,800);
   }
   else if(appKeypressCnt == 3)
   {
     MessageSendLater(&theUi->task,UI_USER_TRIPLE_KEY,NULL,800);
   }
   else
   {
     MessageSendLater(&theUi->task,UI_USER_SINGLE_KEY,NULL,800);
   }
}
/*
static void lhttestaudio(void)
{
    static uint8 audio_count = 0;
    DEBUG_LOGF("lhttestaudio audio_count:%d",audio_count);
    appUiPlayPrompt(audio_count);
    audio_count+=1;
    if(audio_count> PROMPT_LOW_BATTERY)
       audio_count =0;
}
*/
   
static void appUiHandleMessage(Task task, MessageId id, Message message)
{
    uiTaskData *theUi = (uiTaskData *)task;
    bdaddr      peerAddr;
    static  bool was_battery_low = FALSE;
    static  uint8 battery_low_count = 0;
    static  bool av_was_straming = FALSE;
    static  bool forbid_factoryreset = FALSE;
    UNUSED(message);

    if(id != UI_INTERNAL_GETVOLTAGE){
        appPowerOffTimerRestart();
    }
#if defined(UI_LP_8S_AND_DOUBLECLICK_FACTORYRESET)
    if(theUi->is_factoryReset && !(id == APP_MFB_PRESS || id == UI_USER_DOUBLE_KEY || id == CHARGER_MESSAGE_CHARGER_COMPLETE || id == UI_INTERNAL_GETVOLTAGE \
      || id ==UI_INTERNAL_ALLOW_FACTORY_RESET || id == APP_MFB_HELD_R_8S)){
        DEBUG_LOGF("ready to factory reset,forbid other id:%d",id);
        return ;
    }
#endif
    switch (id)
    {
        case UI_INTERNAL_CLEAR_LAST_PROMPT:
            theUi->prompt_last = PROMPT_NONE;
        break;

        /* HFP call/reject & A2DP play/pause */
        case APP_MFB_PRESS:
            DEBUG_LOG("APP_MFB_BUTTON_PRESS");
            multiClickCheck();
            break;

        case UI_USER_SINGLE_KEY:
            DEBUG_LOG("UI_USER_SINGLE_KEY");
            appKeypressCnt = 0;
#ifdef APP_CHECK_DEV_NAME_VALIDATION
            if(!appCheckDevNameValidation())
            {
              DEBUG_LOG("device name invalid");
              appUiError();
              break;
            }
#endif
            if(appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE){
                    DEBUG_LOG("UI_USER_SINGLE_KEY incase Do nothing");
            }
            else{
                    /* peer not paired so retry */
                  if(!appDeviceGetPeerBdAddr(&peerAddr) && !appSmIsPairingPeer()){
                     appSetState(APP_STATE_PEER_PAIRING);
                    }
                  else
                      appPeerSyncSend(FALSE);
           }
          break;

          case UI_USER_DOUBLE_KEY:
            DEBUG_LOG("double click");
            appKeypressCnt = 0;
#ifdef APP_CHECK_DEV_NAME_VALIDATION
         if(!appCheckDevNameValidation())
         {
            DEBUG_LOG("device name invalid");
            appUiError();
            break;
         }
#endif
            if(appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE){
                    DEBUG_LOG("UI_USER_DOUBLE_KEY incase ");
                #if defined(UI_LP_8S_AND_DOUBLECLICK_FACTORYRESET)
                if(0 != MessageCancelAll(appGetUiTask(), UI_INTERNAL_ALLOW_FACTORY_RESET) && theUi->is_factoryReset){
                    #if defined(HAVE_LP_POWERON)
                      SetPoweroffReason(0);
                    #endif
                    theUi->is_factoryReset = FALSE;
                    appSmFactoryReset();
                }
                #endif
            }
            else{
                 /* 1. outof case
                    2. itself not headset pairing
                    3. itself not peer pairing
                    4. peer not headset piring */
                 if (!theUi->is_dut && !appSmIsPairing() && !appPeerSyncIsPeerPairing() && !appSmIsPairingPeer() &&
                     !appDeviceIsHandsetConnected() && !appPeerSyncIsPeerHandsetAvrcpConnected() && appDeviceGetPeerBdAddr(&peerAddr)){
                     DEBUG_LOG("pair handset");
                     appSmPairHandset();
                 }
            }
            break;

        case UI_USER_TRIPLE_KEY:            
            DEBUG_LOG("triple click");
            appKeypressCnt = 0;
#ifdef APP_CHECK_DEV_NAME_VALIDATION
        if(!appCheckDevNameValidation())
        {
            DEBUG_LOG("device name invalid");
            appUiError();
            break;
        }

#endif
            if(appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE){
                DEBUG_LOG("UI_USER_TRIPLE_KEY incase ");
                if(0 != MessageCancelAll(appGetUiTask(),UI_INTERNAL_ALLOW_ENTRY_DUT)&& !appPeerSyncIsPeerHandsetAvrcpConnected() && !appDeviceIsHandsetAvrcpConnected()){
                    /*   No handset conected && TWS paired
                    *    enter in case state & enter DUT mode
                    */
                    appUiEnterDUT();
                }
            }
            else if(appDeviceIsHandsetConnected()|| (appDeviceIsPeerAvrcpConnectedForAv()&&appPeerSyncIsPeerHandsetAvrcpConnected())){
                theUi->eq_mode += 1;
                if(theUi->eq_mode >= 5)  // CUSTOMER_EQ_MODE_MAX
                    theUi->eq_mode =0;
                appUiUpdateEQ(theUi->eq_mode,FALSE);
                #if defined(EQ_MODE_TONE)
                  MessageSendLater(appGetUiTask(), UI_INTERNAL_DELAY_EQ_TONE, NULL,D_SEC(3));
                #endif
            }

            break;
#if defined(EQ_MODE_TONE)
        case UI_INTERNAL_DELAY_EQ_TONE:
            {
                switch(theUi->eq_mode){
                    case 0:
                         appUiPlayTone(app_tone_eq_mode0);
                        break;
                case 1:
                     appUiPlayTone(app_tone_eq_mode1);
                    break;
                case 2:
                     appUiPlayTone(app_tone_eq_mode2);
                    break;
                case 3:
                     appUiPlayTone(app_tone_eq_mode3);
                    break;
                case 4:
                     appUiPlayTone(app_tone_eq_mode4);
                    break;
                default:
                    break;
                }
    }
            break;
#endif
        case APP_MFB_HELD_1200MS:
            DEBUG_LOG("APP_MFB_HELD_1200MS");
            if (appAvIsStreaming()){
               // appUiButton();
                if (appDeviceIsHandsetAvrcpConnected()){
                     appAvPlayToggle(FALSE);
                     av_was_straming = TRUE;
                }
                // If AVRCP is peer is connected and peer is connected to handset, send play or pause
                else if (appDeviceIsPeerAvrcpConnectedForAv() && appPeerSyncIsComplete() && appPeerSyncIsPeerHandsetAvrcpConnected()){
                     appSocHfpFouceSendAvPlayToggle();
                     av_was_straming = TRUE;
                }
            }
            break;

        case APP_MFB_HELD_1500MS:
            DEBUG_LOG("APP_MFB_HELD_1500MS");
            if(appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE){
                DEBUG_LOG("APP_MFB_HELD_1500MS Incase Nothing");
            }
            else{
                appUiButton();
            }
            break;

        case APP_MFB_HELD_R_1500MS:
            DEBUG_LOG("APP_MFB_HELD_R_1500MS");
#ifdef APP_CHECK_DEV_NAME_VALIDATION
           if(!appCheckDevNameValidation())
           {
              DEBUG_LOG("device name invalid");
              appUiError();
              break;
           }
#endif
            if(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE){
                DEBUG_LOG("APP_MFB_HELD_R_1500MS Incase Nothing");
            }
            else {
                if (appHfpIsCallIncoming())
                    appHfpCallAccept();
                else if (appScoFwdIsCallIncoming())
                    appScoFwdCallAccept();
                /* If voice call active, hangup */
                else if (appHfpIsCallActive()){
                     appHfpCallHangup();
                }
                /* Sco Forward can be streaming a ring tone */
                else if (appScoFwdIsReceiving() && !appScoFwdIsCallIncoming()){
                    appScoFwdCallHangup();
                }
                /* If AVRCP to handset connected, send play or pause */
                else if (appDeviceIsHandsetAvrcpConnected() && !av_was_straming){
                    appAvPlayToggle(FALSE);
                }
                /* If AVRCP is peer is connected and peer is connected to handset, send play or pause */
                else if (appDeviceIsPeerAvrcpConnectedForAv() && appPeerSyncIsComplete() && appPeerSyncIsPeerHandsetAvrcpConnected()&& !av_was_straming){
                    appSocHfpFouceSendAvPlayToggle();
                }
            }  
            av_was_straming = FALSE;
            break;

        #if defined(HAVE_LP_POWERON)
            case DONTPOWERON:
                if(appChargerCanPowerOff())
                    PowerDoPowerOff();
                else{
                    SetPoweroffReason(0);
                    MessageSend(appGetAppTask(), POWERONPROMIT, NULL);
                }
              break;

        #endif
        case APP_MFB_HELD_3P5S:
            DEBUG_LOG("APP_MFB_HELD_3P5S");
            if(av_was_straming)
                av_was_straming = FALSE;

            #if defined(HAVE_LP_POWERON)
            {
               bool reson = ReadPoweroffReason();
               if(reson){
                   MessageCancelAll(task, DONTPOWERON);
                   MessageSend(appGetAppTask(), POWERONPROMIT, NULL);
                   SetPoweroffReason(0);
               }
            }
            #endif
            break;

        case APP_MFB_HELD_R_3P5S:
            DEBUG_LOG("APP_MFB_HELD_R_3P5S");

            break;
        case APP_MFB_HELD_4S:
            DEBUG_LOG("APP_MFB_HELD_4S");
            if(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE ){
                DEBUG_LOG("Held 4S incase,Nothing");
            }
            else{
                if(0 == MessageCancelAll(appGetUiTask(),UI_INTERNAL_FORBID_POWEROFF)){
                    if(appChargerCanPowerOff()){
                        appLedSetPattern(app_led_pattern_power_off, LED_PRI_EVENT);
                        #if defined(TWS_PEER_SYNC_POWEROFF)
                            if(appPeerSyncIsComplete()&& !appPeerSyncIsPeerInCase()){
                                appSocHfpPeerSyncPoweroff();
                            }
                        #endif
                        appGetUi()->ui_lock |= APP_LOCK_PROMPT_UI;
                        MessageSendConditionally(appGetUiTask(), UI_INTERNAL_PEER_SYNC_POWEROFF,
                                                 NULL, &appGetUi()->ui_lock);
                        appUiPlayPromptClearLock(PROMPT_POWER_OFF, &appGetUi()->ui_lock, APP_LOCK_PROMPT_UI);
                    }
               }
            }
            break;

        case APP_MFB_HELD_R_4S:
            DEBUG_LOGF("APP_MFB_HELD_R_4S");
            if(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE ){
                MessageCancelAll(appGetUiTask(),UI_INTERNAL_FORBID_POWEROFF);
                DEBUG_LOG("R_4S incase Do nothing");
            }
            /*
            else if(0 == MessageCancelAll(appGetUiTask(),UI_INTERNAL_FORBID_POWEROFF)){
                if(appChargerCanPowerOff()){
                    appLedSetPattern(app_led_pattern_power_off, LED_PRI_EVENT);
                    #if defined(TWS_PEER_SYNC_POWEROFF)
                        if(appPeerSyncIsComplete()&& !appPeerSyncIsPeerInCase()){
                            appSocHfpPeerSyncPoweroff();
                        }
                    #endif
                   //delay 300ms poweroff,for sysc poweroff
                   MessageCancelAll(appGetUiTask(), UI_INTERNAL_PEER_SYNC_POWEROFF);
                   MessageSendLater(appGetUiTask(), UI_INTERNAL_PEER_SYNC_POWEROFF, NULL,500);
                }
            } */
            break;
        case UI_INTERNAL_FORBID_POWEROFF:
            DEBUG_LOG("UI_INTERNAL_FORBID_POWEROFF Do nothing");
            break;

        case CHARGER_MESSAGE_CHARGER_COMPLETE:
        case CHARGER_MESSAGE_DETACHED:
            if(id == CHARGER_MESSAGE_CHARGER_COMPLETE)
                DEBUG_LOG("charger complete power off");
            else
                DEBUG_LOG("charger disconnect power off ");

            if(appGetState() == APP_STATE_IN_CASE_DFU){
                DEBUG_LOG("in dfu don't power off");
            }
            else{
                appPowerOffRequest();
                #if defined(HAVE_LP_POWERON)
                    SetPoweroffReason(1);
                #endif
            }
            break;

        case  APP_MFB_HELD_6S:
            DEBUG_LOGF("APP_MFB_HELD_6S");
            #if defined(PEER_FORCE_PAIRING)
            appUiButton();
            #endif
            break;
        case  APP_MFB_HELD_R_6S:
        #if defined(PEER_FORCE_PAIRING)
            DEBUG_LOGF("APP_MFB_HELD_R_6S peer:%d",ui_fpairing_status.peer);
            if(appPeerSyncIsComplete()&&!appSmIsPairing() && !appPeerSyncIsPeerPairing()){
                ui_fpairing_status.self = TRUE;
                MessageCancelAll(appGetUiTask(), UI_INTERNAL_PEER_FORCED_PAIRING_TIMEOUT);
                MessageSendLater(appGetUiTask(), UI_INTERNAL_PEER_FORCED_PAIRING_TIMEOUT, NULL,2000);
                if(ui_fpairing_status.peer && !(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE )){
                   /* forced to pairing status */
                    uiForcedpairingclean();
                    appSmPairHandset();
                }
                else if(appDeviceIsHandsetConnected()){
                    DEBUG_LOG("Local devices is master ,wait peer hold release");
                }
                else{
                    /*Slave or Never conenct handset, send msg to peer device*/
                    appSOCHfpPeerForcedEntryPairing();
                }
            }
            else if(!appDeviceGetPeerBdAddr(&peerAddr) && !appSmIsPairingPeer()){
                /* peer not paired so retry */
                 appSetState(APP_STATE_PEER_PAIRING);
            }
            else if(!appDeviceIsPeerConnected() && appDeviceGetPeerBdAddr(NULL)){
                appPeerSyncSend(FALSE);
            }
        #else
            DEBUG_LOG("APP_MFB_HELD_R_6S");
        #endif
            break;
        #if defined(PEER_FORCE_PAIRING)
        case UI_INTERNAL_PEER_FORCED_PAIRING:
            DEBUG_LOG("UI_INTERNAL_PEER_FORCED_PAIRING");
            ui_fpairing_status.peer = TRUE;
            if(!(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE ) && ui_fpairing_status.self &&appPeerSyncIsComplete()&&!appSmIsPairing() && !appPeerSyncIsPeerPairing()){
                /* forced to pairing status */
                 uiForcedpairingclean();
                 appSmPairHandset();
            }
            else{
                MessageCancelAll(appGetUiTask(), UI_INTERNAL_PEER_FORCED_PAIRING_TIMEOUT);
                MessageSendLater(appGetUiTask(), UI_INTERNAL_PEER_FORCED_PAIRING_TIMEOUT, NULL,2000);
            }
            break;

        case UI_INTERNAL_PEER_FORCED_PAIRING_TIMEOUT:
            DEBUG_LOG("UI_INTERNAL_PEER_FORCED_PAIRING_TIMEOUT");
            uiForcedpairingclean();
            break;
        #endif
        case UI_INTERNAL_FORBID_FACTORY_RESET:
            DEBUG_LOG("UI_INTERNAL_FORBID_FACTORY_RESET");
            forbid_factoryreset = FALSE;
            break;

        case CHARGER_MESSAGE_ATTACHED:
            DEBUG_LOG("UI  CHARGER_MESSAGE_ATTACHED");
            forbid_factoryreset = TRUE;
            MessageCancelAll(appGetUiTask(), UI_INTERNAL_FORBID_FACTORY_RESET);
            MessageSendLater(appGetUiTask(), UI_INTERNAL_FORBID_FACTORY_RESET, NULL,8500);
            appUiAllowEnterDUT();
            break;

        case  APP_MFB_HELD_8S:
            DEBUG_LOG("APP_MFB_HELD_8S");
            if((TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE)&& !forbid_factoryreset ){
                appLedCancelFilter(1);
                appLedSetFilter(app_led_filter_enter_factory_reset, 1);
                theUi->is_factoryReset = TRUE;

            }
            else{
                DEBUG_LOG("APP_MFB_HELD_8S outofcase ,Do nothing");
            }
            break;
        case APP_MFB_HELD_R_8S:
            DEBUG_LOG("APP_MFB_HELD_R_8S");
            if((TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE)){
                #if defined(UI_LP_8S_AND_DOUBLECLICK_FACTORYRESET)
                    // ready for factory reset
                    MessageCancelAll(appGetUiTask(), UI_INTERNAL_ALLOW_FACTORY_RESET);
                    MessageSendLater(appGetUiTask(), UI_INTERNAL_ALLOW_FACTORY_RESET, NULL,D_SEC(10));
                #else
                if(!forbid_factoryreset && theUi->is_factoryReset){
                    #if defined(HAVE_LP_POWERON)
                      SetPoweroffReason(0);
                    #endif
                    theUi->is_factoryReset = FALSE;
                    appSmFactoryReset();
                }
                #endif
            }
            else{
                DEBUG_LOG("APP_MFB_HELD_R_8S outofcase ,Do nothing");
            }
            break;

        case APP_MFB_HELD_12S:
            DEBUG_LOG("APP_MFB_HELD_12S：");
            if(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE){
                if(theUi->is_factoryReset){
                    theUi->is_factoryReset = FALSE;
                    forbid_factoryreset = FALSE;
                    appLedCancelFilter(1);
                    appUiChargerChargingOk();
                }
            }
            break;

        case APP_MFB_HELD_R_12S:
            DEBUG_LOG("APP_MFB_HELD_R_12S");

            break;

        #if defined(UI_LP_8S_AND_DOUBLECLICK_FACTORYRESET)
        case UI_INTERNAL_ALLOW_FACTORY_RESET:
            DEBUG_LOG("UI_INTERNAL_ALLOW_FACTORY_RESET timeout");
            theUi->is_factoryReset = FALSE;
            appLedCancelFilter(1);
            if(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE){
                appUiChargerChargingOk();
            }
            break;
        #endif

#if defined(MEDIA_AUTO_RESUME_PLAY)
        case AV_INTERNAL_MEDIA_PLAY_RESUME:
        	  DEBUG_LOG("AV_INTERNAL_MEDIA_PLAY_RESUME");
        	  g_peer_master_in_case_flag = FALSE;
        	  if (appDeviceIsHandsetAvrcpConnected())
              {
              	 appAvPlayToggle(FALSE);
              }
            else if (appDeviceIsPeerAvrcpConnectedForAv() && appPeerSyncIsComplete() && appPeerSyncIsPeerHandsetAvrcpConnected())
              {
                 appSocHfpFouceSendAvPlayToggle();
              }   
        	  break;
#endif
        case UI_INTERNAL_VOICE_ACTIVE_TIMEOUT:
            theUi->voice_active = FALSE;
            break;

        case UI_INTERNAL_GETVOLTAGE:
        {
            uint16 vol = appBatteryGetVoltage();
            DEBUG_LOGF("get voltage(%d),was_battery_low:%d,count:%d", vol,was_battery_low,battery_low_count);

            if(theUi->is_dut){
                DEBUG_LOG("prompt dut");
                appUiPlayPrompt(PROMPT_DUT);
                break;
            }

            if (!(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE)){
                if(vol <= appConfigBatteryVoltageLow() && !was_battery_low){
                    /*10% battery power remaining */
                    appUiBatteryLow();
                    battery_low_count = 1;
                    was_battery_low = TRUE;
                }
                else if(vol <= (appConfigBatteryVoltageLow()+60) && was_battery_low){
                    battery_low_count++;
                    DEBUG_LOGF("battery_low_count :%d",battery_low_count);
                    if(battery_low_count >= 11){
                        appUiPlayPrompt(PROMPT_POWER_OFF);
                        MessageCancelAll(appGetUiTask(), UI_INTERNAL_NORMAL_DELAY_POWEROFF);
                        MessageSendLater(appGetUiTask(), UI_INTERNAL_NORMAL_DELAY_POWEROFF, NULL,2000);
                    }
                    else if((battery_low_count >= 2) && appDeviceIsHandsetAvrcpConnected() && appPeerSyncIsComplete()&& !appPeerSyncIsPeerInCase()){
                        peerSyncTaskData* ps = appGetPeerSync();

                        DEBUG_LOGF("Slave Bat:%d",ps->sync_battery_level);
                        if(ps->sync_battery_level > 3400 ){
                            appPeerSyncSend(FALSE);
                            appTestInitiateHandover();
                        }
                    }

                }
                else if(was_battery_low && vol >= (appConfigBatteryVoltageLow()+60)){
                    was_battery_low = FALSE;
                    battery_low_count = 0;
                    appLedStopPattern(LED_PRI_EVENT);
                }
            }
            else{
                if(was_battery_low){
                    appLedStopPattern(LED_PRI_EVENT);
                    was_battery_low = FALSE; // in case status
                    battery_low_count = 0;
                }
            }

            if(!appDeviceIsPeerConnected() && appDeviceGetPeerBdAddr(NULL)){
                appPeerSyncSend(FALSE);
            }
            MessageSendLater(appGetUiTask(), UI_INTERNAL_GETVOLTAGE, NULL, UI_GETVOLTAGE_TIMEOUT);
        }
        break;

        case UI_INTERNAL_POWERON_CONNECT_HANDSET:
            DEBUG_LOG("UI_INTERNAL_POWERON_CONNECT_HANDSET");
            /*
             * after poweron for 5S
             * completed TWS paired,current not connect peer,had connect handset,not in DFU or DUT
             */
            if(!appDeviceIsPeerConnected() && appDeviceGetPeerBdAddr(NULL)&&!appDeviceIsHandsetConnected()&& !appSmIsInDfuMode()&&appDeviceGetHandsetBdAddr(NULL)&&!theUi->is_dut \
            	&& !(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE )){
                appSmSingleEarbudPoweronConnectHandset();
                //appSmConnectHandset();
            }
            else{
                DEBUG_LOG("Don't conenct handset");
            }
        break;

        case UI_INTERNAL_NORMAL_DELAY_POWEROFF:
        DEBUG_LOG("recv UI_INTERNAL_NORMAL_DELAY_POWEROFF");
        if(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE ){
            DEBUG_LOG("NORMAL_DELAY_POWEROFF but in case,Do nothing");
        }
        else if(appPowerOffRequest()){
            appGetPower()->shutdown_reson = 1;
            #if defined(HAVE_LP_POWERON)
                SetPoweroffReason (1);
            #endif
        }
        break;

        #if defined(TWS_PEER_SYNC_POWEROFF)
        case UI_INTERNAL_PEER_SYNC_POWEROFF:
            DEBUG_LOG("recv UI_INTERNAL_PEER_SYNC_POWEROFF");
            if(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE ){
                DEBUG_LOG("SYNC_POWEROFF but in case,Do nothing");
            }
            else if(appPowerOffRequest()){
                appGetPower()->shutdown_reson = 1;
                #if defined(HAVE_LP_POWERON)
                    SetPoweroffReason (1);
                #endif
            }
        break;
        #endif

       case UI_INTERNAL_ALLOW_ENTRY_DUT:
            DEBUG_LOG("UI_INTERNAL_ALLOW_ENTRY_DUT timeout");
            break;
       case UI_INTERNAL_DUT_REBOOT_TIMEOUT:
            DEBUG_LOG("UI_INTERNAL_DUT_REBOOT_TIMEOUT");
            appUiSetDUT(FALSE);
            break;
#ifdef HAVE_LTR2668
        case UI_INTERNAL_READ_PS_HOLD:
        	  DEBUG_LOG("UI_INTERNAL_READ_PS_HOLD");
        	  Itr2668ReadPSHold();
        	  MessageCancelAll(task, UI_INTERNAL_READ_PS_HOLD);
              MessageSendLater(task, UI_INTERNAL_READ_PS_HOLD, NULL, 500);
        	  break;
#endif
    }
}

#ifdef HAVE_LTR2668
void ltr2668StartTimerReadPsHold(void)
{
   DEBUG_LOG("ltr2668StartTimerReadPsHold");
   MessageCancelAll(appGetUiTask(), UI_INTERNAL_READ_PS_HOLD);
   MessageSendLater(appGetUiTask(), UI_INTERNAL_READ_PS_HOLD, NULL, 500);
}
#endif
/*! brief Initialise UI module */
void appUiInit(void)
{
    uiTaskData *theUi = appGetUi();
    uint16 sw_version[8];

    /* Set up task handler */
    theUi->task.handler = appUiHandleMessage;
    
    theUi->voice_active = FALSE;
    /* Initialise input event manager with auto-generated tables for
     * the target platform */
    theUi->input_event_task = InputEventManagerInit(appGetUiTask(), InputEventActions,
                                                    sizeof(InputEventActions),
                                                    &InputEventConfig);

    memset(theUi->prompt_file_indexes, FILE_NONE, sizeof(theUi->prompt_file_indexes));

    sw_version[0] = earbudVersion[0] - 0x30;
    sw_version[1] = earbudVersion[2] - 0x30;
    sw_version[2] = earbudVersion[4] - 0x30;
    
    for(int i=0;i<5;i++)
    {
      sw_version[i+3] = earbudProName[i];
    }
    
    PsStore(PS_SW_VERSION_CONFIG, sw_version, 8);

    theUi->prompt_last = PROMPT_NONE;   
    theUi->is_dut = FALSE;
    theUi->is_factoryReset = FALSE;
    theUi->ui_lock = 0;  // APP_LOCK_PROMPT_UI
#if defined(INCLUDE_EQ)
    app_ui_audio_eq_update_need_update();
#endif
}


#if defined(HAVE_LP_POWERON)
bool ReadPoweroffReason(void)
{
    uint16 Reason = 0;
    uint16 Pskey_valid = 0;

    Pskey_valid = PsRetrieve(PS_USER_POWEROFF_CONFIG,&Reason,0);
    if(Pskey_valid){
        PsRetrieve(PS_USER_POWEROFF_CONFIG, &Reason, 1);
        if(Reason == 1){
            /* user power off, power on need long press */
            return TRUE;
        }
        else{
            if(Reason > 1){
                DEBUG_LOG("ReadPoweroffReason error -> init ");
                Reason = 0;
                PsStore(PS_USER_POWEROFF_CONFIG, &Reason, 1);
            }
            return FALSE;
        }

    }
    else{
        DEBUG_LOG("ReadPoweroffReason PSKEY isn't exit, init ");
        Reason = 0;
        PsStore(PS_USER_POWEROFF_CONFIG, &Reason, 1);
    }
    return FALSE;
}

void SetPoweroffReason(uint16 reason)
{
    DEBUG_LOGF("SetPoweroffReason (%d)",reason);
    PsStore(PS_USER_POWEROFF_CONFIG,&reason,sizeof(reason));
}

#endif

#if defined(TWS_PEER_SYNC_POWEROFF)
void appUiPeerSyncPoweroff(void)
{
    if(!(TRUE == appSmIsInCase() || appGetPhyState()->state == PHY_STATE_IN_CASE )&& appChargerCanPowerOff()){
   // appUiPlayPrompt(PROMPT_POWER_OFF);
    appLedSetPattern(app_led_red_pattern_notify, LED_PRI_EVENT);
    }
    MessageCancelAll(appGetUiTask(), UI_INTERNAL_PEER_SYNC_POWEROFF);
    MessageSendLater(appGetUiTask(), UI_INTERNAL_PEER_SYNC_POWEROFF, NULL,1000);
}
#endif

#if defined(CUSTOMER_DUT)
bool appGetCurrentDUTMode(void)
{
    uint16 Reason = 0;
    uint16 Pskey_valid = 0;

    Pskey_valid = PsRetrieve(PS_USER_DUT_MODE,&Reason,0);
    if(Pskey_valid){
        PsRetrieve(PS_USER_DUT_MODE, &Reason, 1);
        if(Reason == 1){
            /* current mode is DUT mode */
            return TRUE;
        }
        else{
            if(Reason > 1){
                DEBUG_LOG("appGetCurrentMode error -> init ");
                Reason = 0;
                PsStore(PS_USER_DUT_MODE, &Reason, 1);
            }
            return FALSE;
        }

    }
    else{
        DEBUG_LOG("ReadPoweroffReason PSKEY isn't exit, init ");
        Reason = 0;
        PsStore(PS_USER_DUT_MODE, &Reason, 1);
    }
    return FALSE;
}

void appSetRebootDUTMode(bool enable)
{
    uint16 Reason = enable;

    DEBUG_LOGF("appSetRebootDUTMode :%d",Reason);
    PsStore(PS_USER_DUT_MODE, &Reason, 1);
}
#endif

#if defined(INCLUDE_EQ)
void appUiUpdateEQ(uint16 mode,bool is_peer_sync)
{
    uiTaskData *theUi = appGetUi();
    DEBUG_LOGF("appUiUpdateEQ current:%d,next:%d,is_peer:%d",theUi->eq_mode,mode,is_peer_sync);
    if(is_peer_sync){
        if(theUi->eq_mode != mode)
            theUi->eq_mode = mode;
    }
    else{
        appScoFwdPeerSyncUserConfig(mode);
    }
/*local update EQ effect */
    appKymeraA2dpUpdateEq();
}

#endif
