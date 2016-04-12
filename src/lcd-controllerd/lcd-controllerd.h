/*
 * lcd-controllerd.h
 *
 *  Created on: Mar 13, 2016
 *      Author: laurent
 */

#ifndef LCD_CONTROLLERD_H_
#define LCD_CONTROLLERD_H_

#include "lcd.h"

#define SOCK_PATH       "/usr/local/share/lcd_socket"
#define MAX_CONNECTIONS 5

typedef enum {
    E_LCD_PROGRESS_BAR_TYPE_BASIC   = 1,
    E_LCD_PROGRESS_BAR_TYPE_NUMBER
} E_LCD_PROGRESS_BAR_TYPE;

typedef struct {
    E_LCD_PROGRESS_BAR_TYPE type;
    lcd_area_t              area;
    uint8_t                 progress;
} lcd_progress_bar_t;

typedef enum {
    E_LCD_MSG_STOP          = 0,
    E_LCD_MSG_PRINT         = 1,
    E_LCD_MSG_ON            = 2,
    E_LCD_MSG_OFF           = 3,
    E_LCD_MSG_PROGRESS_BAR  = 4,
    E_LCD_MSG_TEXT          = 5,
    E_LCD_MSG_NUMBER
} E_LCD_MSG;

typedef struct {
    E_LCD_MSG   cmd;
    union Data {
        void                *empty;
        lcd_buf_t           buff;
        lcd_progress_bar_t  progress_bar;
        lcd_text_t          text;
    } data;
} lcd_msg_t;

#endif /* LCD_CONTROLLERD_H_ */
