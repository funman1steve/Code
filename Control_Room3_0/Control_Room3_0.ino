  #include <elapsedMillis.h>
  #include <DHT.h>
  #include <SoftwareSerial.h>
    
  #define OutsideTempPin 4 // Pins for Temp Probes
  #define ControlRoomTempPin 12 // Pins for Temp Probes
  #define AtticTempPin 7 // Pins for Temp Probes
  #define DHTTypeOutside DHT22 //Sets the Type of Probe to either DHT11 or DHT22
  #define DHTTypeControlRoom DHT11 //Sets the Type of Probe to either DHT11 or DHT22
  #define DHTTypeAttic DHT22 //Sets the Type of Probe to either DHT11 or DHT22
    DHT OutsideProbe(OutsideTempPin, DHTTypeOutside); DHT CRProbe(ControlRoomTempPin, DHTTypeControlRoom); DHT ATProbe(AtticTempPin, DHTTypeAttic); //Setup for each probe on the correct pin and probe type 
    elapsedMillis LCDTimeClock; //For Tracking LCD Screens
    elapsedMillis UpdateTimeClock; // For Tracking Time to Update
    elapsedMillis AlarmTimeClock; // For Tracking Time on Alarm beeps and lights and such
    elapsedMillis CRLightsTimeClock; // Clock for controlling dimming speed of Control Room Lights 
    elapsedMillis AlarmLEDTimeClock; // Clock for controlling blinking speed of Alarm LED
    elapsedMillis AlarmStrobeTimeClock; // Alarm RGB Strobe Clock
    
    int RedSet = 25; int GrnSet = 25; int BluSet = 25; // Vars for Color Settings
    int CRFanSpeed; byte FanPerc; byte Silence = 0; byte InAlarm = 0; int CRLights; // Vars: Fans, Silence, Alarm, & Control Room Lights
    byte CRSensorOK; byte ATSensorOK; byte OutsideSensorOK; byte FirstBoot; //Vars: Sensors OK, Firstboot count
    int OutsideTempF; int ATTempF; int CRTempF; byte OutsideHum; byte ATHum; byte CRHum; int OutsideHeatIndex; int CRHeatIndex; int ATHeatIndex;// Vars: Holding Temps and Humidity Data
    const byte CLR = 12; const byte Line2 = 148; const byte Line1 = 128; const byte Note = 220; byte DisplayLCDScreen; // Vars for Making LCD Commands Eaiser
    const byte AlarmLight = 13; const byte Red = 11; const byte Grn = 10; const byte Blu = 9; // Set Output pins for LEDs including Alarm
    const byte ControlRoomFan = 3; const int AtticFan = A1; const byte LCDPin = 2;  // Set Output pins for Fans, LCD Communication
    const byte SilenceButtonPin = 8; const byte CRLightsPin = 5; const int CRDoorSWPin = A0; const int AtticFanEnableSW = A2; // Silence Button, CR Lights, CR Door SW, Attic Fan Disable Switch
    
    const byte AtticFanOn = 105; const byte AtticFanOff = 90; // Sets On and Off Temps for Attic Fan(s)
    const byte CRTempMin = 80; const byte CRTempMax = 100; // Temperature to start the fan in the Control Room @ 25% to 100%
    const byte CRTempAlarmOn = 105; const byte CRTempAlarmReset = 90; // Set Alarm Temps Trip and Reset
    const byte CRFanSpeedMin = 85; const byte CRFanSpeedMax = 255; // Set minimum Fan Speed and Maximum Fan Speed
    const byte RGBMaxBright = 255; const byte CRMaxBright = 15; //Sets maximum brightness for the RGB LED and Control Room Lights
    const byte RedAbove = 80; const byte YellowAbove = 76; const byte GreenAbove = 63; const byte AquaAbove = 57; const byte BlueAbove = 36; // Sets Temps for What Colors Display at What Temp. Any Temp Below BlueAbove is Full RED and Full BLUE!
    const byte UpdatePFRGB = 10000; const byte CRLightUpSpeed = 100; //Update Probes, Fans, and RGB LED every x Miliseconds
    const byte ColorSpeed = 1; const int LCDScreenTime = 2000; const int AlarmBeepTime = 5000; // Update RGB Speed & Milseconds to stay on each LCD screen & Beep Alarm Every x Milseconds
    
    SoftwareSerial LCD = SoftwareSerial(255, LCDPin); // Setup for LCD Communications
     
    void setup(void) 
    {
      Serial.begin(9600); // Start Serial Output for Diagnostics
      pinMode(OutsideTempPin, INPUT_PULLUP); pinMode(ControlRoomTempPin, INPUT_PULLUP); pinMode(AtticTempPin, INPUT_PULLUP); // Sets pins for Input with Pull Up Resistor Internally
      pinMode(CRDoorSWPin,INPUT_PULLUP); pinMode(SilenceButtonPin, INPUT_PULLUP); pinMode(AtticFanEnableSW, INPUT_PULLUP);// Sets pins for Input with Pull Up Resistor Internally
      pinMode(AlarmLight, OUTPUT); pinMode(AtticFan, OUTPUT); // Sets pins for Output
      pinMode(LCDPin, OUTPUT); digitalWrite(LCDPin, HIGH); LCD.begin(9600); delay(1000); // Setup for LCD Communications
      OutsideProbe.begin(); CRProbe.begin(); ATProbe.begin(); // Start Probes for Reading Temps and Humidity      
      InAlarm = 0; 
    }
      
    void loop(void) 
    {
      if (FirstBoot == 0) {FirstBootSeq();} // If booting for the first time the run the first boot code     
      ControlRoomLights(); // Update Control Room Lights every program loop
      if (UpdateTimeClock > UpdatePFRGB) // Update Probes, Fans, and GRB LEDs every so often in the loop 
         {UpdateTimeClock = 0; ReadSensorsAndPins(); UpdateFans(); UpdateRGB();}
      UpdateAlarms(); // Update alarms every program loop
      if (LCDTimeClock > LCDScreenTime) {LCDTimeClock = 0; UpdateLCD();} // Update LCD every time elapsed time = time requested via LCDScreenTime Var
      
    }
  
   void ControlRoomLights()  
  {    
    if (CRLightsTimeClock > CRLightUpSpeed)
     {
       CRLightsTimeClock = 0;
        if (digitalRead(CRDoorSWPin)==HIGH){CRLights ++;} // If Control Room door Open; then turn up lights 
        if (digitalRead(CRDoorSWPin)==LOW){CRLights --;} // If Control Room door Closed; then turn down lights 
        if (CRLights > CRMaxBright) {CRLights = CRMaxBright;} // Ensures Var can not exceed max
        if (CRLights < 0) {CRLights = 0;} // Ensures Var can not exceed min
        analogWrite(CRLightsPin, CRLights); // Sets Control Room Lights PWM Setting 
     }
  }
    
    
   void ReadSensorsAndPins()
   { 

    float OutsideHum1 = OutsideProbe.readHumidity();  // Read humidity from Outside Probe
    float OutsideTempF1 = OutsideProbe.readTemperature(true); // Read temperature as Fahrenheit
    float OutsideHeatIndex1 = OutsideProbe.computeHeatIndex(OutsideTempF, OutsideHum);
    OutsideHum = OutsideHum1; OutsideTempF = OutsideTempF1; OutsideHeatIndex = OutsideHeatIndex1; //Change flaoting points to Vars
    if (OutsideTempF < 80){OutsideHeatIndex = OutsideTempF;} // If Temp below 80 then don't use Heat Index Calcs
    if (isnan(OutsideHum1) || isnan(OutsideTempF1)) {OutsideSensorOK = 0; OutsideTempF = -99; OutsideHum = 0; OutsideHeatIndex = 0;}// Can't read probe? Then error out
      else {OutsideSensorOK = 1;} //Read Probe Went OK
    
    
    float CRHum1 = CRProbe.readHumidity();  // Read humidity from Outside Probe
    float CRTempF1 = CRProbe.readTemperature(true); // Read temperature as Fahrenheit
    float CRHeatIndex1 = CRProbe.computeHeatIndex(CRTempF, CRHum);
    CRHum = CRHum1; CRTempF = CRTempF1; CRHeatIndex = CRHeatIndex1; //Change flaoting points to Vars
    if (CRTempF < 80){CRHeatIndex = CRTempF;} // If Temp below 80 then don't use Heat Index Calcs
    if (isnan(CRHum1) || isnan(CRTempF1)) {CRSensorOK = 0; CRTempF = -99; CRHum = 0;}// Can't read probe? Then error out
      else {CRSensorOK = 1;} //Read Probe Went OK 
   
    
    float ATHum1 = ATProbe.readHumidity();  // Read humidity from Outside Probe
    float ATTempF1 = ATProbe.readTemperature(true); // Read temperature as Fahrenheit
    float ATHeatIndex1 = ATProbe.computeHeatIndex(ATTempF, ATHum);
    ATHum = ATHum1; ATTempF = ATTempF1; ATHeatIndex = ATHeatIndex1; //Change flaoting points to Vars
    if (ATTempF < 80){ATHeatIndex = ATTempF;} // If Temp below 80 then don't use Heat Index Calcs
    if (isnan(ATHum1) || isnan(ATTempF1) || ATHum1 == 0) {ATSensorOK = 0; ATTempF = -99; ATHum = 0;}//Can't read probe? Then error out
      else {ATSensorOK = 1;} //Read Probe Went OK
   } 
   
   
  void UpdateFans()
    {
      if (CRTempF >= CRTempMax) {(analogWrite(ControlRoomFan, 255)); CRFanSpeed=255;}// If the temp is CRTempMax or above then Control Room Fans at full speed
          //else if (CRTempF > CRTempMin) {CRFanSpeed = map (CRTempF, CRTempMin, CRTempMax, CRFanSpeedMin, CRFanSpeedMax); (analogWrite(ControlRoomFan, CRFanSpeed));} // If Temp above minimum but below max then take the temp and scale it to the min max for temp and min max fan speeds
          else if (CRTempF > CRTempMin) {CRFanSpeed = CRFanSpeedMax; (analogWrite(ControlRoomFan, CRFanSpeed));}// Gave up on quiet fans for PWM so Full Fans OR Off
          else {(analogWrite(ControlRoomFan, 0)); CRFanSpeed = 0;}  // If Temp is below CRTempMin then turn Fans Off.
      
        
      if (digitalRead(AtticFanEnableSW)==LOW) {if (ATTempF >= AtticFanOn) {digitalWrite(AtticFan, HIGH);}} // Turns Attic Fan On if Switch Enabled (Closed) & AtticFanOn Temp is reached
          else {digitalWrite(AtticFan, LOW);} // Turns the Attic Fan Off (Disables Fan) if Enable Switch (Open)
      if (ATTempF <= AtticFanOff) {digitalWrite(AtticFan, LOW);} // Turns Attic Fan Off if AtticFanOff Temp is Reached & 
    }
   
   
  void UpdateRGB()
    {
    if (OutsideTempF >= RedAbove) {RedSet = RedSet + ColorSpeed; GrnSet = GrnSet - ColorSpeed; BluSet = BluSet - ColorSpeed;} // If Above RedAbove then start to change color to Red only
          else if (OutsideTempF >= YellowAbove) {RedSet = RedSet + ColorSpeed; GrnSet = GrnSet + ColorSpeed; BluSet = BluSet - ColorSpeed;} // If Above YellowAbove then Add Green and Red
          else if (OutsideTempF >= GreenAbove) {RedSet = RedSet - ColorSpeed; GrnSet = GrnSet + ColorSpeed; BluSet = BluSet - ColorSpeed;} // If Above GreenAbove then Change to only Green
          else if (OutsideTempF >= AquaAbove) {RedSet = RedSet - ColorSpeed; GrnSet = GrnSet + ColorSpeed; BluSet = BluSet + ColorSpeed;} // If Above AquaAbove then change to Aqua (Blue and Green) Only
          else if (OutsideTempF >= BlueAbove) {RedSet = RedSet - ColorSpeed; GrnSet = GrnSet - ColorSpeed; BluSet = BluSet + ColorSpeed;} // If Above BlueAbove then start to change to Blue Only
          else {RedSet = RedSet +  ColorSpeed; GrnSet = GrnSet - ColorSpeed; BluSet = BluSet + ColorSpeed;} // If Below BlueAbove then start to add full Red to full blue
      
      // If any color ends up being below 0 or above RGBMaxBright then correct it so no errors via outside allowable parameters (0-255)
      if (RedSet < 0) {RedSet = 0;} if (RedSet > RGBMaxBright) {RedSet = RGBMaxBright;} 
      if (GrnSet < 0) {GrnSet = 0;} if (GrnSet > RGBMaxBright) {GrnSet = RGBMaxBright;} 
      if (BluSet < 0) {BluSet = 0;} if (BluSet > RGBMaxBright) {BluSet = RGBMaxBright;} 
      
      // Sets the decided colors brightness in above code to the LEDS ONLY IF the system is Not In Alarm, or if the Alarm is on BUT has been Silenced.
      if (InAlarm == 1 && Silence == 0){} else {analogWrite(Red, RedSet); analogWrite(Grn, GrnSet); analogWrite(Blu, BluSet);}
    }
  
   
  void UpdateAlarms()
    {
       if (InAlarm == 1 && CRTempF <= CRTempAlarmReset) {InAlarm = 0; Silence = 0; digitalWrite(AlarmLight, LOW);} // Alarm Temp below Reset then Turn OFF Alarm, Light, & Reset Silence
       else if (InAlarm == 0 && CRTempF >= CRTempAlarmOn) {InAlarm = 1;} // If Alarm off but Alarm Temp Reached, then Turn ON Alarm and Light 
       
     if (InAlarm == 1) // If the System is in Alarm then Always blink the Alarm LED 
        { 
          if (AlarmLEDTimeClock > 200){digitalWrite (AlarmLight, LOW);} // Blink Alarm LED Off
          if (AlarmLEDTimeClock > 600){digitalWrite (AlarmLight, HIGH); AlarmLEDTimeClock = 0;} // Blink Alarm LED on
        }
     if (InAlarm == 1 && Silence == 0) // If System is in Alarm and not yet Silenced Then Strobe the RGB Vent LED
        {
        if (AlarmStrobeTimeClock > 55) {analogWrite(Red, 255); delay(40);analogWrite(Red, 0); delay(30); analogWrite(Blu, 200); delay(30);analogWrite(Blu, 0); analogWrite(Grn, 0);delay(40); AlarmStrobeTimeClock = 0;}
        if (AlarmTimeClock > AlarmBeepTime) {AlarmTimeClock = 0; LCD.write(213); LCD.write(219); LCD.write(Note);}
        }  
     if ((digitalRead(SilenceButtonPin)==LOW) && InAlarm == 1 && Silence == 0){LCD.write(Note); Silence = 1; LCD.write(17);}
    }
    
   
  void UpdateLCD() 
   { 
    DisplayLCDScreen++;
    if (DisplayLCDScreen == 5 && InAlarm == 0) {DisplayLCDScreen = 1;}
    if (DisplayLCDScreen == 5 && InAlarm == 1) {DisplayLCDScreen = 1;}

   // Controls LCD Screens
   if (DisplayLCDScreen == 1)
      {
       if (OutsideSensorOK == 1) 
         { 
          LCD.write(CLR); LCD.write(Line1);
          LCD.print("Outside @ "); LCD.print(int(OutsideTempF)); LCD.print("F ");
          LCD.write(Line2); LCD.print("Hum="); LCD.print(OutsideHum); LCD.print("%  "); LCD.print("HT="); LCD.print(OutsideHeatIndex); LCD.print("F");
         }
           else {LCD.write(CLR); LCD.write(Line1); LCD.print("  Outside Temp"); LCD.write(Line2); LCD.print(" Sensor: FAILED");}
      }
     
      
   if (DisplayLCDScreen == 2)
      {
       if (CRSensorOK == 1)
       {
        LCD.write(CLR); LCD.write(Line1);
        LCD.print("Crtl Room @ "); LCD.print(int(CRTempF)); LCD.print("F ");
        LCD.write(Line2); LCD.print("Hum="); LCD.print(CRHum); LCD.print("%  "); LCD.print("HT="); LCD.print(CRHeatIndex); LCD.print("F");
        }
          else {LCD.write(CLR); LCD.write(Line1); LCD.print(" Crtl Room Temp"); LCD.write(Line2); LCD.print(" Sensor: FAILED");}
      }
      
   if (DisplayLCDScreen == 3)
      {
       if (ATSensorOK == 1)
        {
         LCD.write(CLR); LCD.write(Line1);
         LCD.print("Attic @ "); LCD.print(int(ATTempF)); LCD.print("F");
         LCD.write(Line2); LCD.print("Hum="); LCD.print(ATHum); LCD.print("%  "); LCD.print("HT="); LCD.print(ATHeatIndex); LCD.print("F");
         }
           else {LCD.write(CLR); LCD.write(Line1); LCD.print("   Attic Temp"); LCD.write(Line2); LCD.print(" Sensor: FAILED");}    
      }

   if (DisplayLCDScreen == 4)
      {
       LCD.write(CLR); LCD.write(Line1);
       if (CRFanSpeed > CRFanSpeedMin) {FanPerc = map (CRFanSpeed, CRFanSpeedMin, 255, 0, 100); LCD.print("CRm Fans @ "); LCD.print(int(FanPerc)); LCD.print("% ");}
           else {LCD.print("CRm Fans = OFF");}
       LCD.write(Line2); 
       LCD.print("LED R");LCD.print(map (RedSet, 0, RGBMaxBright, 0, 100));  
       LCD.print(" G");LCD.print(map (GrnSet, 0, RGBMaxBright, 0, 100)); 
       LCD.print(" B");LCD.print(map (BluSet, 0, RGBMaxBright, 0, 100)); 
      }
  }

  void FirstBootSeq()
    {
      FirstBoot = 1;
      LCD.write(22); LCD.write(17); LCD.write(CLR); delay (100); // Sets system to on with no cursor, Turn backlight on, Clear 
      analogWrite(CRLightsPin, 255); // Sets Control Room Lights to Full Power for Testing      
      LCD.write(Line1); LCD.print("  Control Room  "); LCD.write(Line2); LCD.print(" Lights on Full "); delay(1000);
      LCD.write(Line1); LCD.print("    Booting:      System Check  "); delay(1500);
      LCD.write(CLR); delay (100); LCD.print("Reading Sensors"); ReadSensorsAndPins(); delay(1500);
      if (OutsideSensorOK == 0) {LCD.write(Note); LCD.write(Line2); LCD.print("Outsde Snsr Fail"); delay(2000);}
      if (CRSensorOK == 0) {LCD.write(Note); LCD.write(Line2); LCD.print("CrtlRm Snsr Fail"); delay(2000);}
      if (ATSensorOK == 0) {LCD.write(Note); LCD.write(Line2); LCD.print("Attic Snsr Fail "); delay(2000);}
      LCD.write(13); LCD.print("Testing All Fans"); delay(500); analogWrite(ControlRoomFan, 255); digitalWrite(AtticFan, HIGH); delay(500);
      analogWrite(Red, RedSet); analogWrite(Grn, GrnSet); analogWrite(Blu, BluSet); delay(5000);
      LCD.write(CLR); delay (100);
    }

