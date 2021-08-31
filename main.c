/*
 * Display.c
 *
 * Created: 22/8/2021 23:53:43
 * Author : Nahuel Medina
 */ 

#include <avr/io.h>

#include "lcd.h"


int main(void){
	lcd_init(LCD_DISP_ON);    							// init lcd y enciende
	
	lcd_puts("Hello World");  							// coloca una cadena de RAM para mostrar (TEXTMODE) 
	lcd_gotoxy(0,2);          							// coloca el cursor en la primera columna en la linea 3
	lcd_puts_p(PSTR("String from flash"));  			// pone flash en forma de string para mostrar (TEXTMODE) 
}

