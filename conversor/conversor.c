#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "ws2818b.pio.h"
#include "inc/ssd1306.h"

// Definições de pinos
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C  // Endereço do display OLED
#define LED_RED 11
#define LED_GREEN 12
#define LED_BLUE 13
#define JOYSTICK_X 26
#define JOYSTICK_Y 27
#define BUTTON_JOY 22
#define BUTTON_A 5
#define SDA_PIN 14
#define SCL_PIN 15
#define TIME_DEBOUNCE 500  // Tempo de debounce para os botões (em milissegundos)


ssd1306_t ssd;  // Instância do display OLED
PIO np_pio;  // Instância do PIO (para controle de LEDs WS2818B)
uint sm;  // Estado da máquina de estados do PIO

// Variáveis globais
bool led_green_state = false;
bool pwm_enabled = true;
int border_style = 0;

// Callback para o botão do joystick
void button_joy_irq(uint gpio, uint32_t events) {
    static absolute_time_t last_press = {0};
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(last_press, now) < 200000) return;
    last_press = now;
    
    led_green_state = !led_green_state;
    gpio_put(LED_GREEN, led_green_state);
    
    border_style = (border_style + 1) % 3;
}

// Callback para o botão A
void button_a_irq(uint gpio, uint32_t events) {
    static absolute_time_t last_press = {0};
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(last_press, now) < 200000) return;
    last_press = now;
    
    pwm_enabled = !pwm_enabled;
}

void setup_pwm(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 4095);
    pwm_set_enabled(slice, true);
}

void update_pwm(uint pin, uint16_t value) {
    if (pwm_enabled) {
        pwm_set_gpio_level(pin, value);
    } else {
        pwm_set_gpio_level(pin, 0);
    }
}

int main() {
    stdio_init_all();
    
    // Configuração dos LEDs
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    setup_pwm(LED_RED);
    setup_pwm(LED_BLUE);
    
    // Configuração do ADC
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
    
    // Configuração dos botões
    gpio_set_irq_enabled_with_callback(BUTTON_JOY, GPIO_IRQ_EDGE_FALL, true, &button_joy_irq);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_a_irq);
    
    // Configura a interface I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    
    int square_x = 60, square_y = 28;
    
    while (true) {
        adc_select_input(0);
        uint16_t x_val = adc_read();
        adc_select_input(1);
        uint16_t y_val = adc_read();
        
        // Atualiza LEDs
        update_pwm(LED_RED, abs((int)x_val - 2048) * 2);
        update_pwm(LED_BLUE, abs((int)y_val - 2048) * 2);
        
        // Atualiza posição do quadrado
        square_x = (x_val * 112) / 4095;
        square_y = (y_val * 48) / 4095;
        
        // Desenha no display
        ssd1306_clear(&ssd);
        ssd1306_rect(&ssd, square_x, square_y, 8, 8, 1, true); // O retângulo será preenchido com pixels "acesos"
        ssd1306_update(&ssd);
        
        sleep_ms(50);
    }
}
