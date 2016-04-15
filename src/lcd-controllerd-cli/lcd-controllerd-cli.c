/*
 * lcd-controllerd-cli.c
 *
 *  Created on: Mar 8, 2016
 *      Author: laurent
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <getopt.h>

#include "lcd-controllerd.h"

#pragma pack(1)
typedef struct {
        char        header[2];
        uint32_t    size;
        uint8_t     reserved[4];
        uint32_t    offset;
} lcd_bmp_file_hdr_t;

#pragma pack(1)
typedef struct {
        uint8_t    r;
        uint8_t    g;
        uint8_t    b;
        uint8_t    reserved;
} lcd_bmp_col_scheme_t;

#pragma pack(1)
typedef struct {
        uint32_t    header_size;
        uint32_t    width;
        uint32_t    height;
        uint16_t    plane;
        uint16_t    bit_counts;
        uint32_t    compression;
        uint32_t    picture_size;
        uint32_t    res_h;
        uint32_t    res_v;
        uint32_t    colors;
        uint32_t    important;

} lcd_bmp_pic_hdr_t;

#pragma pack(1)
typedef struct {
        lcd_bmp_file_hdr_t      file_header;
        lcd_bmp_pic_hdr_t       picture_header;
        lcd_bmp_col_scheme_t    color_scheme[2];
} lcd_bmp_t;

int save_bmp(lcd_buf_t *data, char *file)
{
    int             ret = 0;
    lcd_bmp_t       bmp;
    FILE            *f;

    bmp.file_header.header[0] = 'B';
    bmp.file_header.header[1] = 'M';

    memset(bmp.file_header.reserved, 0xFF, sizeof(bmp.file_header.reserved));
    bmp.file_header.offset = sizeof(bmp.file_header) +
            sizeof(bmp.picture_header) +
            sizeof(bmp.color_scheme);
    bmp.file_header.size = sizeof(lcd_buf_t) + bmp.file_header.offset;

    bmp.picture_header.header_size = sizeof(bmp.picture_header);
    bmp.picture_header.picture_size = sizeof(lcd_buf_t);
    bmp.picture_header.colors = 2;
    bmp.picture_header.compression = 0;
    bmp.picture_header.width = LCD_PX_WIDTH;
    bmp.picture_header.height = LCD_PX_HEIGHT;
    bmp.picture_header.plane = 1;
    bmp.picture_header.bit_counts = 1;
    bmp.picture_header.res_h = 100;
    bmp.picture_header.res_v = 100;
    bmp.picture_header.important = 0;

    bmp.color_scheme[0].r = 0xFF;
    bmp.color_scheme[0].g = 0xFF;
    bmp.color_scheme[0].b = 0xFF;

    bmp.color_scheme[1].r = 0;
    bmp.color_scheme[1].g = 0;
    bmp.color_scheme[1].b = 0;

    f = fopen(file, "wb+");
    if (f == NULL)
    {
        printf("lcd-controllerd-cli: Can't open file '%s'\n", file);
        return -1;
    }

    fwrite(&bmp, sizeof(bmp), 1, f);
    fwrite(data->px, sizeof(lcd_buf_t), 1, f);

    fclose(f);

    return ret;
}

int load_bmp(lcd_buf_t *data, char *file)
{
    int         ret = 0;
    lcd_bmp_t   bmp;
    FILE        *f;

    f = fopen(file, "rb");
    if (f == NULL)
    {
        printf("lcd-controllerd-cli: Can't open file '%s'\n", file);
        return -1;
    }

    fread(&bmp.file_header, sizeof(bmp.file_header), 1, f);
    fread(&bmp.picture_header, sizeof(bmp.picture_header), 1, f);

    if ((bmp.picture_header.width != 128) ||
            (bmp.picture_header.height != 64) ||
            (bmp.picture_header.colors != 2) ||
            (bmp.picture_header.compression != 0) ||
            (bmp.picture_header.bit_counts != 1) ||
            (bmp.picture_header.picture_size != ((LCD_PX_HEIGHT * LCD_PX_WIDTH) / 8)))
    {
        printf("lcd-controllerd-cli: Invalid BMP file.\n");
        ret = -1;
    }
    else
    {
        fseek(f, bmp.file_header.offset, SEEK_SET);
        fread(data->px, sizeof(lcd_buf_t), 1, f);
    }

    fclose(f);

    return ret;
}

void print_usage() {
    printf("lcd-controllerd-cli Usage: \n");
    printf("\t -O : Turn On the LCD\n");
    printf("\t -o : Turn Off the LCD\n");
    printf("\t -p <bmp file> : Print given bmp file on LCD\n");
    printf("\t -t <text> : Print given text string on LCD\n");
    printf("\t -s : Stop the lcd-controllerd daemon\n");
    printf("\t -c : Clear LCD\n");
    printf("\t -r <bmp file> : Store LCD screen to the given bmp file\n");
    printf("\t -h : Display this help message\n");
}

int main(int argc, char *argv[]) {
    int                 option = 0;
    int                 i = 0;
    int                 n = 0;
    char                bmp[100];
    char                svg_bmp[100];
    char                txt[512];
    uint8_t             actions[E_LCD_MSG_NUMBER];
    int                 ret = 0;
    int                 client;
    int                 client_size;
    struct sockaddr_un  remote;
    lcd_msg_t           msg;
    uint8_t             wait_rsp;

    for (i = 0; i < E_LCD_MSG_NUMBER; i++)
    {
        actions[i] = 0;
    }

    while ((option = getopt(argc, argv,"oOschp:t:r:")) != -1) {
        switch (option) {
            case 'o' :
                actions[E_LCD_MSG_OFF] = 1;
                break;
            case 'O' :
                actions[E_LCD_MSG_ON] = 1;
                break;
            case 's' :
                actions[E_LCD_MSG_STOP] = 1;
                break;
            case 'p' :
                actions[E_LCD_MSG_PRINT] = 1;
                strcpy(bmp,optarg);
                break;
            case 't' :
                actions[E_LCD_MSG_TEXT] = 1;
                strcpy(txt,optarg);
                break;
            case 'c' :
                actions[E_LCD_MSG_CLEAR] = 1;
                break;
            case 'r' :
                actions[E_LCD_MSG_READ_REQ] = 1;
                strcpy(svg_bmp,optarg);
                break;
            case 'h':
                print_usage();
                break;
            default:
                print_usage();
                return -1;
        }
    }

    if ((client = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        printf("lcd-controllerd-cli: Can't create socket\n");
        return -1;
    }

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);

    if (connect(client, (struct sockaddr *)&remote, strlen(remote.sun_path) + sizeof(remote.sun_family)) == -1)
    {
        printf("lcd-controllerd-cli: Can't connect to remote socket\n");
        ret = -1;
    }
    else
    {
        for (i = 0; i < E_LCD_MSG_NUMBER; i++)
        {
            if (actions[i] == 1)
            {
                wait_rsp = 0;
                switch (i)
                {
                    case E_LCD_MSG_OFF:
                        msg.cmd = E_LCD_MSG_OFF;
                        msg.data.empty = NULL;
                        break;
                    case E_LCD_MSG_ON:
                        msg.cmd = E_LCD_MSG_ON;
                        msg.data.empty = NULL;
                        break;
                    case E_LCD_MSG_STOP:
                        msg.cmd = E_LCD_MSG_STOP;
                        msg.data.empty = NULL;
                        break;
                    case E_LCD_MSG_PRINT:
                        msg.cmd = E_LCD_MSG_PRINT;
                        if (load_bmp(&msg.data.buff, bmp) == -1)
                        {
                            printf("lcd-controllerd-cli: Can't read BMP file (%s)\n", bmp);
                            ret = -1;
                        }
                        break;
                    case E_LCD_MSG_TEXT:
                        msg.cmd = E_LCD_MSG_TEXT;
                        msg.data.text.area.start.row = 0;
                        msg.data.text.area.start.line = 0;
                        msg.data.text.area.stop.row = (LCD_PX_SIZE / 8 - 1);
                        msg.data.text.area.stop.line = (LCD_PX_WIDTH - 1);
                        strcpy(msg.data.text.text, txt);
                        break;
                    case E_LCD_MSG_CLEAR:
                        msg.cmd = E_LCD_MSG_CLEAR;
                        msg.data.empty = NULL;
                        break;
                    case E_LCD_MSG_READ_REQ:
                        msg.cmd = E_LCD_MSG_READ_REQ;
                        msg.data.empty = NULL;
                        wait_rsp = 1;
                        break;
                    default:
                        printf("lcd-controllerd-cli: Invalid message index (%d)\n", i);
                        break;
                }

                if (ret == 0)
                {
                    if (send(client, &msg, sizeof(msg), 0) == -1)
                    {
                        printf("lcd-controllerd-cli: Can't send message through socket\n");
                        ret = -1;
                    }

                    if (wait_rsp)
                    {
                        n = recv(client, &msg, sizeof(msg), 0);
                        if (n <= 0)
                        {
                            printf("lcd-controllerd-cli: Read error from socket\n");
                        }
                        else
                        {
                            switch(msg.cmd)
                            {
                                case E_LCD_MSG_READ_RSP:
                                    save_bmp(&msg.data.buff, svg_bmp);
                                    break;
                                default:
                                    printf("lcd-controllerd-cli: Invalid response from socket\n");
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }

    close(client);

    return ret;
}
