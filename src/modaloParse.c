#include <stdio.h> // file operations, printf
#include <stdlib.h> //calloc, EOF, free
#include <string.h> //memset defined
#include <stdint.h> //size_t , uint_16 defined

#include "modaloError.h"
#include "cJSON.h"
#include "modaloParse.h"

int validateModaloToken(CONFIG* config, char *parameter, char *value) // validates and saves parameter and value
{
  char error[MODALO_ERROR_MAXLENGTH] = "";
  if(!strcmp(parameter,"COM_PORT"))
  {
    if(strlen(value)<6 &&
        value[0] =='C' &&
        value[1] =='O' &&
        value[2] == 'M' && // check for "COM"
        '1'<=value[3] && value[3]<='9' && // digit 1-9
        (( '0'<=value[4] && value[4]<='9') || value[4] == '\0' ))// digit 1-9 or null character
    strcpy(config->port,value); // valid port
    else
    {
      sprintf(error,"Invalid value for parameter COM_PORT: %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"BAUD_RATE"))
  {
    if(strlen(value)<7 && atoi(value))
    config->baud = atoi(value);
    else
    {
      sprintf(error,"Invalid value for parameter BAUD_RATE: %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"DATA_BITS"))
  {
    if(strlen(value)<3 && (atoi(value)))
    config->dataBits = atoi(value);
    else
    {
      sprintf(error,"Invalid value for parameter DATA_BITS: %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"STOP_BITS"))
  {
    if(strlen(value)<2 && (atoi(value) || value[0]=='0'))
    config->stopBits = atoi(value);
    else
    {
      sprintf(error,"Invalid value for parameter STOP_BITS: %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"PARITY"))
  {
    if(!strcmp(value,"ODD")) config->parity=PARITY_ODD;
    else if(!strcmp(value,"EVEN")) config->parity=PARITY_EVEN;
    else if(!strcmp(value,"NONE")) config->parity=PARITY_NONE;
    else
    {
      sprintf(error,"Invalid value for parameter PARITY: %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"INVERTER_MAKE"))
  {
    if(strlen(value)<MAKE_MODEL_NAMESIZE) strcpy(config->make,value);
    else
    {
      sprintf(error,"Invalid value for parameter INVERTER_MAKE(max 16 characters only): %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"INVERTER_MODEL"))
  {
    if(strlen(value)<MAKE_MODEL_NAMESIZE) strcpy(config->model,value);
    else
    {
      sprintf(error,"Invalid value for parameter INVERTER_MODEL(max 16 characters only): %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"INSTALLED_CAPACITY"))
  {
    if(strlen(value)<7 && atoi(value))
    config->capacity = atoi(value);
    else
    {
      sprintf(error,"Invalid value for parameter INSTALLED_CAPACITY: %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"SAMPLE_INTERVAL"))
  {
    if(strlen(value)<6 && atoi(value) && atoi(value) <= 86400)
    config->sampleInterval = atoi(value);
    else
    {
      sprintf(error,"Invalid value for parameter SAMPLE_INTERVAL: %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"FILE_INTERVAL"))
  {
    if(strlen(value)<5 && atoi(value) && atoi(value) <= 1440)
    config->fileInterval = atoi(value);
    else
    {
      sprintf(error,"Invalid value for parameter FILE_INTERVAL: %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"POLL_INTERVAL"))
  {
    if(strlen(value)<6 && atoi(value))
    config->pollInterval = atoi(value);
    else
    {
      sprintf(error,"Invalid value for parameter POLL_INTERVAL: %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"START_LOG_HHMM"))
  {
    if(strlen(value)==4 &&
        '0'<=value[1] && value[1]<='9' && // 2nd digit 0-9
        '0'<=value[2] && value[2]<='5' && // 3rd digit 0-5
        '0'<=value[3] && value[3]<='9' && // 4th digit 0-9
        ( value[0]!='2' || value[1]<='3'))// check for hour !> 23
    config->startLog = ((int)atoi(value)/100)*60+atoi(value+2); // valid startLog format
    else
    {
      sprintf(error,"Invalid value for parameter START_LOG_HHMM: %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else if(!strcmp(parameter,"STOP_LOG_HHMM"))
  {
    if(strlen(value)==4 &&
        '0'<=value[1] && value[1]<='9' && // 2nd digit 0-9
        '0'<=value[2] && value[2]<='5' && // 3rd digit 0-5
        '0'<=value[3] && value[3]<='9' && // 4th digit 0-9
        ( value[0]!='2' || value[1]<='3'))// check for hour !> 23
    config->stopLog = ((int)atoi(value)/100)*60+atoi(value+2); // valid startLog format
    else
    {
      sprintf(error,"Invalid value for parameter START_LOG_HHMM: %s",value);
      modaloSetLastError(EVALIDATE_PARAMETER,error);
      return 0;
    }
  }
  else { // parameter does not match any validations
    sprintf(error,"Unknown parameter: %s",parameter);
    modaloSetLastError(EVALIDATE_PARAMETER,error);
    return 0;
  }
  return 1; //validated successfully
}

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
  //parameter retreived
  value = strtok(NULL,delimiter);
  if(value==NULL)
  {
    sprintf(error,"Invalid value for parameter: %s",parameter);
    modaloSetLastError(EPARSE_CONFIG_STRING,error);
    return 0;
  }

  //value retrived
  return validateModaloToken(config,parameter,value);
}

MODALO_API int MODALO_CALL parseModaloConfigFile(CONFIG* config, char * FileName)
{
  FILE *file = NULL; //initialise file handler
  char *lineBuffer = NULL; //initialse lineBuffer pointer
  char error[MODALO_ERROR_MAXLENGTH] = "";
  strcpy(config->port,"COM3");
  config->baud=9600;
  config->dataBits=8;
  config->stopBits=1;
  config->parity='N';
  strcpy(config->make,"SOLIS");
  strcpy(config->model,"SOLIS-15K");
  config->capacity=17500;
  config->sampleInterval=60;
  config->fileInterval=900;
  config->pollInterval=5;
  config->startLog=330;
  config->stopLog=1200; // struct to hold config data

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
          return(0);
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
    return 0;
  }
  fclose(file);
  free(lineBuffer);
  return 1; // parsed successfully
}

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

MODALO_API void MODALO_CALL freeMAP(MAP map)
{
  free(map.reg);
}

MODALO_API MAP MODALO_CALL parseModaloJSONFile(char* fileName, char* modelName) // if u use this function, then u should free the memory using freeMAP()
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
