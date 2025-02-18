# Projeto de Controle de LEDs RGB com Joystick e Display SSD1306

Este projeto visa a compreensão do funcionamento do conversor analógico-digital (ADC) no RP2040, utilizando a placa BitDogLab, para controlar a intensidade de LEDs RGB e exibir a posição de um joystick em um display SSD1306. Através de interrupções, controle PWM e comunicação I2C, o sistema integra diversos componentes, proporcionando uma interação rica com o usuário.

## 🛠 Componentes Utilizados

A tarefa requer os seguintes componentes conectados à placa BitDogLab:

| Componente            | Conexão à GPIO     |
|-----------------------|--------------------|
| LED RGB (Azul, Verde, Vermelho) | GPIO 11, 12, 13   |
| Botão Joystick        | GPIO 22            |
| Joystick              | GPIO 26, 27        |
| Botão A               | GPIO 5             |
| Display SSD1306       | I2C (GPIO 14, GPIO 15) |

## 📌 Requisitos da Atividade

1. **Controle de LEDs RGB via PWM**:
   - O LED **Azul** será controlado com base no valor do eixo **Y** do joystick, ajustando sua intensidade luminosa de acordo com a posição do joystick (valores de 0 a 4095).
   - O LED **Vermelho** será controlado com base no valor do eixo **X** do joystick, também ajustando sua intensidade luminosa de acordo com a posição do joystick.
   - **LED Verde**: Será alternado a cada pressionamento do botão do joystick.

2. **Exibição no Display SSD1306**:
   - Um quadrado de 8x8 pixels será exibido no display SSD1306 e se moverá proporcionalmente aos valores capturados pelo joystick (eixos X e Y).

3. **Botão do Joystick**:
   - Alterar o estado do **LED Verde** a cada pressionamento.
   - Modificar a borda do display para indicar o pressionamento, alternando entre diferentes estilos de borda.

4. **Botão A**:
   - Ativar ou desativar os LEDs RGB a cada pressionamento do **Botão A**.

5. **Interrupções e Debouncing**:
   - Implementação das interrupções de botões, com tratamento de debouncing via software.

## 🌊 Instruções de Uso

1. Clone este repositório:

```sh
git clone https://github.com/Sansaocarrasco/Embarcatech-ConversorADC.git
```

2. Abra o projeto no VS Code.
3. Conecte a placa Raspberry Pi Pico W ao computador no modo BOOTSEL (pressionando o botão BOOTSEL ao conectar via USB).
4. Compile o arquivo `conversor.c` e carregue o projeto para a placa.

## 🎥 Vídeo Demonstrativo

O vídeo associado a esta prática pode ser acessado no link a seguir:

https://youtu.be/--yAqKULDds?si=Y4zPzJvmz09p16By

*Fonte: autor*

## 📜 Licença

Este projeto está licenciado sob a Licença MIT. Veja o arquivo `LICENSE` para mais detalhes.
