/*
 * lcd.h
 *
 *  Created on: Apr 26, 2014
 *      Author: laurent
 */

#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>

#define LCD_PX_SIZE     64
#define LCD_PX_HEIGHT   LCD_PX_SIZE
#define LCD_PX_WIDTH    LCD_PX_SIZE * 2

typedef struct {
    uint8_t row;
    uint8_t line;
} lcd_point_t;

typedef struct {
    lcd_point_t start;
    lcd_point_t stop;
} lcd_area_t;

typedef struct lcd_buf_t { uint8_t px[LCD_PX_HEIGHT * LCD_PX_WIDTH / 8]; } lcd_buf_t;
struct lcd_hdl_t;

void *lcd_connect();
void lcd_disconnect(struct lcd_hdl_t *lcd_hdl);

void lcd_init(struct lcd_hdl_t *lcd_hdl);

void lcd_print(struct lcd_hdl_t *lcd_hdl, lcd_buf_t *buff);
void lcd_print_txt(struct lcd_hdl_t *lcd_hdl, uint8_t row, uint8_t line, char *txt, uint8_t len);
/*
 * TODO: TO BE DONE, Doesn't work
void lcd_read(struct lcd_hdl_t *lcd_hdl, lcd_buf_t *buff);*/

void lcd_on(struct lcd_hdl_t *lcd_hdl);
void lcd_off(struct lcd_hdl_t *lcd_hdl);

#endif /* LCD_H_ */
