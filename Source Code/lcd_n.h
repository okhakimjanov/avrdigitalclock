/*
 * LCD_Control1.c
 *
 * Created: 03.04.2020 13:53:02
 * Author : Anvar
 */

#define LCD_WDATA PORTA // write data
#define LCD_WINST PORTA // write instruction
#define LCD_CTRL PORTG  //
#define LCD_EN 0        // Enable
#define LCD_RW 1        //
#define LCD_RS 2

#define On 1
#define Off 0
#define RIGHT 1
#define LEFT 0

void PortInit(void)
{
    DDRA = 0xFF; // Port A is output
    DDRG = 0x0F; // 0000 1111 lower 4 bits => output
}

void LCD_Data(char ch)
{
    LCD_CTRL |= (1 << LCD_RS);  //  OR 0000 0100, make									D2 = 1
    LCD_CTRL &= ~(1 << LCD_RW); // to write data, AND NOT(0000 0010) = AND 1111 1101,	D1 = 0
    LCD_CTRL |= (1 << LCD_EN);  // LCD enable, OR 0000 0001,							D0 = 1
    _delay_us(50);
    LCD_WDATA = ch; // PORTA = character
    _delay_us(50);
    LCD_CTRL &= ~(1 << LCD_EN); // LCD disable, AND NOT(0000 0001) = AND 1111 1110,		D0 = 0
}

void LCD_Comm(char ch)
{
    LCD_CTRL &= ~(1 << LCD_RS); // RS == 0 --> IR selection, AND NOT(0000 0100), AND 1111 1011, D2 = 0
    LCD_CTRL &= ~(1 << LCD_RW); // AND NOT(0000 0010), AND 1111 1101,							D1 = 0
    LCD_CTRL |= (1 << LCD_EN);  // OR 0000 0001,												D0 = 1
    _delay_us(50);
    LCD_WINST = ch; // PORTA = character
    _delay_us(50);
    LCD_CTRL &= ~(1 << LCD_EN); // LCD disable, AND NOT(0000 0001), AND 1111 1110,				D0 = 0
}

void LCD_CHAR(char c)
{
    LCD_Data(c);
    _delay_ms(2);
}
void LCD_STR(char *str)
{
    while (*str != 0)
    {
        LCD_CHAR(*str);
	    str++;
    }
	
}
void LCD_pos(unsigned char col, unsigned char row)
{
    LCD_Comm(0x80 | (row + col * 0x40)); // second line starts from 0x40
                                         // 1000 0000 OR (row + col * 0100 0000),
}
void Cursor_Home(void)
{
    LCD_Comm(0x02); // Cursor Home
    _delay_ms(2);
}
void LCD_Clear(void)
{
    LCD_Comm(0x01);
    _delay_ms(2);
}
void LCD_Shift(char p)
{
    if (p == RIGHT)
    {
        LCD_Comm(0x1C); // shift character to the right
        _delay_ms(1);
    }
    else if (p == LEFT)
    {
        LCD_Comm(0x18); // shift character to the left
        _delay_ms(1);
    }
}
void Cursor_shift(char p)
{
    if (p == RIGHT)
    {
        LCD_Comm(0x14);
        _delay_ms(1);
    }
    else if (p == LEFT)
    {
        LCD_Comm(0x10);
        _delay_ms(1);
    }
}
void LCD_Init(void)
{
    LCD_Comm(0x38); // DDRAM 2 lines, 5 * 7 font, 8 bit, 0011 1000
    _delay_ms(2);
    LCD_Comm(0x38); // 0011 1000
    _delay_ms(2);
    LCD_Comm(0x38); // 0011 1000
    _delay_ms(2);
    LCD_Comm(0x0e); // Display ON/OFF, 0000 1110
    _delay_ms(2);
    LCD_Comm(0x06); // Cursor to right address + 1
    _delay_ms(2);
}
