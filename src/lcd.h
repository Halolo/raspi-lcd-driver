/*
 * lcd.h
 *
 *  Created on: Apr 26, 2014
 *      Author: laurent
 */

#ifndef LCD_H_
#define LCD_H_

#define LCD_PX_SIZE	64

typedef struct lcd_buf_t { char buff[(LCD_PX_SIZE / 8) * LCD_PX_SIZE]; } lcd_buf_t;

#endif /* LCD_H_ */
