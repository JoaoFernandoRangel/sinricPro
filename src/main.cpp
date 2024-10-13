#include <Arduino.h>
#include <WiFi.h>
#include <map>

#include <SinricPro.h>
#include <SinricProDevice.h>
#include <Capabilities/PowerStateController.h>
#include <Capabilities/RangeController.h>

class BombaAutomatica
    : public SinricProDevice,
      public PowerStateController<BombaAutomatica>,
      public RangeController<BombaAutomatica>
{
  friend class PowerStateController<BombaAutomatica>;
  friend class RangeController<BombaAutomatica>;

public:
  BombaAutomatica(const String &deviceId) : SinricProDevice(deviceId, "BombaAutomatica") {};
};

#define APP_KEY "62d9390b-e2bb-4a8d-b296-4fc42fd5560a"
#define APP_SECRET "075fe7d7-fd8b-4bfc-9d93-d34198456d12-e6cb023f-2ef5-404b-be7b-314c41c9fd6f"
#define DEVICE_ID "670b10bfdeddece34bacadd6"

// #define SSID "JORRIBA-2G"
// #define PASS "11BCCE5346"
#define SSID "S23"
#define PASS "bemvindo"

#define retornaMin(x) (60 * 1000 * (x))
#define retornaSeg(x) (1000 * (x))

#define BAUD_RATE 115200
#define RELAYPIN_1 17

BombaAutomatica &bombaAutomatica = SinricPro[DEVICE_ID];

/*************
** Variables *
**************/
// Variáveis de operação automática
uint8_t contador = 0;
bool flag = true;
unsigned long agora = 0, antes0 = 0, antes1 = 0;
unsigned long toff = 1;
unsigned long ton = 10;
// PowerStateController
bool globalPowerState;
// RangeController
std::map<String, int> globalRangeValues;

// Declarações de funções
bool onPowerState(const String &deviceId, bool &state);
bool onRangeValue(const String &deviceId, const String &instance, int &rangeValue);
bool onAdjustRangeValue(const String &deviceId, const String &instance, int &valueDelta);
void updateRangeValue(String instance, int value);
void setupWiFi(uint8_t &contador);
void setupSinricPro();

void setup()
{
  Serial.begin(BAUD_RATE);
  Serial.println(retornaSeg(1));
  pinMode(17, OUTPUT);
  setupWiFi(contador);
  setupSinricPro();
}
void autom()
{
  agora = millis();
  // Se a bomba está desligada e o tempo toff passou
  if (!flag && (agora - antes0 >= retornaMin(toff)))
  {
    Serial.printf("Passou %d segundos\n", toff);
    Serial.printf("Bomba Ligada\n");
    bombaAutomatica.sendPowerStateEvent(true);
    antes0 = agora; // Atualiza o tempo de referência para o próximo acionamento
    antes1 = agora; // Atualiza o tempo de referência para o próximo acionamento
    flag = true;    // Liga a bomba
  }

  // Se a bomba está ligada e o tempo ton passou
  else if (flag && (agora - antes1 >= retornaSeg(ton)))
  {
    Serial.printf("Passou %d segundos\n", ton);
    Serial.printf("Bomba Desligada\n");
    bombaAutomatica.sendPowerStateEvent(false);
    antes1 = agora; // Atualiza o tempo de referência para o próximo desligamento
    flag = false;   // Desliga a bomba
  }
}

/********
 * Loop *
 ********/
void loop()
{
  autom();
  if (WiFi.status() == WL_CONNECTED)
  {
    SinricPro.handle();
  }
  else
  {
    setupWiFi(contador);
    setupSinricPro();
  }
  vTaskDelay(pdMS_TO_TICKS(10));
}

/*************
 * Operação Automática *
 *************/

/*************
 * Callbacks *
 *************/

// PowerStateController
bool onPowerState(const String &deviceId, bool &state)
{
  Serial.printf("[Device: %s]: Powerstate changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");
  // Adicionar digitalWrite na porta correta
  globalPowerState = state;
  bombaAutomatica.sendPowerStateEvent(state);
  return true; // request handled properly
}

// RangeController
bool onRangeValue(const String &deviceId, const String &instance, int &rangeValue)
{
  // Separar o valor para a variável correta de acordo com o instance
  globalRangeValues[instance] = rangeValue;
  if (instance == "TempoOn")
  {
    Serial.printf("[Device: %s]: Value for \"%s\" changed to %d\r\n", deviceId.c_str(), instance.c_str(), rangeValue);
    ton = rangeValue;
    bombaAutomatica.sendRangeValueEvent(instance, rangeValue);
    return true;
  }
  else if (instance == "TempoOff")
  {
    Serial.printf("[Device: %s]: Value for \"%s\" changed to %d\r\n", deviceId.c_str(), instance.c_str(), rangeValue);
    toff = rangeValue;
    bombaAutomatica.sendRangeValueEvent(instance, rangeValue);
    return true;
  }
  return false;
}

bool onAdjustRangeValue(const String &deviceId, const String &instance, int &valueDelta)
{
  globalRangeValues[instance] += valueDelta;
  Serial.printf("[Device: %s]: Value for \"%s\" changed about %d to %d\r\n", deviceId.c_str(), instance.c_str(), valueDelta, globalRangeValues[instance]);
  // Separar o valor para a variável correta de acordo com o instance
  globalRangeValues[instance] = valueDelta;
  return true;
}

/**********
 * Events *
 *************************************************
 * Examples how to update the server status when *
 * you physically interact with your device or a *
 * sensor reading changes.                       *
 *************************************************/
// RangeController
void updateRangeValue(String instance, int value)
{
  bombaAutomatica.sendRangeValueEvent(instance, value);
}

/*********
 * Setup *
 *********/

void setupSinricPro()
{
  // PowerStateController
  bombaAutomatica.onPowerState(onPowerState);
  // RangeController
  bombaAutomatica.onRangeValue("TempoOn", onRangeValue);
  bombaAutomatica.onAdjustRangeValue("TempoOn", onAdjustRangeValue);
  bombaAutomatica.onRangeValue("TempoOff", onRangeValue);
  bombaAutomatica.onAdjustRangeValue("TempoOff", onAdjustRangeValue);
  SinricPro.onConnected([]
                        { Serial.printf("[SinricPro]: Connected\r\n"); });
  SinricPro.onDisconnected([]
                           { Serial.printf("[SinricPro]: Disconnected\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
};

void setupWiFi(uint8_t &contador)
{
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  WiFi.begin(SSID, PASS);
  Serial.printf("[WiFi]: Connecting to %s", SSID);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf(".");
    vTaskDelay(pdMS_TO_TICKS(200));
    contador++;
    if (contador == 50)
    {
      ESP.restart();
      //Libera a task de operação automática que reinicia a placa 00h.
    }
  }
  contador = 0;
  Serial.printf("connected\r\n");
}
