/*
 * lcd-controllerd.h
 *
 *  Created on: Mar 13, 2016
 *      Author: laurent
 */

#ifndef LCD_CONTROLLERD_H_
#define LCD_CONTROLLERD_H_

#include "lcd.h"

typedef enum {
    E_LCD_MSG_STOP  = 0,
    E_LCD_MSG_PRINT = 1,
    E_LCD_MSG_ON    = 2,
    E_LCD_MSG_OFF   = 3,
    E_LCD_MSG_NUMBER
} E_LCD_MSG;

typedef struct {
    E_LCD_MSG   cmd;
    union Data {
        void        *empty;
        lcd_buf_t   buff;
    } data;
} lcd_msg_t;

#endif /* LCD_CONTROLLERD_H_ */
