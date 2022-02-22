
#if defined(ARDUINO_AVR_ITSYBITSY32U4_3V)
# define BOARDNAME "Itsy Bitsy 3.3V"
#elif defined(ARDUINO_AVR_MICRO)
# define BOARDNAME "Arduino Pro Micro"
#elif defined(ARDUINO_TRINKET_M0)
# define BOARDNAME "Trinket M0"
#elif defined(ARDUINO_ESP32C3_DEV)
# define BOARDNAME "ESP32C3"
# define I2CSDA 18
# define I2CSCL 19
#elif defined(ESP_PLATFORM)
# define BOARDNAME "STAMP-C3"
# define I2CSDA 0
# define I2CSCL 1
#else
# error "Unknown platform."
#endif

#if defined(ARDUINO_ESP32C3_DEV)
# define OLED_CS0    10
# define OLED_CS1     3
# define OLED_DC      1
# define OLED_RESET   -1
#elif defined(ARDUINO_AVR_MICRO)
# define OLED_CS     20
# define OLED_DC     19
# define OLED_RESET  -1
#endif

#define __ASSERT_USE_STDERR
#include <assert.h>

#include <SPI.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#undef DOINA3221

#if defined(DOINA3221)
# include "Beastdevices_INA3221.h"
#endif

#include "oled.h"


#define OLEDADDR0 0x3c
#define OLEDADDR1 0x3d

const uint32_t bitrate = 2000000;
static Adafruit_SSD1306 dpy0(128, 64, &SPI, OLED_DC, OLED_RESET, OLED_CS0, bitrate);
static Adafruit_SSD1306 dpy1(128, 64, &SPI, OLED_DC, OLED_RESET, OLED_CS1, bitrate);

#if defined(DOINA3221)
static Beastdevices_INA3221 ina3221(INA3221_ADDR40_GND);
#endif

static char status_lines[2][11];
static uint8_t status_line_dirty[2];

static uint32_t last_time_stamp;

static void setupSerial(void)
{
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  for (int i = 0; i < 20 && !Serial; ++i)
    delay(100);
  Serial.println(BOARDNAME " is now ready.");
}


static void scan(void)
{
  uint8_t count = 0;
  for (byte i = 8; i < 120; i++)
  {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0)
    {
      Serial.print ("Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");
      count++;
      delay (1);  // maybe unneeded?
    } // end of good response
  } // end of for loop
  Serial.println ("Finished.");
  Serial.print ("Discovered ");
  Serial.print (count, DEC);
  Serial.println (" device(s).");
}


static uint8_t update_status_lines(void)
{
  static uint8_t dpy = 0;
  if ( status_line_dirty[dpy] )
  {
    oled_write_row(OLEDADDR0 + dpy, 0, 0, status_lines[dpy], 0x0);
    oled_write_row(OLEDADDR0 + dpy, 1, 0, status_lines[dpy], 0x0);
    status_line_dirty[dpy] = 0;
    dpy = (dpy + 1) & 1;
    return 1;
  }
  dpy = (dpy + 1) & 1;
  return 0;
}


#if defined(DOINA3221)
static void printCurrentForChannel(ina3221_ch_t c)
{
  const float current = ina3221.getCurrent(c);
  const float voltage = ina3221.getVoltage(c);
  Serial.print( "Channel " );
  Serial.print(c);
  Serial.print(": ");
  Serial.print( current, 3 );
  Serial.print("A");
  Serial.print(" ");
  Serial.print( voltage, 3 );
  Serial.println("V");
}
#endif

void setup()
{
  setupSerial();

#if defined( LED_BUILTIN_TX)
  // No TX and RX leds, please.
  pinMode( LED_BUILTIN_TX, INPUT);
  pinMode( LED_BUILTIN_RX, INPUT);
#endif

  // Join the i2c bus as a master.
  Wire.begin(I2CSDA, I2CSCL);

  Wire.setClock(400000);

#if 1
  Serial.println("Scanning...");
  scan();
#endif

#if defined(DOINA3221)
  Serial.println("Initializing INA2113 current sensor.");
  ina3221.begin();
  ina3221.reset();
  ina3221.setShuntRes(100, 100, 100);
  printCurrentForChannel(INA3221_CH1);
  printCurrentForChannel(INA3221_CH2);
  printCurrentForChannel(INA3221_CH3);

#endif

  oled_setup(OLEDADDR0);
  oled_pattern(OLEDADDR0, 0x0, 0 );
  oled_write_row(OLEDADDR0, 0, 0, "(C)2022   ", 0x0);
  oled_write_row(OLEDADDR0, 1, 0, "(C)2022   ", 0x0);
  oled_write_row(OLEDADDR0, 2, 0, "GSAS Inc. ", 0x0);
  oled_write_row(OLEDADDR0, 3, 0, "GSAS Inc. ", 0x0);
  oled_write_row(OLEDADDR0, 4, 0, "Monitor 2 ", 0x0);
  oled_write_row(OLEDADDR0, 5, 0, "Monitor 2 ", 0x0);

  oled_setup(OLEDADDR1);
  oled_pattern(OLEDADDR1, 0x0, 0 );
  oled_write_row(OLEDADDR1, 0, 0, "(C)2022   ", 0x0);
  oled_write_row(OLEDADDR1, 1, 0, "(C)2022   ", 0x0);
  oled_write_row(OLEDADDR1, 2, 0, "GSAS Inc. ", 0x0);
  oled_write_row(OLEDADDR1, 3, 0, "GSAS Inc. ", 0x0);
  oled_write_row(OLEDADDR1, 4, 0, "Monitor 3 ", 0x0);
  oled_write_row(OLEDADDR1, 5, 0, "Monitor 3 ", 0x0);

  oled_set_contrast( OLEDADDR0, 0x60 );
  oled_set_contrast( OLEDADDR1, 0x60 );

  const int ir0 = dpy0.begin(SSD1306_EXTERNALVCC);
  const int ir1 = dpy1.begin(SSD1306_EXTERNALVCC);


  // Setup spi display
  if (!ir0)
  {
    Serial.println(F("SSD1306 allocation failed"));
  }
  else
  {
    dpy0.clearDisplay();
    dpy0.setTextSize(1);
    dpy0.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    dpy0.setCursor(0, 0);
    dpy0.println(F("(C)2022 GSAS Inc."));
    dpy0.println(F("Monitor 0"));
    dpy0.println(F("The quick brown"));
    dpy0.println(F("fox jumps over a"));
    dpy0.println(F("lazy dog."));
    dpy0.println(F("0123456789ABCDEFGHIJK"));
    dpy0.println(F("!@#$%^&*()"));
    dpy0.print  (F("last line."));
    dpy0.display();
  }

  // Setup spi display
  if (!ir1)
  {
    Serial.println(F("SSD1306 allocation failed"));
  }
  else
  {
    dpy1.clearDisplay();
    dpy1.setTextSize(1);
    dpy1.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    dpy1.setCursor(0, 0);
    dpy1.println(F("(C)2022 GSAS Inc."));
    dpy1.println(F("Monitor 1"));
    dpy1.println(F("The quick brown"));
    dpy1.println(F("fox jumps over a"));
    dpy1.println(F("lazy dog."));
    dpy1.println(F("0123456789ABCDEFGHIJK"));
    dpy1.println(F("!@#$%^&*()"));
    dpy1.print  (F("last line."));
    dpy1.display();
  }

  //status_line_dirty[0] = 0xff;
  //status_line_dirty[1] = 0xff;
  last_time_stamp = millis();
}



void loop()
{
  const uint32_t current_time_stamp = millis();
  uint32_t elapsed;
  if ( current_time_stamp < last_time_stamp )
  {
    Serial.println("Timestamp has wrapped around.");
    elapsed = 0; // wrapped around (in 51 days.)
  }
  else
  {
    elapsed = current_time_stamp - last_time_stamp;
  }
  last_time_stamp = current_time_stamp;
}


void __assert(const char* __func, const char* __file, int __lineno, const char *__sexp)
{
  // transmit diagnostic informations through serial link.
  Serial.println(__func);
  Serial.println(__file);
  Serial.println(__lineno, DEC);
  Serial.println(__sexp);
  Serial.flush();
  // abort program execution.
  abort();
}
