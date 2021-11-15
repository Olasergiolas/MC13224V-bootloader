/*****************************************************************************/
/*                                                                           */
/* Sistemas Empotrados                                                       */
/* El "hola mundo" en la Redwire EconoTAG en C                               */
/*                                                                           */
/*****************************************************************************/

#include <stdint.h>
#include "system.h"

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
	*reg_gpio_pad_dir1 = led_red_mask | led_green_mask;
	*reg_gpio_data_set0 = btn_red_o_mask | btn_green_o_mask;
}

/*****************************************************************************/

/*
 * Enciende los leds indicados en la máscara
 * @param mask Máscara para seleccionar leds
 */
void leds_on (uint32_t mask)
{
	/* Encendemos los leds indicados en la máscara */
	*reg_gpio_data_set1 = mask;
}

/*****************************************************************************/

/*
 * Apaga los leds indicados en la máscara
 * @param mask Máscara para seleccionar leds
 */
void leds_off (uint32_t mask)
{
	/* Apagamos los leds indicados en la máscara */
	*reg_gpio_data_reset1 = mask;
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
	const uint32_t data0 = *reg_gpio_data0;
	
	if ((data0 & btn_red_i_mask) != 0)
		return led_red_mask;

	else if ((data0 & btn_green_i_mask) != 0)
		return led_green_mask;

	else
		return last_mask;
}

__attribute__ ((interrupt ("<UNDEF>")))
void undef_handler (void)
{
	leds_on(led_green_mask);
}

/*
 * Máscara del led que se hará parpadear
 */
uint32_t the_led;

/*
 * Programa principal
 */
int main ()
{
	uint32_t if_bits =  excep_disable_ints();
	gpio_init();
	excep_restore_ints(if_bits);
	excep_set_handler(excep_undef, undef_handler);
    the_led = led_red_mask;

	asm(".word 0x26889912\n");
	while (1)
	{
		//the_led = test_buttons(the_led);
		leds_on(the_led);
        pause();

		leds_off(the_led);
		//the_led = test_buttons(the_led);
        pause();
	}
	return 0;
}
/*****************************************************************************/

