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
#define TIME_DEBOUNCE 500  // Tempo de debounce para os botões (em milissegundos)

// Definições de pinos dos LEDs
#define LED_RED_PIN 13
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN 12

// Definir o valor máximo de desvio para considerar o joystick como "no centro"
#define JOYSTICK_DEADZONE 100  // Ajuste esse valor conforme necessário

// Variáveis globais
bool pwm_enabled = true;  // Controle do PWM
bool green_led_state = false;  // Estado do LED verde
bool border_enabled = false;  // Controle da borda do display
int border_style = 0;  // Estilo da borda (0: sólido, 1: pontilhado, 2: sem borda)
ssd1306_t ssd;  // Instância do display OLED

volatile uint32_t last_press_time_A = 0;
volatile uint32_t last_press_time_JOY = 0;

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

// Função para alternar a borda
void toggle_border() {
    border_enabled = !border_enabled;  // Alterna a borda
}

// Função para desenhar a borda
void draw_border(int square_x, int square_y) {
    if (border_enabled) {
        if (border_style == 0) {
            // Borda sólida
            ssd1306_rect(&ssd, square_x - 1, square_y - 1, 10, 10, 1, true); 
        } else if (border_style == 1) {
            // Borda pontilhada: desenha pequenos pontos ao redor
            for (int i = 0; i < 10; i++) {
                ssd1306_pixel(&ssd, square_x - 1 + i, square_y - 1, 1);
                ssd1306_pixel(&ssd, square_x - 1 + i, square_y + 9, 1);
                ssd1306_pixel(&ssd, square_x - 1, square_y - 1 + i, 1);
                ssd1306_pixel(&ssd, square_x + 9, square_y - 1 + i, 1);
            }
        }
    }
}

// Função para desenhar o quadrado
void draw_square(int square_x, int square_y) {
    ssd1306_rect(&ssd, square_x, square_y, 8, 8, 1, true); // Desenha o quadrado
    ssd1306_update(&ssd);
}

// Função de interrupção para o botão A (simplesmente alterna o estado do PWM e borda)
void button_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());  // Tempo atual (em milissegundos)

    if (gpio == BUTTON_A_PIN && current_time - last_press_time_A > TIME_DEBOUNCE){
        pwm_enabled = !pwm_enabled;  // Alterna o estado do PWM
        last_press_time_A = current_time;  // Atualiza o tempo da última pressão do botão A
    }
    
    if (gpio == BUTTON_JOY_PIN && current_time - last_press_time_JOY > TIME_DEBOUNCE){
        gpio_put(LED_RED_PIN, 0);
        gpio_put(LED_BLUE_PIN, 0);

        green_led_state = !green_led_state;  // Alterna o estado do LED verde
        gpio_put(LED_GREEN_PIN, green_led_state ? 1 : 0);  // Liga ou desliga o LED verde

        toggle_border();  // Alterna a borda
        last_press_time_JOY = current_time;  // Atualiza o tempo da última pressão do joystick
    }
}


int main() {
    stdio_init_all();
    
    // Configuração dos LEDs
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    setup_pwm(LED_RED_PIN);
    setup_pwm(LED_BLUE_PIN);

    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN); // Configura o resistor pull-up


    gpio_init(BUTTON_JOY_PIN);
    gpio_set_dir(BUTTON_JOY_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_JOY_PIN); // Configura o resistor pull-up
    

    // Configuração dos botões para uma interrupção simples
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &button_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_JOY_PIN, GPIO_IRQ_EDGE_FALL, true, &button_irq_handler);
    
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
        } else  {
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

        // Limpa o display
        ssd1306_clear(&ssd);

        // Desenha a borda do retângulo se a borda estiver habilitada
        if (border_enabled) {
            draw_border(square_x, square_y);  // Chama a função para desenhar a borda
        }

        // Desenha o quadrado
        draw_square(square_x, square_y);  // Chama a função para desenhar o quadrado
        
        sleep_ms(50);  // Delay para evitar atualizações muito rápidas
    }
}
