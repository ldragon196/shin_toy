/*
 *  audio.hpp
 *
 *  Created on: Aug 24, 2025
 */

#ifndef __AUDIO_HPP_
#define __AUDIO_HPP_

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

/* WAV file header structure */
struct __attribute__((packed)) wav_header_t {
    char RIFF[4];
    uint32_t chunk_size;
    char WAVEfmt[8];
    uint32_t fmt_chunk_size;
    uint16_t audiofmt;
    uint16_t channel;
    uint32_t sample_rate;
    uint32_t byte_per_sec;
    uint16_t block_size;
    uint16_t bit_per_sample;
};

/* WAV subchunk structure */
struct __attribute__((packed)) sub_chunk_t {
    char identifier[4];
    uint32_t chunk_size;
    uint8_t data[1];
};

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/



/******************************************************************************/
/*                              EXPORTED DATA                                 */
/******************************************************************************/



/******************************************************************************/
/*                                FUNCTIONS                                   */
/******************************************************************************/

/*!
 * @brief  Play splash audio
 * @param  None
 * @retval None
 */
void audio_play_splash(void);

/*!
 * @brief  Play smile mode audio
 * @param  Playing
 * @retval None
 */
void audio_set_smile_mode(bool playing);

/*!
 * @brief  Set running state
 * @param  Running
 * @retval None
 */
void audio_set_running(bool running);

/*!
 * @brief  Request play next track
 * @param  None
 * @retval None
 */
void audio_next_request(void);

/*!
 * @brief  Request play prev track
 * @param  None
 * @retval None
 */
void audio_prev_request(void);

/*!
 * @brief  Initialize audio process
 * @param  None
 * @retval None
 */
void audio_init(void);

/******************************************************************************/

#endif /* __AUDIO_HPP_ */