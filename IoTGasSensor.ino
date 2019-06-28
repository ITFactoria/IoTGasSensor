
const int     MQ_PIN = A0;                                        //Analog input channel to use
const int     RL_VALUE= 5;                                        //define the load resistance on the board, in kilo ohms
const float   RO_CLEAN_AIR_FACTOR = 9.83;                         //RO_CLEAR_AIR_FACTOR=(Sensor resistance in clean air)/RO,which is derived from the chart in datasheet
const double  MQ_DELAY = 500;                                     //define which analog input channel you are going to use

const int     CALIBRATION_SAMPLE_TIMES = 50;                      //How many samples you are going to take in the calibration phase
const int     CALIBRATION_SAMPLE_INTERVAL = 500;                  //Time interval(in milisecond) between each samples in the cablibration phase

const int     READ_SAMPLE_TIMES = 5;                              //define how many samples you are going to take in normal operation
const int     READ_SAMPLE_INTERVAL = 50;                          //define the time interal(in milisecond) between each samples in normal operation


float           r0 = 0.0;                                            //Sensor resistance. See sensor specifications 
float           R1 = 0.0;                                            //Sensor resistance. See sensor specifications 
float           rs = 0.0;                                            //Sensor resistance. 


/*** Define gas identity **********************************************************************************************************************************/

int GAS_LPG = 0;
int GAS_H2  = 1;
int GAS_CH4 = 2;
int GAS_CO  = 3;

/**********************************************************************************************************************************************************/

/*** LPG Curve Points Datasheet ***************************************************************************************************************************/

//Get two points of curve based on the chart provided in the MQ-2 datasheet
const float lx0 = 200;
const float ly0 = 1.7;
const float lx1 = 10000;
const float ly1 = 0.28;
 
//Take log of each point {x, y}
const float lp0[] = { log10(lx0), log10(ly0) };
const float lp1[] = { log10(lx1), log10(ly1) };

//Find slope and abcise coordenate
//slope = (y1-y0)/(x1-x0)

const float lslope = (lp1[1] - lp0[1]) / (lp1[0] - lp0[0]);
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

//Find slope and abcise coordenate
//slope = (y1-y0)/(x1-x0)

const float hslope = (hp1[1] - hp0[1]) / (hp1[0] - hp0[0]);
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

//Find slope and abcise coordenate
//slope = (y1-y0)/(x1-x0)

const float sslope = (sp1[1] - sp0[1]) / (sp1[0] - sp0[0]);
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

//Find slope and abcise coordenate
//slope = (y1-y0)/(x1-x0)

const float cslope = (cp1[1] - cp0[1]) / (cp1[0] - cp0[0]);
const float ccoord = cp0[1] - cp0[0] * cslope;

// Define CO Curve
float coCurve[3] = {cp0[0], cp0[1], cslope};

/**********************************************************************************************************************************************************/





void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);                                             //UART setup, baudrate = 9600bps
  
  //1. Sensor calibrating
  //Resistance value of MQ-2 is difference to various kinds and various concentration gases. So,When using this components, sensitivity adjustment is very necessary.
  //The sensor resistor RS and load resistor RL form a voltage divider. Based on the chart provided in the MQ-2 datasheet , RS in clean air under given temperature and 
  //humidity is a constant. The ratio of RS/R0 in clean air is 9.8 as described in datasheet.   //We will first calibrate the sensor. Place sensor in clean air. 
  //We will get R0 value by dividing it by RS/R0 value in clean air.
  
  Serial.print("Start sensor calibrating...\n");
  r0 = MQCalibration(MQ_PIN);                                     //Calibrating the sensor. Please make sure the sensor is in clean air 
  Serial.print("Calibration is done...\n"); 
  
}

void loop() {

  //testSensor();

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
  Serial.print("\t GAS Concentration:"); 
  
  
  Serial.print("\t LPG: "); 
  Serial.print(MQGetGasPercentage(mQRead(MQ_PIN)/r0,GAS_LPG) );
  Serial.print("ppm"); 
  Serial.print("\t H2: "); 
  Serial.print(MQGetGasPercentage(mQRead(MQ_PIN)/r0,GAS_H2) );
  Serial.print("ppm"); 
  Serial.print("\t CH4: "); 
  Serial.print(MQGetGasPercentage(mQRead(MQ_PIN)/r0,GAS_CH4) );
  Serial.print("ppm"); 
  Serial.print("\t CO: "); 
  Serial.print(MQGetGasPercentage(mQRead(MQ_PIN)/r0,GAS_CO) );
  Serial.print("ppm"); 
  Serial.print("\n"); 
 
  
  
}


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
