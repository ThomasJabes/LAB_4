//*********************************************************************
// Universidad del Valle de Guatemala
// IE2023: Programación de Microcontroladores
// Author : Thomas Solis
// Proyecto: Pos - Laboratorio 4
// Descripción: Contador de 8 bits y configurar un canal del ADC.
// Hardware: ATmega328p
// Created: 31/03/2025
//*********************************************************************

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// Variables globales
int valorADC = 0;
int contador = 0;

// Tabla HEX para display de 7 segmentos
const uint8_t tablaHex[] = {0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B, 0x77, 0x1F, 0x4E, 0x3D, 0x4F, 0x47 };

// Inicializar ADC para canal ADC6 (A6)
void ADC_init() {
	ADMUX = 0;
	ADMUX |= (1 << REFS0);                           // Referencia AVcc
	ADMUX |= (1 << ADLAR);                           // Justificación a la izquierda
	ADMUX |= (1 << MUX2) | (1 << MUX1);              // Canal ADC6

	ADCSRA = 0;
	ADCSRA |= (1 << ADEN);                           // Habilitar ADC
	ADCSRA |= (1 << ADIE);                           // Habilitar interrupción
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128

	#ifdef ADC6D
	DIDR0 |= (1 << ADC6D);                           // Deshabilitar entrada digital
	#endif
}

// Mostrar el valor del contador en LEDs
void mostrar_contador(uint8_t valor) {
	PORTB &= ~((1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5));
	PORTD &= ~(1 << PD7);
	PORTC &= ~((1 << PC2) | (1 << PC3) | (1 << PC4));

	if (valor & (1 << 0)) PORTB |= (1 << PB4);
	if (valor & (1 << 1)) PORTB |= (1 << PB3);
	if (valor & (1 << 2)) PORTB |= (1 << PB2);
	if (valor & (1 << 3)) PORTD |= (1 << PD7);
	if (valor & (1 << 4)) PORTB |= (1 << PB5);
	if (valor & (1 << 5)) PORTC |= (1 << PC2);
	if (valor & (1 << 6)) PORTC |= (1 << PC3);
	if (valor & (1 << 7)) PORTC |= (1 << PC4);
}

// Mostrar valor del ADC en displays HEX
void mostrar_display_hex(uint8_t valor) {
	uint8_t digitoBajo = valor & 0x0F;
	uint8_t digitoAlto = (valor >> 4) & 0x0F;

	uint8_t estadoPD7 = PORTD & (1 << PD7);  // Conservar PD7

	// Mostrar dígito alto
	PORTD = estadoPD7 | tablaHex[digitoAlto];
	PORTB |= (1 << PB0);
	_delay_ms(5);
	PORTB &= ~(1 << PB0);

	// Mostrar dígito bajo
	PORTD = estadoPD7 | tablaHex[digitoBajo];
	PORTB |= (1 << PB1);
	_delay_ms(5);
	PORTB &= ~(1 << PB1);
}

int main(void) {
	// Configurar salidas
	DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5);
	DDRD |= 0xFF;  // PD0–PD7
	DDRC |= (1 << PC2) | (1 << PC3) | (1 << PC4) | (1 << PC5); // PC5: LED de alarma

	// Entradas con pull-up para botones PC0 y PC1
	DDRC &= ~((1 << PC0) | (1 << PC1));
	PORTC |= (1 << PC0) | (1 << PC1);

	// Interrupciones por cambio de pin (PC0 y PC1)
	PCICR |= (1 << PCIE1);
	PCMSK1 |= (1 << PCINT8) | (1 << PCINT9);
	sei();  // Habilitar interrupciones globales

	// Inicializar ADC
	ADC_init();
	ADCSRA |= (1 << ADSC);  // Iniciar primera conversión

	uint8_t alternar = 0;

	while (1) {
		// Protección de límites
		if (contador > 255) contador = 0;
		if (contador < 0) contador = 255;

		// Mostrar contador o display en turnos
		if (alternar == 0) {
			mostrar_contador(contador);
			} else {
			mostrar_display_hex(valorADC);
		}

		alternar ^= 1;  // Alternar entre contador y display

		//  Comparar ADC con contador y encender LED de alarma en PC5
		if (valorADC > contador) {
			PORTC |= (1 << PC5);   // Enciende LED de alarma
			} else {
			PORTC &= ~(1 << PC5);  // Apaga LED de alarma
		}
	}
}

// ISR del ADC
ISR(ADC_vect) {
	valorADC = ADCH;              // Leer resultado justificado a la izquierda
	ADCSRA |= (1 << ADSC);        // Iniciar nueva conversión
}

// ISR para botones
ISR(PCINT1_vect) {
	if ((PINC & (1 << PC0)) == 0) {
		contador++;
	}
	if ((PINC & (1 << PC1)) == 0) {
		contador--;
	}
}
