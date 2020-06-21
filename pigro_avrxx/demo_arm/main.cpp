
#include "stm32f1xx.h"
#include "isr.h"
#include <armxx/pin.h>
#include "uart.h"

using namespace demo;

constexpr auto PACKET_MAXLEN = 6;

constexpr uint8_t PKT_ACK = 1;
constexpr uint8_t PKT_NACK = 2;

struct packet_t
{
    uint8_t cmd;
    uint8_t len;
    uint8_t data[PACKET_MAXLEN];
};

static packet_t pkt;

void send_packet();

static void send_ack()
{
    uart1.write_sync(PKT_ACK);
}

static void send_nack()
{
    uart1.write_sync(PKT_NACK);
}

inline void wait()
{
    for(int i = 0; i < (1000000 /3); i++)
    {
        asm ("nop" ::: "memory");
    }
}

/**
 * Обработка команды seta
 */
static void cmd_seta()
{
    //if ( pkt.len == 1 ) PORTA = pkt.data[0];
    if ( pkt.len == 2 )
    {
        pkt.data[0] = 5;
        pkt.data[1] = 2;
        send_packet();
    }
}

/**
 * Отправить пакет сформированный в переменной ptk
 */
void send_packet()
{
    uart1.write_sync(pkt.cmd);
    uart1.write_sync(pkt.len);
    int i;
    for(i = 0; i < pkt.len; i++)
    {
        uart1.write_sync(pkt.data[i]);
    }
}

/**
 * Обработка команд
 */
static void handle_packet()
{
    switch( pkt.cmd )
    {
    case 1:
        cmd_seta();
        return;
    }
}

/**
 * Чтение пакета в блокируещем режиме и его обработка
 */
static void read_packet()
{
    int i;
    uart1.read_sync(&pkt.cmd);
    uart1.read_sync(&pkt.len);

    if ( pkt.len > PACKET_MAXLEN )
    {
        uint8_t dummy;
        send_nack();
        for(i = 0; i < pkt.len; i++) uart1.read_sync(&dummy);
        return;
    }

    for(i = 0; i < pkt.len; i++)
    {
        uart1.read_sync(&pkt.data[i]);
    }
    send_ack();

    handle_packet();
}

class led_pin
{
   arm::output_pin pin; // {GPIOA, 0};

public:

    led_pin(): pin(GPIOA, 0) {
        //GPIOB->ODR = 2;
    }

    void enable() const { pin.disable(); }
    void disable() const { pin.enable(); }
    void set(bool value) const { pin.write(!value); }

};

static bool state;

void SysTick_Handler()
{
    state = !state;
    led_pin().set(state);

    //UART1::write(state);

    /*if ( uart1.write(state) )
    {

    }*/

    /*
    pkt.cmd = 0xAF;
    pkt.len = 1;
    pkt.data[0] = state;
    send_packet();
    */
}

constexpr uint32_t GPIO_CR_DEFAULT = 0x44444444;

static int main_loop()
{
    // enable GPIOA port
    RCC->APB2ENR = tiny::makebits(2, 14);

    // --- GPIO setup ----
    GPIOA->CRL = (GPIO_CR_DEFAULT & ~0xF) | 0x2;
    GPIOA->CRH = (GPIO_CR_DEFAULT & ~(0xFF<<4)) | (0b1010 << 4) | (0b1000 << 8);

    SysTick->LOAD = 1000000 * 4;
    SysTick->VAL = 0;
    SysTick->CTRL = tiny::makebits(SysTick_CTRL_CLKSOURCE_Pos, SysTick_CTRL_TICKINT_Pos, SysTick_CTRL_ENABLE_Pos);

    demo::uart1.clear();
    demo::UART1::init();

    tiny::interrupt_enable();
    NVIC_EnableIRQ(USART1_IRQn);

    state = true;

    led_pin().set(state);

    while ( true )
    {
        //tiny::sleep();

        read_packet();

        /*
        led.disable();
        tiny::sleep();
        //wait();


        led.enable();
        wait();
        */
    }

    /*

    while ( 1 )
    {

        led.enable();
        wait();

        led.disable();
        wait();

    }
    */

}

extern "C" uint32_t _sidata;
extern "C" uint32_t _sdata;
extern "C" uint32_t _edata;
extern "C" uint32_t _sbss;
extern "C" uint32_t _ebss;

extern "C" void init_data()
{
    const uint32_t size = &_edata - &_sdata;
    for(uint32_t i = 0; i < size; i++)
    {
        (&_sdata)[i] = (&_sidata)[i];
    }

    for(uint32_t *dst = &_sbss; dst < &_ebss; dst++)
    {
        *dst = 0;
    }
}

void Reset_Handler()
{
    init_data();
    main_loop();
    InfiniteLoop();
}
