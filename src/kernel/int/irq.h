#pragma once

void handle_irq_0(bool* ts)
{
    if (!ts) abort();
    putchar('.');
    *ts = false;
}

void handle_irq_1()
{
    ;
}

void handle_irq_12()
{
    ;
}