#include <RCSwitch.h>

RCSwitch tx = RCSwitch();

const byte TX_PIN = 10;
const byte BUTTON_PIN = 2; // Botão na porta D2
const byte RELAY_PIN = 4;  // Relé do pisca-alerta na porta D4
const byte LED_PIN = 13;   // LED embutido da placa (L)
const unsigned int BITLEN = 28;
const unsigned int PULSE_US = 531;   // do seu log
const byte PROTO = 6;                // do seu log

// seus códigos
const unsigned long CODE_A = 162701861;

int buttonState;             // leitura atual do botão
int lastButtonState = LOW;   // leitura anterior do botão (estado normal é LOW com pull-down)
unsigned long lastDebounceTime = 0;  // último tempo de mudança
unsigned long debounceDelay = 50;    // tempo de debounce (50ms)

int clickCount = 0;                  // contador de cliques
unsigned long lastClickTime = 0;     // tempo do último clique
const unsigned long CLICK_TIMEOUT = 1000; // intervalo máximo de 1s entre cliques
const unsigned long CODE_A_REPEAT_WINDOW = 17000; // 17s para detectar repetição do duplo clique
const unsigned long CODE_A_SEND_INTERVAL = 1000;  // 1s entre os dois envios

unsigned long lastCodeADoubleClickTime = 0; // quando ocorreu o último duplo clique do CODE_A
byte pendingCodeASends = 0;                 // quantidade de envios pendentes do CODE_A
unsigned long nextCodeASendTime = 0;        // próximo disparo agendado do CODE_A
bool isCodeASendScheduled = false;          // controla o envio assíncrono do CODE_A

// Variáveis para o controle do relé (pisca-alerta)
bool isRelayActive = false;          // Estado geral do pisca-alerta (ligado/desligado)
bool relayState = LOW;               // Estado atual do relé (HIGH/LOW)
unsigned long lastRelayTime = 0;     // Última vez que o relé mudou de estado
const unsigned long RELAY_INTERVAL = 500; // Intervalo de pisca (500ms = 0.5s)

// Variáveis para o LED assíncrono
bool isLedBlinking = false;
bool ledState = LOW;
byte ledTransitionsRemaining = 0;
unsigned long lastLedToggleTime = 0;
unsigned long ledBlinkInterval = 150;

void setup() {
  Serial.begin(115200);

  tx.enableTransmit(TX_PIN);
  tx.setProtocol(PROTO);        // garante mesmo formato
  tx.setPulseLength(PULSE_US);  // ajusta o "delay(us)" capturado
  tx.setRepeatTransmit(12);     // repete o frame pra garantir alcance

  // Configura como entrada simples. O botão liga o D2 ao 3.3V. 
  // IMPORTANTE: use um resistor pull-down (ex: 10K ohms) entre o D2 e o GND.
  pinMode(BUTTON_PIN, INPUT);
  
  // Configura o pino do LED da placa como saída
  pinMode(LED_PIN, OUTPUT);

  // Configura o pino do Relé como saída
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Garante que começa desligado

  Serial.println(F("TX 433MHz pronto. Pressione o botao para enviar."));
  delay(1000);
}

static void sendCode(unsigned long code) {
  Serial.print(F("Enviando: ")); Serial.println(code);
  tx.send(code, BITLEN);        // envia 28 bits
}

void scheduleCodeASends(byte totalSends) {
  if (totalSends == 0) {
    return;
  }

  pendingCodeASends += totalSends;

  if (!isCodeASendScheduled) {
    isCodeASendScheduled = true;
    nextCodeASendTime = millis();
  }
}

// Pisca o LED sem bloquear o loop principal
void startLedBlink(byte times, unsigned long intervalMs = 150) {
  if (times == 0) {
    return;
  }

  isLedBlinking = true;
  ledBlinkInterval = intervalMs;
  ledTransitionsRemaining = (times * 2) - 1;
  ledState = HIGH;
  digitalWrite(LED_PIN, ledState);
  lastLedToggleTime = millis();
}

// Função responsável por monitorar o estado do botão
void checkButtonClicks() {
  // Leitura do estado atual do botão
  int reading = digitalRead(BUTTON_PIN);

  // Verifica se o estado do botão mudou
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Se a leitura mudou
    if (reading != buttonState) {
      buttonState = reading;

      // Conta um clique quando o botão for HIGH (pressionado, conectado ao 3.3V)
      if (buttonState == HIGH) {
        clickCount++;
        lastClickTime = millis();
      }
    }
  }

  lastButtonState = reading;
}

// Função responsável por processar os cliques acumulados
void processClicks() {
  // Verifica se passou o tempo limite (1 seg) desde o último clique e se temos cliques para processar
  if (clickCount > 0 && (millis() - lastClickTime) > CLICK_TIMEOUT) {
    if (clickCount == 1) {
      Serial.println(F("1 clique identificado - Desligando Pisca-Alerta"));
      startLedBlink(1);
      
      // Desativa o pisca-alerta
      isRelayActive = false;
      relayState = LOW;
      digitalWrite(RELAY_PIN, relayState);
    } 
    else if (clickCount == 2) {
      Serial.println(F("2 cliques identificados - Envio de codigo ativado"));
      unsigned long now = millis();

      if (lastCodeADoubleClickTime != 0 &&
          (now - lastCodeADoubleClickTime) <= CODE_A_REPEAT_WINDOW) {
        Serial.println(F("Duplo clique repetido em ate 17s - enviando CODE_A 2x"));
        scheduleCodeASends(2);
        lastCodeADoubleClickTime = 0; // consome a janela; o proximo duplo clique volta a valer 1 envio
      } else {
        scheduleCodeASends(1);
        lastCodeADoubleClickTime = now; // abre uma nova janela de 17s
      }
      startLedBlink(2);
    } 
    else if (clickCount >= 3) {
      Serial.println(F("3 ou mais cliques identificados - Alternando Pisca-Alerta"));
      startLedBlink(3);
      
      // Alterna o estado do pisca-alerta
      isRelayActive = !isRelayActive;
      
      // Se acabou de desligar, garante que o relé de fato apague
      if (!isRelayActive) {
        relayState = LOW;
        digitalWrite(RELAY_PIN, relayState);
      }
    }
    
    // Zera o contador para a próxima sequência de cliques
    clickCount = 0;
  }
}

void handleCodeASends() {
  if (!isCodeASendScheduled) {
    return;
  }

  if ((long)(millis() - nextCodeASendTime) >= 0) {
    sendCode(CODE_A);
    pendingCodeASends--;

    if (pendingCodeASends > 0) {
      nextCodeASendTime = millis() + CODE_A_SEND_INTERVAL;
    } else {
      isCodeASendScheduled = false;
    }
  }
}

// Função assíncrona para piscar o relé
void handleRelayBlink() {
  if (isRelayActive) {
    if ((millis() - lastRelayTime) >= RELAY_INTERVAL) {
      // Inverte o estado do relé
      relayState = !relayState;
      digitalWrite(RELAY_PIN, relayState);
      
      // Atualiza o tempo
      lastRelayTime = millis();
    }
  }
}

void handleLedBlink() {
  if (!isLedBlinking) {
    return;
  }

  if ((millis() - lastLedToggleTime) >= ledBlinkInterval) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    lastLedToggleTime = millis();
    ledTransitionsRemaining--;

    if (ledTransitionsRemaining == 0) {
      isLedBlinking = false;
      ledState = LOW;
      digitalWrite(LED_PIN, ledState);
    }
  }
}

void loop() {
  checkButtonClicks();
  processClicks();
  handleCodeASends();
  handleRelayBlink(); // Processa a lógica do pisca-alerta continuamente
  handleLedBlink();
}
