/*
 * Sistemas operativos empotrados
 * Driver de las uart
 */

#include <fcntl.h>
#include <errno.h>
#include "system.h"
#include "circular_buffer.h"
#include "sys/types.h"

/*****************************************************************************/

/**
 * Acceso estructurado a los registros de control de las uart del MC1322x
 */

typedef struct
{
	union
	{
		struct
		{
			uint32_t TxE		:1;
			uint32_t RxE		:1;
			uint32_t PEN		:1;
			uint32_t EP			:1;
			uint32_t ST2		:1;
			uint32_t SB 		:1;
			uint32_t conTx		:1;
			uint32_t Tx_oen_b	:1;
			uint32_t 			:2;
			uint32_t xTIM		:1;
			uint32_t FCp		:1;
			uint32_t FCe		:1;
			uint32_t mTxR 		:1;
			uint32_t mRxR 		:1;
			uint32_t TST 		:1;

		};
		uint32_t UCON;
	};

	union
	{
		struct
		{
			uint32_t SE		:1;
			uint32_t PE		:1;
			uint32_t FE		:1;
			uint32_t TOE	:1;
			uint32_t ROE	:1;
			uint32_t RUE 	:1;
			uint32_t RxRdy	:1;
			uint32_t TxRdy	:1;
		};
		uint32_t USTAT;
	};

	union
	{
		uint8_t Rx_data;
		uint8_t Tx_data;
		uint32_t UDATA;
	};

	union
	{
		uint32_t RxLevel			:5;
		uint32_t Rx_fifo_addr_diff	:6;
		uint32_t URxCON;
	};

	union
	{
		uint32_t TxLevel			:5;
		uint32_t Tx_fifo_addr_diff	:6;
		uint32_t UTxCON;
	};

	uint32_t UCTS;

	union
	{
		struct
		{
			uint32_t BRMOD	:16;
			uint32_t BRINC 	:16;
		};
		uint32_t BR;
	};
} uart_regs_t;

/*****************************************************************************/

/**
 * Acceso estructurado a los pines de las uart del MC1322x
 */
typedef struct
{
	gpio_pin_t tx,rx,cts,rts;
} uart_pins_t;

/*****************************************************************************/

/**
 * Definición de las UARTS
 */
static volatile uart_regs_t* const uart_regs[uart_max] = {UART1_BASE, UART2_BASE};

static const uart_pins_t uart_pins[uart_max] = {
		{gpio_pin_14, gpio_pin_15, gpio_pin_16, gpio_pin_17},
		{gpio_pin_18, gpio_pin_19, gpio_pin_20, gpio_pin_21} };

static void uart_1_isr (void);
static void uart_2_isr (void);
static const itc_handler_t uart_irq_handlers[uart_max] = {uart_1_isr, uart_2_isr};

/*****************************************************************************/

/**
 * Tamaño de los búferes circulares
 */
#define __UART_BUFFER_SIZE__	256

static volatile uint8_t uart_rx_buffers[uart_max][__UART_BUFFER_SIZE__];
static volatile uint8_t uart_tx_buffers[uart_max][__UART_BUFFER_SIZE__];

static volatile circular_buffer_t uart_circular_rx_buffers[uart_max];
static volatile circular_buffer_t uart_circular_tx_buffers[uart_max];


/*****************************************************************************/

/**
 * Gestión de las callbacks
 */
typedef struct
{
	uart_callback_t tx_callback;
	uart_callback_t rx_callback;
} uart_callbacks_t;

static volatile uart_callbacks_t uart_callbacks[uart_max];

/*****************************************************************************/

/**
 * Inicializa una uart
 * @param uart	Identificador de la uart
 * @param br	Baudrate
 * @param name	Nombre del dispositivo
 * @return		Cero en caso de éxito o -1 en caso de error.
 * 				La condición de error se indica en la variable global errno
 */
int32_t uart_init (uart_id_t uart, uint32_t br, const char *name)
{
	/* ESTA FUNCIÓN SE DEFINIRÁ EN LAS PRÁCTICAS 8, 9 y 10 */
	if (uart >= uart_max){
		errno = ENODEV;
		return -1;
	}

	else if (name == 0){
		errno = EFAULT;
		return -1;
	}

	uint32_t mod = 9999;
	uint32_t inc = br * mod / (CPU_FREQ >> 4);
	
	uart_regs[uart]->UCON = (1 << 13) | (1 << 14);
	uart_regs[uart]->TxE = 0;
	uart_regs[uart]->RxE = 0;
	
	uart_regs[uart]->BR = ( inc << 16 ) | mod;

	uart_regs[uart]->UCON |= (1 << 0) | (1 << 1);
	
	gpio_set_pin_func(uart_pins[uart].tx, gpio_func_alternate_1);
	gpio_set_pin_func(uart_pins[uart].rx, gpio_func_alternate_1);
	gpio_set_pin_func(uart_pins[uart].cts, gpio_func_alternate_1);
	gpio_set_pin_func(uart_pins[uart].rts, gpio_func_alternate_1);

	gpio_set_pin_dir_output(uart_pins[uart].tx);
	gpio_set_pin_dir_output(uart_pins[uart].cts);
	gpio_set_pin_dir_input(uart_pins[uart].rx);
	gpio_set_pin_dir_input(uart_pins[uart].rts);

	circular_buffer_init(&uart_circular_rx_buffers[uart], (uint8_t *) uart_rx_buffers,
		sizeof(uart_rx_buffers[uart]));

	circular_buffer_init(&uart_circular_tx_buffers[uart], (uint8_t *) uart_tx_buffers,
		sizeof(uart_tx_buffers[uart]));

	uart_regs[uart]->RxLevel = 1;
	uart_regs[uart]->TxLevel = 31;

	itc_set_priority(itc_src_uart1 + uart, itc_priority_normal);
	itc_set_handler(itc_src_uart1 + uart, uart_irq_handlers[uart]);
	itc_enable_interrupt(itc_src_uart1 + uart);

	uart_callbacks[uart].rx_callback = 0;
	uart_callbacks[uart].tx_callback = 0;

	uart_regs[uart]->mRxR = 0;

	bsp_register_dev(name, uart, 0, 0, uart_receive, uart_send, 0, 0, 0);
	return 0;
}

/*****************************************************************************/

/**
 * Transmite un byte por la uart
 * Implementación del driver de nivel 0. La llamada se bloquea hasta que transmite el byte
 * @param uart	Identificador de la uart
 * @param c		El carácter
 */
void uart_send_byte (uart_id_t uart, uint8_t c)
{
	uint32_t prev_status = uart_regs[uart]->mTxR;
	uart_regs[uart]->mTxR = 1;
	uint32_t buffer_c = circular_buffer_read(&uart_circular_tx_buffers[uart]);

	while (buffer_c != -1){
		while(uart_regs[uart]->Tx_fifo_addr_diff == 0);
		uart_regs[uart]->Tx_data = buffer_c;
		buffer_c = circular_buffer_read(&uart_circular_tx_buffers[uart]);
	}

	while(uart_regs[uart]->Tx_fifo_addr_diff == 0);
	uart_regs[uart]->Tx_data = c;

	uart_regs[uart]->mTxR = prev_status;
}


/*****************************************************************************/

/**
 * Recibe un byte por la uart
 * Implementación del driver de nivel 0. La llamada se bloquea hasta que recibe el byte
 * @param uart	Identificador de la uart
 * @return		El byte recibido
 */
uint8_t uart_receive_byte (uart_id_t uart)
{
	uint32_t prev_status = uart_regs[uart]->mRxR;
	uart_regs[uart]->mRxR = 1;
	uint8_t read_byte = 0;

	if (!circular_buffer_is_empty(&uart_circular_rx_buffers[uart]))
		read_byte = circular_buffer_read(&uart_circular_rx_buffers[uart]);

	else{
		while(uart_regs[uart]->Rx_fifo_addr_diff == 0);
		read_byte = uart_regs[uart]->Rx_data;
	}

	uart_regs[uart]->mRxR = prev_status;
	return read_byte;
}

/*****************************************************************************/

/**
 * Transmisión de bytes
 * Implementación del driver de nivel 1. La llamada es no bloqueante y se realiza mediante interrupciones
 * @param uart	Identificador de la uart
 * @param buf	Búfer con los caracteres
 * @param count	Número de caracteres a escribir
 * @return	El número de bytes almacenados en el búfer de transmisión en caso de éxito o
 *              -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
ssize_t uart_send (uint32_t uart, char *buf, size_t count)
{
	if (uart >= uart_max){
		errno = ENODEV;
		return -1;
	}

	else if (buf == 0 || count <= 0){
		errno = EFAULT;
		return -1;
	}

	uint32_t prev_status = uart_regs[uart]->mTxR;
	uart_regs[uart]->mTxR = 1;

	uint32_t i = 0;
	while(!circular_buffer_is_full(&uart_circular_tx_buffers[uart]) && count > 0){
		circular_buffer_write(&uart_circular_tx_buffers[uart], buf[i]);
		i++;
		count--;
	}

	if (i > 0)
		uart_regs[uart]->mTxR = 0;
	else
		uart_regs[uart]->mTxR = prev_status;
		
	return i;
}

/*****************************************************************************/

/**
 * Recepción de bytes
 * Implementación del driver de nivel 1. La llamada es no bloqueante y se realiza mediante interrupciones
 * @param uart	Identificador de la uart
 * @param buf	Búfer para almacenar los bytes
 * @param count	Número de bytes a leer
 * @return	El número de bytes realmente leídos en caso de éxito o
 *              -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
ssize_t uart_receive (uint32_t uart, char *buf, size_t count)
{
	if (uart >= uart_max){
		errno = ENODEV;
		return -1;
	}

	else if (buf == 0 || count <= 0){
		errno = EFAULT;
		return -1;
	}

	uint32_t prev_status = uart_regs[uart]->mRxR;
	uart_regs[uart]->mRxR = 1;

	uint32_t i = 0;
	while(!circular_buffer_is_empty(&uart_circular_rx_buffers[uart]) && count > 0){
		buf[i] = circular_buffer_read(&uart_circular_rx_buffers[uart]);
		i++;
		count--;
	}
	
	uart_regs[uart]->mRxR = prev_status;
	return i;
}

/*****************************************************************************/

/**
 * Fija la función callback de recepción de una uart
 * @param uart	Identificador de la uart
 * @param func	Función callback. NULL para anular una selección anterior
 * @return	Cero en caso de éxito o -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
int32_t uart_set_receive_callback (uart_id_t uart, uart_callback_t func)
{
	if (uart >= uart_max){
		errno = ENODEV;
		return -1;
	}

	else if (func == 0){
		errno = EFAULT;
		return -1;
	}

	uart_callbacks[uart].rx_callback = func;
	return 0;
}

/*****************************************************************************/

/**
 * Fija la función callback de transmisión de una uart
 * @param uart	Identificador de la uart
 * @param func	Función callback. NULL para anular una selección anterior
 * @return	Cero en caso de éxito o -1 en caso de error.
 * 		La condición de error se indica en la variable global errno
 */
int32_t uart_set_send_callback (uart_id_t uart, uart_callback_t func)
{
	if (uart >= uart_max){
		errno = ENODEV;
		return -1;
	}

	else if (func == 0){
		errno = EFAULT;
		return -1;
	}

	uart_callbacks[uart].tx_callback = func;
	return 0;
}

/*****************************************************************************/

/**
 * Manejador genérico de interrupciones para las uart.
 * Cada isr llamará a este manejador indicando la uart en la que se ha
 * producido la interrupción.
 * Lo declaramos inline para reducir la latencia de la isr
 * @param uart	Identificador de la uart
 */
static inline void uart_isr (uart_id_t uart)
{
	uint32_t status = uart_regs[uart]->USTAT;

	if (uart_regs[uart]->RxRdy){
		while(uart_regs[uart]->Rx_fifo_addr_diff > 0 && 
			!circular_buffer_is_full(&uart_circular_rx_buffers[uart]))
				circular_buffer_write(&uart_circular_rx_buffers[uart], uart_regs[uart]->Rx_data);

		if (uart_callbacks[uart].rx_callback)
			uart_callbacks[uart].rx_callback();

		if (circular_buffer_is_full(&uart_circular_rx_buffers[uart]))
			uart_regs[uart]->mRxR = 1;
	}

	if (uart_regs[uart]->TxRdy){
		while(!circular_buffer_is_empty(&uart_circular_tx_buffers[uart]) &&
			uart_regs[uart]->Tx_fifo_addr_diff > 0)
				uart_regs[uart]->Tx_data = circular_buffer_read(&uart_circular_tx_buffers[uart]);

			if (uart_callbacks[uart].tx_callback)
				uart_callbacks[uart].tx_callback();

			if (circular_buffer_is_empty(&uart_circular_tx_buffers[uart]))
				uart_regs[uart]->mTxR = 1;
	}
}

/*****************************************************************************/

/**
 * Manejador de interrupciones para la uart1
 */
static void uart_1_isr (void)
{
	uart_isr(uart_1);
}

/*****************************************************************************/

/**
 * Manejador de interrupciones para la uart2
 */
static void uart_2_isr (void)
{
	uart_isr(uart_2);
}

/*****************************************************************************/
