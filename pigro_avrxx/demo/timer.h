#ifndef PIGRO_TIMER_H
#define PIGRO_TIMER_H

#include <tiny/io.h>
#include <avrxx/io.h>

class Timer
{
public:

    static inline volatile uint8_t count;

    static void init()
    {
        TCCR0 = tiny::makebits(WGM01);
        TIMSK = tiny::makebits();
    }

    static void start(uint8_t count_to = 250)
    {
        count = 0;
        TCNT0 = 0;
        OCR0 = count_to;
        avr::pin(TIMSK, OCIE0).set(true);
        TCCR0 = tiny::makebits(WGM01, CS01);
    }

    static void stop()
    {
        avr::pin(TIMSK, OCIE0).set(false);
        TCCR0 = tiny::makebits();
    }

    static void isr()
    {
        count++;
    }

};

#endif // PIGRO_TIMER_H
