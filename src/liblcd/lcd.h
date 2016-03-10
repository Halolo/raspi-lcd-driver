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

void *lcd_connect();
void lcd_disconnect(void * lcd_hdl);

void init_lcd(void *lcd_hdl);

void lcd_print(void *lcd_hdl, int chip, lcd_buf_t *buff);
void lcd_read(void *lcd_hdl, int chip, lcd_buf_t *buff);

void lcd_on(void * lcd_hdl);
void lcd_off(void * lcd_hdl);

#endif /* LCD_H_ */
