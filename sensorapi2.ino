#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

/*    
   This driver uses the Adafruit unified sensor library (Adafruit_Sensor),
   which provides a common 'type' for sensor data and some helper functions.
   
   To use this driver you will also need to download the Adafruit_Sensor
   library and include it in your libraries folder.

   You should also assign a unique ID to this sensor for use with
   the Adafruit Sensor API so that you can identify this particular
   sensor in any data logs, etc.  To assign a unique ID, simply
   provide an appropriate value in the constructor below (12345
   is used by default in this example).
   
   Connections
   ===========
   Connect SCL to I2C SCL Clock
   Connect SDA to I2C SDA Data
   Connect VDD to 3.3V or 5V (whatever your logic level is)
   Connect GROUND to common ground

   Digital 12 to control of light output (nevation vs digital 13)
   Digital 13 to control of light output (negation vs digital 12)

   I2C Address
   ===========
   Default address is 0x39 (floating)
                      0x29 (tie to ground)  TSL2561_ADDR_LOW
                      0x49 (tie to vcc)     TSL2561_ADDR_HIGH

   Sarjamonitorin tulostus
   =======================
   xx.xx lux  anturin mittaama ympäristön valoisuus
   xx         nopeita muutoksia vastustavan laskurin arvo
   x          0 - valo ei pala, 1 - valo palaa
   xxx        valoisuuden raja-arvo, johon vertailua tehdään
   xxxxx      LIGHT OFF / LIGHT ON

   Shield-kortti
   =============
   Relekorttina toimii Electrow.com myymä neljän releen toteutus (D4-D7)
   Kortti on varsinaisesti tarkoitettu Arduino Duemilanove:n kanssa käytettäväksi ja
   Unolla jääkin siksi läpi kytkemättä I2C:n ohjaus ja 5V pinnit.
    
   History
   =======
   14.04.2019 - First version
   16.04.2019 - Added control pins to relay shield
*/
   
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

/**************************************************************************
    Displays some basic information on this sensor from the unified
    sensor API sensor_t type (see Adafruit_Sensor for more information)
**************************************************************************/
void displaySensorDetails()
{
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" lux");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" lux");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" lux");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

/**************************************************************************
    Configures the gain and integration time for the TSL2561
**************************************************************************/
void configureSensor()
{
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  // tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */  
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("16x");
  Serial.print  ("Timing:       "); Serial.println("101ms");
  Serial.println("------------------------------------");
}

/**************************************************************************
    Arduino setup function (automatically called at startup)
**************************************************************************/

const int relayPin1 = 4;   /* Ensimmäisen releen ohjauslähtö */
const int relayPin2 = 5;   /* Toisen releen ohjauslähtö */
const int relayPin3 = 6;   /* Kolmannen releen ohjauslähtö */
const int relayPin4 = 7;   /* Neljännen releen ohjauslähtö */

void setup()
{
  Serial.begin(9600);
  Serial.println("Light Sensor Test"); Serial.println("");

  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(relayPin3, OUTPUT);
  pinMode(relayPin4, OUTPUT);
  
  /* Initialise the sensor */
  //use tsl.begin() to default to Wire, 
  //tsl.begin(&Wire2) directs api to use Wire2, etc.
  if(!tsl.begin())
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  
  /* Display some basic information on this sensor */
  displaySensorDetails();
  
  /* Setup the sensor gain and integration time */
  configureSensor();
  
  /* We're ready to go! */
  Serial.println("");
}

/**************************************************************************
    Arduino loop function, called once 'setup' is complete (your own code
    should go here)
**************************************************************************/

int slowedLoop = 0;       /* Nopeita muutoksia vastustavan laskurin arvo */
int slowedLoop_max = 20;  /* Nopeita muutoksia vastustavan laskurin maksimiarvo */
int light = 0;            /* lamppu ei päällä 0, lamppu päällä 1 */

int light_limit = 100;    /* valoisuuden raja-arvo valon sytytykseen/sammutukseen */
int hyst_up = 120;        /* valon sytyttyä vertailuarvo */
int hyst_down = 100;      /* valon sammuttua vertailuarvo */


void loop() 
{  
  
  /* Get a new sensor event */ 
  sensors_event_t event;
  tsl.getEvent(&event);
 
  /* Display the results (light is measured in lux) */
  if (event.light)
  {
    Serial.print(event.light); Serial.print(" lux ");
  }
  else
  {
    /* If event.light = 0 lux the sensor is probably saturated
       and no reliable data could be generated! */
    Serial.println("Sensor overload or no light");
  }  
  
  /* verrataan ympäristön valoisuutta raja-arvoon */
  if (event.light < light_limit)
  {
      Serial.print(slowedLoop); Serial.print("  ");
      Serial.print(light); Serial.print("  "); Serial.print(light_limit);
        if (light > 0)
        {
          Serial.println(" Light ON");
        } else {
          Serial.println(" Light OFF");
        }
        
      slowedLoop = slowedLoop + 1;

        if (slowedLoop > slowedLoop_max)
        {
          slowedLoop = slowedLoop - 1;
          /* asetetaan valon ohjauspinni ylös */
          digitalWrite(relayPin1, HIGH);
          digitalWrite(relayPin2, LOW);
          digitalWrite(relayPin3, LOW);
          digitalWrite(relayPin4, LOW);
            if (light < 1)
            {
              /* asetetaan valon sytyttyä vertailarvo */
              light_limit = hyst_up;
            }  
          light = 1;
        }
      
  }
  else
  {
      Serial.print(slowedLoop); Serial.print("  ");
      Serial.print(light); Serial.print("  "); Serial.print(light_limit);
        if (light < 1)
        {
          Serial.println(" Light OFF");
        } else {
          Serial.println(" Light ON");
        }
     
      slowedLoop = slowedLoop - 1;

        if (slowedLoop < 0)
        {
          slowedLoop = slowedLoop + 1;
          /* asetetaan valon ohjauspinni alas */
          digitalWrite(relayPin1, LOW);
          digitalWrite(relayPin2, LOW);
          digitalWrite(relayPin3, LOW);
          digitalWrite(relayPin4, LOW);
            if (light > 0)
            {
              /* aseteaan valon sammuttua vertailarvo */
              light_limit = hyst_down;
            }  
          light = 0;
        }
  }
  /* hidastetaan looppia */ 
  delay(100);
}
