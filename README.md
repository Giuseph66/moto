# Projeto Moto - Controle Remoto Arduino

Este projeto consiste em um sistema de controle remoto baseado em Arduino para motocicletas, utilizando transmissão RF 433MHz e controle inteligente de acessórios (pisca-alerta e relés).

## 🚀 Funcionalidades

- **Transmissão RF 433MHz**: Envia códigos específicos (como o `CODE_A`) para acionamento de portões ou outros receptores RF.
- **Acionamento Inteligente**:
  - **1 Clique**: Desliga o pisca-alerta.
  - **2 Cliques**: Envia o código RF. Se repetido em menos de 17 segundos, o código é enviado duas vezes.
  - **3 ou mais Cliques**: Liga/Desliga o pisca-alerta (alterna o estado).
- **Feedback Visual**: O LED embutido (D13) fornece feedback visual para cada ação (pisca conforme o número de cliques processados).
- **Pisca-Alerta Automático**: Controla um relé para acionar as luzes de alerta de forma intermitente (intervalo de 500ms).
- **Operação Assíncrona**: O código utiliza temporização não bloqueante (`millis()`), permitindo que as funções de pisca e transmissão ocorram simultaneamente sem travar o processamento.

## 🛠️ Hardware e Conexões (Pinagem)

| Componente | Pino Arduino | Descrição |
| :--- | :--- | :--- |
| **Transmissor RF (DATA)** | `D10` | Saída de dados para o módulo 433MHz. |
| **Botão de Controle** | `D2` | Entrada digital (requer pulso HIGH). |
| **Relé (Pisca-Alerta)** | `D4` | Saída para controle do relé de luzes. |
| **LED Status** | `D13` | LED interno para feedback de operação. |

> [!IMPORTANT]
> **Resistor Pull-Down**: É obrigatório o uso de um resistor pull-down (ex: 10kΩ) no pino `D2` (Botão) para garantir que o estado seja `LOW` quando não estiver pressionado, evitando disparos falsos.

## ⚙️ Configuração Técnica

- **Biblioteca**: `RCSwitch` para transmissão RF.
- **Protocolo RF**: Protocolo 6 (ajustável no código).
- **Comprimento de Pulso**: 531us.
- **Frequência**: 433MHz.

## 💻 Instalação

1. Certifique-se de ter a IDE do Arduino instalada.
2. Instale a biblioteca `RCSwitch` via Gerenciador de Bibliotecas.
3. Conecte o hardware conforme a tabela de pinagem.
4. Carregue o arquivo `arduino/transmissoooo_433/transmissoooo_433.ino` na sua placa.
5. Monitore o **Serial Monitor** (115200 baud) para logs de depuração.

---
Desenvolvido para automação e segurança em motocicletas.
