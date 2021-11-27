/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.svox.pico;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.Environment;
import android.speech.tts.TextToSpeech;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/*
 * Checks if the voice data for the SVOX Pico Engine is present on the
 * sd card.
 */
public class CheckVoiceData extends Activity {
    // The following constants are the same path constants as the ones defined
    // in external/svox/pico/picoEngine_AndroidWrapper/com_svox_picottsengine.cpp
    private static String PICO_LINGWARE_PATH = "";
            //Environment.getDataDirectory() + "/";

    private final static String PICO_SYSTEM_LINGWARE_PATH = "/system/picoEngine_AndroidWrapper/lang_pico/";

    private final static String[] dataFiles = {
            "de-DE_gl0_sg.bin", "de-DE_ta.bin", "en-GB_kh0_sg.bin", "en-GB_ta.bin",
            "en-US_lh0_sg.bin", "en-US_ta.bin", "es-ES_ta.bin", "es-ES_zl0_sg.bin",
            "fr-FR_nk0_sg.bin", "fr-FR_ta.bin", "it-IT_cm0_sg.bin", "it-IT_ta.bin"
    };

    private final static String[] dataFilesInfo = {
        "deu-DEU", "deu-DEU", "eng-GBR", "eng-GBR", "eng-USA", "eng-USA",
        "spa-ESP", "spa-ESP", "fra-FRA", "fra-FRA", "ita-ITA", "ita-ITA"
    };

    private final static String[] supportedLanguages = {
        "deu-DEU", "eng-GBR", "eng-USA", "spa-ESP", "fra-FRA", "ita-ITA"
    };


    //for installer
    private String rootDirectory = "";
    private CheckVoiceData self;
    private static boolean sInstallationSuccess = false;
    private static boolean sIsInstalling = false;
    private final static Object sInstallerStateLock = new Object();
    private Activity parent;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //PICO_LINGWARE_PATH= getApplicationInfo().dataDir+"/";
        self = this;
        PICO_LINGWARE_PATH= getFilesDir().getPath()+"/";


        int result = TextToSpeech.Engine.CHECK_VOICE_DATA_PASS;
        boolean foundMatch = false;

        ArrayList<String> available = new ArrayList<String>();
        ArrayList<String> unavailable = new ArrayList<String>();

        HashMap<String, Boolean> languageCountry = new HashMap<String, Boolean>();

        Bundle bundle = getIntent().getExtras();
        if (bundle != null){
            ArrayList<String> langCountryVars = bundle.getStringArrayList(
                    TextToSpeech.Engine.EXTRA_CHECK_VOICE_DATA_FOR);
            if (langCountryVars != null){
                for (int i = 0; i < langCountryVars.size(); i++){
                    if (langCountryVars.get(i).length() > 0){
                        languageCountry.put(langCountryVars.get(i), true);
                    }
                }
            }
        }

        // Check for files
        for (int i = 0; i < supportedLanguages.length; i++){
            if ((languageCountry.size() < 1) ||
                (languageCountry.containsKey(supportedLanguages[i]))){
                if (!fileExists(dataFiles[2 * i]) ||
                    !fileExists(dataFiles[(2 * i) + 1])){
                    result = TextToSpeech.Engine.CHECK_VOICE_DATA_MISSING_DATA;
                    unavailable.add(supportedLanguages[i]);
                } else {
                    available.add(supportedLanguages[i]);
                    foundMatch = true;
                }
            }
        }

        //STUB EJ
        //pico can manage 6 languages. If one is missing, run installer.

        if (!unavailable.isEmpty())
        {
            //InstallerActivity install=new InstallerActivity();
            //Resources res = getResources();
            runInstaller(PICO_LINGWARE_PATH);
        }


        if ((languageCountry.size() > 0) && !foundMatch){
            result = TextToSpeech.Engine.CHECK_VOICE_DATA_FAIL;
        }

        // Put the root directory for the sd card data + the data filenames
        Intent returnData = new Intent();
        returnData.putExtra(TextToSpeech.Engine.EXTRA_VOICE_DATA_ROOT_DIRECTORY, PICO_LINGWARE_PATH);
        returnData.putExtra(TextToSpeech.Engine.EXTRA_VOICE_DATA_FILES, dataFiles);
        returnData.putExtra(TextToSpeech.Engine.EXTRA_VOICE_DATA_FILES_INFO, dataFilesInfo);

        returnData.putStringArrayListExtra(TextToSpeech.Engine.EXTRA_AVAILABLE_VOICES, available);
        returnData.putStringArrayListExtra(TextToSpeech.Engine.EXTRA_UNAVAILABLE_VOICES, unavailable);
        setResult(result, returnData);
        finish();
    }

    private boolean fileExists(String filename){
        File tempFile = new File(PICO_LINGWARE_PATH + filename);
        File tempFileSys = new File(PICO_SYSTEM_LINGWARE_PATH + filename);
        if ((!tempFile.exists()) && (!tempFileSys.exists())) {
            return false;
        }
        return true;
    }



    public void runInstaller(String directory){
        try {
            rootDirectory = directory;
            Resources res = getResources();
            AssetFileDescriptor langPackFd = res
                    .openRawResourceFd(R.raw.svoxlangpack);
            InputStream stream = langPackFd.createInputStream();

            (new Thread(new unzipper(stream))).start();
        } catch (IOException e) {
            Log.e("PicoLangInstaller", "Unable to open langpack resource.");
            e.printStackTrace();
        }
        //setContentView(R.layout.installing);
    }


    private boolean unzipLangPack(InputStream stream) {
        FileOutputStream out;
        byte buf[] = new byte[16384];
        try {
            ZipInputStream zis = new ZipInputStream(stream);
            ZipEntry entry = zis.getNextEntry();
            while (entry != null) {
                if (entry.isDirectory()) {
                    File newDir = new File(rootDirectory + entry.getName());
                    newDir.mkdir();
                } else {
                    String name = entry.getName();
                    File outputFile = new File(rootDirectory + name);
                    String outputPath = outputFile.getCanonicalPath();
                    name = outputPath
                            .substring(outputPath.lastIndexOf("/") + 1);
                    outputPath = outputPath.substring(0, outputPath
                            .lastIndexOf("/"));
                    File outputDir = new File(outputPath);
                    outputDir.mkdirs();
                    outputFile = new File(outputPath, name);
                    outputFile.createNewFile();
                    out = new FileOutputStream(outputFile);
                    int numread = 0;
                    do {
                        numread = zis.read(buf);
                        if (numread <= 0) {
                            break;
                        } else {
                            out.write(buf, 0, numread);
                        }
                    } while (true);
                    out.close();
                }
                entry = zis.getNextEntry();
            }
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    private class unzipper implements Runnable {
        public InputStream stream;

        public unzipper(InputStream is) {
            stream = is;
        }

        public void run() {
            boolean result = unzipLangPack(stream);
            synchronized (sInstallerStateLock)
            {
                sInstallationSuccess = true;
                sIsInstalling = false;
            }

            //finish();

            /*if (sInstallationSuccess)*/ {
                // installation completed: signal success (extra set to SUCCESS)

                Intent installCompleteIntent =
                        new Intent(TextToSpeech.Engine.ACTION_TTS_DATA_INSTALLED);
                installCompleteIntent.putExtra(TextToSpeech.Engine.EXTRA_TTS_DATA_INSTALLED, TextToSpeech.SUCCESS);
                self.sendBroadcast(installCompleteIntent);
                self.finish();
            }
        }
    }



}
