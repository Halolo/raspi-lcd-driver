#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "lcd.h"
#include "font.h"

#define BCM2708_PERI_BASE   0x20000000
#define GPIO_BASE           (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

#define BLOCK_SIZE          (4 * 1024)

#define LCD_RESET_X         0xB8
#define LCD_RESET_Y         0x40
#define LCD_RESET_Z         0xC0

#define LCD_ON              0x3F
#define LCD_OFF             0x3E

#define LCD_STATUS_READY    0x00

struct lcd_hdl_t{
        volatile unsigned int   *gpio;
};

typedef enum
{
    E_CHIP_1        = 0,
    E_CHIP_2        = 1,
    E_CHIP_NUMBER
} E_CHIP;

typedef enum
{
    E_GPIO_RS   = 23,
    E_GPIO_RW   = 24,
    E_GPIO_E    = 11,
    E_GPIO_RST  = 9,
    E_GPIO_CS1  = 10,
    E_GPIO_CS2  = 22,
    E_GPIO_DB0  = 18,
    E_GPIO_DB1  = 15,
    E_GPIO_DB2  = 14,
    E_GPIO_DB3  = 2,
    E_GPIO_DB4  = 3,
    E_GPIO_DB5  = 4,
    E_GPIO_DB6  = 17,
    E_GPIO_DB7  = 27
} E_GPIO;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(a,g) *(a + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
#define OUT_GPIO(a,g) *(a + ((g) / 10)) |=  (1 << (((g) % 10) * 3))

#define GPIO_SET(a) *(a + 7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR(a) *(a + 10) // clears bits which are 1 ignores bits which are 0
#define GPIO_LEV(a) *(a + 13) // pin level

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Local functions

void db_out(struct lcd_hdl_t *lcd_hdl)
{
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB0);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_DB0);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB1);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_DB1);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB2);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_DB2);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB3);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_DB3);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB4);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_DB4);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB5);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_DB5);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB6);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_DB6);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB7);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_DB7);

    msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

    return;
}

void db_in(struct lcd_hdl_t *lcd_hdl)
{
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB0);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB1);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB2);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB3);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB4);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB5);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB6);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_DB7);

    msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

    return;
}

void lcd_pulse(struct lcd_hdl_t *lcd_hdl)
{
    usleep(5);
    GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_E;
    msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);
    usleep(5);
    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_E;
    msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);
    usleep(5);

    return;
}

void lcd_fill_data(struct lcd_hdl_t *lcd_hdl, uint8_t data)
{
    if ((data & 0x01) == 0)
    {
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_DB0;
    }
    else
    {
        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_DB0;
    }

    if ((data & 0x02) == 0)
    {
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_DB1;
    }
    else
    {
        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_DB1;
    }

    if ((data & 0x04) == 0)
    {
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_DB2;
    }
    else
    {
        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_DB2;
    }

    if ((data & 0x08) == 0)
    {
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_DB3;
    }
    else
    {
        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_DB3;
    }

    if ((data & 0x10) == 0)
    {
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_DB4;
    }
    else
    {
        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_DB4;
    }

    if ((data & 0x20) == 0)
    {
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_DB5;
    }
    else
    {
        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_DB5;
    }

    if ((data & 0x40) == 0)
    {
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_DB6;
    }
    else
    {
        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_DB6;
    }

    if ((data & 0x80) == 0)
    {
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_DB7;
    }
    else
    {
        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_DB7;
    }

    msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

    return;
}

void lcd_read_data(struct lcd_hdl_t *lcd_hdl, uint8_t *data)
{
    uint8_t bit[8];
    int     i;

    *data = 0;

    bit[7] = ((GPIO_LEV(lcd_hdl->gpio) & (1 << E_GPIO_DB7)) != 0);
    bit[6] = ((GPIO_LEV(lcd_hdl->gpio) & (1 << E_GPIO_DB6)) != 0);
    bit[5] = ((GPIO_LEV(lcd_hdl->gpio) & (1 << E_GPIO_DB5)) != 0);
    bit[4] = ((GPIO_LEV(lcd_hdl->gpio) & (1 << E_GPIO_DB4)) != 0);
    bit[3] = ((GPIO_LEV(lcd_hdl->gpio) & (1 << E_GPIO_DB3)) != 0);
    bit[2] = ((GPIO_LEV(lcd_hdl->gpio) & (1 << E_GPIO_DB2)) != 0);
    bit[1] = ((GPIO_LEV(lcd_hdl->gpio) & (1 << E_GPIO_DB1)) != 0);
    bit[0] = ((GPIO_LEV(lcd_hdl->gpio) & (1 << E_GPIO_DB0)) != 0);

    for (i = 0; i < sizeof(bit); i++)
    {
        if (bit[i] == 0)
        {
            *data &= ~ (1 << i);
        }
        else
        {
            *data |= (1 << i);
        }
    }

    return;
}

int lcd_read_status(struct lcd_hdl_t *lcd_hdl, E_CHIP chip, uint8_t expected)
{
    uint8_t     status = 0;
    uint16_t    timeout_ms = 1000;
    time_t      current;
    time_t      start = time(NULL);

    do
    {
        db_in(lcd_hdl);

        if (chip == E_CHIP_1)
        {
            GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
            GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS2;
        }
        else
        {
            GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
            GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_CS2;
        }

        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_RW;
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RS;

        msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

        usleep(5);
        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_E;
        msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);
        usleep(5);

        lcd_read_data(lcd_hdl, &status);

        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RW;
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_E;
        msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);
        current = time(NULL);
    } while ((status != expected) && (current > (start + timeout_ms)));

    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS2;

    msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

    if (current > start + timeout_ms)
    {
        printf("lcd_read_status: Can't read status, timeout!\n");
        return -1;
    }
    else {
        return 0;
    }
}

int lcd_instruction(struct lcd_hdl_t *lcd_hdl, E_CHIP chip, uint8_t instruction)
{
    int ret = 0;

    db_out(lcd_hdl);

    if (chip == E_CHIP_1)
    {
        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS2;
    }
    else
    {
        GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
        GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_CS2;
    }

    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RW;
    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RS;

    msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

    lcd_fill_data(lcd_hdl, instruction);

    lcd_pulse(lcd_hdl);

    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS2;

    msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

    if (instruction != LCD_OFF)
    {
        ret = lcd_read_status(lcd_hdl, chip, LCD_STATUS_READY);
        if (ret == -1)
        {
            printf("lcd_instruction: Can't perform instruction '0x%.2X'\n", instruction);
        }
    }

    return ret;
}

// Interface functions

void *lcd_connect()
{
    int                 mem_fd;
    struct lcd_hdl_t    *lcd_hdl;

    pthread_mutex_lock(&mutex);

    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
    {
        printf("lcd_connect: can't open /dev/mem (%d)\n", mem_fd);
        return NULL;
    }

    lcd_hdl = (struct lcd_hdl_t *)malloc(sizeof(struct lcd_hdl_t));

    /* mmap GPIO */
    lcd_hdl->gpio = mmap(
            NULL,
            BLOCK_SIZE,
            PROT_READ|PROT_WRITE,
            MAP_SHARED,
            mem_fd,
            GPIO_BASE
    );

    close(mem_fd);

    pthread_mutex_unlock(&mutex);

    if (lcd_hdl->gpio == MAP_FAILED)
    {
        printf("lcd_connect: mmap error\n");
        return NULL;
    }

    return (void *)lcd_hdl;
}

void lcd_disconnect(struct lcd_hdl_t *lcd_hdl)
{
    pthread_mutex_lock(&mutex);

    munmap((void *)lcd_hdl->gpio, BLOCK_SIZE);

    free(lcd_hdl);

    pthread_mutex_unlock(&mutex);

    return;
}

void lcd_init(struct lcd_hdl_t *lcd_hdl)
{
    pthread_mutex_lock(&mutex);

    INP_GPIO(lcd_hdl->gpio, E_GPIO_RW);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_RW);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_RS);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_RS);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_E);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_E);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_RST);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_RST);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_CS1);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_CS1);
    INP_GPIO(lcd_hdl->gpio, E_GPIO_CS2);
    OUT_GPIO(lcd_hdl->gpio, E_GPIO_CS2);

    msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

    db_out(lcd_hdl);

    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RS;
    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RW;
    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_E;
    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS2;
    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RST;

    msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

    /* Reset */
    usleep(5);
    GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_RST;

    msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

    /* Switch off */
    lcd_instruction(lcd_hdl, E_CHIP_1, LCD_OFF);
    lcd_instruction(lcd_hdl, E_CHIP_2, LCD_OFF);

    pthread_mutex_unlock(&mutex);

    return;
}

void lcd_print(struct lcd_hdl_t *lcd_hdl, lcd_buf_t *buff)
{
    int     i,j,k;
    int     x,y,z;
    int     offset;
    E_CHIP  chip;
    uint8_t part[LCD_PX_SIZE * (LCD_PX_SIZE / 8)];
    uint8_t bit[8];
    uint8_t seg;

    bit[0] = 0x80;
    bit[1] = 0x40;
    bit[2] = 0x20;
    bit[3] = 0x10;
    bit[4] = 0x08;
    bit[5] = 0x04;
    bit[6] = 0x02;
    bit[7] = 0x01;

    pthread_mutex_lock(&mutex);

    /* Switch off */
    lcd_instruction(lcd_hdl, E_CHIP_1, LCD_OFF);
    lcd_instruction(lcd_hdl, E_CHIP_2, LCD_OFF);

    for (chip = E_CHIP_1; chip < E_CHIP_NUMBER; chip++)
    {
        if (chip == E_CHIP_1)
        {
            offset = LCD_PX_SIZE / 8;
        }
        else
        {
            offset = 0;
        }

        z = 0;
        for (x = 0; x < LCD_PX_SIZE; x++)
        {
            for (y = 0; y < (LCD_PX_SIZE / 8); y++)
            {
                part[z++] = buff->px[y + x * (LCD_PX_WIDTH / 8) + offset];
            }
        }

        lcd_instruction(lcd_hdl, chip, LCD_RESET_X);
        lcd_instruction(lcd_hdl, chip, LCD_RESET_Y);
        lcd_instruction(lcd_hdl, chip, LCD_RESET_Z);

        for (i = 0; i < 8; i++)
        {
            lcd_instruction(lcd_hdl, chip, LCD_RESET_X + i);

            for (j = 0; j < LCD_PX_SIZE; j++)
            {
                db_out(lcd_hdl);

                if (chip == E_CHIP_1)
                {
                    GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
                    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS2;
                }
                else
                {
                    GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
                    GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_CS2;
                }

                GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RW;
                GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_RS;

                msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

                for (k = 0; k < 8; k++)
                {
                    if ((part[(i * LCD_PX_SIZE) + (j / 8) + (8 * k)] & bit[j % 8]) == 0)
                    {
                        seg &= ~ (1 << k);
                    }
                    else
                    {
                        seg |= (1 << k);
                    }
                }

                lcd_fill_data(lcd_hdl, seg);

                lcd_pulse(lcd_hdl);

                GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RS;

                GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
                GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS2;

                msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

                lcd_read_status(lcd_hdl, chip, LCD_STATUS_READY);
            }
        }
    }

    /* Switch on */
    lcd_instruction(lcd_hdl, E_CHIP_1, LCD_ON);
    lcd_instruction(lcd_hdl, E_CHIP_2, LCD_ON);

    pthread_mutex_unlock(&mutex);

    return;
}

void lcd_print_txt(struct lcd_hdl_t *lcd_hdl, lcd_text_t *txt)
{
    uint8_t x,y;
    uint8_t x_stop, y_stop;
    uint8_t chip;
    uint8_t chip_stop;
    uint8_t index;
    uint8_t i,j;

    if ((txt->area.start.row >= (LCD_PX_HEIGHT / 8)) ||
            (txt->area.start.line >= LCD_PX_WIDTH) ||
            (txt->area.stop.row >= (LCD_PX_HEIGHT / 8)) ||
            (txt->area.stop.line >= LCD_PX_WIDTH))
    {
        printf("lcd_print_txt: Invalid parameter, out of range area\n");
        return;
    }

    if ((txt->area.start.row > txt->area.stop.row) ||
            (txt->area.start.line > txt->area.stop.line))
    {
        printf("lcd_print_txt: Invalid parameter, area definition is incoherent\n");
        return;
    }

    x = txt->area.start.row;
    x_stop = txt->area.stop.row;

    if (txt->area.start.line > LCD_PX_SIZE)
    {
        chip = E_CHIP_1;
        y = txt->area.start.line % LCD_PX_SIZE;
    }
    else
    {
        chip = E_CHIP_2;
        y = txt->area.start.line;
    }

    if (txt->area.stop.line > LCD_PX_SIZE)
    {
        chip_stop = E_CHIP_1;
        y_stop = txt->area.stop.line % LCD_PX_SIZE;
    }
    else
    {
        chip_stop = E_CHIP_2;
        y_stop = txt->area.stop.line;
    }

    pthread_mutex_lock(&mutex);

    /* Switch off */
    lcd_instruction(lcd_hdl, E_CHIP_1, LCD_OFF);
    lcd_instruction(lcd_hdl, E_CHIP_2, LCD_OFF);

    lcd_instruction(lcd_hdl, chip, LCD_RESET_Z);

    for (i = 0; i < strlen(txt->text); i++)
    {
        if ((txt->text[i] >= LCD_FONT_MIN) && (txt->text[i] <= LCD_FONT_MAX))
        {
            index = txt->text[i] - LCD_FONT_MIN;
        }
        else
        {
            // Space instead of non-recognized char
            index = 0;
        }

        for (j = 0; j < LCD_FONT_WIDTH; j++)
        {
            if ((chip_stop == chip) && (y == y_stop))
            {
                y = 0;
                if (x == x_stop)
                {
                    x = 0;
                }
                else
                {
                    x = x + 1;
                }
            }

            if (y == (LCD_PX_SIZE - 1))
            {
                if (chip == E_CHIP_2)
                {
                    y = 0;
                    chip = E_CHIP_1;
                }
                else
                {
                    chip = E_CHIP_2;

                }
            }
            else
            {
                y = y + 1;
            }

            lcd_instruction(lcd_hdl, chip, LCD_RESET_X + x);
            lcd_instruction(lcd_hdl, chip, LCD_RESET_Y + y);

            db_out(lcd_hdl);

            if (chip == E_CHIP_1)
            {
                GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
                GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS2;
            }
            else
            {
                GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
                GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_CS2;
            }

            GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RW;
            GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_RS;

            msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

            lcd_fill_data(lcd_hdl, lcd_font[index][j]);

            lcd_pulse(lcd_hdl);

            GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RS;

            GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
            GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS2;

            msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

            lcd_read_status(lcd_hdl, chip, LCD_STATUS_READY);
        }
    }

    /* Switch on */
    lcd_instruction(lcd_hdl, E_CHIP_1, LCD_ON);
    lcd_instruction(lcd_hdl, E_CHIP_2, LCD_ON);

    pthread_mutex_unlock(&mutex);

    return;
}

// TODO: TO BE DONE, Doesn't work
void lcd_read(struct lcd_hdl_t *lcd_hdl, lcd_buf_t *buff)
{
    E_CHIP  chip;
    int     i;
    int     x;
    uint8_t part[(LCD_PX_SIZE / 8) * LCD_PX_SIZE];

    pthread_mutex_lock(&mutex);

    for (chip = E_CHIP_1; chip < E_CHIP_NUMBER; chip++)
    {
        for (i = 0; i < LCD_PX_SIZE; i++)
        {
            db_in(lcd_hdl);

            if (chip == E_CHIP_1)
            {
                GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
                GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS2;
            }
            else
            {
                GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
                GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_CS2;
            }

            GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_RW;
            GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_RS;

            usleep(5);
            GPIO_SET(lcd_hdl->gpio) = 1 << E_GPIO_E;
            msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);
            usleep(5);

            lcd_read_data(lcd_hdl, &part[i]);

            GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_E;
            GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RS;
            GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_RW;

            GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS1;
            GPIO_CLR(lcd_hdl->gpio) = 1 << E_GPIO_CS2;

            msync((void *)lcd_hdl->gpio, BLOCK_SIZE, MS_SYNC);

            lcd_read_status(lcd_hdl, chip, LCD_STATUS_READY);

            for (x = 0; x < LCD_PX_SIZE; x++)
            {
                memcpy(&part[x * (LCD_PX_SIZE / 8)], &buff->px[(x * (LCD_PX_WIDTH / 8)) + chip * (LCD_PX_SIZE / 8)], (LCD_PX_SIZE / 8));
            }
        }
    }

    pthread_mutex_unlock(&mutex);

    return;
}

void lcd_on(struct lcd_hdl_t *lcd_hdl)
{
    pthread_mutex_lock(&mutex);

    lcd_instruction(lcd_hdl, E_CHIP_1, LCD_ON);
    lcd_instruction(lcd_hdl, E_CHIP_2, LCD_ON);

    pthread_mutex_unlock(&mutex);

    return;
}

void lcd_off(struct lcd_hdl_t *lcd_hdl)
{
    pthread_mutex_lock(&mutex);

    lcd_instruction(lcd_hdl, E_CHIP_1, LCD_OFF);
    lcd_instruction(lcd_hdl, E_CHIP_2, LCD_OFF);

    pthread_mutex_unlock(&mutex);

    return;
}
