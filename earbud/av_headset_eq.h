#ifndef _AV_HEADSET_EQ_H_
#define _AV_HEADSET_EQ_H_

#include "av_headset.h"

/* After OTA ,app update EQ data timers */
#define PS_EQ_VERIFY_TIMES_MAX   5

#define APP_EQ_MODE_MAX     5

#define UCID_PS_EQ_SHIFT                           (1)
#define CAPID_PS_EQ_SHIFT                          (7)
#define CAPABILITY_ID_EQ               (0x0049)
#define AUDIO_PS_EQ(ucid)               ((CAPABILITY_ID_EQ << CAPID_PS_EQ_SHIFT) | (ucid << UCID_PS_EQ_SHIFT))
#define AUDIO_EQ_MODE0_KEY  (AUDIO_PS_EQ(0))
#define AUDIO_EQ_MODE1_KEY  (AUDIO_PS_EQ(1))
#define AUDIO_EQ_MODE2_KEY  (AUDIO_PS_EQ(2))
#define AUDIO_EQ_MODE3_KEY  (AUDIO_PS_EQ(3))
#define AUDIO_EQ_MODE4_KEY  (AUDIO_PS_EQ(4))

#define AUDIO_EQ_DATA_LEN   91
#define AUDIO_EQ_WRITE_ARRAY_LEN   182

bool appEQupdateVerifyPStore(void);

bool app_ui_audio_eq_update_need_update(void);

void lhtdebugfunc(void);
#endif
