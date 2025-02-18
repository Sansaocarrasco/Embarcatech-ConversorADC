#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C
#define JOYSTICK_X_PIN 26  // GPIO para eixo X
#define JOYSTICK_Y_PIN 27  // GPIO para eixo Y
#define BUTTON_JOY_PIN 22  // GPIO para botão do Joystick
#define BUTTON_A_PIN 5     // GPIO para botão A

// Definições de pinos dos LEDs
#define LED_RED_PIN 11
#define LED_GREEN_PIN 12
#define LED_BLUE_PIN 13

// Definir o tipo de borda (3 estilos)
#define BORDER_STYLE_1 0
#define BORDER_STYLE_2 1
#define BORDER_STYLE_3 2

// Variáveis globais
bool led_green_state = false;
bool pwm_enabled = true;
int border_style = BORDER_STYLE_1;
ssd1306_t ssd;  // Instância do display OLED

// Função para configurar o PWM nos LEDs
void setup_pwm(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 4095); // Define o valor máximo de PWM
    pwm_set_enabled(slice, true);
}

// Função para atualizar o PWM (intensidade dos LEDs)
void update_pwm(uint pin, uint16_t value) {
    if (pwm_enabled) {
        pwm_set_gpio_level(pin, value);
    } else {
        pwm_set_gpio_level(pin, 0);
    }
}

// Callback para o botão do joystick
void button_joy_irq(uint gpio, uint32_t events) {
    static absolute_time_t last_press = {0};
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(last_press, now) < 200000) return;
    last_press = now;
    
    // Alterna o estado do LED verde
    led_green_state = !led_green_state;
    gpio_put(LED_GREEN_PIN, led_green_state);
    
    // Modifica o estilo da borda
    border_style = (border_style + 1) % 3;
}

// Callback para o botão A
void button_a_irq(uint gpio, uint32_t events) {
    static absolute_time_t last_press = {0};
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(last_press, now) < 200000) return;
    last_press = now;
    
    // Alterna o estado de PWM dos LEDs
    pwm_enabled = !pwm_enabled;
}

int main() {
    stdio_init_all();
    
    // Configuração dos LEDs
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    setup_pwm(LED_RED_PIN);
    setup_pwm(LED_BLUE_PIN);
    
    // Configuração dos botões
    gpio_set_irq_enabled_with_callback(BUTTON_JOY_PIN, GPIO_IRQ_EDGE_FALL, true, &button_joy_irq);
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &button_a_irq);
    
    // Configuração do display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    
    // Configuração do ADC para o joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);
    
    uint16_t adc_value_x;
    uint16_t adc_value_y;
    int square_x = 60, square_y = 28; // Posição inicial do quadrado
    
    // Ajuste para centralizar 30px a direita e 30px para baixo
    int offset_x = 30;
    int offset_y = 30;
    
    while (true) {
        // Lê os valores dos eixos X e Y do joystick
        adc_select_input(0);
        adc_value_x = adc_read();
        adc_select_input(1);
        adc_value_y = adc_read();
        
        // Atualiza os PWM dos LEDs
        update_pwm(LED_RED_PIN, abs((int)adc_value_x - 2048) * 2);  // LED vermelho controla pelo eixo X
        update_pwm(LED_BLUE_PIN, abs((int)adc_value_y - 2048) * 2); // LED azul controla pelo eixo Y
        
        // Atualiza a posição do quadrado com o ajuste para centralizar
        // Reverter o eixo X e Y
        square_x = offset_x + (127 - ((adc_value_x * 128) / 4095));
        square_y = offset_y + ((adc_value_y * 64) / 4095);

        // Limita as coordenadas para não ultrapassarem os limites da tela
        square_x = (square_x > (127 - 8)) ? (127 - 8) : square_x;  // Limita a direita
        square_x = (square_x < (63 + 8)) ? (63 + 8) : square_x;  // Limita a esquerda

        square_y = (square_y > (127 - 8)) ? (127 - 8) : square_y;  // Limita para baixo
        square_y = (square_y < 8) ? 8 : square_y;  // Limita para cima

        // Limpa o display e desenha o quadrado
        ssd1306_clear(&ssd);
        ssd1306_rect(&ssd, square_x, square_y, 8, 8, 1, true); // Desenha o quadrado
        ssd1306_update(&ssd);
        
        // Desenha a borda, dependendo do estilo
        if (border_style == BORDER_STYLE_1) {
            ssd1306_rect(&ssd, 0, 0, 127, 63, 1, false);  // Borda simples
        } else if (border_style == BORDER_STYLE_2) {
            ssd1306_rect(&ssd, 0, 0, 127, 63, 1, false);  // Borda com maior espessura
            ssd1306_rect(&ssd, 1, 1, 125, 61, 1, false);  // Borda interna
        } else if (border_style == BORDER_STYLE_3) {
            ssd1306_rect(&ssd, 0, 0, 127, 63, 1, true);   // Borda invertida
        }

        sleep_ms(50);  // Delay para evitar atualizações muito rápidas
    }
}
