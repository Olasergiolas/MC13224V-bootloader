/*****************************************************************************/
/*                                                                           */
/* Sistemas Empotrados                                                       */
/* El "hola mundo" en la Redwire EconoTAG en C                               */
/*                                                                           */
/*****************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include "system.h"
#include <string.h>
#include <stdio.h>

/*
 * Constantes relativas a la plataforma
 */

/* Dirección del registro de control de dirección del GPIO32-GPIO63 */
volatile uint32_t * const reg_gpio_pad_dir1    = (uint32_t *) 0x80000004;

/* Dirección del registro de activación de bits del GPIO32-GPIO63 */
volatile uint32_t * const reg_gpio_data_set1   = (uint32_t *) 0x8000004c;

volatile uint32_t * const reg_gpio_data_set0   = (uint32_t *) 0x80000048;

volatile uint32_t * const reg_gpio_data0   = (uint32_t *) 0x80000008;

/* Dirección del registro de limpieza de bits del GPIO32-GPIO63 */
volatile uint32_t * const reg_gpio_data_reset1 = (uint32_t *) 0x80000054;

/* El led rojo está en el GPIO 44 (el bit 12 de los registros GPIO_X_1) */
uint32_t const led_red_mask = (1 << 12);
uint32_t const led_green_mask = (1 << 13);

uint32_t const btn_red_o_mask = (1 << 23);
uint32_t const btn_red_i_mask = (1 << 27);

uint32_t const btn_green_o_mask = (1 << 22);
uint32_t const btn_green_i_mask = (1 << 26);

/*
 * Constantes relativas a la aplicacion
 */
uint32_t const delay = 0x10000;
 
/*****************************************************************************/

/*
 * Inicialización de los pines de E/S
 */
void gpio_init(void)
{
	/* Configuramos el GPIO44 para que sea de salida */
	gpio_set_pin_dir_output(gpio_pin_44);
	gpio_set_pin_dir_output(gpio_pin_45);
	gpio_set_pin(gpio_pin_23);
	gpio_set_pin(gpio_pin_22);
}

/*****************************************************************************/

/*
 * Enciende los leds indicados en la máscara
 * @param mask Máscara para seleccionar leds
 */
void leds_on (uint32_t mask)
{
	/* Encendemos los leds indicados en la máscara */
 	gpio_set_port(gpio_port_1, mask);

}

/*****************************************************************************/

/*
 * Apaga los leds indicados en la máscara
 * @param mask Máscara para seleccionar leds
 */
void leds_off (uint32_t mask)
{
	/* Apagamos los leds indicados en la máscara */
	gpio_clear_port(gpio_port_1, mask);
}

/*****************************************************************************/

/*
 * Retardo para el parpedeo
 */
void pause(void)
{
    uint32_t i;
	for (i=0 ; i<delay ; i++);
}

/*****************************************************************************/


uint32_t const test_buttons(uint32_t last_mask){	
	uint32_t* status = malloc(sizeof(int));
	gpio_get_port(gpio_port_0, status);

	if ((*status & btn_red_i_mask) != 0)
		return led_red_mask;

	else if ((*status & btn_green_i_mask) != 0)
		return led_green_mask;

	else
		return last_mask;
}

__attribute__ ((interrupt ("UNDEF")))
void undef_handler (void)
{
	leds_on(led_green_mask);
}

void int_asm_handler (void)
{
	itc_unforce_interrupt(itc_src_asm);
	leds_on(led_green_mask);
}

/*
 * Máscara del led que se hará parpadear
 */
uint32_t the_led;
uint8_t red_blinking = 1;
uint8_t green_blinking = 1;
uart_id_t uart = uart_1;
char error[] = "Uso: r(ed) g(reen)\n\r";

void uart_rx_callback(void){
	char c = '\0'; 
	c = getchar();

	if (c == 'r')
		red_blinking = !red_blinking;
	
	else if (c == 'g')
		green_blinking = !green_blinking;

	else if (c != '\0')
		printf(error);
}

/*
 * Programa principal
 */
int main ()
{
	gpio_init();
	uart_set_receive_callback(uart, uart_rx_callback);

	while (1)
	{
		pause();
		if (red_blinking)
			leds_on(led_red_mask);

		if (green_blinking)
			leds_on(led_green_mask);

		pause();
		leds_off(led_red_mask);
		leds_off(led_green_mask);
	}
	return 0;
}
/*****************************************************************************/

