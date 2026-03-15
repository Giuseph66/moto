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

// Variáveis para o controle do relé (pisca-alerta)
bool isRelayActive = false;          // Estado geral do pisca-alerta (ligado/desligado)
bool relayState = LOW;               // Estado atual do relé (HIGH/LOW)
unsigned long lastRelayTime = 0;     // Última vez que o relé mudou de estado
const unsigned long RELAY_INTERVAL = 500; // Intervalo de pisca (500ms = 0.5s)

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

// Função para piscar o LED L da placa
void blinkLed(int times, int delayTime = 150) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayTime);
    digitalWrite(LED_PIN, LOW);
    if (i < times - 1) {
      delay(delayTime);
    }
  }
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
      blinkLed(1);
      
      // Desativa o pisca-alerta
      isRelayActive = false;
      relayState = LOW;
      digitalWrite(RELAY_PIN, relayState);
    } 
    else if (clickCount == 2) {
      Serial.println(F("2 cliques identificados - Envio de codigo ativado"));
      sendCode(CODE_A); // Deixado comentado conforme solicitado para não abrir o portão
      
      // Pisca o LED "L" duas vezes chamando a função
      blinkLed(2);
    } 
    else if (clickCount >= 3) {
      Serial.println(F("3 ou mais cliques identificados - Alternando Pisca-Alerta"));
      blinkLed(3);
      
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

void loop() {
  checkButtonClicks();
  processClicks();
  handleRelayBlink(); // Processa a lógica do pisca-alerta continuamente
}
