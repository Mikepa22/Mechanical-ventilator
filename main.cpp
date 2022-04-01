/*
 * GccApplication1.cpp
 *
 * Created: 25/03/2022 8:10:27 p. m.
 * Author : carlos
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "adc.setup.c"
#include "lcd.setup.c"
#include "conversion_ASCII.c"
#include "lcd_write.c"
#include "virtual_write.c"
#include "serial_setup.c"
#include "Datosguradar.c"

#include <util/delay.h>
#include <stdio.h>


#include "DEF_ATMEGA328P.h"
#include "DHT22.h"

    

uint16_t converted_signal = 0, signal_converted=0;
float output_signal = 0, cmH2O=0, Pascales, Velocidad,Preciondiferencial, flujovolumetrico,volumen=1.5, temperatura,  humedad; ;

uint8_t y,w,z,a, entero, decimal_1, decimal_2, Q, led = 0,p,q, Convercion=0,contador=0;;

uint16_t x;









int main(void)
{
	//configuration ports
	DDRA = 0xFF; //--------------SALIDA
	DDRB = 0xFF;
	DDRL = 0XFF;
	
	PORTA = 0X00; //-------------INICAILIZADAS EN 0
	PORTB = 0x00;
	PORTL = 0x00;
	
	//ADC configuration
	adc_setup(); 
	
	 // Configuración de interrupción externa 0 por flanco de subida
	 EIMSK = 0x07;
	 EICRA = 0x3f;
	 
	 // confuguramos taimer desbordamiento a 1seg
	 
	asm("LDI R16,0<<2|1<<1|1<<0	; PRESCALER DE 64");
	asm("STS 0x81,R16				; AQUÍ INICIA EL TIMER");	// 0x81 = TCCR1B
	asm("LDI R16,0xff				 ; SE PRECARGA EL TIMER 1");
	asm("STS 0x85,R16				; CON 0x0BDC=ff06");		// 0x85 = TCNT1H
	asm("LDI R16,0x06				; VALOR NECESARIO PARA");
	asm("STS 0x84,R16				; DESBORDAR A 1 Seg");		// 0x84 = TCNT1L
	asm("LDI R16,1<<0				; HABILITACIÓN DE INT");
	asm("STS 0x6F,R16				; DE TIMER 1");				// 0x6F = TIMSK1
	  
	 
	 //configurations lcd
	 lcd_setup();
	 	
	//global interruptions
	sei();
	
	//start conversion
	ADCSRA |= (1 << ADSC);	
	
	DHT22_init();
	
    /* Replace with your application code */
    while (1) 
    {
	 
    }
	
	
}

ISR(ADC_vect)
{
	if(Convercion ==0) // Precion Absoluta
	{
		
		//====================================================================
		// Lectura del ACD
		//====================================================================
		
		signal_converted=ADCL;
		converted_signal = ADCH;
		converted_signal = converted_signal << 8;
		converted_signal |=  signal_converted;
		
		//====================================================================
		//conversion result de pascales a cmH20
		//====================================================================
		
		output_signal = converted_signal * 0.00488758;		//Vref = 5V;   5/1023=0.00488758
		
		Pascales= ((output_signal*7375000)-223947) / 3693205;
		cmH2O=10.1972*Pascales;
		
		// Alarma de Precion
		if (cmH2O<=40)
		{ 
			PORTL=0xFF; alarmaPrecion (0);
		}
		
		else if ((cmH2O>=55))
		{
			PORTL=0xFF; alarmaPrecion (1);
		}
		
		else
		{
	       PORTL=0x00;  alarmaPrecion (3);
		}
		
	
		//Desconpocicion del dato para convertirse en ascii
		x = cmH2O*10;
		y = x/100;				//Decenas 1
		w = x % 100;
		z = w/10;				//Unidades
		a = w%10;				//decimal 
		
		
		entero = conversion_ASCII(y);
		decimal_1 = conversion_ASCII(z);
		decimal_2 = conversion_ASCII(a);
		
		//LCD write Data, virtual_write2
		lcd_write(0,entero, decimal_1, decimal_2);
		
		// iniciamos convercion precion diferencial 
		Convercion=1;
		asm("LDI R16, 0x41");	// ADMUX =  (0<<REFS1) | (1 << REFS0) | (0 << MUX0); // referencia con condensador y  puerto 1
		asm("STS 0x7C, R16");	// 0x7C = ADMUX 
		ADCSRA |= (1 << ADSC);		
	}
	
	else if (Convercion==1) // precion diferencial 
	{
		//====================================================================
		// Lectura del ACD
		//====================================================================
		
		signal_converted=ADCL;
		converted_signal = ADCH;
		converted_signal = converted_signal << 8;
		converted_signal |=  signal_converted;
		
		//====================================================================
		//conversion result de pascales a cmH20
		//====================================================================
		
		output_signal = converted_signal * 0.00488758;		//Vref = 5V;   5/1023=0.00488758
		Preciondiferencial = (2*10000000*output_signal-139080)/9999339; // calculamos la precion diferencial 
		
		////====================================================================
		// Calculamos la velocidad del fluido por el conducto dencidad del agia 997kg/m3
		//====================================================================
		Velocidad=sqrt(((2*(Preciondiferencial*1000))/997)/15);
		flujovolumetrico=Velocidad * ((3.1416*(0.1))/4); // area de las tuberis 
	
		Convercion=0;
		asm("LDI R16, 0x40");	// ADMUX =  (0<<REFS1) | (1 << REFS0) | (0 << MUX0); // referencia con condensador y  puerto 1
		asm("STS 0x7C, R16");	// 0x7C = ADMUX 
		ADCSRA |= (1 << ADSC);
	}
	
}

ISR(INT0_vect)
{
	if (volumen>=3)
	{
		volumen=1.5;
	}
	
	else 
	{
		volumen=volumen+0.5;
	}
	//Desconpocicion del dato para convertirse en ascii
		x = volumen*100;
		y = x/100;				//Decenas 1
		w = x % 100;
		z = w/10;				//Unidades
		a = w%10;				//decimal
		
		
		//asm("LDI R16, 0x41");	// ADMUX =  (0<<REFS1) | (1 << REFS0) | (0 << MUX0); // referencia con condensador y  puerto 1
		
		entero = conversion_ASCII(y);
		decimal_1 = conversion_ASCII(z);
		decimal_2 = conversion_ASCII(a);
		
		//LCD write Data, virtual_write2
		lcd_write(1,entero, decimal_1, decimal_2);


}

ISR(INT1_vect)
{
	if (volumen<=1.5)
	{
		volumen=3;
	}
	
	else
	{
		volumen=volumen-0.5;
	}
	//Desconpocicion del dato para convertirse en ascii
	x = volumen*100;
	y = x/100;				//Decenas 1
	w = x % 100;
	z = w/10;				//Unidades
	a = w%10;				//decimal
	
	
	//asm("LDI R16, 0x41");	// ADMUX =  (0<<REFS1) | (1 << REFS0) | (0 << MUX0); // referencia con condensador y  puerto 1
	
	entero = conversion_ASCII(y);
	decimal_1 = conversion_ASCII(z);
	decimal_2 = conversion_ASCII(a);
	
	//LCD write Data, virtual_write2
	lcd_write(1,entero, decimal_1, decimal_2);
}

ISR(INT2_vect)
{
	ADCSRA = (0<< ADEN); //Desativar sensado
	PORTL=0x00;
	 stoptodo();//sistema detenido
	
	while (1) // parpadeo
	{
	comando(0x08); // enceder y 
	delay(50000);
	comando(0x0C); // apagar
	delay(50000);
	}
	
	
}

ISR(TIMER1_OVF_vect)
{
	contador++;
	
	if(contador==2)
	{			//Para leer el DHT22 cada 200x10ms = 2000ms y no utilizar retardos bloqueantes de 2s
		contador=0;
		
		uint8_t status = DHT22_read(&temperatura, &humedad);
		if (status)
		{
			x = temperatura*10;
			y = x/100;				//Decenas 1
			w = x % 100;
			z = w/10;				//Unidades
			a = w%10;				//decimal
			
			entero = conversion_ASCII(y);
			decimal_1 = conversion_ASCII(z);
			decimal_2 = conversion_ASCII(a);
			
			lcd_write(2,entero, decimal_1, decimal_2);
			
			x = humedad;
			y = x/100;				//Decenas 1
			w = x % 100;
			z = w/10;				//Unidades
			a = w%10;				//decimal
			
			entero = conversion_ASCII(y);
			decimal_1 = conversion_ASCII(z);
			decimal_2 = conversion_ASCII(a);
			
			lcd_write(3,entero, decimal_1, decimal_2);
		}
	}
}

