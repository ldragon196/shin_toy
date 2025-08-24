/*
 *  app_config.hpp
 *
 *  Created on: Aug 24, 2025
 */

#ifndef __APP_CONFIG_HPP_
#define __APP_CONFIG_HPP_

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdint.h>

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/

#define HOLDING_TIME_MS 1000
#define CHANGE_VOL_INTERVAL_MS 50
#define HOLDING_BACK_TIME_MS 1000
#define MAGIC_NUMBER 0xAA55A55A

enum {
    SCREEN_HOME = 0,
    SCREEN_PLAY_MUSIC,
    SCREEN_SMILE,
};

enum {
    HOME_PLAY_MUSIC = 0,
    HOME_SMILE,
    HOME_SUB_COUNT,
};

typedef struct {
    uint32_t magic;
    uint8_t volume;
    uint16_t play_index;
} system_config_t;

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/



/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/

extern system_config_t system_config;

/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

/*!
 * @brief  Save configuration to EEPROM
 * @param  None
 * @retval None
 */
void save_configuration(void);

/******************************************************************************/

#endif /* __APP_CONFIG_HPP_ */