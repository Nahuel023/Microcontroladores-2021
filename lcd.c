/*
 * lcd.c
 *
 * Created: 22/8/2021 23:54:42
 *  Author: Nahuel Medina
 */ 

#include "lcd.h"
#include "font.h"
#include <string.h>


static struct {
	uint8_t x;
	uint8_t y;
} cursorPosition;

static uint8_t charMode = NORMALSIZE;

#if defined TEXTMODE
#else
#error "No valid displaymode! Refer lcd.h"
#endif


const uint8_t init_sequence [] PROGMEM = {    	// Secuencia de inicializacion - almacena datos en memoria flash
	LCD_DISP_OFF,    							// Display OFF (sleep mode)
	0x20, 0b00,      							// Modo de direccionamiento de memoria (address)
												// 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
												// 10=Page Addressing Mode (RESET); 11=Invalid
	0xB0,            							// Establecer dirección de inicio de página para el modo de direccionamiento de página, 0-7
	0xC8,            							// Establecer dirección de escaneo de salida COM
	0x00,            							// --establecer dirección de columna baja
	0x10,            							// --establecer la dirección de la columna alta
	0x40,            							// --establecer la dirección de la línea de inicio
	0x81, 0x3F,      							// Set contrast control register
	0xA1,            							// Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
	0xA6,            							// Set display mode. A6=Normal; A7=Inverse
	0xA8, DISPLAY_HEIGHT-1, 					// Set multiplex ratio(1 to 64)
	0xA4,            							// Output RAM to Display
												// 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
	0xD3, 0x00,      							// Set display offset. 00 = no offset
	0xD5,            							// --set display clock divide ratio/oscillator frequency
	0xF0,            							// --set divide ratio
	0xD9, 0x22,      							// Set pre-charge period
	// Establecer la configuración del hardware de los pines com
	#if DISPLAY_HEIGHT==64
	0xDA, 0x12,
	#elif DISPLAY_HEIGHT==32
	0xDA, 0x02,
	#endif
	0xDB,            // --set vcomh
	0x20,            // 0x20,0.77xVcc
	0x8D, 0x14,      // Set DC-DC enable
	
	
};
#pragma mark LCD COMMUNICATION
void lcd_command(uint8_t cmd[], uint8_t size) {
	#if defined I2C
	i2c_start((LCD_I2C_ADR << 1) | 0);
	i2c_byte(0x00);    // 0x00 for command, 0x40 for data
	for (uint8_t i=0; i<size; i++) {
		i2c_byte(cmd[i]);
	}
	i2c_stop();
	#endif
}
void lcd_data(uint8_t data[], uint16_t size) {
	#if defined I2C
	i2c_start((LCD_I2C_ADR << 1) | 0);
	i2c_byte(0x40);    // 0x00 for command, 0x40 for data
	for (uint16_t i = 0; i<size; i++) {
		i2c_byte(data[i]);
	}
	i2c_stop();
	#endif
}
#pragma mark -
#pragma mark GENERAL FUNCTIONS
void lcd_init(uint8_t dispAttr){
	#if defined I2C
	i2c_init();
	#endif

	uint8_t commandSequence[sizeof(init_sequence)+1];
	for (uint8_t i = 0; i < sizeof (init_sequence); i++) {
		commandSequence[i] = (pgm_read_byte(&init_sequence[i]));
	}
	commandSequence[sizeof(init_sequence)]=(dispAttr);
	lcd_command(commandSequence, sizeof(commandSequence));
	lcd_clrscr();
}
void lcd_gotoxy(uint8_t x, uint8_t y){
	x = x * sizeof(FONT[0]);
	lcd_goto_xpix_y(x,y);
}
void lcd_goto_xpix_y(uint8_t x, uint8_t y){
	if( x > (DISPLAY_WIDTH) || y > (DISPLAY_HEIGHT/8-1)) return;// out of display
	cursorPosition.x=x;
	cursorPosition.y=y;
	
	uint8_t commandSequence[] = {0xb0+y, 0x21, 0x00+((2+x) & (0x0f)), 0x10+( ((2+x) & (0xf0)) >> 4 ), 0x7f};
	
	lcd_command(commandSequence, sizeof(commandSequence));
}
void lcd_clrscr(void){
	#ifdef defined TEXTMODE
	uint8_t displayBuffer[DISPLAY_WIDTH];
	memset(displayBuffer, 0x00, sizeof(displayBuffer));
	for (uint8_t i = 0; i < DISPLAY_HEIGHT/8; i++){
		lcd_gotoxy(0,i);
		lcd_data(displayBuffer, sizeof(displayBuffer));
	}
	#endif
	lcd_home();
}
void lcd_home(void){
	lcd_gotoxy(0, 0);
}
void lcd_invert(uint8_t invert){
	uint8_t commandSequence[1];
	if (invert != YES) {
		commandSequence[0] = 0xA6;
		} else {
		commandSequence[0] = 0xA7;
	}
	lcd_command(commandSequence, 1);
}
void lcd_sleep(uint8_t sleep){
	uint8_t commandSequence[1];
	if (sleep != YES) {
		commandSequence[0] = 0xAF;
		} else {
		commandSequence[0] = 0xAE;
	}
	lcd_command(commandSequence, 1);
}
void lcd_set_contrast(uint8_t contrast){
	uint8_t commandSequence[2] = {0x81, contrast};
	lcd_command(commandSequence, sizeof(commandSequence));
}
void lcd_putc(char c){
	switch (c) {
		case '\b':
		// backspace
		lcd_gotoxy(cursorPosition.x-charMode, cursorPosition.y);
		lcd_putc(' ');
		lcd_gotoxy(cursorPosition.x-charMode, cursorPosition.y);
		break;
		case '\t':
		// tab
		if( (cursorPosition.x+charMode*4) < (DISPLAY_WIDTH/ sizeof(FONT[0])-charMode*4) ){
			lcd_gotoxy(cursorPosition.x+charMode*4, cursorPosition.y);
			}else{
			lcd_gotoxy(DISPLAY_WIDTH/ sizeof(FONT[0]), cursorPosition.y);
		}
		break;
		case '\n':
		// linefeed
		if(cursorPosition.y < (DISPLAY_HEIGHT/8-1)){
			lcd_gotoxy(cursorPosition.x, cursorPosition.y+charMode);
		}
		break;
		case '\r':
		// carrige return
		lcd_gotoxy(0, cursorPosition.y);
		break;
		default:
		// char doesn't fit in line
		if( (cursorPosition.x >= DISPLAY_WIDTH-sizeof(FONT[0])) || (c < ' ') ) break;
		// mapping char
		c -= ' ';
		if (c >= pgm_read_byte(&special_char[0][1]) ) {
			char temp = c;
			c = 0xff;
			for (uint8_t i=0; pgm_read_byte(&special_char[i][1]) != 0xff; i++) {
				if ( pgm_read_byte(&special_char[i][0])-' ' == temp ) {
					c = pgm_read_byte(&special_char[i][1]);
					break;
				}
			}
			if ( c == 0xff ) break;
		}
		// imprimir caracteres en pantalla
		#ifdef defined TEXTMODE
		if (charMode == DOUBLESIZE) {
			uint16_t doubleChar[sizeof(FONT[0])];
			uint8_t dChar;
			if ((cursorPosition.x+2*sizeof(FONT[0]))>DISPLAY_WIDTH) break;
			
			for (uint8_t i=0; i < sizeof(FONT[0]); i++) {
				doubleChar[i] = 0;
				dChar = pgm_read_byte(&(FONT[(uint8_t)c][i]));
				for (uint8_t j=0; j<8; j++) {
					if ((dChar & (1 << j))) {
						doubleChar[i] |= (1 << (j*2));
						doubleChar[i] |= (1 << ((j*2)+1));
					}
				}
			}
			uint8_t data[sizeof(FONT[0])*2];
			for (uint8_t i = 0; i < sizeof(FONT[0]); i++)
			{
				// print font to ram, print 6 columns
				data[i<<1]=(doubleChar[i] & 0xff);
				data[(i<<1)+1]=(doubleChar[i] & 0xff);
			}
			lcd_data(data, sizeof(FONT[0])*2);
			
			#if defined SH1106
			uint8_t commandSequence[] = {0xb0+cursorPosition.y+1,
				0x21,
				0x00+((2+cursorPosition.x) & (0x0f)),
				0x10+( ((2+cursorPosition.x) & (0xf0)) >> 4 ),
			0x7f};
			#endif
			lcd_command(commandSequence, sizeof(commandSequence));
			
			for (uint8_t i = 0; i < sizeof(FONT[0]); i++)
			{
				// print font to ram, print 6 columns
				data[i<<1]=(doubleChar[i] >> 8);
				data[(i<<1)+1]=(doubleChar[i] >> 8);
			}
			lcd_data(data, sizeof(FONT[0])*2);
			
			commandSequence[0] = 0xb0+cursorPosition.y;
			#if defined (SSD1306) || defined (SSD1309)
			commandSequence[2] = cursorPosition.x+(2*sizeof(FONT[0]));
			#elif defined SH1106
			commandSequence[2] = 0x00+((2+cursorPosition.x+(2*sizeof(FONT[0]))) & (0x0f));
			commandSequence[3] = 0x10+( ((2+cursorPosition.x+(2*sizeof(FONT[0]))) & (0xf0)) >> 4 );
			#endif
			lcd_command(commandSequence, sizeof(commandSequence));
			cursorPosition.x += sizeof(FONT[0])*2;
			} else {
			uint8_t data[sizeof(FONT[0])];
			if ((cursorPosition.x+sizeof(FONT[0]))>DISPLAY_WIDTH) break;
			
			for (uint8_t i = 0; i < sizeof(FONT[0]); i++)
			{
				// print font to ram, print 6 columns
				data[i]=(pgm_read_byte(&(FONT[(uint8_t)c][i])));
			}
			lcd_data(data, sizeof(FONT[0]));
			cursorPosition.x += sizeof(FONT[0]);
		}
		#endif
		break;
	}
	
}
void lcd_charMode(uint8_t mode){
	charMode = mode;
}
void lcd_puts(const char* s){
	while (*s) {
		lcd_putc(*s++);
	}
}
void lcd_puts_p(const char* progmem_s){
	register uint8_t c;
	while ((c = pgm_read_byte(progmem_s++))) {
		lcd_putc(c);
	}
}
