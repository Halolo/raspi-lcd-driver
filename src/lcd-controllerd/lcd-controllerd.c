/*
 * lcd-controllerd.c
 *
 *  Created on: Mar 13, 2016
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

int main()
{
    int                 ret = 0;
    struct lcd_hdl_t    *lcd_hdl = NULL;
    int                 server, client;
    int                 remote_len, local_len;
    struct sockaddr_un  local, remote;
    uint8_t             done, stop;
    int                 n;
    lcd_msg_t           msg;

    if ((server = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        printf("lcd-controllerd: Can't create socket\n");
        return -1;
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    local_len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(server, (struct sockaddr *)&local, local_len) == -1)
    {
        printf("lcd-controllerd: Can't bind to socket\n");
        ret = -1;
    }
    else if (listen(server, MAX_CONNECTIONS) == -1)
    {
        printf("lcd-controllerd: Can't listen socket\n");
        ret = -1;
    }
    else
    {
        stop = 0;
        do
        {
            remote_len = sizeof(remote);
            if ((client = accept(server, (struct sockaddr *)&remote, &remote_len)) == -1) {
                printf("lcd-controllerd: Connexion refused\n");
                ret = -1;
                stop = 1;
            }
            else
            {
                lcd_hdl = lcd_connect();
                if (lcd_hdl == NULL)
                {
                    printf("lcd-controllerd: Can't connect to lcd\n");
                }
                else
                {
                    lcd_init(lcd_hdl);

                    done = 0;
                    do
                    {
                        n = recv(client, &msg, sizeof(msg), 0);
                        if (n <= 0)
                        {
                            if (n < 0)
                            {
                                printf("lcd-controllerd: Read error from socket\n");
                            }
                            done = 1;
                        }

                        switch (msg.cmd)
                        {
                            case E_LCD_MSG_ON:
                                lcd_on(lcd_hdl);
                                break;
                            case E_LCD_MSG_OFF:
                                lcd_off(lcd_hdl);
                                break;
                            case E_LCD_MSG_PRINT:
                                lcd_print(lcd_hdl, &msg.data.buff);
                                break;
                            case E_LCD_MSG_STOP:
                                stop = 1;
                                break;
                            default:
                                printf("lcd-controllerd: Invalid command: %d\n", msg.cmd);
                                break;
                        }
                    } while (! done);

                    lcd_off(lcd_hdl);

                    lcd_disconnect(lcd_hdl);
                }

                close(client);
            }
        } while (! stop);
    }

    close(server);

    return ret;
}
