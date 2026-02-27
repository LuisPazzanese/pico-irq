#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define BTN1_PIN   15
#define BTN2_PIN   14
#define BUZZER_PIN 16

#define FREQ1      1000
#define FREQ2      6000
#define TIME1_MS   100
#define TIME2_MS   300
#define SILENCE_MS 50

#define DEBOUNCE_MS 100

volatile bool play_dot  = false;
volatile bool play_dash = false;
static volatile absolute_time_t last_btn1_time;
static volatile absolute_time_t last_btn2_time;

void play_tone(uint32_t freq_hz, uint32_t duration_ms, uint pin) {
    gpio_put(pin, 0);
    sleep_ms(SILENCE_MS);

    uint32_t half_period_us = 1000000 / (2 * freq_hz);
    uint32_t total_us       = duration_ms * 1000;
    uint32_t elapsed_us     = 0;

    while (elapsed_us < total_us) {
        gpio_put(pin, 1);
        sleep_us(half_period_us);
        gpio_put(pin, 0);
        sleep_us(half_period_us);
        elapsed_us += 2 * half_period_us;
    }

    gpio_put(pin, 0);
}

static void gpio_callback(uint gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_FALL))
        return;

    absolute_time_t now = get_absolute_time();

    if (gpio == BTN1_PIN) {
        if (absolute_time_diff_us(last_btn1_time, now) > DEBOUNCE_MS * 1000) {
            last_btn1_time = now;
            play_dot = true;
        }
    } else if (gpio == BTN2_PIN) {
        if (absolute_time_diff_us(last_btn2_time, now) > DEBOUNCE_MS * 1000) {
            last_btn2_time = now;
            play_dash = true;
        }
    }
}

int main(void) {
    stdio_init_all();

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);

    gpio_init(BTN1_PIN);
    gpio_set_dir(BTN1_PIN, GPIO_IN);
    gpio_pull_up(BTN1_PIN);

    gpio_init(BTN2_PIN);
    gpio_set_dir(BTN2_PIN, GPIO_IN);
    gpio_pull_up(BTN2_PIN);

    last_btn1_time = nil_time;
    last_btn2_time = nil_time;

    gpio_set_irq_enabled_with_callback(BTN1_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BTN2_PIN, GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        if (play_dot) {
            play_dot = false;
            play_tone(FREQ1, TIME1_MS, BUZZER_PIN);
        }

        if (play_dash) {
            play_dash = false;
            play_tone(FREQ2, TIME2_MS, BUZZER_PIN);
        }

        tight_loop_contents();
    }

    return 0;
}