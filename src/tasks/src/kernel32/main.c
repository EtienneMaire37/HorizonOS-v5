#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include <horizonos.h>

int main()
{
    // long double f = 0.56846956523535265758646564865865489625202582002020254786547854785841236814589874148524586325632563252656;
    // printf("%lf %u %s\n", f, 150, "ABCDEF");

    printf("--- Start of HorizonOS configuration ---\n\n");
    printf("Please enter your preferred keyboard layout:\n");
    printf("1: us_qwerty      2: fr_azerty\n");
    printf("->");
    uint8_t kb_layout_choice = 0;
    char kb_layout_choice_str[2] = { 0 };
    scanf("%1[1-9]", &kb_layout_choice);
    return 0;
}