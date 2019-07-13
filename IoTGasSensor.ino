#include <SoftwareSerial.h>

#define RX 10                                                                   //Serial Port Software
#define TX 11                                                                   //Serial Port Software


const int     MQ_PIN = A0;                                                      //Analog input channel to use
const int     RL_VALUE= 5;                                                      //define the load resistance on the board, in kilo ohms
const float   RO_CLEAN_AIR_FACTOR = 9.83;                                       //RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,which is derived from the chart in datasheet
const double  MQ_DELAY = 500;                                                   //define which analog input channel you are going to use

const int     CALIBRATION_SAMPLE_TIMES = 50;                                    //How many samples you are going to take in the calibration phase
const int     CALIBRATION_SAMPLE_INTERVAL = 500;                                //Time interval(in milisecond) between each samples in the cablibration phase

const int     READ_SAMPLE_TIMES = 5;                                            //define how many samples you are going to take in normal operation
const int     READ_SAMPLE_INTERVAL = 50;                                        //define the time interal(in milisecond) between each samples in normal operation


const String AP = "";                                                     // WIFI name
const String PASS = "";                                                 // WIFI password
const String APIKEY = "";                                       // ThingSpeak API Key
const String HOST = "api.thingspeak.com";                                       // API IoT ThingSpeak Plattform
const String PORT = "";                                                       // Listen Port
const String field1 = "field1";
const String field2 = "field2";
const String field3 = "field3";
const String field4 = "field4";


String urlBase = "GET https://api.thingspeak.com/update?api_key=";              //URL estatic ThingSpeak API
String urlData = "";                                                            //URL dinamic ThingSpeak API
int countTrueCommand;
int countTimeCommand;
int countRetry = 5;                                                             // Attempts count 

int dataLPG;                                                                    //Propano or butano
int dataH2;                                                                     //Hydrogen
int dataCH4;                                                                    //Metano
int dataCO;                                                                     //Carbon Monoxide


int loopTimes = 10;                                                              // loop simulation times
boolean found = false;
boolean statusResponse = false;


float           r0 = 0.0;                                                       //Sensor resistance. See sensor specifications 
float           R1 = 0.0;                                                       //Sensor resistance. See sensor specifications 
float           rs = 0.0;                                                       //Sensor resistance. 


/********* Gas Identity **********************************************************************************************************************************/
int GAS_LPG = 0;                                                    //Propano or butano
int GAS_H2  = 1;                                                    //Hydrogen
int GAS_CH4 = 2;                                                    //Metano
int GAS_CO  = 3;                                                    //Carbon monoxide
/**********************************************************************************************************************************************************/

/*** LPG Curve Points Datasheet ***************************************************************************************************************************/

//Get two points of curve based on the chart provided in the MQ-2 datasheet
const float lx0 = 200;
const float ly0 = 1.7;
const float lx1 = 10000;
const float ly1 = 0.28;
 
//Take log10 of each point {x, y}
const float lp0[] = { log10(lx0), log10(ly0) };
const float lp1[] = { log10(lx1), log10(ly1) };

//Find slope: slope = (y1-y0)/(x1-x0)
const float lslope = (lp1[1] - lp0[1]) / (lp1[0] - lp0[0]);

//Find abcise coordenate: slope = (y1-y0)/(x1-x0)
const float lcoord = lp0[1] - lp0[0] * lslope;

// Define LPG Curve
float lpgCurve[3] = {lp0[0], lp0[1], lslope};

/**********************************************************************************************************************************************************/

/*** H2 Curve Points Datasheet ***************************************************************************************************************************/

//Get two points of curve based on the chart provided in the MQ-2 datasheet
const float hx0 = 200;
const float hy0 = 2.1;
const float hx1 = 10000;
const float hy1 = 0.34;
 
//Take log of each point {x, y}
const float hp0[] = { log10(hx0), log10(hy0) };
const float hp1[] = { log10(hx1), log10(hy1) };

//Find slope and abcise coordenate: slope = (y1-y0)/(x1-x0)
const float hslope = (hp1[1] - hp0[1]) / (hp1[0] - hp0[0]);

//Find abcise coordenate: coordenate = b = y -mx
const float hcoord = hp0[1] - hp0[0] * hslope;

// Define LPG Curve
float h2Curve[3] = {hp0[0], hp0[1], hslope};

/**********************************************************************************************************************************************************/

/*** SMOKE/CH4 Curve Points Datasheet ***************************************************************************************************************************/

//Get two points of curve based on the chart provided in the MQ-2 datasheet
const float sx0 = 200;
const float sy0 = 3.0;
const float sx1 = 10000;
const float sy1 = 0.7;
 
//Take log of each point {x, y}
const float sp0[] = { log10(sx0), log10(sy0) };
const float sp1[] = { log10(sx1), log10(sy1) };

//Find slope and abcise coordenate: slope = (y1-y0)/(x1-x0)
const float sslope = (sp1[1] - sp0[1]) / (sp1[0] - sp0[0]);

//Find abcise coordenate: coordenate = b = y -mx
const float scoord = sp0[1] - sp0[0] * sslope;

// Define SMOKE Curve
float ch4Curve[3] = {sp0[0], sp0[1], sslope};

/**********************************************************************************************************************************************************/


/*** CO Curve Points Datasheet ***************************************************************************************************************************/

//Get two points of curve based on the chart provided in the MQ-2 datasheet
const float cx0 = 200;
const float cy0 = 5.0;
const float cx1 = 10000;
const float cy1 = 1.5;
 
//Take log of each point {x, y}
const float cp0[] = { log10(cx0), log10(cy0) };
const float cp1[] = { log10(cx1), log10(cy1) };

//Find slope and abcise coordenate: slope = (y1-y0)/(x1-x0)
const float cslope = (cp1[1] - cp0[1]) / (cp1[0] - cp0[0]);

//Find abcise coordenate: coordenate = b = y -mx
const float ccoord = cp0[1] - cp0[0] * cslope;

// Define CO Curve
float coCurve[3] = {cp0[0], cp0[1], cslope};

/**********************************************************************************************************************************************************/



SoftwareSerial esp8266(RX,TX); 

void setup() {
 
  Serial.begin(9600);
  esp8266.begin(115200);
  
  //1. Sensor calibrating
  //Resistance value of MQ-2 is difference to various kinds and various concentration gases. So,When using this components, sensitivity adjustment is very necessary.
  //The sensor resistor RS and load resistor RL form a voltage divider. Based on the chart provided in the MQ-2 datasheet , RS in clean air under given temperature and 
  //humidity is a constant. The ratio of RS/R0 in clean air is 9.8 as described in datasheet.   //We will first calibrate the sensor. Place sensor in clean air. 
  //We will get R0 value by dividing it by RS/R0 value in clean air.
  
  Serial.println("*******************Init***************************");
  Serial.print("Start sensor calibrating...\n");
  
  //Calibrating the sensor. Please make sure the sensor is in clean air 
  r0 = MQCalibration(MQ_PIN);                                     
  Serial.println("Calibration sensor done...\n"); 

  //Excecute AT Commands Test
  Serial.println("Exec AT commands test...\n"); 
  sendCommand("AT",countRetry,2000,"OK");

  if(statusResponse == true){

    if(connectWiFi()){
      urlData = urlBase + APIKEY +"&";

      //Simulate loop() function
      for(int i=0; i<= loopTimes; i++){
        loopSimulation();
        urlData = urlBase + APIKEY +"&";
        }
      }
    else{Serial.println("WiFi Connection Error");}
   }
  else{Serial.println("AT Command error");}
  
  Serial.println("***************End********************");

}

void loop() {
  
  
  }

/*************************** sendCommand ***************************************************************************************************************
Input:    command:     command to execute
          maxTimes:    Number of attempts to execute command
          readReplay:  Command response
Output:  Void
Remarks: AT Send command 
********************************************************************************************************************************************************/ 

void sendCommand(String command, int maxTimes, int delayTime, char readReplay[]) {
  statusResponse = false;
  while(countTimeCommand < (maxTimes*1))
  {
    Serial.print("exec command => " + command + "\t" );
    esp8266.println(command);
    delay(delayTime);
    
        
    if(esp8266.find(readReplay))
    {
      found = true;
      break;
    }
  
    countTimeCommand++;
  }
  
  if(found == true)
  {
    Serial.println("\t OKI");
    countTrueCommand++;
    countTimeCommand = 0;
    statusResponse = true;
  }
  
  if(found == false)
  {
    Serial.println("FAIL");
    countTrueCommand = 0;
    countTimeCommand = 0;
    statusResponse = false;
  }
  
  found = false;
 }


/*************************** connectWiFi ***************************************************************************************************************
Input:   NA
Output:  Connection status
Remarks: WiFi connection
********************************************************************************************************************************************************/ 

 boolean connectWiFi(){
  
  //Sets the current Wi-Fi mode: 1: Station mode
  sendCommand("AT+CWMODE=1",countRetry,2000,"OK");
  if(statusResponse == true){
    sendCommand("AT+CWJAP=\""+ AP +"\",\""+ PASS +"\"",5000,20,"OK");
    if(statusResponse == true){
      Serial.println("WiFi connection succesful");
      return true;
      }
    else{
      Serial.println("AT+CWMODE Command error");
      return false;
      }
    }
  else{
    Serial.println("AT+CWMODE Command error");
    return false;
    }
  
}






/*************************** Function loopSimulation ************************************************************************************************
Input:    command:     command to execute
          maxTimes:    Number of attempts to execute command
          readReplay:  Command response
Output:  Void
Remarks: Loop Simulation
*****************************************************************************************************************************************************/ 

void loopSimulation(){

  getDataPIN();
  dataLPG = getDataLPG();
  dataH2 = getDataH2();
  dataCH4 = getDataCH4();
  dataCO = getDataCO();
  Serial.print("\n"); 
  

  //build URL Thingspeak API
  //GET https://api.thingspeak.com/update?api_key=QFU3PR1ZPCPXAQDU&field1=3
  //String getData = "GET /update?api_key="+ API +"&"+ field +"="+String(valSensor);
  //String urlData = "GET https://api.thingspeak.com/update?api_key="+ APIKEY +"&"+ field +"="+String(valueSensor);

  urlData += field1 + "=" + String (dataLPG)+ "&" + field2 + "=" + String (dataH2)+ "&" + field3 + "=" + String (dataCH4)+ "&" + field4 + "=" + String (dataCO);
  Serial.println("UrlData: "+urlData + "\t");
  



 
  //Enable Multiple Connections
  sendCommand("AT+CIPMUX=1",countRetry,0,"OK");

  //Establish TCP Connection
  String command = "AT+CIPSTART=0,\"TCP\",\""+HOST+"\","+ PORT;
  sendCommand(command,15,0,"OK");

  if(statusResponse == true){
    //Send Data
    sendCommand("AT+CIPSEND=0," +String(urlData.length()+4),4,0,">");

    if(statusResponse == true){
      esp8266.println(urlData);
      delay(5000);
  
      //Closes the TCP/UDP/SSL Connection
      sendCommand("AT+CIPCLOSE=0",countRetry,0,"OK");
  
      }
    else{Serial.println("AT+CIPSEND Error");}

    
    }
  else{Serial.println("AT+CIPSTART Error");}
    
  countTrueCommand++;
  
}


/*************************** getDataPIN ***************************************************************************************************************
Input:   NA
Output:  dataLPG
Remarks: get LPG
********************************************************************************************************************************************************/ 
void getDataPIN(){
  
  //2. Get the PPM concentration
  //2.1. For calculating the concentration of gas in ppm take two points from the curve of particular gas from the graph provided in the MQ-2 datasheet. 
  //2.2. Then alculate a slope of that line.
  rs = mQRead(MQ_PIN);                                  
  Serial.print ("\t rs= ");
  Serial.print (rs);
  Serial.print ("\t r0= ");
  Serial.print (r0);
  Serial.print ("\t rs/r0= ");
  Serial.print (rs/r0);
  }



/*************************** getDataLPG ***************************************************************************************************************
Input:   NA
Output:  dataLPG
Remarks: get LPG
********************************************************************************************************************************************************/ 
int getDataLPG(){
  dataLPG =  MQGetGasPercentage(mQRead(MQ_PIN)/r0,GAS_LPG); 
  Serial.print("\t GAS Concentration:"); 
  Serial.print("\t LPG: ");
  Serial.print(dataLPG);
  Serial.print("ppm"); 
  return dataLPG;
  }

/*************************** getDataH2 ***************************************************************************************************************
Input:   NA
Output:  dataH2
Remarks: get H2 data
********************************************************************************************************************************************************/ 
int getDataH2(){
  dataH2 = MQGetGasPercentage(mQRead(MQ_PIN)/r0,GAS_H2);
  Serial.print("\t H2: ");
  Serial.print(dataH2);
  Serial.print("ppm");
  return dataH2; 
  }
  
/*************************** getDataCH4 ***************************************************************************************************************
Input:   NA
Output:  dataCH4
Remarks: get CH4 data
********************************************************************************************************************************************************/ 
int getDataCH4(){
  dataCH4 = MQGetGasPercentage(mQRead(MQ_PIN)/r0,GAS_CH4);
  Serial.print("\t CH4: "); 
  Serial.print(dataCH4);
  Serial.print("ppm");
  return dataCH4; 
  }

/*************************** getDataCO** ***************************************************************************************************************
Input:   NA
Output:  dataCO
Remarks: get CO data
********************************************************************************************************************************************************/ 
int getDataCO(){
  dataCO = MQGetGasPercentage(mQRead(MQ_PIN)/r0,GAS_CO);
  Serial.print("\t CO: "); 
  Serial.print(dataCO);
  Serial.print("ppm");
  return dataCO; 
  }



/*************************** Test Sensor ***************************************************************************************************************
Input:    command:     command to execute
          maxTimes:    Number of attempts to execute command
          readReplay:  Command response
Output:  Void
Remarks: AT Send command 
********************************************************************************************************************************************************/ 

void testSensor(){
  int sensorValue = analogRead(A0);
  float sensorVoltage = (sensorValue*5.0)/1024;
  Serial.print("sensorValue = ");
  Serial.println(sensorValue);
  Serial.print("sensorVoltage = ");
  Serial.print(sensorVoltage);
  Serial.println("V");
  delay(1000);
  
  }


/*************************** MQCalibration ******************************************************************************************************
Input:   mq_pin - analog channel
Output:  R0 of the sensor
Remarks: This function assumes that the sensor is in clean air. It use MQResistanceCalculation to calculates the sensor resistance in clean air 
         and then divides it with RO_CLEAN_AIR_FACTOR. RO_CLEAN_AIR_FACTOR is about 10, which differs slightly between different sensors.
************************************************************************************************************************************************/ 

float MQCalibration(int mq_pin)
{
  
  float resistance=0.0;

  for (int i=0;i<CALIBRATION_SAMPLE_TIMES;i++) {                                    //take multiple samples
    resistance += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  
  resistance = resistance/CALIBRATION_SAMPLE_TIMES;                                 //calculate the average value
  resistance = resistance/RO_CLEAN_AIR_FACTOR;                                      //divided by RO_CLEAN_AIR_FACTOR yields the Ro 
  return resistance; 
}

//**************** MQResistanceCalculation ****************************************************************************************************************************************
//Input:   raw_adc - raw value read from adc, which represents the voltage
//Output:  the calculated sensor resistance
//Remarks: The sensor and the load resistor forms a voltage divider. Given the voltage across the load resistor and its resistance, the resistance of the sensorcould be derived.
//********************************************************************************************************************************************************************************* 
float MQResistanceCalculation(int sensorValue)
{
  return ( ((float)RL_VALUE*(1023-sensorValue)/sensorValue));
}


//*************************** mQRead***********************************************************************************************************************************************
//Input:    mq_pin - analog channel
//Output:   Rs of the sensor
//Remarks:  This function use MQResistanceCalculation to caculate the sensor resistence (Rs). The Rs changes as the sensor is in the different concentration of the target gas. 
//*********************************************************************************************************************************************************************************** 

float mQRead(int mq_pin)
{
  
  float rs=0.0;
 
  for (int i=0;i<READ_SAMPLE_TIMES;i++) {                  //take multiple samples
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }
  return (rs / READ_SAMPLE_TIMES);
  
}


/***************************  MQGetGasPercentage ***************************************************************************************************************************
Input:   rs_ro_ratio - Rs divided by Ro
         gas_id      - target gas type
Output:  ppm of the target gas
Remarks: This function passes different curves to the MQGetPercentage function which calculates the ppm (parts per million) of the target gas.
****************************************************************************************************************************************************************************/ 

int MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
  if ( gas_id == GAS_LPG ) {
     return MQGetPercentage(rs_ro_ratio,lpgCurve);
  }
  else if ( gas_id == GAS_H2 ) {
     return MQGetPercentage(rs_ro_ratio,h2Curve);
  }
  else if ( gas_id == GAS_CH4 ) {
     return MQGetPercentage(rs_ro_ratio,ch4Curve);
  } 
  else if ( gas_id == GAS_CO ) {
     return MQGetPercentage(rs_ro_ratio,coCurve);
  }    
  return 0;
}


/***************************  MQGetPercentage ***************************************************************************************************************************************
Input:    rs_ro_ratio - Rs divided by Ro
          pcurve      - pointer to the curve of the target gas
Output:   ppm of the target gas
Remarks:  By using the slope and a point of the line. The x(logarithmic value of ppm) of the line could be derived if y(rs_ro_ratio) is provided. As it is a logarithmic coordinate, 
          power of 10 is used to convert the result to non-logarithmic value.
*************************************************************************************************************************************************************************************/ 

int  MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
  return (pow(10,( ((log(rs_ro_ratio)-pcurve[1])/pcurve[2]) + pcurve[0])));
}
