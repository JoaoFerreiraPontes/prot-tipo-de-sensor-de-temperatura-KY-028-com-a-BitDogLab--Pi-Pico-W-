# Monitor de Temperatura com Raspberry Pi Pico W e ThingSpeak

![Linguagem C](https://img.shields.io/badge/Linguagem-C-blue.svg)
![Plataforma](https://img.shields.io/badge/Plataforma-Raspberry%20Pi%20Pico%20W-green.svg)
![Licença](https://img.shields.io/badge/License-MIT-yellow.svg)

Este projeto transforma um **Raspberry Pi Pico W** em um dispositivo de monitoramento de temperatura conectado à Internet. Ele é projetado para receber dados de temperatura de outro dispositivo na rede local, exibi-los em um display OLED, fornecer alertas visuais e sonoros, e enviar os dados para a plataforma de IoT **ThingSpeak**.

---

## 🚀 Funcionalidades Principais

* **Servidor Local:** Atua como um servidor TCP na porta 80, escutando por requisições HTTP GET para receber atualizações de temperatura (ex: `GET /update?temp=62.5`).
* **Display OLED:** Exibe a temperatura atual e o status do sistema ("NORMAL", "TEMP MAX ATINGIDA", etc.) em tempo real em um display SSD1306.
* **Alertas Visuais:** Utiliza um LED vermelho para indicar temperaturas acima do limite máximo (65°C) e um LED azul para temperaturas abaixo do limite mínimo (60°C).
* **Alarme Sonoro:** Ativa um buzzer com padrões diferentes para alertas de temperatura alta ou baixa.
* **Integração com IoT:** Envia os dados de temperatura recebidos para um canal no **ThingSpeak** a cada 15 segundos, permitindo o monitoramento e registro remoto.
* **Conectividade Wi-Fi:** Conecta-se a uma rede Wi-Fi para comunicação na rede local e com a internet.

---

## 🛠️ Hardware Necessário

* **Raspberry Pi Pico W**
* **Display OLED 128x64** com controlador SSD1306 (interface I2C)
* **LED Vermelho**
* **LED Azul**
* **Buzzer Ativo**
* Um segundo dispositivo (como um ESP32, outro Pico, etc.) para enviar os dados de temperatura para o IP do Pico W.

---

## 🔌 Pinagem (Conexões)

| Componente      | Pino no Pico W |
| :-------------- | :------------- |
| Display OLED SDA | GPIO 14        |
| Display OLED SCL | GPIO 15        |
| LED Vermelho    | GPIO 13        |
| LED Azul        | GPIO 12        |
| Buzzer          | GPIO 21        |

---

## ⚙️ Configuração e Compilação

### 1. Pré-requisitos
Certifique-se de ter o ambiente de desenvolvimento para o **Raspberry Pi Pico C/C++ SDK** configurado corretamente em sua máquina.

### 2. Configurar Credenciais
Antes de compilar, abra o arquivo `main.c` e edite as seguintes linhas com suas informações:
```c
// ========== CONFIGURAÇÕES GLOBAIS ==========
#define WIFI_SSID "SEU_WIFI_AQUI"
#define WIFI_PASS "SUA_SENHA_AQUI"
#define THINGSPEAK_API_KEY "SUA_CHAVE_THINGSPEAK_AQUI"
```

### 3. Compilar o Projeto
Navegue até o diretório do projeto no terminal e execute os comandos padrão do `cmake` para compilar:
```bash
# Criar um diretório de build
mkdir build
cd build

# Executar o CMake para configurar o projeto
cmake ..

# Compilar o código
make
```

### 4. Enviar para o Pico
Após a compilação, um arquivo `Tarefa7_JOAOPONTES.uf2` será gerado dentro da pasta `build`. Arraste e solte este arquivo no seu Pico W (enquanto ele estiver no modo BOOTSEL) para programá-lo.

---

## 📜 Licença

Este projeto está distribuído sob a licença MIT. Veja o arquivo `LICENSE` para mais detalhes.
