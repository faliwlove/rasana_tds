#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <DallasTemperature.h> //memasukkan library untuk sensor temperatur
#include <OneWire.h>
#include <Wire.h>
//=================DEKLARASI OBJEK UNTUK SENSOR SUHU==============
#define ONE_WIRE_BUS D3 //DEKLARASI PIN INPUT UNTUK SENSOR SUHU
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensorSuhu(&oneWire);


//masukan username dan password agar andika_doorlock dapat terhubung ke server
const char* ssid = "EduFarm RR#1"; // masukan Nama Wifi nya
const char* password = "rasana#1"; // isi password dari wifi

String moist_value_string;
String moist_value_string2;
String postData;
String postData2;

#define TdsSensorPin A0  //Deklarasi pin untuk input dari sensor
#define VREF 3.2         //Analog reference voltage dari Wemos D1 mini
#define SCOUNT  30       //Jumlah array untuk sampel data
int analogBuffer[SCOUNT],analogBufferTemp[SCOUNT],analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0, EC,ppm,temperature = 25;

void readTDS(){
//=================Pembacaan sensor setiap 40 ms==================
    static unsigned long analogSampleTimepoint = millis();
       if(millis()-analogSampleTimepoint > 40U)     
       {
         analogSampleTimepoint = millis();
         analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //baca sensor
         analogBufferIndex++;
         if(analogBufferIndex == SCOUNT)analogBufferIndex = 0; //membaca data sebanyak 30 kali
       }

//===================Print data setiap 800 ms==================
       static unsigned long printTimepoint = millis();
       if(millis()-printTimepoint > 800U)
       {
          printTimepoint = millis();
          for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
            analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
          averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0; // hasil akhir merupakan nilai median dari 30 sampel data sensor
          float compensationCoefficient=1.0+0.02*(temperature-25.0);    //rumua koefisien temperatur
          float compensationVoltage=averageVoltage/compensationCoefficient;  //temperature compensation
          tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5; //convert voltage value to tds value
          EC=tdsValue/500;  // menkonversi nilai TDS ke EC
         
          Serial.println("    " + String(ppm) + "ppm");
          Serial.print("    TDS Value: ");
          Serial.print(tdsValue,0);  // menampilkan nilai TDS
          Serial.println(" ppm");
          Serial.print("    EC Value: "); // menampilkan nilai EC
          Serial.println(EC,2);
          Serial.print("    Temperature: "); // menampilkan nilai suhu air
          Serial.println(temperature,1);
          
       }
}

//=========Filtering data dengan mencari nilai mediannya=========
int getMedianNum(int bArray[], int iFilterLen)
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      /*Sorting data dari nilai terkecil ke terbesar*/
      for (j = 0; j < iFilterLen - 1; j++)
      {
      for (i = 0; i < iFilterLen - j - 1; i++)
          {
        if (bTab[i] > bTab[i+1])
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i+1];
        bTab[i+1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
      }else{
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
      }
}
void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);

  WiFi.begin(ssid, password); //--> menghubungkan ke router wifi
  Serial.println("");
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("Sudah Terhubung ke : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  readTDS();
  moist_value_string = String(tdsValue);
  moist_value_string2 = String(temperature);
  //program koneksi ke server
  WiFiClient client;
  HTTPClient http;
  postData = "tds=" + moist_value_string;
  postData2 = "wt=" + moist_value_string2;
  http.begin(client, "http://192.168.137.1/rasana/getdata_tds.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(postData);
  String payload = http.getString();
  http.end();  //Close connection
  delay(500);
}
