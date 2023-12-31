#include <stdio.h> // file operations, printf
#include <stdlib.h> //calloc, EOF, free
#include <string.h> //memset defined
#include <stdint.h> //size_t , uint_16 defined

#include "modaloError.h"
#include "cJSON.h"
#include "modaloParse.h"

// To clear any half parsed device information
void cleanModaloConfigStruct(CONFIG *config) {
  for(int i=0;i<MAX_MODBUS_DEVICES;i++)   {
    if(config->device[i].assetID[0] == '\0')  config->device[i].slaveID = 0;
    if(config->device[i].model[0] == '\0')  config->device[i].slaveID = 0;
    if(config->device[i].make[0] == '\0')  config->device[i].slaveID = 0;
    if(config->device[i].capacity == 0)  config->device[i].slaveID = 0;
    if(config->device[i].plantCode == 0)  config->device[i].slaveID = 0;
    // if any value is not initialized we are clearing the slaveID to indicate empty
  }
  return;
}

// validates device token
int validateModaloDeviceToken(CONFIG* config, char * indexParameter, char * childParameter, char * value) {
  char error[MODALO_ERROR_MAXLENGTH] = "";
  int index = 0;

  // index validation
  if(!validateModaloIntegerString(indexParameter,1,MAX_MODBUS_DEVICES)) { // out of range
    sprintf(error,"Invalid index for device_: %s",indexParameter);
    modaloSetLastError(EPARSE_CONFIG_STRING,error);
    return 0;
  }
  index = atoi(indexParameter)-1; // index validated

  // MAKE value validation
  if(!strcmp(childParameter,"MAKE") && strlen(value)<MAKE_MODEL_NAMESIZE) { 
    strcpy(config->device[index].make,value);
    return 1;
  }

  // MODEL value validation
  if(!strcmp(childParameter,"MODEL") && strlen(value)<MAKE_MODEL_NAMESIZE) { 
    strcpy(config->device[index].model,value);
    return 1;
  }

  // ASSETID value validation
  if(!strcmp(childParameter,"ASSETID") && strlen(value)<MAKE_MODEL_NAMESIZE) { 
    strcpy(config->device[index].assetID,value);
    return 1;
  }

  // CAPACITY value validation
  if(!strcmp(childParameter,"CAPACITY") && validateModaloIntegerString(value,1000,1000000)) { 
    config->device[index].capacity=atoi(value);
    return 1;
  }

  // plantcode value validation
  if(!strcmp(childParameter,"PLANTCODE") && validateModaloIntegerString(value,1,9999)) { 
    config->device[index].plantCode=atoi(value);
    return 1;
  }

  // slave ID value validation
  if(!strcmp(childParameter,"SLAVEID") && validateModaloIntegerString(value,1,255)) { 
    config->device[index].slaveID=atoi(value);
    return 1;
  }

  // did not return => no valid value
  sprintf(error,"Invalid child parameter or value for device_%s:%s = %s",
    indexParameter,childParameter,value);
  modaloSetLastError(EPARSE_CONFIG_STRING,error);
  return 0;
}

// validate COMPORT String
int validateModaloCOMPORTString(char* value) {
  if(strlen(value)<6 && // maximum length
  value[0] =='C' &&  
  value[1] =='O' &&
  value[2] == 'M' && // check for "COM"
  '1'<=value[3] && value[3]<='9' && // digit 1-9
  (( '0'<=value[4] && value[4]<='9') || value[4] == '\0' )) // digit 1-9 or null character
  return 1; // valid
  return 0; // invalid
}

// validate HHMM String
int validateModaloHHMMString(char* value) {
  if(strlen(value)==4 && 
  '0'<=value[1] && value[1]<='9' && // 2nd digit 0-9
  '0'<=value[2] && value[2]<='5' && // 3rd digit 0-5
  '0'<=value[3] && value[3]<='9' && // 4th digit 0-9
  ( value[0]!='2' || value[1]<='3'))// check for hour !> 23
  return 1; // valid
  return 0; // invalid
}

// validates integer string
int validateModaloIntegerString(char * valString, int lLimit, int uLimit) { 
  int num = 0;
  int i=0;

  while(valString[i]!='\0') { // loop over each characters
    if(valString[i] < '0' || valString[i] > '9') return 0; // not a digit      
    ++i;
  }
  // all digits only
  num = atoi(valString);
  if(num>uLimit) return 0; // exceeds upper limit
  if(num<lLimit) return 0; // exceeds lower limit
  return 1;                // valid integer string
}

// validate filePath string
int validateModaloFilePathString(char* value) {
  int len = 0;

  len = strlen(value);
  if(len<FILEPATHSIZE && // less than buffer size
  value[len-1] == '/') // last character is '/' for signifying folder location
  return 1; // valid
  return 0; // invalid
}

// validates token string
int validateModaloToken(CONFIG* config, char *parameter, char *value) // validates and saves parameter and value
{
  const char delimiter[4]=PARAMETER_SEPARATOR; //separator as delimiter array
  char error[MODALO_ERROR_MAXLENGTH] = "";
  char *parentParameter = NULL;
  char *childParameter = NULL;
  char *indexParameter = NULL;

  // validation for COM_PORT
  if(!strcmp(parameter,"COM_PORT") && validateModaloCOMPORTString(value)) {
    strcpy(config->port,value); // valid port
    return 1;
  }

  // validation for BAUD_RATE
  if(!strcmp(parameter,"BAUD_RATE") && validateModaloIntegerString(value,1000,MAX_BAUD_RATE)) {
    config->baud = atoi(value);
    return 1;
  }
  
  // validation for DATA_BITS
  if(!strcmp(parameter,"DATA_BITS") && validateModaloIntegerString(value,MIN_DATA_BITS,MAX_DATA_BITS)) {
    config->dataBits = atoi(value);
    return 1;
  }
  
  // validation for STOP_BITS
  if(!strcmp(parameter,"STOP_BITS") && validateModaloIntegerString(value,0,2)) {
    config->stopBits = atoi(value);
    return 1;
  }
  
  // validation for PARITY
  if(!strcmp(parameter,"PARITY"))
  {
    if(!strcmp(value,"ODD")) { config->parity=PARITY_ODD; return 1; }
    if(!strcmp(value,"EVEN")) { config->parity=PARITY_EVEN; return 1; }
    if(!strcmp(value,"NONE")) { config->parity=PARITY_NONE; return 1; }
  }
  
  // validation for device
  if(!strncmp(parameter,"DEVICE_",7))
  {
    parentParameter=strtok(parameter,delimiter); // get device
    if(parentParameter==NULL) // check for token error
    {
      sprintf(error,"Invalid parameter: %s",parameter);
      modaloSetLastError(EPARSE_CONFIG_STRING,error);
      return 0;
    }

    indexParameter=strtok(NULL,delimiter); // get index value
    if(indexParameter==NULL) // check for token error
    {
      sprintf(error,"Invalid parameter index: %s",parameter);
      modaloSetLastError(EPARSE_CONFIG_STRING,error);
      return 0;
    }

    childParameter=strtok(NULL,delimiter); // get child parameter
    if(childParameter==NULL) // check for token error
    {
      sprintf(error,"Invalid child parameter for parent: %s_%s",parentParameter,indexParameter);
      modaloSetLastError(EPARSE_CONFIG_STRING,error);
      return 0;
    }
    return validateModaloDeviceToken(config,indexParameter,childParameter,value);
  }

  // validation for SAMPLE_INTERVAL
  if(!strcmp(parameter,"SAMPLE_INTERVAL") && validateModaloIntegerString(value,1,MAX_SAMPLE_INTERVAL)) {
    config->sampleInterval = atoi(value);
    return 1;
  }

  else if(!strcmp(parameter,"FILE_INTERVAL") && validateModaloIntegerString(value,1,MAX_FILE_INTERVAL)) {
    config->fileInterval = atoi(value);
    return 1;
  }

  // validation for POLL_INTERVAL
  if(!strcmp(parameter,"POLL_INTERVAL") && validateModaloIntegerString(value,1,MAX_SAMPLE_INTERVAL)) {
    config->pollInterval = atoi(value);
    return 1;
  }

  // validation for START_LOG_HHMM
  if(!strcmp(parameter,"START_LOG_HHMM") && validateModaloHHMMString(value)) {
    config->startLog = ((int)atoi(value)/100)*60+atoi(value+2); // valid startLog format
    return 1;
  }

  // validation for STOP_LOG_HHMM
  if(!strcmp(parameter,"STOP_LOG_HHMM") && validateModaloHHMMString(value)) {
    config->stopLog = ((int)atoi(value)/100)*60+atoi(value+2); // valid startLog format
    return 1;
  }

  // validation for LOG_FILEPATH
  if(!strcmp(parameter,"LOG_FILEPATH") && validateModaloFilePathString(value)) { 
    strcpy(config->logFilePath,value);
    return 1;
  }

  // no validation returned => validation failed
  sprintf(error,"Invalid parameter or value for %s:%s",parameter,value);
  modaloSetLastError(EPARSE_CONFIG_STRING,error);
  return 0;
}

// parse the config parameter string and passed for further validation
int parseModaloConfigString(CONFIG* config,char * line)
{
  const char delimiter[4]=VALUE_SEPARATOR; //separator as delimiter array
  char* parameter=NULL; // token holder for parameter
  char* value=NULL; // token holder for value
  char error[MODALO_ERROR_MAXLENGTH] = ""; // buffer to hold error

  if(line[MAXLINELENGTH-2]!='\0'
  && line[MAXLINELENGTH-2]!='\n'
  && line[MAXLINELENGTH-2]!=EOF) // no new line or null character or EOF in buffer
  {
    sprintf(error,"Configuration statement exceeded maximum character limit of %d",MAXLINELENGTH);
    modaloSetLastError(EPARSE_CONFIG_STRING,error);
    return 0;
  }
  //parameter statement less than MAXLINELENGTH
  parameter = strtok(line,delimiter);
  if(parameter==NULL)
  {
    modaloSetLastError(EPARSE_CONFIG_STRING,"Unknown Parameter!");
    return 0;
  }
  //parameter retrieved
  value = strtok(NULL,delimiter);
  if(value==NULL)
  {
    sprintf(error,"Invalid value for parameter: %s",parameter);
    modaloSetLastError(EPARSE_CONFIG_STRING,error);
    return 0;
  }

  //value retrieved
  return validateModaloToken(config,parameter,value);
}

// API function to parse configuration file -> Psuedo Entry point to DLL
MODALO_API int MODALO_CALL parseModaloConfigFile(CONFIG* config, char * FileName)
{
  FILE *file = NULL; //initialise file handler
  char *lineBuffer = NULL; //initialse lineBuffer pointer
  char error[MODALO_ERROR_MAXLENGTH] = "";

  // clear configuration static variable
  memset(config,'\0',sizeof(CONFIG));

  // default modbus configurations
  strcpy(config->port,"COM3");
  config->baud=9600;
  config->dataBits=8;
  config->stopBits=1;
  config->parity='N';

  // dafult sampling configurations
  config->sampleInterval=60;
  config->fileInterval=900;
  config->pollInterval=5;
  config->startLog=330;
  config->stopLog=1200;
  strcpy(config->logFilePath,"../log/");

  // default device configurations
  config->device[0].capacity = 17500;
  config->device[0].slaveID = 1;
  config->device[0].plantCode = 1211;
  strcpy(config->device[0].make,"SOLIS");
  strcpy(config->device[0].model,"SOLIS-15K");
  strcpy(config->device[0].assetID,"123");

  file = fopen(FileName,"r"); //open for read
  if(file==NULL) //file open failed
  {
    sprintf(error,"Unable to open configuration file: %s",FileName);
    modaloSetLastError(EVALIDATE_PARAMETER,error);
    return 0;
  }

  //File opened successfully
  lineBuffer = (char *)calloc(MAXLINELENGTH,sizeof(char)); //buffer for line with all zeros
  if(lineBuffer == NULL) {
        modaloSetLastError(EPARSE_CONFIG_FILE,"Error allocating memory for line buffer.");
        fclose(file);
        return 0;
    }

  //Buffer allocated successfully
  while(fgets(lineBuffer,MAXLINELENGTH,file)!=NULL)
  {
    //LINE READ
    switch(lineBuffer[0])
    {
      case '\n': //empty line
      case ' ': //space lines
      case COMMENT_CHAR:
      {
        while(lineBuffer[MAXLINELENGTH-2]!='\0'
        && lineBuffer[MAXLINELENGTH-2]!='\n'
        && lineBuffer[MAXLINELENGTH-2]!=EOF) // no new line or null character or EOF in buffer
        {
          //comment or space line longer than buffer
          memset(lineBuffer,'\0',MAXLINELENGTH*sizeof(char)); // clear buffer
          if(fgets(lineBuffer,MAXLINELENGTH,file)==NULL) break;  // read again
        }
        break;
      }
      default :
      {
        if(!parseModaloConfigString(config,lineBuffer))
        {
          free(lineBuffer);
          fclose(file);
          cleanModaloConfigStruct(config); // to clear half parsed configurations and restore default
          return 0;
        }
      }
    }
    memset(lineBuffer,'\0',MAXLINELENGTH*sizeof(char)); // clear buffer
  }
  if(!feof(file))
  {
    sprintf(error,"Read Error from config file: %s",FileName); //fgets failed
    modaloSetLastError(EPARSE_CONFIG_FILE,error);
    free(lineBuffer);
    fclose(file);
    cleanModaloConfigStruct(config); // to clear half parsed configurations and restore default
    return 0;
  }
  cleanModaloConfigStruct(config); // to clear half parsed configurations and restore default
  fclose(file);
  free(lineBuffer);
  return 1; // parsed successfully
}

// to print settings of modalo config struct
MODALO_API void MODALO_CALL printModaloConfig(CONFIG config) {
  // modbus parameters
  printf("\nPort: %s\n",config.port);
  printf("Baud: %d\n",config.baud);
  printf("Parity: %d\n",config.parity);
  printf("Stop Bits: %d\n",config.stopBits);

  // sampling and logging parameters
  printf("Sample interval: %d\n",config.sampleInterval);
  printf("File interval: %d\n",config.fileInterval);
  printf("Poll interval: %d\n",config.pollInterval);
  printf("Start Log min: %d\n",config.startLog);
  printf("Stop log min: %d\n",config.stopLog);
  printf("Log file path: %s\n",config.logFilePath);

  // device parameters
  for(int i=0;i<MAX_MODBUS_DEVICES;i++)   {
    if(config.device[i].slaveID == 0) continue; // empty device configuration
    printf("Device %d - Make: %s\n",i+1,config.device[i].make);
    printf("Device %d - Model: %s\n",i+1,config.device[i].model);
    printf("Device %d - Asset ID: %s\n",i+1,config.device[i].assetID);
    printf("Device %d - Slave ID: %d\n",i+1,config.device[i].slaveID);
    printf("Device %d - Plant Code: %d\n",i+1,config.device[i].plantCode);
    printf("Device %d - Capacity: %d\n",i+1,config.device[i].capacity);
  }
  printf("\n");
}

// to print mapping of modalo map structure
MODALO_API void MODALO_CALL printModaloMap(MAP map) {
  int i=0;
  for(i=0;i<map.mapSize;i++) {
    printf("\nReg Name: %s\n",map.reg[i].regName);
    printf("Reg Add: %d\n",map.reg[i].regAddress);
    printf("Reg SizeU16: %d\n",map.reg[i].regSizeU16);
    printf("Byte Reverse: %d\n",map.reg[i].byteReversed);
    printf("Bit Reverse: %d\n",map.reg[i].bitReversed);
    printf("Function Code: %d\n",map.reg[i].functionCode);
    printf("Multiplier:  %d\n",map.reg[i].multiplier);
    printf("Divisor: %d\n",map.reg[i].divisor);
    printf("Moving Avg Filter: %d\n",map.reg[i].movingAvgFilter);
  }
}

// parses and entire file to buffer and then returns pointer : Memory to be made free after use. 
char* readFileToBuffer(char * fileName)
{
  cJSON* root=NULL; // hold root object
  FILE* fp=NULL; //hold our files
  char* buffer=NULL; // pointer to file buffer
  char* temp=NULL; // temp to store realloc result
  char error[MODALO_ERROR_MAXLENGTH] = ""; // to store error
  size_t readBytes =0; // no of bytes read
  size_t bufferSize = 0; // to store total size of buffer
  size_t bufferUsed = 0; // to store the used size of buffer


  fp = fopen(fileName, "r");
  if (fp == NULL) { //handle error
    sprintf(error,"Unable to open JSON file: %s",fileName);
    modaloSetLastError(EPARSE_CJSON_FILE,error);
    return NULL;
  }

  //file opened
  bufferSize = BUFFER_SIZE*sizeof(char)+1; // extra 1 byte for '\0'
  buffer = (char *)malloc(bufferSize); //allocate buffer
  if(buffer == NULL) { // handle error
      modaloSetLastError(EPARSE_CJSON_FILE,"Unable to allocate buffer.");
      fclose(fp);
      return NULL;
  }

  //buffer allocated
  while(1) {
    readBytes = fread(buffer+bufferUsed,sizeof(char),BUFFER_SIZE,fp); // read to buffer
    if(readBytes!=BUFFER_SIZE) { // handle mismatch in read bytes
      if (feof(fp)) { //file ended and complete file read
        bufferUsed += readBytes;
        buffer[bufferUsed] = '\0';
        fclose(fp);
        return buffer;
      }
      else { // read error
        modaloSetLastError(EPARSE_CJSON_FILE,"Unable to read from file.");
        fclose(fp);
        free(buffer);
        return NULL;
      }
    }

    //no error and full buffer read
    bufferUsed += readBytes;
    bufferSize += BUFFER_SIZE*sizeof(char);
    if(bufferSize>MAX_FILESIZE) { // maximum file size exceeded
      modaloSetLastError(EPARSE_CJSON_FILE,"File size too high.");
      fclose(fp);
      free(buffer);
      return NULL;
    }
    temp = (char*) realloc(buffer,bufferSize); // reallocate buffer
    if(temp == NULL) { //handle error
      modaloSetLastError(EPARSE_CJSON_FILE,"Unable to reallocate buffer.");
      fclose(fp);
      free(buffer);
      return NULL;
    }

    //buffer reallocated and ready for next read
    buffer = temp;
    readBytes = 0;
  }
}

// To free MAP allocated by JSON Parser
MODALO_API void MODALO_CALL freeMAP(MAP map)
{
  free(map.reg);
}

// API function to parse JSON. After use u should free the memory using freeMAP()
MODALO_API MAP MODALO_CALL parseModaloJSONFile(char* fileName, char* modelName) 
{
  cJSON* root = NULL; //for holding root Object
  cJSON* model = NULL; // for holding the model Object
  cJSON* reg = NULL; // for holding the register object
  char* file = NULL; // for holding file in strings
  char error[MODALO_ERROR_MAXLENGTH] = ""; // for holding error string
  MAP map = {NULL, 0}; // structure for returning with map information

  file = readFileToBuffer(fileName);
  if(file == NULL) return map; //file not read to buffer

  root = cJSON_Parse(file);
  if(root == NULL) { // parse error occured
    const char* errorPtr = cJSON_GetErrorPtr();
    if (errorPtr != NULL) modaloSetLastError(EPARSE_CJSON_STRING,(char *)errorPtr);
    else modaloSetLastError(EPARSE_CJSON_STRING,"Unknown Error!");
    free(file);
    cJSON_Delete(root);
    return map;
  }

  model = cJSON_GetObjectItemCaseSensitive(root,modelName);
  if(model == NULL) { // object not found
    sprintf(error,"Unable to find object %s.",modelName);
    modaloSetLastError(EPARSE_CJSON_STRING,error);
    free(file);
    cJSON_Delete(root);
    return map;
  }

  // starting map assignement
  map.mapSize = cJSON_GetArraySize(model); // size of map
  map.reg = (REG*) calloc(map.mapSize,sizeof(REG)); // dynamically allocate memory for map
  if(map.reg == NULL) { //error allocation memory for map
    modaloSetLastError(EPARSE_CJSON_STRING,"Unable to allocate memory for map.");
    free(file);
    cJSON_Delete(root);
    return map;
  }

  int i=0; // itteration variable
  cJSON_ArrayForEach(reg,model) { //MACRO to itterate over array
    cJSON* regName = cJSON_GetObjectItemCaseSensitive(reg,"regName");
    cJSON* regAddress = cJSON_GetObjectItemCaseSensitive(reg,"regAddress");
    cJSON* regSizeU16 = cJSON_GetObjectItemCaseSensitive(reg,"regSizeU16");
    cJSON* byteReversed = cJSON_GetObjectItemCaseSensitive(reg,"byteReversed");
    cJSON* bitReversed = cJSON_GetObjectItemCaseSensitive(reg,"bitReversed");
    cJSON* functionCode = cJSON_GetObjectItemCaseSensitive(reg,"functionCode");
    cJSON* multiplier = cJSON_GetObjectItemCaseSensitive(reg,"multiplier");
    cJSON* divisor = cJSON_GetObjectItemCaseSensitive(reg,"divisor");
    cJSON* movingAvgFilter = cJSON_GetObjectItemCaseSensitive(reg,"movingAvgFilter");

    if(cJSON_IsString(regName) && strlen(regName->valuestring)<REGNAME_MAXSIZE) // regName validated
      strcpy(map.reg[i].regName,regName->valuestring);
    else break;

    //default type validations
    if(cJSON_IsNumber(regAddress)) map.reg[i].regAddress = regAddress->valueint; else break;//validated
    if(cJSON_IsNumber(regSizeU16)) map.reg[i].regSizeU16 = regSizeU16->valueint; else break;//validated
    if(cJSON_IsBool(byteReversed)) map.reg[i].byteReversed = byteReversed->valueint; else break;//validated
    if(cJSON_IsBool(bitReversed)) map.reg[i].bitReversed = bitReversed->valueint; else break;//validated
    if(cJSON_IsNumber(functionCode)) map.reg[i].functionCode = functionCode->valueint; else break;//validated
    if(cJSON_IsNumber(multiplier)) map.reg[i].multiplier = multiplier->valueint; else break;//validated
    if(cJSON_IsNumber(divisor)) map.reg[i].divisor = divisor->valueint; else break;//validated
    if(cJSON_IsBool(movingAvgFilter)) map.reg[i].movingAvgFilter = movingAvgFilter->valueint; else break;//validated

    //extra validations based on implementation
    if(map.reg[i].regSizeU16 != 1 && map.reg[i].regSizeU16 != 2) break; // error if regSize is neither 1 or 2
    if(map.reg[i].multiplier == 0 || map.reg[i].divisor == 0) break; // error if either multiplier or divisor is zero

    i++;
    if(i==map.mapSize) break;
  }
  if(i!=map.mapSize) { //parse error: one of the validation has failed and loop has breaked.
    modaloSetLastError(EPARSE_CJSON_STRING,"JSON map not as per modalo standard. (refer documentation)");
    free(file);
    cJSON_Delete(root);
    free(map.reg);
    map.reg=NULL;
    return map;
  }

  //parsed successfully
  free(file);
  cJSON_Delete(root);
  return map;
}
