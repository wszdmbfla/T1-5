/*!
\copyright  Copyright (c) 2017 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    Part of 6.3.0
\file       av_headset_tap_sensor.h
\brief      Header file for tap sensor support
*/

#ifndef AV_HEADSET_TAP_SENSOR_H
#define AV_HEADSET_TAP_SENSOR_H

#include "av_headset_tasklist.h"
#include "av_headset_message.h"

/*! Enumeration of messages the proximity sensor can send to its clients */
enum tap_sensor_messages
{
    TAP_SENSOR_MESSAGE_DOUBLE_CLICK = PROXIMITY_MESSAGE_BASE+5,
};

/*! Forward declaration of a config structure (type dependent) */
struct __tap_sensor_config;
/*! Proximity config incomplete type */
typedef struct __tap_sensor_config TapSensorConfig;

/*! @brief Proximity module state. */
typedef struct
{
    /*! tap State module message task. */
    TaskData task;
    /*! Handle to the bitserial instance. */
    bitserial_handle handle;
    /*! List of registered client tasks */
    TaskList *clients;
    /*! The config */
    const TapSensorConfig *config;
} TapSensorTaskData;

/*! \brief Register with proximity to receive notifications.
    \param task The task to register.
    \return TRUE if the client was successfully registered.
            FALSE if registration was unsuccessful or if the platform does not
            have a proximity sensor.
    The sensor will be enabled the first time a client registers.
*/
#if defined(INCLUDE_TAP_SENSOR)
extern bool appTapSensorClientRegister(Task task);
#else
#define appTapSensorClientRegister(task) FALSE
#endif

#if defined(INCLUDE_TAP_SENSOR)
extern void appTapSensorClientUnregister(Task task);
#else
#define appTapSensorClientUnregister(task) ((void)task)
#endif

#endif // AV_HEADSET_TAP_SENSOR_H
