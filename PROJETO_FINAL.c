#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "inc/ssd1306.h"

// Configuração de LEDs e Botões
#define LED_G   11
#define LED_B   12
#define LED_R   13
#define BTN_A   5
#define BTN_B   6
#define BTN_JOY 22
#define LED_COUNT 25

// Configuração do Joystick e Potenciômetro
// Usaremos ADC0 para o eixo Y e ADC1 para o potenciômetro (controle do LED verde e leitura da tensão)
#define EIXO_Y 26    // ADC0
#define EIXO_X 27    // ADC1
#define PWM_WRAP 4095

// Configuração do I2C para o OLED
#define I2C_PORT i2c1
#define I2C_SDA  14
#define I2C_SCL  15

// Configuração do Display OLED
#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 64
#define SQUARE_SIZE    8

// Variáveis para posicionamento (se necessário para outros gráficos)
int pos_x = (DISPLAY_WIDTH - SQUARE_SIZE) / 2;
int pos_y = (DISPLAY_HEIGHT - SQUARE_SIZE) / 2;
const int SPEED = 2;
const int MAX_X = DISPLAY_WIDTH - SQUARE_SIZE;
const int MAX_Y = DISPLAY_HEIGHT - SQUARE_SIZE;

// Variáveis globais
// Inicia o motor desligado, ou seja, pwm_on inicia com false.
volatile bool pwm_on = false;
volatile bool borda = false;
volatile bool led_r_estado = false;
volatile bool led_g_estado = false;
volatile bool led_b_estado = false;
bool cor = true;
absolute_time_t last_interrupt_time = 0;
float rpm = 0;

// Protótipos de funções
void gpio_callback(uint gpio, uint32_t events);
void JOYSTICK(uint slice1);
void update_menu(uint8_t *ssd, struct render_area *frame_area);

// Variáveis globais adicionais
volatile char c = '~';
volatile bool new_data = false;
volatile int current_digit = 0;

// Função auxiliar (mantida do código original, se necessário)
void process_command(char c, int digit, char *line1, char *line2, uint8_t *ssd, struct render_area *frame_area) {
    if (BTN_A) {
        sleep_ms(5);
    } else {
        printf("digdim: %c\n", c);
    }
    // Atualiza o OLED
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, frame_area);
    ssd1306_draw_string(ssd, 5, 0, line1);
    ssd1306_draw_string(ssd, 5, 8, line2);
    render_on_display(ssd, frame_area);
}

int main() {
    stdio_init_all();

    // Inicializa e configura os LEDs
    gpio_set_function(LED_G, GPIO_FUNC_PWM);
    gpio_init(LED_R);
    gpio_init(LED_B);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_put(LED_R, 0);
    gpio_put(LED_B, 0);

    // Inicializa e configura os botões
    gpio_init(BTN_A);
    gpio_init(BTN_B);
    gpio_init(BTN_JOY);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_set_dir(BTN_JOY, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_pull_up(BTN_B);
    gpio_pull_up(BTN_JOY);

    // Habilita interrupções para os botões
    gpio_set_irq_enabled(BTN_A, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_B, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_JOY, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_callback(gpio_callback);
    irq_set_enabled(IO_IRQ_BANK0, true);
    
    // Inicializa o ADC para o joystick e o potenciômetro
    adc_init();
    adc_gpio_init(EIXO_Y); // ADC0
    adc_gpio_init(EIXO_X); // ADC1
    
    // Inicializa o PWM para o LED verde
    uint slice1 = pwm_gpio_to_slice_num(LED_G);
    pwm_set_wrap(slice1, PWM_WRAP);
    pwm_set_clkdiv(slice1, 2.0f);
    pwm_set_enabled(slice1, true);

    // Inicializa o I2C para o display OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o OLED SSD1306
    ssd1306_init();

    // Configura a área de renderização do display OLED
    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    
    while (true) {
        // Se pwm_on estiver ativo, lê o potenciômetro e ajusta o LED verde;
        // Caso contrário, desliga o LED verde.
        if (pwm_on) {
            adc_select_input(1);
            uint16_t pot_value = adc_read();
            pwm_set_gpio_level(LED_G, pot_value);
        } else {
            pwm_set_gpio_level(LED_G, 0);
        }
        
        // Atualiza o menu do display
        update_menu(ssd, &frame_area);
        
        cor = !cor;
        sleep_ms(50);
    }
}

// Callback de interrupção dos botões
void gpio_callback(uint gpio, uint32_t events) {
    absolute_time_t now = get_absolute_time();
    int64_t diff = absolute_time_diff_us(last_interrupt_time, now);

    if (diff < 250000) return; // debounce
    last_interrupt_time = now;

    if (gpio == BTN_A) {
        led_r_estado = !led_r_estado;
        gpio_put(LED_R, led_r_estado);
        if (led_r_estado) {
            printf("Falha no Estator\n");
            c = '#';
        } else {
            printf("Estator Normal\n");
            c = '$';
        }
        new_data = true;
    }
    if (gpio == BTN_B) {
        led_b_estado = !led_b_estado;
        gpio_put(LED_B, led_b_estado);
        if (led_b_estado) {
            printf("Falha no Rotor\n");
        } else {
            printf("Rotor Normal\n");
        }
    }
    
    // Alterna o estado do pwm_on para ligar/desligar o LED verde
    if (gpio == BTN_JOY) {
        pwm_on = !pwm_on;
    }
}

// Função para ler os eixos do joystick (mantida para referência, se necessário)
void JOYSTICK(uint slice1) {
    const uint16_t CENTER = 2047;
    const uint16_t DEADZONE = 170;  // zona morta

    // Lê o eixo Y (ADC0)
    adc_select_input(0);
    uint16_t y_value = adc_read();
    
    // Lê o eixo X (ADC1)
    adc_select_input(1);
    uint16_t x_value = adc_read();
    
    int16_t x_diff = (int16_t)x_value;
    int16_t y_diff = (int16_t)y_value;

    // Ajusta o PWM considerando a zona morta
    uint16_t pwm_y = (abs(y_diff) <= DEADZONE) ? 0 : abs(y_diff);
    uint16_t pwm_x = (abs(x_diff) <= DEADZONE) ? 0 : abs(x_diff);

    if (pwm_on) {
        pwm_set_gpio_level(LED_G, pwm_x);
    } else {
        pwm_set_gpio_level(LED_G, 0);
    }
}

// Função para atualizar o menu do display OLED com o layout:
// Motor
//   <estado do motor>
// Rotor
//   <estado do rotor>
// Estator
//   <estado do estator>
//
// Se o LED verde estiver desligado (pwm_on == false), exibe "motor desligado" apenas.
void update_menu(uint8_t *ssd, struct render_area *frame_area) {
    memset(ssd, 0, ssd1306_buffer_length);

    // Se o LED verde estiver desligado, mostra a mensagem "motor desligado"
    if (!pwm_on) {
        ssd1306_draw_string(ssd, 0, 20, "motor desligado");
        render_on_display(ssd, frame_area);
        return;
    }
    
    char motor_status[15];
    char rotor_status[10];
    char estator_status[10];

    // Lê o valor do potenciômetro (ADC1) para determinar o estado do motor
    adc_select_input(1);
    uint16_t pot_value = adc_read();
    if (pot_value > 3000) {
        strcpy(motor_status, "sobretensao");
    } else if (pot_value < 1000) {
        strcpy(motor_status, "subtensao");
    } else {
        strcpy(motor_status, "normal");
    }

    // Determina o estado do rotor (LED azul)
    if (led_b_estado) {
        strcpy(rotor_status, "falha");
    } else {
        strcpy(rotor_status, "normal");
    }

    // Determina o estado do estator (LED vermelho)
    if (led_r_estado) {
        strcpy(estator_status, "falha");
    } else {
        strcpy(estator_status, "normal");
    }

    // Organiza o display em 6 linhas (verticalmente com espaçamento de 10 pixels)
    ssd1306_draw_string(ssd, 0, 0, "TENSAO");
    char buffer[30];
    sprintf(buffer, "  %s", motor_status);
    ssd1306_draw_string(ssd, 0, 10, buffer);

    ssd1306_draw_string(ssd, 0, 20, "ROTOR");
    sprintf(buffer, "  %s", rotor_status);
    ssd1306_draw_string(ssd, 0, 30, buffer);

    ssd1306_draw_string(ssd, 0, 40, "ESTATOR");
    sprintf(buffer, "  %s", estator_status);
    ssd1306_draw_string(ssd, 0, 50, buffer);

    render_on_display(ssd, frame_area);
}
