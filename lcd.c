#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define BCM2708_PERI_BASE   0x20000000
#define GPIO_BASE           (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

#define BLOCK_SIZE          (4 * 1024)

#define LCD_RESET_X         0xB8
#define LCD_RESET_Y         0x40
#define LCD_RESET_Z         0xC0

#define LCD_ON              0x3F
#define LCD_OFF             0x3E

#define LCD_STATUS_READY    0x00

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
#define INP_GPIO(gpio,g) *(gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
#define OUT_GPIO(gpio,g) *(gpio + ((g) / 10)) |=  (1 << (((g) % 10) * 3))

#define GPIO_SET(gpio) *(gpio + 7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR(gpio) *(gpio + 10) // clears bits which are 1 ignores bits which are 0
#define GPIO_LEV(gpio) *(gpio + 13) // pin level

void * setup_io();

void db_out(volatile unsigned int * gpio)
{
    INP_GPIO(gpio, E_GPIO_DB0);
    OUT_GPIO(gpio, E_GPIO_DB0);
    INP_GPIO(gpio, E_GPIO_DB1);
    OUT_GPIO(gpio, E_GPIO_DB1);
    INP_GPIO(gpio, E_GPIO_DB2);
    OUT_GPIO(gpio, E_GPIO_DB2);
    INP_GPIO(gpio, E_GPIO_DB3);
    OUT_GPIO(gpio, E_GPIO_DB3);
    INP_GPIO(gpio, E_GPIO_DB4);
    OUT_GPIO(gpio, E_GPIO_DB4);
    INP_GPIO(gpio, E_GPIO_DB5);
    OUT_GPIO(gpio, E_GPIO_DB5);
    INP_GPIO(gpio, E_GPIO_DB6);
    OUT_GPIO(gpio, E_GPIO_DB6);
    INP_GPIO(gpio, E_GPIO_DB7);
    OUT_GPIO(gpio, E_GPIO_DB7);

    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

    return;
}

void db_in(volatile unsigned int * gpio)
{
    INP_GPIO(gpio, E_GPIO_DB0);
    INP_GPIO(gpio, E_GPIO_DB1);
    INP_GPIO(gpio, E_GPIO_DB2);
    INP_GPIO(gpio, E_GPIO_DB3);
    INP_GPIO(gpio, E_GPIO_DB4);
    INP_GPIO(gpio, E_GPIO_DB5);
    INP_GPIO(gpio, E_GPIO_DB6);
    INP_GPIO(gpio, E_GPIO_DB7);

    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

    return;
}

void lcd_pulse(volatile unsigned int * gpio)
{
    usleep(5);
    GPIO_SET(gpio) = 1 << E_GPIO_E;
    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);
    usleep(5);
    GPIO_CLR(gpio) = 1 << E_GPIO_E;
    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);
    usleep(5);

    return;
}

void lcd_fill_data(volatile unsigned int * gpio, char data)
{
    if ((data & 0x01) == 0)
    {
        GPIO_CLR(gpio) = 1 << E_GPIO_DB0;
    }
    else
    {
        GPIO_SET(gpio) = 1 << E_GPIO_DB0;
    }

    if ((data & 0x02) == 0)
    {
        GPIO_CLR(gpio) = 1 << E_GPIO_DB1;
    }
    else
    {
        GPIO_SET(gpio) = 1 << E_GPIO_DB1;
    }

    if ((data & 0x04) == 0)
    {
        GPIO_CLR(gpio) = 1 << E_GPIO_DB2;
    }
    else
    {
        GPIO_SET(gpio) = 1 << E_GPIO_DB2;
    }

    if ((data & 0x08) == 0)
    {
        GPIO_CLR(gpio) = 1 << E_GPIO_DB3;
    }
    else
    {
        GPIO_SET(gpio) = 1 << E_GPIO_DB3;
    }

    if ((data & 0x10) == 0)
    {
        GPIO_CLR(gpio) = 1 << E_GPIO_DB4;
    }
    else
    {
        GPIO_SET(gpio) = 1 << E_GPIO_DB4;
    }

    if ((data & 0x20) == 0)
    {
        GPIO_CLR(gpio) = 1 << E_GPIO_DB5;
    }
    else
    {
        GPIO_SET(gpio) = 1 << E_GPIO_DB5;
    }

    if ((data & 0x40) == 0)
    {
        GPIO_CLR(gpio) = 1 << E_GPIO_DB6;
    }
    else
    {
        GPIO_SET(gpio) = 1 << E_GPIO_DB6;
    }

    if ((data & 0x80) == 0)
    {
        GPIO_CLR(gpio) = 1 << E_GPIO_DB7;
    }
    else
    {
        GPIO_SET(gpio) = 1 << E_GPIO_DB7;
    }

    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

    return;
}

void lcd_read_data(volatile unsigned int * gpio, char * data)
{
    char    bit[8];
    int     i;

    *data = 0;

    bit[7] = ((GPIO_LEV(gpio) & (1 << E_GPIO_DB7)) != 0);
    bit[6] = ((GPIO_LEV(gpio) & (1 << E_GPIO_DB6)) != 0);
    bit[4] = ((GPIO_LEV(gpio) & (1 << E_GPIO_DB5)) != 0);
    bit[4] = ((GPIO_LEV(gpio) & (1 << E_GPIO_DB4)) != 0);
    bit[3] = ((GPIO_LEV(gpio) & (1 << E_GPIO_DB3)) != 0);
    bit[2] = ((GPIO_LEV(gpio) & (1 << E_GPIO_DB2)) != 0);
    bit[1] = ((GPIO_LEV(gpio) & (1 << E_GPIO_DB1)) != 0);
    bit[0] = ((GPIO_LEV(gpio) & (1 << E_GPIO_DB0)) != 0);

    for (i = 0; i < sizeof(bit); i++)
    {
        if (bit[i] == 0)
        {
            *data &= ~ (0 << i);
        }
        else
        {
            *data |= (1 << i);
        }
    }

    return;
}

void lcd_read_status(volatile unsigned int * gpio, int chip, char expected)
{
    char    status = 0;

    do
    {
        db_in(gpio);

        if (chip == 1)
        {
            GPIO_SET(gpio) = 1 << E_GPIO_CS1;
            GPIO_CLR(gpio) = 1 << E_GPIO_CS2;
        }
        else
        {
            GPIO_CLR(gpio) = 1 << E_GPIO_CS1;
            GPIO_SET(gpio) = 1 << E_GPIO_CS2;
        }

        GPIO_SET(gpio) = 1 << E_GPIO_RW;
        GPIO_CLR(gpio) = 1 << E_GPIO_RS;

        msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

        usleep(5);
        GPIO_SET(gpio) = 1 << E_GPIO_E;
        msync((void *)gpio, BLOCK_SIZE, MS_SYNC);
        usleep(5);

        lcd_read_data(gpio, &status);

        GPIO_CLR(gpio) = 1 << E_GPIO_RW;
        GPIO_SET(gpio) = 1 << E_GPIO_E;
        msync((void *)gpio, BLOCK_SIZE, MS_SYNC);
    } while (status != expected);

    GPIO_CLR(gpio) = 1 << E_GPIO_CS1;
    GPIO_CLR(gpio) = 1 << E_GPIO_CS2;

    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

    return;
}

void lcd_instruction(volatile unsigned int * gpio, int chip, char instruction)
{
    db_out(gpio);

    if (chip == 1)
    {
        GPIO_SET(gpio) = 1 << E_GPIO_CS1;
        GPIO_CLR(gpio) = 1 << E_GPIO_CS2;
    }
    else
    {
        GPIO_CLR(gpio) = 1 << E_GPIO_CS1;
        GPIO_SET(gpio) = 1 << E_GPIO_CS2;
    }

    GPIO_CLR(gpio) = 1 << E_GPIO_RW;
    GPIO_CLR(gpio) = 1 << E_GPIO_RS;

    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

    lcd_fill_data(gpio, instruction);

    lcd_pulse(gpio);

    GPIO_CLR(gpio) = 1 << E_GPIO_CS1;
    GPIO_CLR(gpio) = 1 << E_GPIO_CS2;

    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

    return;
}

void init_lcd(volatile unsigned int * gpio)
{
    INP_GPIO(gpio, E_GPIO_RW);
    OUT_GPIO(gpio, E_GPIO_RW);
    INP_GPIO(gpio, E_GPIO_RS);
    OUT_GPIO(gpio, E_GPIO_RS);
    INP_GPIO(gpio, E_GPIO_E);
    OUT_GPIO(gpio, E_GPIO_E);
    INP_GPIO(gpio, E_GPIO_RST);
    OUT_GPIO(gpio, E_GPIO_RST);
    INP_GPIO(gpio, E_GPIO_CS1);
    OUT_GPIO(gpio, E_GPIO_CS1);
    INP_GPIO(gpio, E_GPIO_CS2);
    OUT_GPIO(gpio, E_GPIO_CS2);

    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

    db_out(gpio);

    GPIO_CLR(gpio) = 1 << E_GPIO_RS;
    GPIO_CLR(gpio) = 1 << E_GPIO_RW;
    GPIO_CLR(gpio) = 1 << E_GPIO_E;
    GPIO_CLR(gpio) = 1 << E_GPIO_CS1;
    GPIO_CLR(gpio) = 1 << E_GPIO_CS2;
    GPIO_CLR(gpio) = 1 << E_GPIO_RST;

    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

    /* Reset */
    usleep(5);
    GPIO_SET(gpio) = 1 << E_GPIO_RST;

    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

    /* Switch on */
    lcd_instruction(gpio, 1, LCD_ON);
    lcd_instruction(gpio, 2, LCD_ON);

    return;
}

void lcd_print(volatile unsigned int * gpio, int chip, char * buff, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        db_out(gpio);

        if (chip == 1)
        {
            GPIO_SET(gpio) = 1 << E_GPIO_CS1;
            GPIO_CLR(gpio) = 1 << E_GPIO_CS2;
        }
        else
        {
            GPIO_CLR(gpio) = 1 << E_GPIO_CS1;
            GPIO_SET(gpio) = 1 << E_GPIO_CS2;
        }

        GPIO_CLR(gpio) = 1 << E_GPIO_RW;
        GPIO_SET(gpio) = 1 << E_GPIO_RS;

        msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

        lcd_fill_data(gpio, buff[i]);

        lcd_pulse(gpio);

        GPIO_CLR(gpio) = 1 << E_GPIO_RS;

        msync((void *)gpio, BLOCK_SIZE, MS_SYNC);
    }

    GPIO_CLR(gpio) = 1 << E_GPIO_CS1;
    GPIO_CLR(gpio) = 1 << E_GPIO_CS2;

    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

    return;
}

void lcd_read(volatile unsigned int * gpio, int chip, char * buff, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        db_in(gpio);

        if (chip == 1)
        {
            GPIO_SET(gpio) = 1 << E_GPIO_CS1;
            GPIO_CLR(gpio) = 1 << E_GPIO_CS2;
        }
        else
        {
            GPIO_CLR(gpio) = 1 << E_GPIO_CS1;
            GPIO_SET(gpio) = 1 << E_GPIO_CS2;
        }

        GPIO_SET(gpio) = 1 << E_GPIO_RW;
        GPIO_SET(gpio) = 1 << E_GPIO_RS;

        usleep(5);
        GPIO_SET(gpio) = 1 << E_GPIO_E;
        msync((void *)gpio, BLOCK_SIZE, MS_SYNC);
        usleep(5);

        lcd_read_data(gpio, &buff[i]);

        GPIO_CLR(gpio) = 1 << E_GPIO_E;
        GPIO_CLR(gpio) = 1 << E_GPIO_RS;
        GPIO_CLR(gpio) = 1 << E_GPIO_RW;
        msync((void *)gpio, BLOCK_SIZE, MS_SYNC);
    }

    GPIO_CLR(gpio) = 1 << E_GPIO_CS1;
    GPIO_CLR(gpio) = 1 << E_GPIO_CS2;

    msync((void *)gpio, BLOCK_SIZE, MS_SYNC);

    return;
}

int main(int argc, char **argv)
{
    volatile unsigned * gpio = NULL;
    char                buff[16];
    char                rbuff[64];
    int                 ret = 0;

    // Set up gpi pointer for direct register access
    gpio = setup_io();
    if (ret != 0)
    {
        printf("main: can't init gpio\n");
        return -1;
    }

    init_lcd(gpio);

    lcd_instruction(gpio, 1, LCD_RESET_X);
    lcd_instruction(gpio, 2, LCD_RESET_X);
    lcd_instruction(gpio, 1, LCD_RESET_Y);
    lcd_instruction(gpio, 2, LCD_RESET_Y);
    lcd_instruction(gpio, 1, LCD_RESET_Z);
    lcd_instruction(gpio, 2, LCD_RESET_Z);

    memset(buff, 0x0F, sizeof(buff));
    lcd_print(gpio, 1, buff, sizeof(buff));
    lcd_print(gpio, 2, buff, sizeof(buff));

    memset(buff, 0x55, sizeof(buff));
    lcd_print(gpio, 1, buff, sizeof(buff));
    lcd_print(gpio, 2, buff, sizeof(buff));

    memset(buff, 0xF0, sizeof(buff));
    lcd_print(gpio, 1, buff, sizeof(buff));
    lcd_print(gpio, 2, buff, sizeof(buff));

    memset(buff, 0xAA, sizeof(buff));
    lcd_print(gpio, 1, buff, sizeof(buff));
    lcd_print(gpio, 2, buff, sizeof(buff));

    sleep(10);

    lcd_instruction(gpio, 1, LCD_RESET_X);
    lcd_instruction(gpio, 2, LCD_RESET_X);
    lcd_instruction(gpio, 1, LCD_RESET_Y);
    lcd_instruction(gpio, 2, LCD_RESET_Y);
    lcd_instruction(gpio, 1, LCD_RESET_Z);
    lcd_instruction(gpio, 2, LCD_RESET_Z);

    lcd_read(gpio, 1, rbuff, sizeof(rbuff));
    printf("Chip 1 - First field : 0x%.2X\n", rbuff[0]);
    printf("Chip 1 - Second field : 0x%.2X\n", rbuff[16]);
    printf("Chip 1 - Third field : 0x%.2X\n", rbuff[32]);
    printf("Chip 1 - Fourth field : 0x%.2X\n", rbuff[48]);
    lcd_read(gpio, 2, rbuff, sizeof(rbuff));
    printf("Chip 2 - First field : 0x%.2X\n", rbuff[0]);
    printf("Chip 2 - Second field : 0x%.2X\n", rbuff[16]);
    printf("Chip 2 - Third field : 0x%.2X\n", rbuff[32]);
    printf("Chip 2 - Fourth field : 0x%.2X\n", rbuff[48]);

    lcd_instruction(gpio, 1, LCD_OFF);
    lcd_instruction(gpio, 2, LCD_OFF);

    fflush(stdout);

    munmap((void *)gpio, BLOCK_SIZE); 

    return 0;
}

//
// Set up a memory regions to access GPIO
//
void * setup_io()
{
    int     mem_fd;
    void *  map = NULL;

    /* open /dev/mem */
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
    {
        printf("setup_io: can't open /dev/mem\n");
        return NULL;
    }

    /* mmap GPIO */
    map = mmap(
            NULL,                   //Any adddress in our space will do
            BLOCK_SIZE,             //Map length
            PROT_READ|PROT_WRITE,   // Enable reading & writting to mapped memory
            MAP_SHARED,             //Shared with other processes
            mem_fd,                 //File to map
            GPIO_BASE               //Offset to GPIO peripheral
    );

    close(mem_fd);  //No need to keep mem_fd open after mmap

    if (map == MAP_FAILED)
    {
        printf("setup_io: mmap error\n");
        return NULL;
    }

    return map;
}
