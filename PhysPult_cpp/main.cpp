﻿#include <iostream>
#include <chrono>
#include <thread>
#include <conio.h>
#include <fstream>
#include "process.h"
#include "interface.h"
#include "include/SimpleSerial.h"

using namespace std;

void endProgram(SimpleSerial& Serial) {
    if (Serial.CloseSerialPort()) {
        exit(EXIT_SUCCESS);
        cout << "Closed connection to COM port\n";
    }
    else {
        exit(EXIT_FAILURE);
        cerr << "Failed to close connection to COM port\n";
    }
}

// Initializing based on config file
void init(string config_PATH, string& indicators_PATH, string& switches_PATH, short& FREQ_HZ, DWORD& COM_BAUD_RATE, short& TOTALINDICATORS, short& TOTALSWITCHES) {
    ifstream config(config_PATH);
    if (!config.is_open()) {
        cerr << "Couldn't open config file!\n";
        system("pause");
        exit(EXIT_FAILURE);
    }
    getline(config, indicators_PATH); // set path to file with indicators state
    getline(config, switches_PATH); // set path to file with switches state

    ifstream infile(indicators_PATH); // check if indicators file is valid
    if (!infile.is_open()) {
        cerr << "Couldn't open file with indicators state!\n";
        system("pause");
        exit(EXIT_FAILURE);
    }

    string temp;
    getline(config, temp);
    if(!temp.empty()) FREQ_HZ = stoi(temp); // set frequency
    if (FREQ_HZ > 60) cout << "High frequence is set (>60 Hz). Are you sure you need this much?\n";

    getline(config, temp); 
    if (!temp.empty()) COM_BAUD_RATE = stoi(temp); // set baud rate

    getline(config, temp);
    if (!temp.empty()) TOTALINDICATORS = stoi(temp); // set indicators count (string length)

    getline(config, temp);
    if (!temp.empty()) TOTALSWITCHES = stoi(temp); // set switches count (string length)

    cout << "Initialized succesfully!\nIndicators state file: " << indicators_PATH << "\nSwitches state file: " << switches_PATH
    << "\nFrequency: " << FREQ_HZ << "\nBaud rate: " << COM_BAUD_RATE
    << "\nIndicators number: " << TOTALINDICATORS << "\nSwitches number: " << TOTALSWITCHES << "\n\n";
}

int main(int argc, char* argv[])
{   
    //--- INITIALISING ---

    string indicators_PATH = "lamps.txt", switches_PATH = "switches.txt", config_PATH = "physpult_config.txt";
    short FREQ_HZ = 10, TOTALINDICATORS = 32, TOTALSWITCHES = 64;
    DWORD COM_BAUD_RATE = 9600; // Set default values for all variables
    if (argv[1] != NULL) config_PATH = argv[1];

    init(config_PATH, indicators_PATH, switches_PATH, FREQ_HZ, COM_BAUD_RATE, TOTALINDICATORS, TOTALSWITCHES); // Initialize based on config file

    string indicatorsPrevious(TOTALINDICATORS, '0'), switchesPrevious(TOTALSWITCHES, '0');

    //--- COM PORT INITIALISING ---
    InitCOMPort:

    SimpleSerial Serial(&SelectCOMport()[0], COM_BAUD_RATE);

    do {
        if (!Serial.connected_) {
            cout << "Failed to connect! Press 'q' to exit, '2' to select another COM port or any other key to retry.\n";
            switch (_getch()) {
            case 'q': exit(EXIT_FAILURE); break;
            case '2': goto InitCOMPort;
            } 
        }
        break;
        /*else {
            bool init = 0;
            for (int i = 0; i < 10; i++) {
                if (init) break;

                cout << "wrt {PhysPultInit} " << Serial.WriteSerialPort((char*)"{PhysPultInit}") << "\n";
                string rec = Serial.ReadSerialPort(1, "json");
                cout << "rec " << rec << "\n";
                if (rec == "PhysPultInitOK") init = 1;
            }
            if (init) {
                cout << "Arduino succesfully connected.\n\n";
                break;
            }
            cerr << "Selected port is not Arduino set up for PhysPult. Press q to exit or any key to select other COM port.\n\n";
            switch (_getch()) {
                case 'q': endProgram(Serial); break;
            }
            goto InitCOMPort;
        }*/
    } while (true);

    //--- BODY ---

    cout << "Starting. Press '2' to pause.\n\n";
    system("timeout 1 > nul");

    using namespace chrono;
    char c = 0;
    short linenumber = 0;
    while (c != 13)
    {
        if (linenumber > 30) {
            system("cls");
            linenumber = 0;
        }
        c = 0;
        if (_kbhit()) c = _getch();
        if (c == '2') {
            cout << "Paused! Press 'r' to reload config or any other key to continue.\n";
            switch (_getch()) {
            case 'r':  init(config_PATH, indicators_PATH, switches_PATH, FREQ_HZ, COM_BAUD_RATE, TOTALINDICATORS, TOTALSWITCHES);
            }
            continue;
        }

        high_resolution_clock::time_point t = high_resolution_clock::now();
        //---

        string sent = "{" + to_string(linenumber) + (string)"}";
        cout << "wrt " << sent << " " << Serial.WriteSerialPort(&sent[0]) << "\n";
        cout << "rec {" << Serial.ReadSerialPort(1, "json") << "}\n";

        //updateControls(Serial, indicators_PATH, indicatorsPrevious, switches_PATH, switchesPrevious);

        //---
        int us = duration_cast<microseconds>(high_resolution_clock::now() - t).count();
        if (us < 1000000/FREQ_HZ) this_thread::sleep_for(microseconds(1000000 / FREQ_HZ - us));
        linenumber++;
    }

    endProgram(Serial);
}