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

typedef enum {
    E_LCD_TEXT_TYPE_BASIC       = 1,
    E_LCD_TEXT_TYPE_SCROLL_LINE = 2,
    E_LCD_TEXT_TYPE_NUMBER
} E_LCD_TEXT_TYPE;

typedef struct {
    E_LCD_TEXT_TYPE type;
    lcd_area_t      area;
    char            text[512];
} lcd_text_t;

typedef struct lcd_buf_t { uint8_t px[LCD_PX_HEIGHT * LCD_PX_WIDTH / 8]; } lcd_buf_t;
struct lcd_hdl_t;

void *lcd_connect();
void lcd_disconnect(struct lcd_hdl_t *lcd_hdl);

void lcd_init(struct lcd_hdl_t *lcd_hdl);

void lcd_print(struct lcd_hdl_t *lcd_hdl, lcd_buf_t *buff);
void lcd_print_txt(struct lcd_hdl_t *lcd_hdl, lcd_text_t *txt);

void lcd_read(struct lcd_hdl_t *lcd_hdl, lcd_buf_t *buff);

void lcd_on(struct lcd_hdl_t *lcd_hdl);
void lcd_off(struct lcd_hdl_t *lcd_hdl);

#endif /* LCD_H_ */
