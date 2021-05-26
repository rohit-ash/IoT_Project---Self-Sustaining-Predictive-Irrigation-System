      
//---------DEFINITIONS--------//      
      
      // ESP8266 WIFI MODULE 
      
        #include <SoftwareSerial.h>
        #define RX 2
        #define TX 3
        String AP = "TP-Link_41EC";       // AP NAME
        String PASS = "yjvg8604$"; // AP PASSWORD
        String API = "5MANDCF04CXNGYUR";   // Write API KEY
        String HOST = "api.thingspeak.com";
        String PORT = "80";
        int countTrueCommand;
        int countTimeCommand; 
        boolean found = false; 
        int valSensor = 1;
        SoftwareSerial esp8266(RX,TX); 
  
      //TEMPERATURE AND HUMIDITY SENSOR 
       
        #include <dht.h>
        #define dht_apin A1
        dht DHT;
        int air_temp = 0;
        int air_hum = 0; 

      // SOIL MOISTURE SENSOR

        #define soil_moist A0
        const int AirValue = 620;   
        const int WaterValue = 310; //calibration units 
        int soilMoistureValue = 0;
        int soil_moisture = 0;

    // ULTRASONIC SENSOR
    
        const int EchoPin = 8;
        const int TrigPin = 9;
        int water_level = 0;
        
    // RELAY MODULE - SOLENOID VALVE
    
        const int relay = 11;
        volatile byte relayState = LOW;
        int relay_time = 0;


//---------SETUP_CODE--------//

void setup() 
{
   // SETTING UP ESP_8266 //
   
   Serial.begin(9600); // open serial port, set the baud rate to 9600 bps
   esp8266.begin(115200); // set baud rate of esp8266 to 115200
   
   sendCommand("AT",5,"OK");
   sendCommand("AT+CWMODE=1",5,"OK");
   sendCommand("AT+CWJAP=\""+ AP +"\",\""+ PASS +"\"",20,"OK");

  // ULTRASONIC SENSOR //
  
    pinMode(TrigPin, OUTPUT);
    pinMode(EchoPin, INPUT);

  // RELAY MODULE //

    pinMode(relay, OUTPUT);    
}

//---------LOOP_CODE--------//
void loop()
{
  
//-----GETTING SENSOR VALUES-----//
   
  // SOIL MOISTURE SENSOR //

    soilMoistureValue = analogRead(A0); 
    soil_moisture = Soil_mois(soilMoistureValue);

  // ULTRASONIC SENSOR //
  
    water_level = Ultrasonic_dist();

  // TEMPERATURE AND HUMIDITY //

    air_temp = temperature();
    air_hum = humidity();
    
  // RELAY MODULE //

    relay_time = Relay_state(soilMoistureValue, water_level, air_temp, air_hum); 
    if(relay_time > 0)
       {
        digitalWrite(11,HIGH);
        delay(relay_time);
        digitalWrite(11,LOW);
       }

//SENDING DATA TO THINGSPEAK SERVER
   {
     String getData = "GET /update?api_key="+ API +"&field1="+air_temp+"&field2="+air_hum+"&field3="+soil_moisture;
     sendCommand("AT+CIPMUX=1",5,"OK");
     sendCommand("AT+CIPSTART=0,\"TCP\",\""+ HOST +"\","+ PORT,15,"OK");
     sendCommand("AT+CIPSEND=0," +String(getData.length()+4),4,">");
     esp8266.println(getData);
     delay(1500);
     countTrueCommand++;
     sendCommand("AT+CIPCLOSE=0",5,"OK");
  }
}


//---------FUNCTIONS--------//

// ULTRASONIC
int Ultrasonic_dist()
{
        long distance = 0;
        long duration = 0;
        long height = 0;
        digitalWrite(TrigPin,LOW);
        delayMicroseconds(2);

        digitalWrite(TrigPin,HIGH);
        delayMicroseconds(10);
        digitalWrite(TrigPin,LOW);

        duration = pulseIn(EchoPin, HIGH);
        distance = 0.017*duration;
        Serial.print("Distance: ");
        Serial.println(distance);  

        // distance is measured from top
        // distance > 15 - LOW
        // 5 < distance < 15 - MEDIUM
        // distance < 5 - HIGH
        
return height; //RETURN DISTANCE IN CENTIMETER
}

// SOIL MOISTURE
int Soil_mois(int soilMoistureValue)
{
    int soilmoisturepercent = 0;
    
    Serial.println(soilMoistureValue); //serial monitor print
    soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
    if(soilmoisturepercent >= 100)
       {
         Serial.println("100 %");
       }
    else if(soilmoisturepercent <=0)
       {
         Serial.println("0 %");
       }
    else if(soilmoisturepercent >0 && soilmoisturepercent < 100)
       {
         Serial.print(soilmoisturepercent);
         Serial.println("%");
       }
   delay(250);

return soilmoisturepercent; //RETURN MOSITURE AS PERCENTAGE
}

// AIR TEMPERATURE
int temperature()
{
    int temp;
    DHT.read11(dht_apin);
    temp = DHT.temperature; 
    
return temp;
}

// AIR HUMIDITY
int humidity()
{
    int hum;
    DHT.read11(dht_apin);
    hum = DHT.humidity;
    
return hum;
}

// RELAY MODE - (Sample shown to present report)
int Relay_state(int soilMoistureValue,int water_level, int air_temp, int air_hum)
{
   if(soil_moisture<20 && water_level>5 && air_temp<=35 && air_hum>=50)
     { 
         relay_time = 3000; 
          //return the amount of time the valve neeeds to be turned on;
     }
   else if(20<soil_moisture<60 && water_level>10 && air_temp<=35 && air_hum>=50) 
          //additional constraints can be added
     {
         relay_time = 1000;
     }
   else if(soil_moisture>80 && water_level>=0 && air_temp<=35 && air_hum>=60)
    {
         relay_time = 0;   
    }
return relay_time;
}

//SEND COMMAND TO ESP8266
void sendCommand(String command, int maxTime, char readReplay[]) 
{
  Serial.print(countTrueCommand);
  Serial.print(". at command => ");
  Serial.print(command);
  Serial.print(" ");
  while(countTimeCommand < (maxTime*1))
  {
    esp8266.println(command);//at+cipsend
    if(esp8266.find(readReplay))//ok
    {
      found = true;
      break;
    }
  
    countTimeCommand++;
  }
  
  if(found == true)
  {
    Serial.println("OYI");
    countTrueCommand++;
    countTimeCommand = 0;
  }
  
  if(found == false)
  {
    Serial.println("Fail");
    countTrueCommand = 0;
    countTimeCommand = 0;
  }
  found = false;
 }
