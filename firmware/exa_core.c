#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include <stdio.h>

/* HP amp */
#define HP_AMP_ADDR 0x60
#define I2C_SDA_PIN 16
#define I2C_SCL_PIN 17

void blink_error(const uint32_t ms) {
    printf("entering blink loop with %d ms blink\n", ms);
    while (1) {
        sleep_ms(ms);
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(ms);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
    }
}

uint8_t adc_read_to_five_bit() {
    /* ratio between 0b111111 and ~0xc46 */
    /* magic 0xc46 is roughly the max voltage on the noir with the pot fully turned (~2.5V) */
    const float conversion_factor = 63.f / 0xc45;
    const float vconversion_factor = 3.3f / (1 << 12);

    const uint16_t result = adc_read();
    /* printf("Raw value: 0x%03x, voltage: %f V\n", result, result * vconversion_factor); */

    /* Prevent blasting audio if the pot is disconnected */
    if (result > 0xc45) {
        return 0;
    }

    if ((int)(result * conversion_factor) >= 0b111111) {
        return 0b111111;
    }

    return (int)(result * conversion_factor) & 0b111111;
}

uint8_t i2c_single_read(uint8_t reg) {
    int ret;
    uint8_t rxdata;
    const uint64_t now = time_us_64();

    i2c_write_blocking_until(i2c_default, HP_AMP_ADDR, &reg, 1, false, now + 10000);
    ret = i2c_read_blocking_until(i2c_default, HP_AMP_ADDR, &rxdata, 1, false, now + 20000);
    if (ret > 0) {
        printf("READ %x from reg %x\n", rxdata, reg);
    } else {
        printf("READ FAIL ON REG %x\n", reg);
        blink_error(250);
    }

    return rxdata;
}

void i2c_single_write(uint8_t reg, uint8_t data) {
    int ret;
    uint8_t txdata[2] = {reg, data};
    const uint64_t now = time_us_64();

    /* ret = i2c_write_blocking(i2c_default, HP_AMP_ADDR, txdata, 2, false); */
    ret = i2c_write_blocking_until(i2c_default, HP_AMP_ADDR, txdata, 2, false, now + 10000);
    if (ret > 0) {
        printf("WROTE %x to reg %x\n", data, reg);
    } else {
        printf("WRITE FAIL ON REG %x\n", reg);
        blink_error(500);
    }
}

void i2c_test() {
    printf("\n\n\n");
    i2c_single_read(1);
    i2c_single_read(2);

    i2c_single_write(1, 0b11000000);
    i2c_single_write(2, 0b00000000);
    /* i2c_single_write(2, 0b00111111); */

    /* printf("read time yo\n"); */
    i2c_single_read(1);
    i2c_single_read(2);
    i2c_single_read(3);
    sleep_ms(10);
}

void hp_loop() {
    i2c_single_write(1, 0b11000000);
    uint8_t old_adc = adc_read_to_five_bit();
    printf("setting volume to %d\n", old_adc);
    i2c_single_write(2, old_adc);

    while (1) {
        const uint8_t new_adc = adc_read_to_five_bit();
        uint8_t diff = 0;
        if (old_adc > new_adc) {
            diff = old_adc - new_adc;
        } else {
            diff = new_adc - old_adc;
        };
        if (diff > 1) {
            printf("setting volume to %d\n", new_adc);
            i2c_single_write(2, new_adc);
            old_adc = new_adc;
        }
        sleep_ms(50);
    }
}

void hp_init() {
    printf("hp_init\n");
    /* HP SD */
    gpio_init(19);
    gpio_set_dir(19, GPIO_OUT);
    gpio_put(19, 1);

    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(I2C_SDA_PIN, I2C_SCL_PIN, GPIO_FUNC_I2C));

    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);
    gpio_pull_down(26);

    sleep_ms(1000);
}

void core1_entry() {
    sleep_ms(500);
    hp_init();
    sleep_ms(500);
    printf("exa_core init done.\n");

    /* i2c_test(); */
    hp_loop();
}

void start_exa_core(void) { multicore_launch_core1(core1_entry); }
