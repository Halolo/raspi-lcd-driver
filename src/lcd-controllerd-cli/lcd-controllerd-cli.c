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

#include "lcd-controllerd.h"

#define BMP_PATH ""

void load_bmp(lcd_buf_t *data)
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
    int                 ret;
    int                 client;
    int                 client_size;
    struct sockaddr_un  remote;
    lcd_msg_t           msg;

    if ((client = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        printf("lcd-controllerd-cli: Can't create socket\n");
        return -1;
    }

    printf("lcd-controllerd-cli: Trying to connect...\n");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);

    if (connect(client, (struct sockaddr *)&remote, strlen(remote.sun_path) + sizeof(remote.sun_family)) == -1)
    {
        printf("lcd-controllerd-cli: Can't connect to remote socket\n");
        return -1;
    }

    printf("lcd-controllerd-cli: Connected\n");

    msg.cmd = E_LCD_MSG_STOP;
    msg.data.empty = NULL;

    if (send(client, &msg, sizeof(msg), 0) == -1)
    {
        printf("lcd-controllerd-cli: Can't send message through socket\n");
        ret = -1;
    }

    close(client);

    return ret;
}
