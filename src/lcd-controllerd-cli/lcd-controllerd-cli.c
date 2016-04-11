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

int load_bmp(lcd_buf_t *data, char *bmp)
{
    int                 ret = 0;
    char                header[3];
    int                 offset;
    int                 size;
    char                reserved[4];
    FILE                *f;

    memset(header, 0, sizeof(header));

    f = fopen(bmp, "rb");
    if (f == NULL)
    {
        printf("Can't open file '%s'\n", bmp);
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
        ret = -1;
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
            ret = -1;
        }
        else
        {
            fseek(f, offset, SEEK_SET);

            fread(data->px, sizeof(lcd_buf_t), 1, f);
        }
    }

    fclose(f);

    return ret;
}

void print_usage() {
    printf("lcd-controllerd-cli: Usage: \n");
    printf("\t -O : LCD ON\n");
    printf("\t -o : LCD OFF\n");
    printf("\t -p <bmp file> : LCD PRINT\n");
    printf("\t -t <text> : LCD PRINT text\n");
    printf("\t -s : STOP LCD daemon\n");
}

int main(int argc, char *argv[]) {
    int                 option = 0;
    int                 i = 0;
    char                bmp[100];
    char                txt[512];
    uint8_t             actions[E_LCD_MSG_NUMBER];
    int                 ret = 0;
    int                 client;
    int                 client_size;
    struct sockaddr_un  remote;
    lcd_msg_t           msg;

    for (i = 0; i < E_LCD_MSG_NUMBER; i++)
    {
        actions[i] = 0;
    }

    while ((option = getopt(argc, argv,"oOsp:t:")) != -1) {
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
                        strcpy(msg.data.text.text, txt);
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
                }
            }
        }
    }

    close(client);

    return ret;
}
