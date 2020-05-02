#include <avr/io.h>
//#include <util/delay.h>

#include <Ethernet.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <EEPROM.h>


#define FOSC 16000000L // Clock Speed
#define BAUD 250000
#define MYUBRR (FOSC/16/BAUD)-1

#define NR_OF_CHAN 50

#define DEBUG 0

struct dmxval {
  unsigned char currentval = 0;
  unsigned char targetval = 0;
  int transition = 0;
};
#if DEBUG
const char* mqtt_server = "192.168.2.1";
#else
const char* mqtt_server = "192.168.2.1";
#endif

EthernetClient ethClient;
PubSubClient client(ethClient);

long lastMsg = 0;
char msg[50];
int value = 0;

char* SwitchSernum = "D0000";
char* inTopic = "D0000/+/command";

unsigned long nextTime = 0;
bool updateVals = false;


dmxval* data[NR_OF_CHAN];


void callback(char* topic, byte* payload, unsigned int length) {
  String title = "";
  for (int i = 0; i < 15; i++) {
    title += topic[i];
  }

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  title = title.substring(title.indexOf("/") + 1);
  title = title.substring(0, title.indexOf("/"));

  unsigned char target = 0;
  unsigned char dmxchannel = title.toInt();


  //Serial.println(title);

  if (message.indexOf("state") != -1)
  {
    //Serial.println(message);
    if (message.indexOf("ON") != -1)
      target = 255;
    else if (message.indexOf("OFF") != -1)
      target = 0;

    if (message.indexOf("brightness") != -1)
    {
      String bright = message.substring(message.indexOf("brightness"));
      bright = bright.substring((bright.indexOf(":") + 2), (bright.indexOf(":") + 5));
      if (bright.indexOf("}") != -1)
      {
        bright = bright.substring(0, bright.indexOf("}"));
      }
      if (bright.indexOf(",") != -1)
      {
        bright = bright.substring(0, bright.indexOf(","));
      }

      int inttarget = bright.toInt();
      target = inttarget;

      //Serial.println(bright);

    }

    data[dmxchannel]->targetval = target;
    if (message.indexOf("transition") != -1)
    {
      String trans = message.substring(message.indexOf("transition"));
      trans = trans.substring((trans.indexOf(":") + 2), (trans.indexOf(":") + 5));
      if (trans.indexOf("}") != -1)
      {
        trans = trans.substring(0, trans.indexOf("}"));
      }
      if (trans.indexOf(",") != -1)
      {
        trans = trans.substring(0, trans.indexOf(","));
      }

      int inttrans = trans.toInt();
      if (inttrans == 0)
      {
        data[dmxchannel]->transition = inttrans;
      }
      else
      {
        int transTarget = ((int) data[dmxchannel]->targetval - (int)data[dmxchannel]->currentval) / (inttrans * 20);
        if (transTarget == 0 && data[dmxchannel]->targetval != data[dmxchannel]->currentval)
        {
          transTarget = 1;
        }

        data[dmxchannel]->transition = transTarget;
      }
      //Serial.println(transTarget);
    }
    else
    {
      data[dmxchannel]->transition = 0;
    }
  }

  //Serial.println(data[1]->currentval);

}

void reconnect() {
  // Loop until we're reconnected
  if (!client.connected()) {
    // Attempt to connect
    #if DEBUG
    Serial.println("Trying to connect...");
    #endif
    if (client.connect(SwitchSernum, "sibrecht", "iPWxQfnTJ7h2DHEbP12o0X5NDZLj")) { //clientId
      //Serial.println(inTopic);
      client.subscribe(inTopic);
      #if DEBUG
      Serial.println("Connected");
      #else
      client.publish("onlinedevice", SwitchSernum);
      #endif
    }
  }
}


void setup() {

#if DEBUG

  Serial.begin(9600);

#endif
  for (int i = 0; i < 5; i++)
  {
    SwitchSernum[i] = EEPROM.read(i);
  }

  (String(SwitchSernum) + "/+/command").toCharArray(inTopic, 17);

  int intSwitchSernum = String(SwitchSernum).substring(1).toInt();

  byte mac[] = {
    0xDE, 0xAD, 0xBE, 0xEF, (intSwitchSernum / 100), (intSwitchSernum % 100)
  };

  IPAddress nonValidIp(0, 0, 0, 0);

  do
  {
    delay(5);

    Ethernet.begin(mac);

    delay(5);
  }
  while (Ethernet.localIP() == nonValidIp);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //DMX PART
  INIT_values();

#if not DEBUG

  // set serial port and set pin to output:
  USART_Init(MYUBRR);
  //pinMode(1, OUTPUT);
  DDRD |= (1 << DDD1);
#endif

}

void loop() {

  ////////////KEEP MQTT RUNNING
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
#if DEBUG
  Serial.println(data[25]->currentval);
#endif
  if (nextTime < millis())
  {
    nextTime = millis() + 50;
    updateVals = true;
  }

#if not DEBUG

  Break();

#endif
  for (int i = 0; i < NR_OF_CHAN; i++)
  {
    unsigned char * currentval = &(data[i]->currentval);
    unsigned char * targetval = &(data[i]->targetval);
    int * transition = &(data[i]->transition);

    if (updateVals)
    {
      if (*transition != 0)
      {
        if (abs((int)*currentval - (int)*targetval) < abs(*transition ))
        {
          *currentval = *targetval;
          *transition = 0;
        }
        else
        {
          *currentval += *transition;
        }
      }
      else
      {
        *currentval = *targetval;
      }
    }

#if not DEBUG

    USART_Transmit(data[i]->currentval);

#endif
  }

  updateVals = false;
  delay(10);
}

void INIT_values()
{
  for (int i = 0; i < NR_OF_CHAN;  i++)
  {
    data[i] = new dmxval();
  }
  data[0]->currentval = 0;
}

void USART_Init(unsigned int ubrr)
{
  /*Set baud rate */
  UBRR0H = (unsigned char)(ubrr >> 8);
  UBRR0L = (unsigned char)ubrr;

  /* Set frame format: 8data, 2stop bit */
  UCSR0C = 0;
  UCSR0C = (3 << UCSZ00) | (1 << USBS0);
}

void Break(void)
{

  //Disable TX
  UCSR0B &= ~(1 << TXEN0);

  //Break min 88µs
  PORTD &= ~(1 << PORTD1);
  delayMicroseconds(100);

  //MAB min 8µs
  PORTD |= (1 << PORTD1);
  delayMicroseconds(20);

  //Enable TX
  UCSR0B = (1 << TXEN0);
}

void USART_Transmit(unsigned char data)
{
  /* Wait for empty transmit buffer */
  while (!(UCSR0A & (1 << UDRE0)))
  {}
  /* Put data into buffer, sends the data */
  UDR0 = data;
}
