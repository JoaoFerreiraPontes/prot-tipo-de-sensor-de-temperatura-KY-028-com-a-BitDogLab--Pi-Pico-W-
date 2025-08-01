// Inclusão de bibliotecas necessárias
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"           // Biblioteca básica do Raspberry Pi Pico
#include "pico/cyw43_arch.h"       // Biblioteca para controle do chip WiFi CYW43
#include "hardware/i2c.h"          // Biblioteca para comunicação I2C
#include "hardware/gpio.h"         // Biblioteca para controle de GPIO
#include "libs/ssd1306.h"          // Biblioteca para controle do display OLED SSD1306
#include "lwip/netif.h"            // Biblioteca LWIP para interfaces de rede
#include "lwip/dhcp.h"             // LWIP - Protocolo DHCP
#include "lwip/tcp.h"              // LWIP - Protocolo TCP
#include "lwip/dns.h"              // LWIP - Sistema de Nomes de Domínio
#include "hardware/pwm.h"          // Biblioteca para controle de PWM
#include "hardware/clocks.h"       // Biblioteca para controle de clocks

// ========== CONFIGURAÇÕES GLOBAIS ==========
// Configurações de rede WiFi
#define WIFI_SSID "NININHA_2.4G"                     // INSIRA AQUI SEU LOGIN WI-FI
#define WIFI_PASS "qwert531"                        // INSIRA AQUI O PASSWORD DO SEU WIFI
#define THINGSPEAK_API_KEY "UHA0FA3AEZYGR8VE"      // INSIRA AQUI sua Chave da API do ThingSpeak
#define THINGSPEAK_URL "api.thingspeak.com"       // URL do serviço ThingSpeak
#define RECONNECT_INTERVAL_MS 1000               // Intervalo de reconexão WiFi
#define SEND_INTERVAL_MS 15000                  // Intervalo entre envios de dados
#define THINGSPEAK_FIELD "field1"              // Campo usado no ThingSpeak
// Configurações do Buzzer, e LEDS
#define BUZZER_PIN 21                         // Pino do buzzer
#define LED_PIN_R 13                         // Pino do LED vermelho
#define LED_PIN_B 12                        // Pino do LED azul

// Configurações do display OLED
const uint32_t I2C_SDA = 14;      // Pino SDA do I2C
const uint32_t I2C_SCL = 15;      // Pino SCL do I2C
#define OLED_WIDTH 128            // Largura do display
#define OLED_HEIGHT 64            // Altura do display
#define OLED_ADDRESS 0x3C         // Endereço I2C do display

// Variáveis globais
ip_addr_t server_ip;              // Armazena o IP do servidor ThingSpeak
ssd1306_t oled;                   // Estrutura de controle do OLED
float current_temperature = 0.0;  // Armazena a temperatura atual
struct tcp_pcb *server_pcb;       // Estrutura de controle do protocolo TCP
static absolute_time_t last_send_time; // Marca o último envio de dados
bool wifi_connected = false;      // Status da conexão WiFi

// ========== PROTÓTIPOS DE FUNÇÃO ==========
void connect_to_wifi();           // Conecta à rede WiFi
void check_wifi_connection();     // Verifica status da conexão
void update_display();            // Atualiza o display OLED
err_t tcp_connect_callback(void *arg, struct tcp_pcb *tpcb, err_t err); // Callback de conexão TCP
err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len);    // Callback de envio TCP
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err); // Callback de recebimento TCP
err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err); // Callback de aceitação de conexão
void send_to_thingspeak(float temp); // Envia dados para o ThingSpeak
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *arg); // Callback de resolução DNS
void setup_led_pwm();             // Configura PWM para os LEDs
void update_led_based_on_temperature(); // Atualiza LEDs conforme temperatura
void setup_buzzer_pwm();          // Configura PWM para o buzzer
void update_buzzer_alarm();       // Ativa alarme sonoro conforme temperatura

// ========== FUNÇÕES DO DISPLAY OLED ==========
// Inicializa o display OLED
void display_init() {
    i2c_init(i2c1, 400000); // Inicializa I2C a 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);   // Habilita pull-up nos pinos I2C
    gpio_pull_up(I2C_SCL);

    oled.external_vcc = false; // Define tipo de alimentação do OLED
    ssd1306_init(&oled, OLED_WIDTH, OLED_HEIGHT, OLED_ADDRESS, i2c1); // Inicializa display
    
    ssd1306_clear(&oled);    // Limpa o display
    ssd1306_show(&oled);     // Atualiza o display fisicamente
    sleep_ms(10);            // Pequena pausa para estabilização
}

// Atualiza o conteúdo do display com a temperatura e status
void update_display() {
    char temp_str[16];
    char value_str[16];
    char status_str[20];
    
    snprintf(temp_str, sizeof(temp_str), "TEMPERATURA:");
    snprintf(value_str, sizeof(value_str), "%.1fC", current_temperature);
    ssd1306_clear(&oled); // Limpa buffer interno
    
    // Desenha strings no buffer
    ssd1306_draw_string(&oled, 0, 5, 1, temp_str); // Linha superior
    ssd1306_draw_string(&oled, 0, 25, 2, value_str); // Valor grande
    
    // Determina status com base na temperatura
    if(current_temperature > 65.0f) {
        snprintf(status_str, sizeof(status_str), "TEMP MAX ATINGIDA!");
    } 
    else if(current_temperature < 60.0f) {
        snprintf(status_str, sizeof(status_str), "TEMP MIN ATINGIDA!");
    }
    else {
        snprintf(status_str, sizeof(status_str), "NORMAL");
    }
    ssd1306_draw_string(&oled, 0, 55, 1, status_str); // Linha inferior
    
    ssd1306_show(&oled); // Envia buffer para o display
}

// ========== FUNÇÕES DE REDE E TCP/IP ==========
// Callback para resolução DNS
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *arg) {
    struct tcp_pcb *pcb = (struct tcp_pcb *)arg;
    if (ipaddr != NULL) {
        printf("[DNS] Resolvido: %s -> %s\n", name, ipaddr_ntoa(ipaddr));
        tcp_connect(pcb, ipaddr, 80, tcp_connect_callback); // Conecta após resolução
    } else {
        printf("[DNS] Falha na resolução\n");
        tcp_close(pcb);
    }
}

// Callback de conexão TCP estabelecida
err_t tcp_connect_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK) {
        printf("[TCP] Erro na conexão\n");
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    // Prepara requisição HTTP GET para o ThingSpeak
    float temp = ((uintptr_t)tpcb->callback_arg) / 100.0f; // Recupera temperatura
    char request[256];
    snprintf(request, sizeof(request),
        "GET /update?api_key=%s&%s=%.2f HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n",
        THINGSPEAK_API_KEY, THINGSPEAK_FIELD, temp, THINGSPEAK_URL);

    tcp_sent(tpcb, tcp_sent_callback); // Configura callback de envio
    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY); // Escreve dados
    tcp_output(tpcb); // Força envio
    
    return ERR_OK;
}

// Callback de confirmação de envio TCP
err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    printf("[ThingSpeak] Dados enviados!\n");
    last_send_time = get_absolute_time(); // Atualiza temporização
    tcp_close(tpcb); // Fecha conexão após envio
    return ERR_OK;
}

// Callback de recebimento de dados TCP (respostas HTTP)
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p != NULL) {
        char *data = (char *)p->payload;
        data[p->tot_len] = '\0'; // Garante terminação de string
        
        // Processa requisições do ESP32 (se aplicável)
        char *temp_ptr = strstr(data, "GET /update?temp=");
        if (temp_ptr) {
            temp_ptr += strlen("GET /update?temp=");
            current_temperature = atof(temp_ptr); // Atualiza temperatura
            update_display(); // Atualiza OLED
            send_to_thingspeak(current_temperature); // Repassa para ThingSpeak
        }
        
        // Resposta HTTP básica
        const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
        tcp_close(tpcb); // Fecha conexão
        pbuf_free(p);    // Libera buffer
    }
    return ERR_OK;
}

// Definição do callback para aceitação de conexões TCP
err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    // Esta função é chamada quando uma nova conexão é estabelecida no servidor
    
    /* Configura o callback de recebimento de dados para a nova conexão
       tcp_recv_callback: Função que será chamada quando dados chegarem
     */
    tcp_recv(newpcb, tcp_recv_callback);
    
    /* Retorno padrão para aceitação bem-sucedida
     * - ERR_OK (0) indica que a conexão foi aceita corretamente
     * - Outros códigos de erro encerrariam a conexão
     */
    return ERR_OK;
    
}
// Inicializa servidor TCP na porta 80
void init_server() {
    server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY); // Cria novo PCB
    tcp_bind(server_pcb, IP_ANY_TYPE, 80);         // Associa à porta 80
    server_pcb = tcp_listen(server_pcb);           // Coloca em modo escuta
    tcp_accept(server_pcb, tcp_accept_callback);   // Configura callback
    printf("[Servidor] Iniciado na porta 80\n");
}

// ========== GERENCIAMENTO DE DADOS PARA THINGSPEAK ==========
void send_to_thingspeak(float temp) {
    int64_t elapsed = absolute_time_diff_us(last_send_time, get_absolute_time()) / 1000;
    if (elapsed < SEND_INTERVAL_MS) { // Verifica intervalo mínimo
        printf("[ThingSpeak] Aguardando próximo envio\n");
        return;
    }

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        printf("[ThingSpeak] Falha ao criar PCB\n");
        return;
    }

    // Armazena temperatura como inteiro no callback_arg
    uint32_t temp_fixed = (uint32_t)(temp * 100);
    pcb->callback_arg = (void*)(uintptr_t)temp_fixed;

    // Inicia resolução DNS
    err_t dns_status = dns_gethostbyname(THINGSPEAK_URL, &server_ip, dns_callback, pcb);
    
    if (dns_status == ERR_OK) { // IP já em cache
        tcp_connect(pcb, &server_ip, 80, tcp_connect_callback);
    } else if (dns_status == ERR_INPROGRESS) {
        printf("[DNS] Resolução em progresso\n");
    } else {
        tcp_close(pcb);
    }
}

// ========== GERENCIAMENTO WIFI ==========
void connect_to_wifi() {
    if (cyw43_arch_init()) { // Inicializa hardware WiFi
        printf("[WiFi] Falha na inicialização\n");
        return;
    }

    cyw43_arch_enable_sta_mode(); // Modo estação (cliente)
    printf("[WiFi] Conectando...\n");
    
    int retry = 0;
    while (retry < 5) { // Tentativas de conexão
        if (cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK) == 0) {
            wifi_connected = true;
            printf("[WiFi] Conectado! IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));
            init_server(); // Inicia servidor após conexão
            return;
        }
        printf("[WiFi] Tentativa %d falhou\n", ++retry);
        sleep_ms(RECONNECT_INTERVAL_MS);
    }
    printf("[WiFi] Falha permanente\n");
}

// Verifica periodicamente a conexão WiFi
void check_wifi_connection() {
    static absolute_time_t last_check = 0;
    if (absolute_time_diff_us(get_absolute_time(), last_check) < 1000000) return;
    
    if (cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_UP) {
        wifi_connected = false;
        printf("[WiFi] Conexão perdida\n");
        connect_to_wifi(); // Tenta reconectar
    }
    last_check = get_absolute_time();
}

// ========== CONTROLE DE LEDS E BUZZER ==========
// Configura PWM para os LEDs
void setup_led_pwm() {
    // LED Vermelho
    gpio_set_function(LED_PIN_R, GPIO_FUNC_PWM);
    uint slice_num_r = pwm_gpio_to_slice_num(LED_PIN_R);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, 0xFFFF);
    pwm_init(slice_num_r, &config, true);
    pwm_set_gpio_level(LED_PIN_R, 0); // Inicia desligado

    // LED Azul
    gpio_set_function(LED_PIN_B, GPIO_FUNC_PWM);
    uint slice_num_b = pwm_gpio_to_slice_num(LED_PIN_B);
    pwm_init(slice_num_b, &config, true);
    pwm_set_gpio_level(LED_PIN_B, 0);
}

// Atualiza intensidade dos LEDs conforme temperatura
void update_led_based_on_temperature() {
    // Vermelho para temperatura alta
    pwm_set_gpio_level(LED_PIN_R, (current_temperature > 65.0f) ? 0xFFFF : 0);
    // Azul para temperatura baixa
    pwm_set_gpio_level(LED_PIN_B, (current_temperature < 60.0f) ? 0xFFFF : 0);
}

// Configura PWM para o buzzer
void setup_buzzer_pwm() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, 0xFFFF);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0); // Inicia silencioso
}

// Ativa alarmes sonoros conforme temperatura
void update_buzzer_alarm() {
    static uint32_t counter = 0;
    static bool high_note = false;
    
    bool high_alarm = (current_temperature >= 65.0f);
    bool low_alarm = (current_temperature <= 60.0f);
    
    if (high_alarm) { // Sirene para temperatura alta
        if (counter++ % 10 == 0) { // Alterna notas
            high_note = !high_note;
            uint32_t freq = high_note ? 2000 : 1500;
            float divider = clock_get_hz(clk_sys) / (freq * 0xFFFF);
            pwm_set_clkdiv(pwm_gpio_to_slice_num(BUZZER_PIN), divider);
        }
        pwm_set_gpio_level(BUZZER_PIN, 0x8000); // 50% duty cycle
    } 
    else if (low_alarm) { // Bips intermitentes para baixa
        if (counter++ % 20 == 0) { // Bip a cada 2 ciclos
            high_note = !high_note;
            uint32_t freq = 1000;
            float divider = clock_get_hz(clk_sys) / (freq * 0xFFFF);
            pwm_set_clkdiv(pwm_gpio_to_slice_num(BUZZER_PIN), divider);
        }
        pwm_set_gpio_level(BUZZER_PIN, (counter % 20 < 4) ? 0x8000 : 0);
    }
    else { // Desliga buzzer
        pwm_set_gpio_level(BUZZER_PIN, 0);
        counter = 0;
    }
}

// ========== FUNÇÃO PRINCIPAL ==========
int main() {
    stdio_init_all(); // Inicializa stdio (UART por padrão)
    display_init();   // Configura display
    setup_led_pwm();  // Configura LEDs
    setup_buzzer_pwm(); // Configura buzzer
    
    last_send_time = get_absolute_time() - (SEND_INTERVAL_MS * 1000 * 2); // Força primeiro envio
    connect_to_wifi(); // Conecta ao WiFi

    while (true) {
        if (wifi_connected) {
            check_wifi_connection(); // Verifica conexão
            update_led_based_on_temperature(); // Atualiza LEDs
            update_buzzer_alarm();   // Atualiza buzzer
            cyw43_arch_poll();       // Processa eventos WiFi
        }
        sleep_ms(100); // Loop principal a cada 100ms
    }

    cyw43_arch_deinit(); // Nunca chega aqui, mas é boa prática
    return 0;
}