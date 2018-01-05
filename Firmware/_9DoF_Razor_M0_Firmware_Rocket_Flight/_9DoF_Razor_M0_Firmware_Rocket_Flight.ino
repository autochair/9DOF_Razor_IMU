/******************************************************************************
SparkFun 9DoF Razor M0 Example Firmware
Jim Lindblom @ SparkFun Electronics
Original creation date: November 22, 2016
https://github.com/sparkfun/9DOF_Razor_IMU/Firmware

This example firmware for the SparkFun 9DoF Razor IMU M0 
demonstrates how to grab accelerometer, gyroscope, magnetometer,
and quaternion values from the MPU-9250's digital motion processor
(DMP). It prints those values to a serial port and, if a card is
present, an SD card.

Values printed can be configured using the serial port. Settings
can be modified using the included "config.h" file.

Resources:
SparkFun MPU9250-DMP Arduino Library:
  https://github.com/sparkfun/SparkFun_MPU-9250-DMP_Arduino_Library
FlashStorage Arduino Library
  https://github.com/cmaglie/FlashStorage

Development environment specifics:
  Firmware developed using Arduino IDE 1.6.12

Hardware:
  SparkFun 9DoF Razor IMU M0 (SEN-14001)
  https://www.sparkfun.com/products/14001
******************************************************************************/
                                                         // MPU-9250 Digital Motion Processing (DMP) Library
#include <SparkFunMPU9250-DMP.h>
                                                         // SD Library manages file and hardware control
#include <SD.h>
                                                         // config.h manages default logging parameters and can be used
                                                         // to adjust specific parameters of the IMU
#include "config.h"
                                                         // Flash storage (for nv storage on ATSAMD21)
#ifdef ENABLE_NVRAM_STORAGE
#include <FlashStorage.h>
#endif

MPU9250_DMP imu; // Create an instance of the MPU9250_DMP class

/////////////////////////////
// Logging Control Globals //
/////////////////////////////
// Defaults all set in config.h
bool enableSDLogging = ENABLE_SD_LOGGING;
bool enableSerialLogging = ENABLE_UART_LOGGING;
bool enableTimeLog = ENABLE_TIME_LOG;
bool enableCalculatedValues = ENABLE_CALCULATED_LOG;
bool enableAccel = ENABLE_ACCEL_LOG;
bool enableGyro = ENABLE_GYRO_LOG;
bool enableCompass = ENABLE_MAG_LOG;
bool enableQuat = ENABLE_QUAT_LOG;
bool enableEuler = ENABLE_EULER_LOG;
bool enableHeading = ENABLE_HEADING_LOG;
unsigned short accelFSR = IMU_ACCEL_FSR;
unsigned short gyroFSR = IMU_GYRO_FSR;
unsigned short fifoRate = DMP_SAMPLE_RATE;

/////////////////////
// SD Card Globals //
/////////////////////
bool sdCardPresent = false; // Keeps track of if SD card is plugged in
String logFileName; // Active logging file
String logFileBuffer; // Buffer for logged data. Max is set in config

///////////////////////
// LED Blink Control //
///////////////////////
//bool ledState = false;
uint32_t lastBlink = 0;
void blinkLED()
{
  static bool ledState = false;
  digitalWrite(HW_LED_PIN, ledState);
  ledState = !ledState;
}

#ifdef ENABLE_NVRAM_STORAGE
///////////////////////////
// Flash Storage Globals //
///////////////////////////
// Logging parameters are all stored in non-volatile memory.
// They should maintain the previously set config value.
FlashStorage(flashEnableSDLogging, bool);
FlashStorage(flashFirstRun, bool);
FlashStorage(flashEnableSD, bool);
FlashStorage(flashEnableSerialLogging, bool);
FlashStorage(flashenableTime, bool);
FlashStorage(flashEnableCalculatedValues, bool);
FlashStorage(flashEnableAccel, bool);
FlashStorage(flashEnableGyro, bool);
FlashStorage(flashEnableCompass, bool);
FlashStorage(flashEnableQuat, bool);
FlashStorage(flashEnableEuler, bool);
FlashStorage(flashEnableHeading, bool);
FlashStorage(flashAccelFSR, unsigned short);
FlashStorage(flashGyroFSR, unsigned short);
FlashStorage(flashLogRate, unsigned short);
#endif

void setup()
{
  // Initialize LED, interrupt input, and serial port.
  // LED defaults to off:
  initHardware(); 
#ifdef ENABLE_NVRAM_STORAGE
  // Load previously-set logging parameters from nvram:
  initLoggingParams();
#endif

  // Initialize the MPU-9250. Should return true on success:
  if ( !initIMU() ) 
    {
      LOG_PORT.println("Error connecting to MPU-9250");
      while (1) ; // Loop forever if we fail to connect
      // LED will remain off in this state.
    }

  // Check for the presence of an SD card, and initialize it:
  if ( initSD() )
    {
      sdCardPresent = true;
      // Get the next, available log file name
      logFileName = nextLogFile(); 
    }
}

void loop()
{
  // The loop constantly checks for new serial input:
  if ( LOG_PORT.available() )
    {
      // If new input is available on serial port
      parseSerialInput(LOG_PORT.read()); // parse it
    }

  // Then check IMU for new data, and log it
  if ( !imu.fifoAvailable() ) // If no new data is available
    return;                   // return to the top of the loop

  // Read from the digital motion processor's FIFO
  if ( imu.dmpUpdateFifo() != INV_SUCCESS )
    return; // If that fails (uh, oh), return to top

  // If enabled, read from the compass.
  if ( (enableCompass || enableHeading) && (imu.updateCompass() != INV_SUCCESS) )
    return; // If compass read fails (uh, oh) return to top

  // If logging (to either UART and SD card) is enabled
  if ( enableSerialLogging || enableSDLogging)
    logIMUDataROSMOD(); // Log new data 
}

void logIMUDataROSMOD(void)
{
  String imuLogHeader = "ROSMOD::"; // Create a fresh line to log
  String imuLogAccel = "";
  String imuLogAccelx = "";
  String imuLogAccely = "";
  String imuLogAccelz = "";
  String imuLogGyro = "";
  String imuLogGyrox = "";
  String imuLogGyroy = "";
  String imuLogGyroz = "";
  String imuLogCompass = "";
  String imuLogCompassx = "";
  String imuLogCompassy = "";
  String imuLogCompassz = "";
  String imuLogQuat = "";
  String imuLogQuatw = "";
  String imuLogQuatx = "";
  String imuLogQuaty = "";
  String imuLogQuatz = "";
  String imuLogEuler = "";
  String imuLogEulerp = "";
  String imuLogEulerr = "";
  String imuLogEulery = "";
  String imuLogHeading = "";
  String imuLog = "";
  String imuLogTime = String(imu.time) + "000000:: "; // Add time to log string
 
   
   
  if (enableAccel) // If accelerometer logging is enabled
    {
      imuLogAccelx = imuLogHeader + "Ax::" + imuLogTime;
      imuLogAccely = imuLogHeader + "Ay::" + imuLogTime;
      imuLogAccelz = imuLogHeader + "Az::" + imuLogTime;
      if ( enableCalculatedValues ) // If in calculated mode
        {
          imuLogAccelx += String(imu.calcAccel(imu.ax)) + "\r\n";
          imuLogAccely += String(imu.calcAccel(imu.ay)) + "\r\n";
          imuLogAccelz += String(imu.calcAccel(imu.az)) + "\r\n";
        }
      else
        {
          imuLogAccelx += String(imu.ax) + "\r\n";
          imuLogAccely += String(imu.ay) + "\r\n";
          imuLogAccelz += String(imu.az) + "\r\n";
        }
      imuLogAccel = imuLogAccelx + imuLogAccely + imuLogAccelz;
    }
  if (enableGyro) // If gyroscope logging is enabled
    {
      imuLogGyrox = imuLogHeader + "Gx::" + imuLogTime;
      imuLogGyroy = imuLogHeader + "Gy::" + imuLogTime;
      imuLogGyroz = imuLogHeader + "Gz::" + imuLogTime;
      if ( enableCalculatedValues ) // If in calculated mode
        {
          imuLogGyrox += String(imu.calcGyro(imu.gx)) + "\r\n";
          imuLogGyroy += String(imu.calcGyro(imu.gy)) + "\r\n";
          imuLogGyroz += String(imu.calcGyro(imu.gz)) + "\r\n";
        }
      else
        {
          imuLogGyrox += String(imu.gx) + "\r\n";
          imuLogGyroy += String(imu.gy) + "\r\n";
          imuLogGyroz += String(imu.gz) + "\r\n";
        }
      imuLogGyro = imuLogGyrox + imuLogGyroy + imuLogGyroz;
    }
  if (enableCompass) // If magnetometer logging is enabled
    {
      imuLogCompassx = imuLogHeader + "Mx::" + imuLogTime;
      imuLogCompassy = imuLogHeader + "My::" + imuLogTime;
      imuLogCompassz = imuLogHeader + "Mz::" + imuLogTime;
      if ( enableCalculatedValues ) // If in calculated mode
        {
          imuLogCompassx += String(imu.calcMag(imu.mx)) + "\r\n";
          imuLogCompassy += String(imu.calcMag(imu.my)) + "\r\n";
          imuLogCompassz += String(imu.calcMag(imu.mz)) + "\r\n";    
        }
      else
        {
          imuLogCompassx += String(imu.mx) + "\r\n";
          imuLogCompassy += String(imu.my) + "\r\n";
          imuLogCompassz += String(imu.mz) + "\r\n";
        }
      imuLogCompass = imuLogCompassx + imuLogCompassy + imuLogCompassz;
    }
  if (enableQuat) // If quaternion logging is enabled
    {
      imuLogQuatw = imuLogHeader + "Qw::" + imuLogTime;
      imuLogQuatx = imuLogHeader + "Qx::" + imuLogTime;
      imuLogQuaty = imuLogHeader + "Qy::" + imuLogTime;
      imuLogQuatz = imuLogHeader + "Qz::" + imuLogTime;
      if ( enableCalculatedValues )
        {
          imuLogQuatw += String(imu.calcQuat(imu.qw), 4) + "\r\n";
          imuLogQuatx += String(imu.calcQuat(imu.qx), 4) + "\r\n";
          imuLogQuaty += String(imu.calcQuat(imu.qy), 4) + "\r\n";
          imuLogQuatz += String(imu.calcQuat(imu.qz), 4) + "\r\n";
        }
      else
        {
          imuLogQuatw += String(imu.qw) + "\r\n";
          imuLogQuatx += String(imu.qx) + "\r\n";
          imuLogQuaty += String(imu.qy) + "\r\n";
          imuLogQuatz += String(imu.qz) + "\r\n";
        }
      imuLogQuat = imuLogQuatw + imuLogQuatx + imuLogQuaty + imuLogQuatz;
    }
  if (enableEuler) // If Euler-angle logging is enabled
    {
      imuLogEulerp = imuLogHeader + "Pitch::" + imuLogTime;
      imuLogEulerr = imuLogHeader + "Roll::" + imuLogTime;
      imuLogEulery = imuLogHeader + "Yaw::" + imuLogTime;
      imu.computeEulerAngles();
      imuLogEulerp += String(imu.pitch, 2) + "\r\n";
      imuLogEulerr += String(imu.roll, 2) + "\r\n";
      imuLogEulery += String(imu.yaw, 2) + "\r\n";
      imuLogEuler = imuLogEulerp + imuLogEulerr + imuLogEulery;
    }
  if (enableHeading) // If heading logging is enabled
    {
      imuLogHeading = imuLogHeader + "Heading::" + imuLogTime;
      imuLogHeading += String(imu.computeCompassHeading(), 2) + "\r\n";
    }
  
  imuLog = imuLogAccel + imuLogGyro + imuLogCompass + imuLogQuat + imuLogEuler + imuLogHeading;

  if (enableSerialLogging)  // If serial port logging is enabled
    LOG_PORT.print(imuLog); // Print log line to serial port

  // If SD card logging is enabled & a card is plugged in
  if ( sdCardPresent && enableSDLogging)
    {
      // If adding this log line will put us over the buffer length:
      if (imuLog.length() + logFileBuffer.length() >=
          SD_LOG_WRITE_BUFFER_SIZE)
        {
          sdLogString(logFileBuffer); // Log SD buffer
          logFileBuffer = ""; // Clear SD log buffer 
          blinkLED(); // Blink LED every time a new buffer is logged to SD
        }
      // Add new line to SD log buffer
      logFileBuffer += imuLog;
    }
  else
    {
      // Blink LED once every second (if only logging to serial port)
      if ( millis() > lastBlink + UART_BLINK_RATE )
        {
          blinkLED(); 
          lastBlink = millis();
        }
    }
}

void initHardware(void)
{
  // Set up LED pin (active-high, default to off)
  pinMode(HW_LED_PIN, OUTPUT);
  digitalWrite(HW_LED_PIN, LOW);

  // Set up MPU-9250 interrupt input (active-low)
  pinMode(MPU9250_INT_PIN, INPUT_PULLUP);

  // Set up serial log port
  LOG_PORT.begin(SERIAL_BAUD_RATE);
}

bool initIMU(void)
{
  // imu.begin() should return 0 on success. Will initialize
  // I2C bus, and reset MPU-9250 to defaults.
  if (imu.begin() != INV_SUCCESS)
    return false;

  // Set up MPU-9250 interrupt:
  imu.enableInterrupt(); // Enable interrupt output
  imu.setIntLevel(1);    // Set interrupt to active-low
  imu.setIntLatched(1);  // Latch interrupt output

  // Configure sensors:
  // Set gyro full-scale range: options are 250, 500, 1000, or 2000:
  imu.setGyroFSR(gyroFSR);
  // Set accel full-scale range: options are 2, 4, 8, or 16 g 
  imu.setAccelFSR(accelFSR);
  // Set gyro/accel LPF: options are5, 10, 20, 42, 98, 188 Hz
  imu.setLPF(IMU_AG_LPF); 
  // Set gyro/accel sample rate: must be between 4-1000Hz
  // (note: this value will be overridden by the DMP sample rate)
  imu.setSampleRate(IMU_AG_SAMPLE_RATE); 
  // Set compass sample rate: between 4-100Hz
  imu.setCompassSampleRate(IMU_COMPASS_SAMPLE_RATE); 

  // Configure digital motion processor. Use the FIFO to get
  // data from the DMP.
  unsigned short dmpFeatureMask = 0;
  if (ENABLE_GYRO_CALIBRATION)
    {
      // Gyro calibration re-calibrates the gyro after a set amount
      // of no motion detected
      dmpFeatureMask |= DMP_FEATURE_GYRO_CAL;
      dmpFeatureMask |= DMP_FEATURE_SEND_CAL_GYRO;
    }
  else
    {
      // Otherwise add raw gyro readings to the DMP
      dmpFeatureMask |= DMP_FEATURE_SEND_RAW_GYRO;
    }
  // Add accel and quaternion's to the DMP
  dmpFeatureMask |= DMP_FEATURE_SEND_RAW_ACCEL;
  dmpFeatureMask |= DMP_FEATURE_6X_LP_QUAT;

  // Initialize the DMP, and set the FIFO's update rate:
  imu.dmpBegin(dmpFeatureMask, fifoRate);

  return true; // Return success
}

bool initSD(void)
{
  // SD.begin should return true if a valid SD card is present
  if ( !SD.begin(SD_CHIP_SELECT_PIN) )
    {
      return false;
    }

  return true;
}

// Log a string to the SD card
bool sdLogString(String toLog)
{
  // Open the current file name:
  File logFile = SD.open(logFileName, FILE_WRITE);
  
  // If the file will get too big with this new string, create
  // a new one, and open it.
  if (logFile.size() > (SD_MAX_FILE_SIZE - toLog.length()))
    {
      logFileName = nextLogFile();
      logFile = SD.open(logFileName, FILE_WRITE);
    }

  // If the log file opened properly, add the string to it.
  if (logFile)
    {
      logFile.print(toLog);
      logFile.close();

      return true; // Return success
    }

  return false; // Return fail
}

// Find the next available log file. Or return a null string
// if we've reached the maximum file limit.
String nextLogFile(void)
{
  String filename;
  int logIndex = 0;

  for (int i = 0; i < LOG_FILE_INDEX_MAX; i++)
    {
      // Construct a file with PREFIX[Index].SUFFIX
      filename = String(LOG_FILE_PREFIX);
      filename += String(logIndex);
      filename += ".";
      filename += String(LOG_FILE_SUFFIX);
      // If the file name doesn't exist, return it
      if (!SD.exists(filename))
        {
          return filename;
        }
      // Otherwise increment the index, and try again
      logIndex++;
    }

  return "";
}

// Parse serial input, take action if it's a valid character
void parseSerialInput(char c)
{
  unsigned short temp;
  switch (c)
    {
    case PAUSE_LOGGING: // Pause logging on SPACE
      enableSerialLogging = !enableSerialLogging;
#ifdef ENABLE_NVRAM_STORAGE
      flashEnableSerialLogging.write(enableSerialLogging);
#endif
      break;
    case START_LOGGING: // Pause logging on SPACE
      enableSerialLogging = true;
#ifdef ENABLE_NVRAM_STORAGE
      flashEnableSerialLogging.write(enableSerialLogging);
#endif
      break;
    case STOP_LOGGING: // Pause logging on SPACE
      enableSerialLogging = false;
#ifdef ENABLE_NVRAM_STORAGE
      flashEnableSerialLogging.write(enableSerialLogging);
#endif
      break;
    case ENABLE_TIME: // Enable time (milliseconds) logging
      enableTimeLog = !enableTimeLog;
#ifdef ENABLE_NVRAM_STORAGE
      flashenableTime.write(enableTimeLog);
#endif
      break;
    case ENABLE_ACCEL: // Enable/disable accelerometer logging
      enableAccel = !enableAccel;
#ifdef ENABLE_NVRAM_STORAGE
      flashEnableAccel.write(enableAccel);
#endif
      break;
    case ENABLE_GYRO: // Enable/disable gyroscope logging
      enableGyro = !enableGyro;
#ifdef ENABLE_NVRAM_STORAGE
      flashEnableGyro.write(enableGyro);
#endif
      break;
    case ENABLE_COMPASS: // Enable/disable magnetometer logging
      enableCompass = !enableCompass;
#ifdef ENABLE_NVRAM_STORAGE
      flashEnableCompass.write(enableCompass);
#endif
      break;
    case ENABLE_CALC: // Enable/disable calculated value logging
      enableCalculatedValues = !enableCalculatedValues;
#ifdef ENABLE_NVRAM_STORAGE
      flashEnableCalculatedValues.write(enableCalculatedValues);
#endif
      break;
    case ENABLE_QUAT: // Enable/disable quaternion logging
      enableQuat = !enableQuat;
#ifdef ENABLE_NVRAM_STORAGE
      flashEnableQuat.write(enableQuat);
#endif
      break;
    case ENABLE_EULER: // Enable/disable Euler angle (roll, pitch, yaw)
      enableEuler = !enableEuler;
#ifdef ENABLE_NVRAM_STORAGE
      flashEnableEuler.write(enableEuler);
#endif
      break;
    case ENABLE_HEADING: // Enable/disable heading output
      enableHeading = !enableHeading;
#ifdef ENABLE_NVRAM_STORAGE
      flashEnableHeading.write(enableHeading);
#endif
      break;
    case SET_LOG_RATE: // Increment the log rate from 1-100Hz (10Hz increments)
      temp = imu.dmpGetFifoRate(); // Get current FIFO rate
      if (temp == 1) // If it's 1Hz, set it to 10Hz
        temp = 10;
      else
        temp += 10; // Otherwise increment by 10
      if (temp > 100)  // If it's greater than 100Hz, reset to 1
        temp = 1;
      imu.dmpSetFifoRate(temp); // Send the new rate
      temp = imu.dmpGetFifoRate(); // Read the updated rate
#ifdef ENABLE_NVRAM_STORAGE
      flashLogRate.write(temp); // Store it in NVM and print new rate
#endif
      LOG_PORT.println("IMU rate set to " + String(temp) + " Hz");
      break;
    case SET_ACCEL_FSR: // Increment accelerometer full-scale range
      temp = imu.getAccelFSR();      // Get current FSR
      if (temp == 2) temp = 4;       // If it's 2, go to 4
      else if (temp == 4) temp = 8;  // If it's 4, go to 8
      else if (temp == 8) temp = 16; // If it's 8, go to 16
      else temp = 2;                 // Otherwise, default to 2
      imu.setAccelFSR(temp); // Set the new FSR
      temp = imu.getAccelFSR(); // Read it to make sure
#ifdef ENABLE_NVRAM_STORAGE
      flashAccelFSR.write(temp); // Update the NVM value, and print
#endif
      LOG_PORT.println("Accel FSR set to +/-" + String(temp) + " g");
      break;
    case SET_GYRO_FSR:// Increment gyroscope full-scale range
      temp = imu.getGyroFSR();           // Get the current FSR
      if (temp == 250) temp = 500;       // If it's 250, set to 500
      else if (temp == 500) temp = 1000; // If it's 500, set to 1000
      else if (temp == 1000) temp = 2000;// If it's 1000, set to 2000
      else temp = 250;                   // Otherwise, default to 250
      imu.setGyroFSR(temp); // Set the new FSR
      temp = imu.getGyroFSR(); // Read it to make sure
#ifdef ENABLE_NVRAM_STORAGE
      flashGyroFSR.write(temp); // Update the NVM value, and print
#endif
      LOG_PORT.println("Gyro FSR set to +/-" + String(temp) + " dps");
      break;
    case ENABLE_SD_LOGGING: // Enable/disable SD card logging
      enableSDLogging = !enableSDLogging;
#ifdef ENABLE_NVRAM_STORAGE
      flashEnableSDLogging.write(enableSDLogging);
#endif
      break;
    default: // If an invalid character, do nothing
      break;
    }
}

#ifdef ENABLE_NVRAM_STORAGE
// Read from non-volatile memory to get logging parameters
void initLoggingParams(void)
{
  // Read from firstRun mem location, should default to 0 on program
  if (flashFirstRun.read() == 0) 
    {
      // If we've got a freshly programmed board, program all of the
      // nvm locations:
      flashEnableSDLogging.write(enableSDLogging);
      flashEnableSerialLogging.write(enableSerialLogging);
      flashenableTime.write(enableTimeLog);
      flashEnableCalculatedValues.write(enableCalculatedValues);
      flashEnableAccel.write(enableAccel);
      flashEnableGyro.write(enableGyro);
      flashEnableCompass.write(enableCompass);
      flashEnableQuat.write(enableQuat);
      flashEnableEuler.write(enableEuler);
      flashEnableHeading.write(enableHeading);
      flashAccelFSR.write(accelFSR);
      flashGyroFSR.write(gyroFSR);
      flashLogRate.write(fifoRate);
      
      flashFirstRun.write(1); // Set the first-run boolean
    }
  else // If values have been previously set:
    {
      // Read from NVM and set the logging parameters:
      enableSDLogging = flashEnableSDLogging.read();
      enableSerialLogging = flashEnableSerialLogging.read();
      enableTimeLog = flashenableTime.read();
      enableCalculatedValues = flashEnableCalculatedValues.read();
      enableAccel = flashEnableAccel.read();
      enableGyro = flashEnableGyro.read();
      enableCompass = flashEnableCompass.read();
      enableQuat = flashEnableQuat.read();
      enableEuler = flashEnableEuler.read();
      enableHeading = flashEnableHeading.read();
      accelFSR = flashAccelFSR.read();
      gyroFSR = flashGyroFSR.read();
      fifoRate = flashLogRate.read();
    }
}
#endif
  

