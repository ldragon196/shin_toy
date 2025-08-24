/*
 *  lvgl_gui.hpp
 *
 *  Created on: Aug 24, 2025
 */

#ifndef _LVGL_GUI_HPP_
#define _LVGL_GUI_HPP_

/******************************************************************************/

/******************************************************************************/
/*                              INCLUDE FILES                                 */
/******************************************************************************/

#define LVGL_TICK_HANDLER 10

/******************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                         */
/******************************************************************************/



/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/



/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

/**
 * @brief  Set battery percentage
 * @param  Battery level
 * @retval None
 */
void lvgl_set_battery(uint8_t percentage);

/**
 * @brief  Set battery percentage
 * @param  Mode and sub mode
 * @retval None
 */
void lvgl_set_menu_mode(uint8_t mode, uint8_t sub_mode);

/**
 * @brief  Change next image resource
 * @param  None
 * @retval None
 */
void lvgl_change_next_smile(void);

/**
 * @brief  Change image resource
 * @param  None
 * @retval None
 */
void lvgl_change_prev_smile(void);

/**
 * @brief  Set display playing sate
 * @param  Playing
 * @retval None
 */
void lvgl_set_play_state(bool playing);

/**
 * @brief  Set song name
 * @param  Song name
 * @retval None
 */
void lvgl_set_song_name(const char *name);

/**
 * @brief  Initialize LVGL for gui
 * @param  None
 * @retval None
 */
void lvgl_gui_init(void);

/******************************************************************************/

#endif /* _LVGL_GUI_HPP_ */
