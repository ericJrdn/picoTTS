/*
 * Copyright (C) 2021 eric_jrdn
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <iostream>
#include <cstring>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utils/Log.h>
#include <picoapi.h>
#include <picodefs.h>
#include <tts.h>
#include <TtsEngine.h>

using namespace std;

const char *PICO_LINGWARE_PATH = "/data/user/0/com.svox.pico/files/";

#define PICO_MEMORY_SIZE 2500000
#define MIN_IO_SIZE 6
#define MAX_OUTBUF_SIZE 128

const char *PICO_VOICE_NAME = "PicoVoice";
synthDoneCB_t *picoSynthDoneCBPtr = NULL;
pico_System picoSystem = NULL;
pico_Resource picoTaResource = NULL;
pico_Resource picoSgResource = NULL;
pico_Engine picoEngine = NULL;
android_tts_engine_t *androidEngineStruct = NULL;
void *picoMemory = NULL;
int currentRate = 100;
int currentPitch = 100;
int currentVolume = 100;

int stopRequest = 0;

typedef struct picoLanguagedefinition {
    const char *lang;
    const char *country;
    const char *locale;
    const char *taFile;
    const char *SgFile;
} LanguageDef;

const unsigned int picoNumSupportedLanguages = 6;
unsigned int currentLanguage = -1;

const LanguageDef availableLanguages[] =
{
        {"eng", "USA", "en-US", "en-US_ta.bin", "en-US_lh0_sg.bin"},
        {"eng", "GBR", "en-GB", "en-GB_ta.bin", "en-GB_kh0_sg.bin"},
        {"deu", "DEU", "de-DE", "de-DE_ta.bin", "de-DE_gl0_sg.bin"},
        {"spa", "ESP", "es-ES", "es-ES_ta.bin", "es-ES_zl0_sg.bin"},
        {"fra", "FRA", "fr-FR", "fr-FR_ta.bin", "fr-FR_nk0_sg.bin"},
        {"ita", "ITA", "it-IT", "it-IT_ta.bin", "it-IT_cm0_sg.bin"}
};


int cnvIpaToXsampa(const char16_t *ipaString, size_t ipaStringSize, char **outXsampaString) {
    //not implemented
    return 0;
}


const char *createPhonemeString(const char *s, int length) {
    return "\0";
}

const char *configureOutput(const char *s) {
    std::string mystring(s), val;
    const char *ret;

    val="<speed level='"+std::to_string(currentRate)+"'>";
    mystring.insert (0,val);

    val="<pitch level='"+std::to_string(currentPitch)+"'>";
    mystring.insert (0,val);

    val="<volume level='"+std::to_string(currentVolume)+"'>";
    mystring.insert (0,val);

    mystring+="</speed>";
    mystring+="</pitch>";
    mystring+="</volume>";
    ret = mystring.c_str();
    return ret;
}


pico_Status initPicoSystem()
{
    pico_Status status = PICO_OK;
    if (NULL==picoMemory)
    {
        picoMemory = malloc(PICO_MEMORY_SIZE);
    }

    if (NULL==picoSystem) {
        //create pico system
        status = pico_initialize(picoMemory, PICO_MEMORY_SIZE, &picoSystem);
        if (PICO_OK != status || NULL == picoSystem) {
            if (NULL != picoMemory) {
                free(picoMemory);
                picoMemory = NULL;
            }
            ALOGE("initPicoSystemfailed");
        }
    }
    return status;
}

android_tts_result_t init(void *eng, synthDoneCB_t synthDonePtr, const char *engineConfig) {
    if (NULL == androidEngineStruct || NULL == synthDonePtr) {
        ALOGE("init failed, androidEngineStruct or callback is null");
        return ANDROID_TTS_FAILURE;
    }
    picoSynthDoneCBPtr = synthDonePtr;
    pico_Status status = initPicoSystem();
    if (PICO_OK != status)
    {
        return ANDROID_TTS_FAILURE;
    }
    else
    {
        return ANDROID_TTS_SUCCESS;
    }
}


android_tts_result_t releaseLanguageResources() {
    if (NULL == picoSystem) {
        ALOGE("releaseLanguageResources failed");
        return ANDROID_TTS_FAILURE;
    }
    if (picoTaResource) {
        pico_unloadResource(picoSystem, &picoTaResource);
        picoTaResource = NULL;
    }

    if (picoSgResource) {
        pico_unloadResource(picoSystem, &picoSgResource);
        picoSgResource = NULL;
    }

    if (picoEngine) {
        pico_disposeEngine( picoSystem, &picoEngine );
        pico_releaseVoiceDefinition( picoSystem, (pico_Char *) PICO_VOICE_NAME );
        picoEngine = NULL;
    }

    //crash if not done
    if (picoSystem) {
        pico_terminate(&picoSystem);
        picoSystem = NULL;
    }

    return ANDROID_TTS_SUCCESS;
}


android_tts_result_t shutdown(void *eng) {
    releaseLanguageResources();
        if (NULL != androidEngineStruct)
        {
            if (NULL != androidEngineStruct->funcs)
            {
                free(androidEngineStruct->funcs);
            }
            free(androidEngineStruct);
            androidEngineStruct=NULL;
        }
        if (NULL!=picoMemory)
        {
            free(picoMemory);
            picoMemory=NULL;
        }
        return ANDROID_TTS_SUCCESS;
}


android_tts_result_t stop(void *eng) {
    stopRequest = 1;
    return ANDROID_TTS_SUCCESS;
}


bool checkLanguageInstallation(int languageIndex)
{
    if (-1 == languageIndex || languageIndex>=picoNumSupportedLanguages)
        return false;
    //check if files are installed.
    char* fileName = (char*)malloc(PICO_MAX_DATAPATH_NAME_SIZE + PICO_MAX_FILE_NAME_SIZE);
    FILE * pFile=NULL;

    strcpy(fileName, PICO_LINGWARE_PATH);
    strcat(fileName, availableLanguages[languageIndex].taFile);
    pFile = fopen(fileName, "r");
    if (pFile == NULL) {
        return false;
    }
    else
    {
        fclose(pFile);
        free(fileName);
        return true;
    }
}

unsigned int getLanguageIndex(const char *lang)
{
    unsigned int ret = -1;
    if (NULL == lang || strcmp(lang,"\0")==0) {
        return ret;
    }
    for (int i = 0; i < picoNumSupportedLanguages; i++) {
        if (strcmp(lang, availableLanguages[i].lang) == 0) {
            if (true == checkLanguageInstallation(i))
            {
                return i;
            }
            else
            {
                return -1;
            }
        }
    }
    return ret;
}


unsigned int getLanguageIndex(const char *lang, const char *country) {
    unsigned int ret = -1;
    if (NULL==lang || strcmp(lang, "\0")==0)
    {
        return ret;
    }
    else if (NULL == country || strcmp(country,"\0")==0) {
        return getLanguageIndex(lang);
    }
    for (int i = 0; i < picoNumSupportedLanguages; i++) {
        if (strcmp(lang, availableLanguages[i].lang) == 0 &&
            strcmp(country, availableLanguages[i].country) == 0) {
            if (true == checkLanguageInstallation(i))
            {
                return i;
            }
            else
            {
                return -1;
            }
        }
    }
    return ret;
}


android_tts_support_result_t
isLanguageAvailable(void *eng, const char *lang, const char *country, const char *locale) {
    if (-1 == getLanguageIndex(lang, country)) {
        return ANDROID_TTS_LANG_NOT_SUPPORTED;
    } else {
        return ANDROID_TTS_LANG_AVAILABLE;
    }
}


android_tts_result_t
setLanguage(void *eng, const char *lang, const char *country, const char *locale) {
    pico_Status status = PICO_OK;
    pico_Char *picoTaResourceName = (pico_Char *) malloc(PICO_MAX_RESOURCE_NAME_SIZE);
    pico_Char *picoSgResourceName = (pico_Char *) malloc(PICO_MAX_RESOURCE_NAME_SIZE);
    char* fileName = (char*)malloc(PICO_MAX_DATAPATH_NAME_SIZE + PICO_MAX_FILE_NAME_SIZE);

    unsigned int requestedLanguage = getLanguageIndex(lang, country);

    if (-1 == requestedLanguage || requestedLanguage >= picoNumSupportedLanguages) {
        ALOGE("setLanguage %d not supported", requestedLanguage);
        return ANDROID_TTS_FAILURE;
    }

    //unload previous resources
    status = releaseLanguageResources();
    if (PICO_OK != status) ALOGE("setLanguage : releaseLanguageResources failed");

    //create the new pico system
    if (PICO_OK == status)
        status = initPicoSystem();

    if (PICO_OK != status || NULL == picoSystem) ALOGE("initPicoSystem failed");

    //load text analysis res
    strcpy(fileName, PICO_LINGWARE_PATH);
    strcat(fileName, availableLanguages[requestedLanguage].taFile);

    if (PICO_OK == status)
        status = pico_loadResource(picoSystem, (const pico_Char *)fileName,&picoTaResource);
    else
        ALOGE("setLanguage : pico_loadResource failed");

    if (PICO_OK == status)
        status = pico_getResourceName(picoSystem, picoTaResource, (char *) picoTaResourceName);
    else
        ALOGE("setLanguage : pico_getResourceName failed");

    //load signal generation res
    strcpy(fileName, PICO_LINGWARE_PATH);
    strcat(fileName, availableLanguages[requestedLanguage].SgFile);

    if (PICO_OK == status)
        status = pico_loadResource(picoSystem, (const pico_Char *)fileName,&picoSgResource);
    else
        ALOGE("setLanguage : pico_loadResource failed");

    if (PICO_OK == status)
        status = pico_getResourceName(picoSystem, picoSgResource, (char *) picoSgResourceName);
    else
        ALOGE("setLanguage : pico_getResourceName failed");

    //create voice
    if (PICO_OK == status)
        status = pico_createVoiceDefinition(picoSystem, (const pico_Char *) PICO_VOICE_NAME);
    else
        ALOGE("setLanguage : pico_createVoiceDefinition failed");

    if (PICO_OK == status)
        status = pico_addResourceToVoiceDefinition(picoSystem, (const pico_Char *) PICO_VOICE_NAME,
                                                   picoTaResourceName);
    else
        ALOGE("setLanguage : pico_addResourceToVoiceDefinition failed");

    if (PICO_OK == status)
        status = pico_addResourceToVoiceDefinition(picoSystem, (const pico_Char *) PICO_VOICE_NAME,
                                                   picoSgResourceName);
    else
        ALOGE("setLanguage : pico_addResourceToVoiceDefinition failed");

    //create pico androidEngineStruct
    if (PICO_OK == status)
        status = pico_newEngine(picoSystem, (const pico_Char *) PICO_VOICE_NAME, &picoEngine);
    else
        ALOGE("setLanguage : pico_newEngine failed %d", status);

    free(picoTaResourceName);
    free(picoSgResourceName);
    free(fileName);
    currentLanguage = requestedLanguage;

    if (PICO_OK != status) {
        ALOGE("setLanguage %s %s %s failed", *lang, *country, *locale);
        releaseLanguageResources();
        return ANDROID_TTS_FAILURE;
    } else {
        return ANDROID_TTS_SUCCESS;
    }
}


android_tts_result_t setLanguageByIdx(int idx) {
    if (-1==idx)
    {
        ALOGE("setLanguageByIdx failed");
        return ANDROID_TTS_FAILURE;
    }
    else
    {
        return setLanguage(NULL, availableLanguages[idx].lang, availableLanguages[idx].country, availableLanguages[idx].locale);
    }
}


android_tts_result_t
loadLanguage(void *eng, const char *lang, const char *country, const char *locale) {
    setLanguage(androidEngineStruct, lang, country, locale);
}


android_tts_result_t getLanguage(void *eng, char *language, char *country, char *locale) {
    if (-1 == currentLanguage) {
        strcpy(language, "\0");
        strcpy(country, "\0");
        strcpy(locale, "\0");
        ALOGE("getLanguage %s %s %s failed", *language, *country, *locale);
        return ANDROID_TTS_FAILURE;
    } else {
        strcpy(language, availableLanguages[currentLanguage].lang);
        strcpy(country, availableLanguages[currentLanguage].country);
        strcpy(locale, availableLanguages[currentLanguage].locale);
    }
    return ANDROID_TTS_SUCCESS;
}

android_tts_result_t
setAudioFormat(void *eng, android_tts_audio_format_t *pEncoding, uint32_t *pRate,
               int *pChannels) {
    *pEncoding = ANDROID_TTS_AUDIO_FORMAT_PCM_16_BIT;
    *pRate = 16000;
    *pChannels = 1;
    return ANDROID_TTS_SUCCESS;
}

android_tts_result_t
setProperty(void *eng, const char *property, const char *value, const size_t size) {
    if (NULL == property) {
        ALOGE("setProperty failed with null property");
        return ANDROID_TTS_PROPERTY_UNSUPPORTED;
    }
    if (value == NULL) {
        ALOGE("setProperty failed with null value");
        return ANDROID_TTS_VALUE_INVALID;
    }

    if (strncmp(property, "language", 8) == 0) {
        int idx = getLanguageIndex(value);
        return setLanguageByIdx(idx);
    }
    else if (strncmp(property, "rate", 4) == 0) {
        currentRate = atoi(value);
    }
    else if (strncmp(property, "pitch", 5) == 0) {
        currentPitch = atoi(value);
    }
    else if (strncmp(property, "volume", 6) == 0) {
        currentVolume = atoi(value);
    }
    else
    {
        return ANDROID_TTS_PROPERTY_UNSUPPORTED;
    }
    return ANDROID_TTS_SUCCESS;
}

android_tts_result_t getProperty(void *eng, const char *property, char *value, size_t *iosize) {
    if (NULL == property) {
        ALOGE("getProperty failed with null property");
        return ANDROID_TTS_PROPERTY_UNSUPPORTED;
    }

    if (*iosize < MIN_IO_SIZE) {
        *iosize = MIN_IO_SIZE;
        return ANDROID_TTS_PROPERTY_SIZE_TOO_SMALL;
    }

    if (strncmp(property, "language", 8) == 0) {
        if (-1 == currentLanguage) {
            strcpy(value, "");
        } else {
            strcpy(value, availableLanguages[currentLanguage].lang);
        }
    }
    else if (strncmp(property, "rate", 4) == 0) {
        sprintf(value, "%d", currentRate);
    }
    else if (strncmp(property, "pitch", 5) == 0) {
        sprintf(value, "%d", currentPitch);
    }
    else if (strncmp(property, "volume", 6) == 0) {
        sprintf(value, "%d", currentVolume);
    }
    else
    {
        return ANDROID_TTS_PROPERTY_UNSUPPORTED;
    }
    return ANDROID_TTS_SUCCESS;
}

android_tts_result_t
synthesizeText(void *eng, const char *text0, int8_t *buffer, size_t bufferSize, void *userdata) {
    const char *text = configureOutput(text0);
    if (NULL==picoSynthDoneCBPtr)
    {
        ALOGE("picoSynthDoneCBPtr is NULL");
        return ANDROID_TTS_FAILURE;
    }
    if (NULL==buffer)
    {
        ALOGE("buffer is NULL");
        return ANDROID_TTS_FAILURE;
    }

    pico_Char *currentPos = NULL;
    pico_Status picoStatus;
    int callbackStatus;

    pico_Int16 acceptedSize, remainingSize;
    pico_Int16 receivedDataSize, dataType;
    size_t dataBufferSize;
    short localBuf[MAX_OUTBUF_SIZE / 2];
    remainingSize = strlen(text) + 1;
    currentPos = (pico_Char *) text;
    stopRequest = 0;

    while (remainingSize>0) {
        if (1 == stopRequest) {
            picoStatus = pico_resetEngine(picoEngine, PICO_RESET_SOFT);
            return ANDROID_TTS_FAILURE;
        }
        //send text
        picoStatus = pico_putTextUtf8(picoEngine, currentPos, remainingSize, &acceptedSize);
        if (picoStatus != PICO_OK) {
            stopRequest = 1;
            ALOGE("synthesizeText pico_putTextUtf8 failed %d ", picoStatus);
            return ANDROID_TTS_FAILURE;
        }
        remainingSize -= acceptedSize;
        currentPos += acceptedSize;

        //get speech data
        while (1)
        {
            picoStatus = pico_getData(picoEngine, (void *) localBuf, MAX_OUTBUF_SIZE,
                                      &receivedDataSize, &dataType);
            if (0!=receivedDataSize && PICO_STEP_BUSY==picoStatus) {
                //copy for multithreading
                memcpy(buffer, (int8_t *) localBuf, receivedDataSize);
                dataBufferSize = receivedDataSize;
                callbackStatus = picoSynthDoneCBPtr(&userdata, 16000,
                                                    ANDROID_TTS_AUDIO_FORMAT_PCM_16_BIT, 1,
                                                    &buffer, &dataBufferSize,
                                                    ANDROID_TTS_SYNTH_PENDING);
                if (callbackStatus == ANDROID_TTS_CALLBACK_HALT) {
                    stopRequest = 1;
                    break;
                }
            }
            else if(PICO_STEP_IDLE==picoStatus) {
                //done and ok -> stop
                break;
            }
            else if(PICO_STEP_ERROR==picoStatus){
                stopRequest = 1;
                ALOGE("synthesizeText pico_getData failed");
                break;
            }
            else
            {
                //busy and no data received
            }
        }
    }

    //notify done
    picoSynthDoneCBPtr(&userdata, 16000,
                                        ANDROID_TTS_AUDIO_FORMAT_PCM_16_BIT, 1,
                                        &buffer, &dataBufferSize,
                                        ANDROID_TTS_SYNTH_DONE);

    return ANDROID_TTS_SUCCESS;
}

// Function to get TTS Engine
android_tts_engine_t *getTtsEngine()
{
    if (androidEngineStruct == NULL)
    {
        androidEngineStruct = (android_tts_engine_t*) malloc(sizeof(android_tts_engine_t));
        android_tts_engine_funcs_t* functable = (android_tts_engine_funcs_t*) malloc(sizeof(android_tts_engine_funcs_t));
        functable->init = &init;
        functable->shutdown = &shutdown;
        functable->stop = &stop;
        functable->isLanguageAvailable = &isLanguageAvailable;
        functable->loadLanguage = &loadLanguage;
        functable->setLanguage = &setLanguage;
        functable->getLanguage = &getLanguage;
        functable->setAudioFormat = &setAudioFormat;
        functable->setProperty = &setProperty;
        functable->getProperty = &getProperty;
        functable->synthesizeText = &synthesizeText;
        androidEngineStruct->funcs = functable;
    }
    return androidEngineStruct;
}

android_tts_engine_t *android_getTtsEngine()
{
    return getTtsEngine();
}
