/*
 * test.c
 *
 *  Created on: Mar 8, 2016
 *      Author: laurent
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lcd.h"

#define BMP_PATH    "/usr/share/test-lcd/linux.bmp"

void test_bmp(lcd_buf_t *data)
{
    char                header[3];
    int                 offset;
    int                 size;
    char                reserved[4];
    FILE                *f;

    memset(header, 0, sizeof(header));

    f = fopen(BMP_PATH, "rb");
    if (f == NULL)
    {
        printf("Can't open file '%s'\n", BMP_PATH);
    }

    fread(&header, (sizeof(header) - 1), 1, f);

    printf("Header = '%s'\n", header);

    if (    (strcmp(header, "BM") != 0) &&
            (strcmp(header, "BA") != 0) &&
            (strcmp(header, "CI") != 0) &&
            (strcmp(header, "CP") != 0) &&
            (strcmp(header, "IC") != 0) &&
            (strcmp(header, "PT") != 0))
    {
        printf("Invalid BMP file.\n");
    }
    else
    {
        fread(&size, sizeof(size), 1, f);
        fread(reserved, sizeof(reserved), 1, f);
        fread(&offset, sizeof(offset), 1, f);

        printf("Size = %d\n", size);
        printf("Offset = %d\n", offset);

        if (size < offset)
        {
            printf("Invalid BMP file.\n");
        }
        else
        {
            fseek(f, offset, SEEK_SET);

            fread(data->px, sizeof(lcd_buf_t), 1, f);

            fclose(f);
        }
    }
}


int main()
{
    struct lcd_hdl_t    *lcd_hdl = NULL;
    int                 ret = 0;
    lcd_buf_t           data;

    memset(data.px, 0, sizeof(data));

    lcd_hdl = lcd_connect();
    if (lcd_hdl == NULL)
    {
        printf("main: can't connect to lcd\n");
        return -1;
    }

    test_bmp(&data);

    lcd_init(lcd_hdl);

    lcd_print(lcd_hdl, &data);

    sleep(10);

    memset(data.px, 0, sizeof(data));

    data.px[0] = 0xF0;
    data.px[15] = 0x0F;
    data.px[1007] = 0xFF;
    data.px[1023] = 0x00;

    lcd_init(lcd_hdl);

    lcd_print(lcd_hdl, &data);

    sleep(10);

    lcd_off(lcd_hdl);

    lcd_disconnect(lcd_hdl);

    fflush(stdout);

    return 0;
}
