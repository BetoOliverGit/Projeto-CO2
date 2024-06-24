#include <Adafruit_MCP4725.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define addr_ADC  0x60
#define addr_OLED 0x3C

#define LARGURA_OLED 128
#define ALTURA_OLED 64
#define RESET_OLED -1

#define tamanho_buffer_uart 26
#define tamanho_msg_uart 16

//#define adc_max_step 4096
//#define adc_max_volt 3300

#define max_mV_saida 1000.0

#define max_step_saida 1241.0// 1241.0 steps corresponem a 1000.0mV

//#define max_CO2_ppm 200000.0// 20% CO2
//#define max_CO2_percent 20.0

#define span_mV 50.0  //span
#define offtset_mV 287.0 //offset  OBS: AO COLOCAR ESSE VALOR O DISPLAY DA INCUBADORA MOSTRA 0.0
#define min_valor_DAC 357

//dac 500  402mv  2,3%
//dac 400  322mv  0,7%
//dac 1200 967mv 13,6%

// 9,2%       758mV
//20,0%         YmV
//Y = 1648mV

//4095step   3300mV
//x          1000mV
//x = 1241step

//200000     1648
//50000      Zstep
//Z=412step

//1241   1000
//vari   W

//de 0 a 4095step 0 a 3300mV  0 a 200000ppm 
//20.0% 200.000ppm 1.0% 10.000ppm 0.1% 1.000ppm
 
Adafruit_SSD1306 display(LARGURA_OLED, ALTURA_OLED, &Wire, RESET_OLED);

Adafruit_MCP4725 dac;

char inData[ tamanho_buffer_uart - 1 ];
char inChar;
byte indice = 0;
String palavra;

int CO2_filter = 0;
int CO2_ufilter = 0;

long CO2_ppm_filter = 0;
long CO2_ppm_ufilter = 0;

int dac_step = min_valor_DAC;

float display_CO2_ppm = 0.0;

int display_mV_DAC = offtset_mV;

void tela_setup()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(5,25);
    display.print("Medidor de Co2 20%");
    display.display();
    delay(3000);
    display.clearDisplay();
}

void tela_leitura()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
	
    display.setCursor(10,10);
    display.print(display_CO2_ppm,1);

    display.setCursor(60,10);
    display.print("% Co2");

    display.setCursor(10,25);
    display.print(dac_step);

    display.setCursor(60,25);
    display.print("step");

    display.setCursor(10,40);
    display.print(display_mV_DAC);

    display.setCursor(60,40);
    display.print("mV");

    display.display();
}

void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600); //serial via usb

    Serial1.begin(9600); //Serial1.begin(9600); serial pinos TX RX

    dac.begin(addr_ADC);

    dac.setVoltage(dac_step,false);

    display.begin(SSD1306_SWITCHCAPVCC, addr_OLED);

    tela_setup();
}

void loop() {
  // put your main code here, to run repeatedly:
    indice = 0;
    
    while( Serial1.available() > 0 )//while( Serial1.available() > 0 )
    {
      inChar = Serial1.read();//Serial1.read();
      inData[indice] = inChar;
      indice++;
    }

    while ( ( inData[indice-1] !=  '\r' )&&( indice > 0 ) )
    {
      indice--;      
    }

    if ( indice > tamanho_msg_uart - 1 )
    {
      palavra = inData;
      Serial.println(palavra);
      delay(10);

      //0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15
      //Z     #  #  #  #  #  \r \n
      //Z     2  0  0  0  0     z     2   0   0   0   0   \r

      if ( ( inData[indice -16] == 'Z' )&&( inData[indice -8] == 'z' ) )
      {     
        CO2_ppm_ufilter = inData[indice -2] - '0';
        CO2_ppm_ufilter = CO2_ppm_ufilter + ( inData[indice -3] - '0' )*10;
        CO2_ppm_ufilter = CO2_ppm_ufilter + ( inData[indice -4] - '0' )*100;
        CO2_ppm_ufilter = CO2_ppm_ufilter + ( inData[indice -5] - '0' )*1000;
        CO2_ppm_ufilter = CO2_ppm_ufilter + ( inData[indice -6] - '0' )*10000;
        CO2_ppm_ufilter = CO2_ppm_ufilter*10;

        CO2_ppm_filter = inData[indice -10] - '0';
        CO2_ppm_filter = CO2_ppm_filter + ( inData[indice -11] - '0' )*10;
        CO2_ppm_filter = CO2_ppm_filter + ( inData[indice -12] - '0' )*100;
        CO2_ppm_filter = CO2_ppm_filter + ( inData[indice -13] - '0' )*1000;
        CO2_ppm_filter = CO2_ppm_filter + ( inData[indice -14] - '0' )*10000;
        CO2_ppm_filter = CO2_ppm_filter*10;        

        display_CO2_ppm = (float)(CO2_ppm_filter)/10000.0;

        display_mV_DAC = offtset_mV + span_mV*display_CO2_ppm;

        if ( display_mV_DAC < offtset_mV +1 )
        {
          display_mV_DAC = offtset_mV;
        }

        dac_step = max_step_saida*( (float)(display_mV_DAC)/max_mV_saida );

        //dac_step = max_step_saida*( (float)CO2_ppm_filter/CO2_max_ppm );

        //if ( dac_step < valor_min_DAC )
        //{
        //  dac_step = valor_min_DAC;
        //}

        dac.setVoltage(dac_step,false);

        //diplay_mv_DAC = max_volt_saida*( (float)dac_step/max_step_saida );

        /*Serial.print("CO2 ppm: ");
        Serial.println(CO2_ppm_filter);
        delay(10);

        Serial.print("step DAC: ");
        Serial.println(dac_step);
        delay(10);

        Serial.print("mV Saida: ");
        Serial.println(display_mV_DAC);
        delay(10);
        */
      }   
    }

    tela_leitura();

    delay(1000);
}
