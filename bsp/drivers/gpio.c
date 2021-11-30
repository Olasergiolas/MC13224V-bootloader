/*
 * Sistemas operativos empotrados
 * Driver para el GPIO del MC1322x
 */

#include "system.h"
#include <stdbool.h>

/*****************************************************************************/

/**
 * Acceso estructurado a los registros de control del gpio del MC1322x
 */
typedef struct
{
	uint32_t GPIO_PAD_DIR[2];
	uint32_t GPIO_DATA[2];
	uint32_t GPIO_PAD_PU_EN[2];
	uint32_t GPIO_FUNC_SEL_0_1[2];
	uint32_t GPIO_FUNC_SEL_2_3[2];
	uint32_t GPIO_DATA_SEL[2];
	uint32_t GPIO_PAD_PU_SEL[2];
	uint32_t GPIO_PAD_HYST_EN[2];
	uint32_t GPIO_PAD_KEEP[2];
	uint32_t GPIO_DATA_SET[2];
	uint32_t GPIO_DATA_RESET[2];
	uint32_t GPIO_PAD_DIR_SET[2];
	uint32_t GPIO_PAD_DIR_RESET[2];

} gpio_regs_t;

static volatile gpio_regs_t* const gpio_regs = GPIO_BASE;

bool check_gpio_port(gpio_port_t port){
	return (port < gpio_port_max);
}

bool check_gpio_pin(gpio_pin_t pin){
	return (pin < gpio_pin_max);
}

/*****************************************************************************/

/**
 * Fija la dirección de los pines seleccionados en la máscara como de entrada
 *
 * @param 	port 	Puerto
 * @param 	mask 	Máscara para seleccionar los pines
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_set_port_dir_input (gpio_port_t port, uint32_t mask)
{
	gpio_regs->GPIO_PAD_DIR_RESET[port] = mask;

	return check_gpio_port(port) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/

/**
 * Fija la dirección de los pines seleccionados en la máscara como de salida
 *
 * @param	port 	Puerto
 * @param	mask 	Máscara para seleccionar los pines
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_set_port_dir_output (gpio_port_t port, uint32_t mask)
{
	gpio_regs->GPIO_PAD_DIR_SET[port] = mask;

	return check_gpio_port(port) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/

/**
 * Fija la dirección del pin indicado como de entrada
 *
 * @param	pin 	Número de pin
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_set_pin_dir_input (gpio_pin_t pin)
{
	gpio_regs->GPIO_PAD_DIR_RESET[(pin >> 5) & 1] = 1 << (pin & 0x1F);

	return check_gpio_pin(pin) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/

/**
 * Fija la dirección del pin indicado como de salida
 *
 * @param	pin 	Número de pin
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_set_pin_dir_output (gpio_pin_t pin)
{
	gpio_regs->GPIO_PAD_DIR_SET[(pin >> 5) & 1] = 1 << (pin & 0x1F);

	return check_gpio_pin(pin) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/

/**
 * Escribe unos en los pines seleccionados en la máscara
 *
 * @param	port 	Puerto
 * @param	mask 	Máscara para seleccionar los pines
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_set_port (gpio_port_t port, uint32_t mask)
{
	gpio_regs->GPIO_DATA_SET[port] = mask;

	return check_gpio_port(port) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/

/**
 * Escribe ceros en los pines seleccionados en la máscara
 *
 * @param	port 	Puerto
 * @param	mask 	Máscara para seleccionar los pines
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_clear_port (gpio_port_t port, uint32_t mask)
{
	gpio_regs->GPIO_DATA_RESET[port] = mask;

	return check_gpio_port(port) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/

/**
 * Escribe un uno en el pin indicado
 *
 * @param	pin 	Número de pin
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_set_pin (gpio_pin_t pin)
{
	gpio_regs->GPIO_DATA_SET[(pin >> 5) & 1] = 1 << (pin & 0x1F);

	return check_gpio_pin(pin) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/

/**
 * Escribe un cero en el pin indicado
 *
 * @param	pin 	Número de pin
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_clear_pin (gpio_pin_t pin)
{
	gpio_regs->GPIO_DATA_RESET[(pin >> 5) & 1] = 1 << (pin & 0x1F);

	return check_gpio_pin(pin) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/

/**
 * Lee el valor de los pines de un puerto
 *
 * @param	port	  Puerto
 * @param	port_data Valor de los pines del puerto
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			  gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_get_port (gpio_port_t port, uint32_t *port_data)
{
	*port_data = gpio_regs->GPIO_DATA[port];

	return check_gpio_port(port) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/

/**
 * Lee el valor del pin indicado
 *
 * @param	pin	  Número de pin
 * @param       pin_data  Cero si el pin está a cero, distinto de cero en otro caso
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			  gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_get_pin (gpio_pin_t pin, uint32_t *pin_data)
{
	*pin_data = (gpio_regs->GPIO_DATA[(pin >> 5) & 1] >> (pin & 0x1F)) & 1;

	return check_gpio_pin(pin) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/

/**
 * Asigna un modo de funcionamiento a los pines seleccionados por la máscara
 *
 * @param   port    Puerto
 * @param   func    Modo de funcionamiento
 * @param	mask	Máscara para seleccionar los pines
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_set_port_func (gpio_port_t port, gpio_func_t func, uint32_t mask)
{
	gpio_pin_t pin;
	for (unsigned i = 0; i < 32; ++i){
		if ((mask >> i) & 1){
			pin = (port == gpio_port_1 ? (i + 0x20) : i);
			gpio_set_pin_func(pin, func);
		}
	}

	return check_gpio_port(port) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/

/**
 *  Asigna un modo de funcionamiento al pin seleccionado
 *
 * @param	pin 	Pin
 * @param	func	Modo de funcionamiento
 * @return	gpio_no_error si los parámetros de entrada son corectos o
 *			gpio_invalid_parameter en otro caso
 */
inline gpio_err_t gpio_set_pin_func (gpio_pin_t pin, gpio_func_t func)
{
	uint32_t offset = (pin & 0xF) << 1;
	uint32_t port = (pin >> 5) & 1;

	if (port == gpio_port_0){
		uint32_t half = (pin <= 15 ? 0 : 1);
		gpio_regs->GPIO_FUNC_SEL_0_1[half] &= ~(3 << offset);
		gpio_regs->GPIO_FUNC_SEL_0_1[half] |= (func << offset);
	}

	else if (port == gpio_port_1){
		uint32_t half = (pin <= 47 ? 0 : 1);
		gpio_regs->GPIO_FUNC_SEL_2_3[half] &= ~(3 << offset);
		gpio_regs->GPIO_FUNC_SEL_2_3[half] |= (func << offset);
	}

	return check_gpio_pin(pin) ? gpio_no_error : gpio_invalid_parameter;
}

/*****************************************************************************/
