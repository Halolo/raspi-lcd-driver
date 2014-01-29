#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define BCM2708_PERI_BASE  0x20000000
#define GPIO_BASE          (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

#define PAGE_SIZE          (4 * 1024)
#define BLOCK_SIZE         (4 * 1024)

#define DEVICE_MEM_PATH    "/dev/mem"

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

int  mem_fd;
void *gpio_map;

/* I/O access */

volatile unsigned *gpio;


/* GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y) */

#define INP_GPIO(g)        *(gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
#define OUT_GPIO(g)        *(gpio + ((g) / 10)) |=  (1 << (((g) % 10) * 3))
#define SET_GPIO_ALT(g, a) *(gpio + (((g) / 10))) |= (((a) <= 3 ? (a) + 4 : (a) == 4 ? 3 : 2) << (((g) % 10) * 3))

#define GPIO_SET *(gpio + 7)  /* sets   bits which are 1 ignores bits which are 0 */
#define GPIO_CLR *(gpio + 10) /* clears bits which are 1 ignores bits which are 0 */
#define GPIO_LEV *(gpio + 13) /* pin level */

void setup_io();

void db_out()
{
    INP_GPIO(E_GPIO_DB0);
    OUT_GPIO(E_GPIO_DB0);
    INP_GPIO(E_GPIO_DB1);
    OUT_GPIO(E_GPIO_DB1);
    INP_GPIO(E_GPIO_DB2);
    OUT_GPIO(E_GPIO_DB2);
    INP_GPIO(E_GPIO_DB3);
    OUT_GPIO(E_GPIO_DB3);
    INP_GPIO(E_GPIO_DB4);
    OUT_GPIO(E_GPIO_DB4);
    INP_GPIO(E_GPIO_DB5);
    OUT_GPIO(E_GPIO_DB5);
    INP_GPIO(E_GPIO_DB6);
    OUT_GPIO(E_GPIO_DB6);
    INP_GPIO(E_GPIO_DB7);
    OUT_GPIO(E_GPIO_DB7);

    usleep(1000);
}

void db_in()
{
    INP_GPIO(E_GPIO_DB0);
    INP_GPIO(E_GPIO_DB1);
    INP_GPIO(E_GPIO_DB2);
    INP_GPIO(E_GPIO_DB3);
    INP_GPIO(E_GPIO_DB4);
    INP_GPIO(E_GPIO_DB5);
    INP_GPIO(E_GPIO_DB6);
    INP_GPIO(E_GPIO_DB7);

    usleep(1000);
}

void lcd_reset()
{
    GPIO_CLR = 1 << E_GPIO_RST;

    usleep(100000);

    GPIO_SET = 1 << E_GPIO_RST;

    usleep(50000);
}

int lcd_read_status()
{
    unsigned int  value;
    unsigned int  ret;

    GPIO_CLR = 1 << E_GPIO_E;

    usleep(100000);

    db_in();

    GPIO_SET = 1 << E_GPIO_RW;
    GPIO_CLR = 1 << E_GPIO_RS;
    GPIO_CLR = 1 << E_GPIO_CS1;
    GPIO_CLR = 1 << E_GPIO_CS2;

    usleep(100000);

    GPIO_SET = 1 << E_GPIO_E;

    usleep(100000);

    value = GPIO_LEV;

    ret = 0;

    ret |= ((value & (1 << E_GPIO_DB0)) != 0) << 0;
    ret |= ((value & (1 << E_GPIO_DB1)) != 0) << 1;
    ret |= ((value & (1 << E_GPIO_DB2)) != 0) << 2;
    ret |= ((value & (1 << E_GPIO_DB3)) != 0) << 3;
    ret |= ((value & (1 << E_GPIO_DB4)) != 0) << 4;
    ret |= ((value & (1 << E_GPIO_DB5)) != 0) << 5;
    ret |= ((value & (1 << E_GPIO_DB6)) != 0) << 6;
    ret |= ((value & (1 << E_GPIO_DB7)) != 0) << 7;

    return ret;
}

void lcd_on_off(int on)
{
    db_out();

    GPIO_CLR = 1 << E_GPIO_E;

    usleep(100000);

    GPIO_CLR = 1 << E_GPIO_RW;
    GPIO_CLR = 1 << E_GPIO_RS;
    GPIO_CLR = 1 << E_GPIO_CS1;
    GPIO_SET = 1 << E_GPIO_CS2;

    GPIO_SET = 1 << E_GPIO_E;

    if (on == 1)
    {
        GPIO_SET = 1 << E_GPIO_DB0;
    }
    else
    {
        GPIO_CLR = 1 << E_GPIO_DB0;
    }

    GPIO_SET = 1 << E_GPIO_DB1;
    GPIO_SET = 1 << E_GPIO_DB2;
    GPIO_SET = 1 << E_GPIO_DB3;
    GPIO_SET = 1 << E_GPIO_DB4;
    GPIO_SET = 1 << E_GPIO_DB5;
    GPIO_CLR = 1 << E_GPIO_DB6;
    GPIO_CLR = 1 << E_GPIO_DB7;

    usleep(100000);

    GPIO_CLR = 1 << E_GPIO_E;

    usleep(100000);
}

void init_lcd()
{
    INP_GPIO(E_GPIO_RW);
    OUT_GPIO(E_GPIO_RW);
    INP_GPIO(E_GPIO_RS);
    OUT_GPIO(E_GPIO_RS);
    INP_GPIO(E_GPIO_E);
    OUT_GPIO(E_GPIO_E);
    INP_GPIO(E_GPIO_RST);
    OUT_GPIO(E_GPIO_RST);
    INP_GPIO(E_GPIO_CS1);
    OUT_GPIO(E_GPIO_CS1);
    INP_GPIO(E_GPIO_CS2);
    OUT_GPIO(E_GPIO_CS2);

    db_out();

    lcd_reset();
}

void lcd_print(char * buff)
{
    db_out();

}

int main(int argc, char **argv)
{
    char    screen[128];

    /* Set up gpi pointer for direct register access */
    setup_io();

    init_lcd();

    lcd_on_off(1);

    printf("Status = 0x%.8X\n", lcd_read_status());

    memset(screen, 0x42, sizeof(screen));

    /* lcd_print(screen); */

    sleep(1);

    lcd_on_off(0);

    printf("Status = 0x%.8X\n", lcd_read_status());

    return 0;
} /* main */


/**
 * Set up a memory regions to access GPIO
 */
void setup_io()
{
    /* open /dev/mem */
    if ((mem_fd = open(DEVICE_MEM_PATH, O_RDWR|O_SYNC) ) < 0) {
        printf("can't open %s \n", DEVICE_MEM_PATH);
        exit(-1);
    }

    /* mmap GPIO */
    gpio_map = mmap(
            NULL,                 /* Any adddress in our space will do */
            BLOCK_SIZE,           /* Map length */
            PROT_READ|PROT_WRITE, /* Enable reading & writting to mapped memory */
            MAP_SHARED,           /* Shared with other processes */
            mem_fd,               /* File to map */
            GPIO_BASE             /* Offset to GPIO peripheral */
    );

    close(mem_fd); /* No need to keep mem_fd open after mmap */

    if (gpio_map == MAP_FAILED) {
        printf("mmap error\n");
        exit(-1);
    }

    /* Always use volatile pointer! */
    gpio = (volatile unsigned *)gpio_map;
} /* setup_io */

