/*
 * test.c
 *
 *  Created on: Mar 8, 2016
 *      Author: laurent
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcd.h"

int main(int argc, char **argv)
{
    void                *lcd_hdl = NULL;
    int                 ret = 0;
    char                header[3];
    int                 offset;
    int                 size;
    char                reserved[4];
    lcd_buf_t           data;
    FILE                *f;
    int                 i;

    memset(header, 0, sizeof(header));

    memset(data.buff, 0, sizeof(data));

    lcd_hdl = lcd_connect();
    if (lcd_hdl == NULL)
    {
        printf("main: can't connect to lcd\n");
        return -1;
    }

    if (argc != 2)
    {
        printf("Invalid parameter.\n");
        return -1;
    }

    f = fopen(argv[1], "rb");
    if (f == NULL)
    {
        printf("Can't open file '%s'\n", argv[1]);
        return -1;
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

            fread(data.buff, sizeof(data), 1, f);

            init_lcd(lcd_hdl);

            lcd_print(lcd_hdl, 1, &data);

            for (i = 0; i < sizeof(data); i++)
            {
                data.buff[i] = ~ data.buff[i];
            }

            lcd_print(lcd_hdl, 2, &data);

            sleep(10);

            lcd_off(lcd_hdl);
        }
    }

    fclose(f);

    lcd_disconnect(lcd_hdl);

    fflush(stdout);

    return 0;
}
