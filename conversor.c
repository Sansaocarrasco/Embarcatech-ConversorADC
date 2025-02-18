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
#define LED_RED_PIN 13
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN 12
#define LED_DEBUG_PIN 25  // LED para debugging

// Definir o valor máximo de desvio para considerar o joystick como "no centro"
#define JOYSTICK_DEADZONE 100  // Ajuste esse valor conforme necessário

// Variáveis globais
bool pwm_enabled = true;
bool leds_on = true;  // Estado dos LEDs (ligados ou desligados)
bool green_led_state = false;  // Estado do LED verde
int border_style = 0;  // Estilo da borda (0: sólido, 1: pontilhado, 2: sem borda)
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

// Callback para o botão A
void button_a_irq(uint gpio, uint32_t events) {
    static absolute_time_t last_press = {0};
    absolute_time_t now = get_absolute_time();
    
    // Debounce: se o botão for pressionado novamente em menos de 200ms, ignora
    if (absolute_time_diff_us(last_press, now) < 200000) return;  
    last_press = now;
    
    // Desliga os LEDs RGB (controlados por PWM)
    update_pwm(LED_RED_PIN, 0);  // Desliga o LED vermelho
    update_pwm(LED_GREEN_PIN, 0);  // Desliga o LED verde
    update_pwm(LED_BLUE_PIN, 0);  // Desliga o LED azul
    
    pwm_enabled = false;  // Desabilita o controle PWM dos LEDs

    // Também liga o LED de debug para indicar que os LEDs RGB estão desligados
    gpio_put(LED_DEBUG_PIN, 1);
    
    // Desliga o controle de LEDs
    leds_on = false;  // Os LEDs estão desligados agora
}

// Callback para o botão do joystick
void button_joy_irq(uint gpio, uint32_t events) {
    static absolute_time_t last_press = {0};
    absolute_time_t now = get_absolute_time();
    
    // Debounce: se o botão for pressionado novamente em menos de 200ms, ignora
    if (absolute_time_diff_us(last_press, now) < 200000) return;  
    last_press = now;
    
    // Alterna o LED verde
    green_led_state = !green_led_state;  // Alterna o estado do LED verde
    gpio_put(LED_GREEN_PIN, green_led_state ? 1 : 0);  // Liga ou desliga o LED verde
    
    // Modifica o estilo da borda
    border_style = (border_style + 1) % 3;  // Alterna entre 3 estilos de borda (sólido, pontilhado, sem borda)
}

int main() {
    stdio_init_all();
    
    // Configuração dos LEDs
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    setup_pwm(LED_RED_PIN);
    setup_pwm(LED_BLUE_PIN);
    gpio_init(LED_DEBUG_PIN);
    gpio_set_dir(LED_DEBUG_PIN, GPIO_OUT);  // LED de debug para visualização do estado do botão

    gpio_pull_up(BUTTON_A_PIN); // Configura o resistor pull-up
    gpio_pull_up(BUTTON_JOY_PIN); // Configura o resistor pull-up

    
    // Configuração do botão A
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &button_a_irq);
    
    // Configuração do botão do joystick
    gpio_set_irq_enabled_with_callback(BUTTON_JOY_PIN, GPIO_IRQ_EDGE_FALL, true, &button_joy_irq);
    
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
        
        // Verifica se o joystick está no centro (deadzone)
        if (abs((int)adc_value_x - 2048) < JOYSTICK_DEADZONE && abs((int)adc_value_y - 2048) < JOYSTICK_DEADZONE) {
            // Desliga todos os LEDs quando o joystick estiver no centro
            update_pwm(LED_RED_PIN, 0);
            update_pwm(LED_GREEN_PIN, 0);
            update_pwm(LED_BLUE_PIN, 0);
        } else {
            // Se o eixo X for movido, controla o brilho do LED vermelho
            update_pwm(LED_RED_PIN, abs((int)adc_value_x - 2048) * 2);  // LED vermelho controla pelo eixo X

            // Se o eixo Y for movido, controla o brilho do LED azul e desliga os outros LEDs
            if (abs((int)adc_value_y - 2048) > JOYSTICK_DEADZONE) {
                update_pwm(LED_BLUE_PIN, abs((int)adc_value_y - 2048) * 2);  // LED azul controla pelo eixo Y
                update_pwm(LED_RED_PIN, 0);  // Desliga o LED vermelho
                update_pwm(LED_GREEN_PIN, 0);  // Desliga o LED verde
            } else {
                // Se o eixo Y estiver centralizado ou com pouco desvio, desliga o LED azul
                update_pwm(LED_BLUE_PIN, 0);
            }
        }
        
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
        
        // Desenha a borda do retângulo de acordo com o estilo escolhido
        if (border_style == 0) {
            ssd1306_rect(&ssd, square_x - 1, square_y - 1, 10, 10, 1, true); // Borda sólida
        } else if (border_style == 1) {
            // Borda pontilhada: desenha pequenos pontos ao redor
            for (int i = 0; i < 10; i++) {
                ssd1306_pixel(&ssd, square_x - 1 + i, square_y - 1, 1);
                ssd1306_pixel(&ssd, square_x - 1 + i, square_y + 9, 1);
                ssd1306_pixel(&ssd, square_x - 1, square_y - 1 + i, 1);
                ssd1306_pixel(&ssd, square_x + 9, square_y - 1 + i, 1);
            }
        } else if (border_style == 2) {
            // Sem borda: não desenha nada
        }
        
        // Desenha o quadrado
        ssd1306_rect(&ssd, square_x, square_y, 8, 8, 1, true); // Desenha o quadrado
        ssd1306_update(&ssd);
        
        sleep_ms(50);  // Delay para evitar atualizações muito rápidas
    }
}
