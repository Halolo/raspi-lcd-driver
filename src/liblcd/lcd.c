#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "lcd.h"

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
#define INP_GPIO(lcd_hdl,g) *((unsigned int *)lcd_hdl + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
#define OUT_GPIO(lcd_hdl,g) *((unsigned int *)lcd_hdl + ((g) / 10)) |=  (1 << (((g) % 10) * 3))

#define GPIO_SET(lcd_hdl) *((unsigned int *)lcd_hdl + 7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR(lcd_hdl) *((unsigned int *)lcd_hdl + 10) // clears bits which are 1 ignores bits which are 0
#define GPIO_LEV(lcd_hdl) *((unsigned int *)lcd_hdl + 13) // pin level

static volatile unsigned *gpio = NULL;

// Local functions

void db_out(void *lcd_hdl)
{
	INP_GPIO(lcd_hdl, E_GPIO_DB0);
	OUT_GPIO(lcd_hdl, E_GPIO_DB0);
	INP_GPIO(lcd_hdl, E_GPIO_DB1);
	OUT_GPIO(lcd_hdl, E_GPIO_DB1);
	INP_GPIO(lcd_hdl, E_GPIO_DB2);
	OUT_GPIO(lcd_hdl, E_GPIO_DB2);
	INP_GPIO(lcd_hdl, E_GPIO_DB3);
	OUT_GPIO(lcd_hdl, E_GPIO_DB3);
	INP_GPIO(lcd_hdl, E_GPIO_DB4);
	OUT_GPIO(lcd_hdl, E_GPIO_DB4);
	INP_GPIO(lcd_hdl, E_GPIO_DB5);
	OUT_GPIO(lcd_hdl, E_GPIO_DB5);
	INP_GPIO(lcd_hdl, E_GPIO_DB6);
	OUT_GPIO(lcd_hdl, E_GPIO_DB6);
	INP_GPIO(lcd_hdl, E_GPIO_DB7);
	OUT_GPIO(lcd_hdl, E_GPIO_DB7);

	msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

	return;
}

void db_in(void *lcd_hdl)
{
	INP_GPIO(lcd_hdl, E_GPIO_DB0);
	INP_GPIO(lcd_hdl, E_GPIO_DB1);
	INP_GPIO(lcd_hdl, E_GPIO_DB2);
	INP_GPIO(lcd_hdl, E_GPIO_DB3);
	INP_GPIO(lcd_hdl, E_GPIO_DB4);
	INP_GPIO(lcd_hdl, E_GPIO_DB5);
	INP_GPIO(lcd_hdl, E_GPIO_DB6);
	INP_GPIO(lcd_hdl, E_GPIO_DB7);

	msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

	return;
}

void lcd_pulse(void *lcd_hdl)
{
	usleep(5);
	GPIO_SET(lcd_hdl) = 1 << E_GPIO_E;
	msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);
	usleep(5);
	GPIO_CLR(lcd_hdl) = 1 << E_GPIO_E;
	msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);
	usleep(5);

	return;
}

void lcd_fill_data(void *lcd_hdl, char data)
{
	if ((data & 0x01) == 0)
	{
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_DB0;
	}
	else
	{
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_DB0;
	}

	if ((data & 0x02) == 0)
	{
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_DB1;
	}
	else
	{
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_DB1;
	}

	if ((data & 0x04) == 0)
	{
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_DB2;
	}
	else
	{
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_DB2;
	}

	if ((data & 0x08) == 0)
	{
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_DB3;
	}
	else
	{
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_DB3;
	}

	if ((data & 0x10) == 0)
	{
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_DB4;
	}
	else
	{
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_DB4;
	}

	if ((data & 0x20) == 0)
	{
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_DB5;
	}
	else
	{
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_DB5;
	}

	if ((data & 0x40) == 0)
	{
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_DB6;
	}
	else
	{
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_DB6;
	}

	if ((data & 0x80) == 0)
	{
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_DB7;
	}
	else
	{
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_DB7;
	}

	msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

	return;
}

void lcd_read_data(void *lcd_hdl, char *data)
{
	char    bit[8];
	int     i;

	*data = 0;

	bit[7] = ((GPIO_LEV(lcd_hdl) & (1 << E_GPIO_DB7)) != 0);
	bit[6] = ((GPIO_LEV(lcd_hdl) & (1 << E_GPIO_DB6)) != 0);
	bit[5] = ((GPIO_LEV(lcd_hdl) & (1 << E_GPIO_DB5)) != 0);
	bit[4] = ((GPIO_LEV(lcd_hdl) & (1 << E_GPIO_DB4)) != 0);
	bit[3] = ((GPIO_LEV(lcd_hdl) & (1 << E_GPIO_DB3)) != 0);
	bit[2] = ((GPIO_LEV(lcd_hdl) & (1 << E_GPIO_DB2)) != 0);
	bit[1] = ((GPIO_LEV(lcd_hdl) & (1 << E_GPIO_DB1)) != 0);
	bit[0] = ((GPIO_LEV(lcd_hdl) & (1 << E_GPIO_DB0)) != 0);

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

void lcd_read_status(void *lcd_hdl, int chip, char expected)
{
	char    status = 0;

	do
	{
		db_in(lcd_hdl);

		if (chip == 1)
		{
			GPIO_SET(lcd_hdl) = 1 << E_GPIO_CS1;
			GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS2;
		}
		else
		{
			GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS1;
			GPIO_SET(lcd_hdl) = 1 << E_GPIO_CS2;
		}

		GPIO_SET(lcd_hdl) = 1 << E_GPIO_RW;
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_RS;

		msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

		usleep(5);
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_E;
		msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);
		usleep(5);

		lcd_read_data(lcd_hdl, &status);

		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_RW;
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_E;
		msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);
	} while (status != expected);

	GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS1;
	GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS2;

	msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

	return;
}

void lcd_instruction(void *lcd_hdl, int chip, char instruction)
{
	db_out(lcd_hdl);

	if (chip == 1)
	{
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_CS1;
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS2;
	}
	else
	{
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS1;
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_CS2;
	}

	GPIO_CLR(lcd_hdl) = 1 << E_GPIO_RW;
	GPIO_CLR(lcd_hdl) = 1 << E_GPIO_RS;

	msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

	lcd_fill_data(lcd_hdl, instruction);

	lcd_pulse(lcd_hdl);

	GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS1;
	GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS2;

	msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

	if (instruction != LCD_OFF)
	{
		lcd_read_status(lcd_hdl, chip, LCD_STATUS_READY);
	}

	return;
}

char *bin (unsigned long int i)
{
	int		k = 0;
    static char	buffer[1 + sizeof(unsigned long int) * 8] = { 0 };
    char *	p = buffer - 1 + sizeof(unsigned long int) * 8;

    do
    {
    	*--p = '0' + (i & 1);
    	*--p = ' ';
    	i >>= 1;
    	k++;
    } while (k < 8);

    return p;
}

// Interface functions

void *lcd_connect()
{
    int                 mem_fd;

    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
    {
        printf("lcd_connect: can't open /dev/mem (%d)\n", mem_fd);
        return NULL;
    }

    /* mmap GPIO */
    gpio = mmap(
            NULL,
            BLOCK_SIZE,
            PROT_NONE|PROT_READ|PROT_WRITE,
            MAP_PRIVATE|MAP_LOCKED,
            mem_fd,
            GPIO_BASE
    );

    close(mem_fd);

    if (gpio == MAP_FAILED)
    {
        printf("lcd_connect: mmap error\n");
        return NULL;
    }

    return (void *)gpio;
}

void lcd_disconnect(void * lcd_hdl)
{
    munmap(lcd_hdl, BLOCK_SIZE);
}

void init_lcd(void *lcd_hdl)
{
    INP_GPIO(lcd_hdl, E_GPIO_RW);
    OUT_GPIO(lcd_hdl, E_GPIO_RW);
    INP_GPIO(lcd_hdl, E_GPIO_RS);
    OUT_GPIO(lcd_hdl, E_GPIO_RS);
    INP_GPIO(lcd_hdl, E_GPIO_E);
    OUT_GPIO(lcd_hdl, E_GPIO_E);
    INP_GPIO(lcd_hdl, E_GPIO_RST);
    OUT_GPIO(lcd_hdl, E_GPIO_RST);
    INP_GPIO(lcd_hdl, E_GPIO_CS1);
    OUT_GPIO(lcd_hdl, E_GPIO_CS1);
    INP_GPIO(lcd_hdl, E_GPIO_CS2);
    OUT_GPIO(lcd_hdl, E_GPIO_CS2);

    msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

    db_out(lcd_hdl);

    GPIO_CLR(lcd_hdl) = 1 << E_GPIO_RS;
    GPIO_CLR(lcd_hdl) = 1 << E_GPIO_RW;
    GPIO_CLR(lcd_hdl) = 1 << E_GPIO_E;
    GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS1;
    GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS2;
    GPIO_CLR(lcd_hdl) = 1 << E_GPIO_RST;

    msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

    /* Reset */
    usleep(5);
    GPIO_SET(lcd_hdl) = 1 << E_GPIO_RST;

    msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

    /* Switch on */
    lcd_instruction(lcd_hdl, 1, LCD_ON);
    lcd_instruction(lcd_hdl, 2, LCD_ON);

    return;
}

void lcd_print(void *lcd_hdl, int chip, lcd_buf_t *buff)
{
	int		i,j,k,l=0;
	char	bit[8];
	char	seg;

	bit[0] = 0x80;
	bit[1] = 0x40;
	bit[2] = 0x20;
	bit[3] = 0x10;
	bit[4] = 0x08;
	bit[5] = 0x04;
	bit[6] = 0x02;
	bit[7] = 0x01;

    lcd_instruction(lcd_hdl, chip, LCD_RESET_X);
    lcd_instruction(lcd_hdl, chip, LCD_RESET_Y);
    lcd_instruction(lcd_hdl, chip, LCD_RESET_Z);

	for (i = 0; i < 8; i++)
	{
		lcd_instruction(lcd_hdl, 1, LCD_RESET_X + i);
		lcd_instruction(lcd_hdl, 2, LCD_RESET_X + i);

		for (j = 0; j < LCD_PX_SIZE; j++)
		{
			db_out(lcd_hdl);

			if (chip == 1)
			{
				GPIO_SET(lcd_hdl) = 1 << E_GPIO_CS1;
				GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS2;
			}
			else
			{
				GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS1;
				GPIO_SET(lcd_hdl) = 1 << E_GPIO_CS2;
			}

			GPIO_CLR(lcd_hdl) = 1 << E_GPIO_RW;
			GPIO_SET(lcd_hdl) = 1 << E_GPIO_RS;

			msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

			for (k = 0; k < 8; k++)
			{
				if ((buff->buff[(i * LCD_PX_SIZE) + (j / 8) + (8 * k)] & bit[j % 8]) == 0)
				{
					seg &= ~ (1 << k);
				}
				else
				{
					seg |= (1 << k);
				}
			}

			printf("%s", bin(buff->buff[l++]));
			if ((l % 8) == 0)
			{
				printf("\n");
			}

			lcd_fill_data(lcd_hdl, seg);

			lcd_pulse(lcd_hdl);

			GPIO_CLR(lcd_hdl) = 1 << E_GPIO_RS;

			GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS1;
			GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS2;

			msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

			lcd_read_status(lcd_hdl, 1, LCD_STATUS_READY);
		}
	}

	return;
}

void lcd_read(void *lcd_hdl, int chip, lcd_buf_t *buff)
{
	int i;

	for (i = 0; i < LCD_PX_SIZE; i++)
	{
		db_in(lcd_hdl);

		if (chip == 1)
		{
			GPIO_SET(lcd_hdl) = 1 << E_GPIO_CS1;
			GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS2;
		}
		else
		{
			GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS1;
			GPIO_SET(lcd_hdl) = 1 << E_GPIO_CS2;
		}

		GPIO_SET(lcd_hdl) = 1 << E_GPIO_RW;
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_RS;

		usleep(5);
		GPIO_SET(lcd_hdl) = 1 << E_GPIO_E;
		msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);
		usleep(5);

		lcd_read_data(lcd_hdl, &buff->buff[i]);

		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_E;
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_RS;
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_RW;

		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS1;
		GPIO_CLR(lcd_hdl) = 1 << E_GPIO_CS2;

		msync((void *)lcd_hdl, BLOCK_SIZE, MS_SYNC);

		lcd_read_status(lcd_hdl, 1, LCD_STATUS_READY);
	}

	return;
}

void lcd_on(void * lcd_hdl)
{
    lcd_instruction(lcd_hdl, 1, LCD_ON);
    lcd_instruction(lcd_hdl, 2, LCD_ON);
}

void lcd_off(void * lcd_hdl)
{
    lcd_instruction(lcd_hdl, 1, LCD_OFF);
    lcd_instruction(lcd_hdl, 2, LCD_OFF);
}
