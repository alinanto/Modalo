/* Written by ALIN ANTO

POWER GRID CORPORATION OF INDA LIMITED

License:Restricted

No part/piece may be reused without explicit permission of POWERGRID */

# define MODALO_VERSION 2.0
/////// REVISION NOTES /////////
//
// V1.0 support for solis inverter
// V1.1 added editable config file for portability
//      added parsing of config file
//      added support for U16 and U32 registers.
// V1.2 added support for detailed error handling & error module
//      added support for parsing json file (beta)
//      added support for other endien (big/little) register
//      added support for reading register map from json (beta)
//      shifted core functionality of modbus to dll file for version control.
// V1.3 bug fix while reading little endien register
//      bug fix while parsing incomplete config file
//      changed time format to UTC with Z terminator
//      bug fix in file interval - Active throughout the valid minute
// V1.4 added plant code to config file.
//      added plant code to logfileName
//      cleaned and optimised token validation function for json parser
//      bug fix while printing parity to console window
//      recombiled for standard UCRT windows runtime library. removed support for windows 8 and below.

// V2.0 Added functionality for polling multiple devices
//      Added preprocessor directives for hidding nonAPI function from dll export (demotivate dll injection attack)
//      Removed asset ID from config file and use slave ID instead as device Identification.

// VX.X (pending)
// WINAPI main for hiding console window and using window UI
// preprocessor directives for supporting verbose mode
// Implement log file name as dynamically allocated and only pointer part of device config
// support for IEEE float32. 
// add float register type to map.json (U32,U16, IEEE-float32)
// UI editor for configuration files and map files
/////// REVISION NOTES /////////

// windows runtime libraries
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

// headers to my dll libraris
#include "modbus.h"
#include "modaloParse.h"
#include "modaloError.h"
// #include "cJSON.h"

// program static configuration
#define CONFIGFILE "../config/modalo.conf"
#define MODBUS_MAX_RETRY 5
#define MODBUS_RETRY_INTERVAL 1000 // in milliseconds (ms)
#define MODALO_SLEEP 900 // sleep time of loop in ms(exactly 1s might overshoot and skip a second)
#define MODALO_SLEEP_MORE 100 // sleep time in case 1s did not elapse (in ms)

// function definitions
modbus_t* initialiseModbus(CONFIG * config);
int pollNextReg(CONFIG* config, modbus_t* ctx);
void updateLogFileName(CONFIG* config,struct tm *UTCTimeS);
int readReg(modbus_t* ctx, unsigned int slaveID, REG* reg);
int writeLogFile(DEVICE* device,char* logFilePath,struct tm *UTCTimeS);
int modbusDataLog(CONFIG* config,struct tm* UTCTimeS);
uint16_t reverseBits(uint16_t num);

//main function
int main(int argc, char *argv[])
{
  // variables related to time
  time_t seconds=0;      // for seconds holding in windows time API
	struct tm *localTimeS, *UTCTimeS; // for time holding structure in windows time API

  // variables related to logfile
  int startLoggingFLag=0; // Flag for starting logging

  // variables related to configuration and mapping table and modbus
  CONFIG config; // for storing modbus configuration, logging configuration and device configuration 
  modbus_t *ctx=NULL; // modbus context pointer
  char mapFileName [MAXNAMESIZE+20] = "../config/SOLIS.json";
  
  // setbuf(stdout,NULL); // set std out to unbuffered

  // parse the configuration file
  printf("Parsing configuration File: %s\n",CONFIGFILE);
  if(!parseModaloConfigFile(&config,CONFIGFILE)) // parse failed
  {
    modaloPrintLastError();
    printf("Continuing with default configuration.\n");
  }
  printModaloConfig(config);

  // starting to read map files and save to device configuration structure
  _MODALO_forEachDevice(device,config) { // MACRO to loop over each device in config structure
    if(device->slaveID == 0) continue;   // skip over empty device declarations
    sprintf(mapFileName,"../config/%s.json",device->make);
    printf("Parsing json file for reg map: %s\n",mapFileName);
    device->map = parseModaloJSONFile(mapFileName,device->model); // get map from JSON file
    if(device->map.reg == NULL){ // handle error
      modaloPrintLastError();
      freeMAP(config);
      system("pause");
      return -1;
    }
  }

  // initialise modbus  
  printf("\nInitialsing modbus connection.\n");
  ctx=initialiseModbus(&config);
	if(ctx==NULL) //modbus failed to initialise
  {
    modaloPrintLastError();
    freeMAP(config);
    system("pause");
    return -1;
  }

  seconds=time(NULL); // update time
  localTimeS = localtime(&seconds); // update time structure with local time
  UTCTimeS = gmtime(&seconds); //update time structure with UTC Time
  updateLogFileName(&config,UTCTimeS); // initialse logFilename for all devices
  seconds = 0; // reset seconds to enter loop without delay
  do{
    while(time(NULL)==seconds)  // if time did not increment
      Sleep(MODALO_SLEEP_MORE);      // wait a bit extra until next second

    // if seconds incremented (code below acts on 1 sec timer roughly - never double counts or under counts)
    seconds=time(NULL); // update time
    localTimeS = localtime(&seconds); // update time structure with local time
    UTCTimeS = gmtime(&seconds); // update time structure with UTC time

    if(localTimeS->tm_sec == 0) { // 1 minute timer
      
      if((localTimeS->tm_min+(localTimeS->tm_hour*60)-config.startLog)%config.fileInterval == 0) { //file interval starts

        updateLogFileName(&config,UTCTimeS); // update logFilename for all devices

      } // file interval ends.
    } // 1 minute timer ends

    if(seconds%config.pollInterval == 0) { //poll interval starts => Poll next register
      if(!pollNextReg(&config,ctx)) modaloPrintLastError(); // read reg error
      else { // read success
        system("cls");  // clear screen
        printModaloMap(config); //print map
      }
    } // poll interval ends.

    if(seconds%config.sampleInterval == 0) { //sample interval starts
      if(!modbusDataLog(&config,UTCTimeS)) {
        modaloPrintLastError();
      }
    } // sample interval ends.

    Sleep(MODALO_SLEEP); // sleep for defined time
	}while(1); // 1 sec loop ends

  // lose the connection and free MAP
  modbus_close(ctx);
  modbus_free(ctx);
  freeMAP(config);
  return 0;
}

// function to initalise modbus
modbus_t* initialiseModbus(CONFIG * config)
{
  modbus_t* ctx = NULL; // modbus context pointer
  int count=0;
  char winComPort[16] = "\\\\.\\"; //buffer to hold comport in windows format
  char error[MODALO_ERROR_MAXLENGTH]=""; //buffer to hold error
  
  if (config == NULL) { //NULL config
    modaloSetLastError(EMODBUS_INIT,"Invalid / NULL Configuration.");
    return ctx;
	}

  strcat(winComPort,config->port); // converting com port to windows format
	do{
    memset(error,'\0',sizeof(char)*MODALO_ERROR_MAXLENGTH); //clear error for next try
		ctx = modbus_new_rtu(winComPort,
      config->baud,
      config->parity,
      config->dataBits,
      config->stopBits);
		if (ctx == NULL)
		{
	    modaloSetLastError(EMODBUS_INIT,"Unable to create the libmodbus context!");
		}
    else if(modbus_connect(ctx) == -1 )
		{
	    sprintf(error,"Unable to connect to %s, ERROR CODE:%s",config->port,modbus_strerror(errno));
      modaloSetLastError(EMODBUS_INIT,error);
	    modbus_free(ctx);
      ctx=NULL;
		}/* set slave removed from initialisation for using multiple devices
    else if(modbus_set_slave(ctx, config->device[0].slaveID) == -1)
    {
      sprintf(error,"Unable to select slave: %s, ERROR CODE:%s",device[0].slaveID,modbus_strerror(errno));
      modaloSetLastError(EMODBUS_INIT,error);
	    modbus_free(ctx);
      ctx=NULL;
    }*/
    count++;
		Sleep(MODBUS_RETRY_INTERVAL);
	}while(ctx == NULL && count<MODBUS_MAX_RETRY); //check for failure and max retries
  return ctx;
}

// function to read the given register and update the value in the given context
int readReg(modbus_t* ctx, unsigned int slaveID, REG* reg)
{
  char error[MODALO_ERROR_MAXLENGTH] = "";
  int mResult = 0;

  if(reg == NULL) { // invalid REG
    modaloSetLastError(EMODBUS_READ,"Invalid / NULL Register");
  	return 0;
  }

  if(ctx == NULL) { // invalid modbus context
  	sprintf(error, "Reading register %s failed with CODE: %s",reg->regName,modbus_strerror(errno));
    modaloSetLastError(EMODBUS_READ,error);
  	return 0;
  }

  // set slave as current device configuration
  if(modbus_set_slave(ctx,slaveID) == -1)   { // set slave error
    sprintf(error,"Unable to select slave: %s, ERROR CODE:%s",slaveID,modbus_strerror(errno));
    modaloSetLastError(EMODBUS_INIT,error);
    return 0;
  }

  // function code 3: Read Holding Register
  if(reg->functionCode==3) mResult = modbus_read_registers(ctx,reg->regAddress,reg->regSizeU16,reg->valueU16);

  // function code 4: Read input register
  if(reg->functionCode==4) mResult = modbus_read_input_registers(ctx,reg->regAddress,reg->regSizeU16,reg->valueU16);

  if(mResult != reg->regSizeU16)	{ // error reading register
  	sprintf(error, "SLAVE ID: %u => Reading register %s, failed with CODE: %s",slaveID,reg->regName,modbus_strerror(errno));
  	modbus_flush(ctx);
    modaloSetLastError(EMODBUS_READ,error);
  	return 0;
  }

  if(reg->byteReversed) { // reverse the bytes
    uint16_t temp;
    temp = reg->valueU16[0];
    reg->valueU16[0] = reg->valueU16[1];
    reg->valueU16[1] = temp;
  }
  if(reg->bitReversed) { // reverse the Bits
    reg->valueU16[0] = reverseBits(reg->valueU16[0]);
    reg->valueU16[1] = reverseBits(reg->valueU16[1]);
  }

  if(reg->regSizeU16 == 2) // U32
    reg->value = (((double)reg->valueU16[0]*65536 + (double)reg->valueU16[1]) * (double)reg->multiplier) / (double)reg->divisor;
  else // U16
    reg->value = ((double)reg->valueU16[0] * (double)reg->multiplier)/ (double)reg->divisor;

  return 1; // read success
}

// function to write to log file
int writeLogFile(DEVICE* device,char* logFilePath,struct tm *UTCTimeS) {
  FILE* logFileHandle=NULL;  // Handle for holding log file
  MAP map=device->map;   // map to update in file
  int count=0; // for counting the no. of registers written
  int fResult = 0; // for error checking fprintf
  char fullFileString [FILENAMESIZE+FILEPATHSIZE] = "";


  sprintf(fullFileString,"%s%s",logFilePath,device->logFileName);
  logFileHandle = fopen(fullFileString,"a"); // open log file in append mode
  if (logFileHandle == NULL) {
    modaloSetLastError(ELOG_FILE,"Unable to open log file!");
    return 0; //error opening files
  }
  // file opened successfully
  fseek(logFileHandle,0,SEEK_END); // goto end of file
  if(ftell(logFileHandle)==0) { // size of file is 0, now we can write file header
    fResult=fprintf(logFileHandle,"Asset_ID, Time_Stamp_(ISO8601)"); // Asset ID & Timestamp is always included
    if(fResult<1) { // error writing to file
      modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
      fclose(logFileHandle);
      return 0;
    }
    while(count<map.mapSize) { // loop over each register
      fResult=fprintf(logFileHandle,", %s",map.reg[count].regName); // print register Name
      if(fResult<1) { // error writing to file
        modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
        fclose(logFileHandle);
        return 0;
      }
      count++; // increment to next register
    }
    fResult=fprintf(logFileHandle,"\n"); // new line after headers
    if(fResult<1) { // error writing to file
      modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
      fclose(logFileHandle);
      return 0;
    }
  } // header completed

  // Asset ID is column 1: always included as "PPPP-SS"
  // where PPPP is plant code & SS is salve ID
  fResult=fprintf(logFileHandle,"%04d-%02d, ",
    device->plantCode,      // plant code
    device->slaveID);       // slave ID
  if(fResult<1) { // error writing to file
    modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
    fclose(logFileHandle);
    return 0;
  }

  //timestamp is column 2: always included ISO 8601 string
  fResult=fprintf(logFileHandle,"%04d-%02d-%02dT%02d:%02d:%02dz",
    UTCTimeS->tm_year+1900, // The number of years since 1900
    UTCTimeS->tm_mon+1,     // month, range 0 to 11
    UTCTimeS->tm_mday,      // day of the month, range 1 to 31
    UTCTimeS->tm_hour,      // hours, range 0 to 23
    UTCTimeS->tm_min,       // minutes, range 0 to 59
    UTCTimeS->tm_sec);      // seconds, range 0 to 59
  if(fResult<1) { // error writing to file
    modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
    fclose(logFileHandle);
    return 0;
  }
  count=0; // reset count before loop
  while(count<map.mapSize) {  // loop over each register
    fResult=fprintf(logFileHandle,", %0.2f",map.reg[count].value); // print register value
    if(fResult<1) { // error writing to file
      modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
      fclose(logFileHandle);
      return 0;
    }
    count++; // increment to next register
  }
  fResult=fprintf(logFileHandle,"\n"); // new line after headers
  if(fResult<1) { // error writing to file
    modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
    fclose(logFileHandle);
    return 0;
  }
  fclose(logFileHandle);
  return 1; // write successfull
}

// Function to reverse bits of num
uint16_t reverseBits(uint16_t num)
{
    int count = 15;
    uint16_t reverse_num = num;

    num >>= 1;
    while (num) {
        reverse_num <<= 1;
        reverse_num |= num & 1;
        num >>= 1;
        count--;
    }
    reverse_num <<= count;
    return reverse_num;
}

// Function to update the values of all logFileNames with required nomenclature
void updateLogFileName(CONFIG* config,struct tm *UTCTimeS) {
  //save index values for recovery
  unsigned int tempDevIndex = config->devIndex;
  
  // MACRO to loop over each device in config structure
  _MODALO_forEachDevice(device,*config) { 
    if(device->slaveID == 0) continue;   // skip over empty device declarations

    // update log file name for each device first use
    sprintf(device->logFileName,"modalo_%04d-%02d_%04d%02d%02dT%02d%02d%02dz.csv",
      device->plantCode,      // plant code
      device->slaveID,        // slave ID
      UTCTimeS->tm_year+1900, // The number of years since 1900
      UTCTimeS->tm_mon+1,     // month, range 0 to 11
      UTCTimeS->tm_mday,      // day of the month, range 1 to 31
      UTCTimeS->tm_hour,      // hours, range 0 to 23
      UTCTimeS->tm_min,       // minutes, range 0 to 59
      UTCTimeS->tm_sec);      //update the ISO 8601 time strings
    //printf("\n\nNew Log File: %s%s\n",config.logFilePath,device->logFileName);
  }

  //restore devIndex
  config->devIndex = tempDevIndex;
  return;
}

// function to update all log files
int modbusDataLog(CONFIG* config,struct tm* UTCTimeS) {
  //save index values for recovery
  unsigned int tempDevIndex = config->devIndex;
  int result = 1; // assume success by default

  // MACRO to loop over each device in config structure
  _MODALO_forEachDevice(device,*config) { 
    if(device->slaveID == 0) continue;   // skip over empty device declarations
    result &= writeLogFile(device,config->logFilePath,UTCTimeS); // try all valid devices
  }
  config->devIndex = tempDevIndex; // reset the devIndex
  return result; // return result
}

// Function to read from the next valid register
int pollNextReg(CONFIG* config, modbus_t* ctx) {

  MAP* pollMap = &(config->device[config->devIndex].map); // sets the current map
  char error[MODALO_ERROR_MAXLENGTH] = "";
  unsigned int slaveID=0;
  
  // checks for dev index overflow
  if(config->devIndex >= MAX_MODBUS_DEVICES) {
    config->devIndex = 0; // reset devIndex    
    pollMap = &(config->device[config->devIndex].map); // sets the current map
    pollMap->regIndex = 0; //reset the regIndex
  }

  // checks for reg index overflow
  if(pollMap->regIndex>=pollMap->mapSize) {
    pollMap->regIndex=0; // reset index (old)
    do {
      config->devIndex++; // move on to next device
      if(config->devIndex >= MAX_MODBUS_DEVICES) config->devIndex = 0; // reset device on overflow
    }while(config->device[config->devIndex].slaveID == 0);
    pollMap = &(config->device[config->devIndex].map); // sets the current map
    pollMap->regIndex = 0; //reset the regIndex (new)
  }

  

  // reads the current register and increments the regIndex
  return readReg(ctx,config->device[config->devIndex].slaveID, &(pollMap->reg[pollMap->regIndex++]));
}