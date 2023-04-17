// USB2CAN_DataFilter_CorrectTimeStamp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fileapi.h>
#include <sys/stat.h>
#include <string.h>
#include <sstream>

#define elementSize 1 //read 1 byte at a time
#define numOfElements   500 // make this equal to size of buf to which it will be written after the read operation
//#define readFileName "C:\\HarmonyProjects_unreleasedInitialU2C\\U2C Aura pausing testing\\"
//#define writeFileName "C:\\HarmonyProjects_unreleasedInitialU2C\\U2C Aura pausing testing\\"

std::string FileDirectory = "C:\\HarmonyProjects_unreleasedInitialU2C\\U2C Aura pausing testing\\";

struct stat file_status;

uint32_t DataStartatIndex(char* buf, uint8_t Instances)
{
    std::string usbData = (char*)buf;
    uint32_t commaDelimiterCntr = 0;
    uint32_t dataStartIndex = 0;

    while (commaDelimiterCntr < Instances)// keep finding the next instance of comma until the record column
    {
        dataStartIndex = (uint32_t)usbData.find(',', dataStartIndex) + 1;
        commaDelimiterCntr++;
    }
    //commaDelimiterCntr = 0; // reset for the next operation

    return dataStartIndex;
}

bool checkTheDatatoCopy(uint8_t* buf)
{
    static int32_t dataDirIN = -1;
    //static 
    static char usbSizeBuf[50] = { 0 };
    static bool leveTracker = false; // saved flag to read the data only

    char CDC_string[2][20] = {"CDC OUT Data", "CDC IN Data"};
    int usbReadSize = 0;
    bool bufCopy = false;
    //bool bufSizeReadDone = false;    
    uint32_t commaDelimiterCntr = 0;
    //uint8_t hexBuf[500] = { 0 };
    //uint32_t hexBufCntr = 0;
    uint32_t dataStartIndex = 0;
    //uint32_t timestampData = 0;    
    std::string usbData = (char*)buf; 
    std::string tempDataStorage;
    bool USBTransactionData = false; // flag to read in/out transactions only

    if (!leveTracker)
    {
        /*if (buf[0] >= '0' && buf[0] < '2') //when not connected to a hub the OUT Txn info is on level1
            leveTracker = true;
        else // avoid looking for the details for the next few rows (unwanted data)
        {
            bufCopy = false;
            leveTracker = false;
            dataDirIN = -1;
            memset(usbSizeBuf, 0, sizeof(usbSizeBuf));
            return bufCopy; 
        }*/

        while (commaDelimiterCntr < 9) // keep finding the next instance of comma until the record column
        {
            dataStartIndex = (uint32_t)usbData.find(',', dataStartIndex) + 1;
            commaDelimiterCntr++;

            if (commaDelimiterCntr == 5)
            {
                snprintf(usbSizeBuf, sizeof(usbSizeBuf),"%d B", std::strtol((char*)buf + dataStartIndex, NULL, 10));
            }
        }

        if (strncmp("IN txn", (char*)(buf + dataStartIndex + 3*(buf[0] - 0x30)), strlen("IN txn")) == 0) //adjust for indent in the record cloumn
        {
            dataDirIN = 1;
        }
        else if (strncmp("OUT txn", (char*)(buf + dataStartIndex + 3 * (buf[0] - 0x30)), strlen("OUT txn")) == 0)
        {
            dataDirIN = 0;
        }
        else
        {
            /*if (buf[0] == '0') // IN/OUT txn info was not in level0, look into next level
                leveTracker = false;*/
            dataDirIN = -1;
        }
        
        usbReadSize = std::strtol(usbSizeBuf, NULL, 10);

        dataStartIndex = DataStartatIndex((char*)buf, 10); // keep finding the next instance of comma until reached the data column
        tempDataStorage = (char*)(buf + dataStartIndex); // skip the usb related preamble byte
        
        // SOF/"31 60" detected, avoid this USB data
        //if ((dataDirIN < 0) || (dataDirIN == 1 && tempDataStorage.find("EF") > 0 && tempDataStorage.find("EF") < numOfElements && usbReadSize < 5))
        if ((dataDirIN < 0) || (dataDirIN == 1 && tempDataStorage.find("BE") > numOfElements && usbReadSize < 3))
        {
            leveTracker = false;
            dataDirIN = -1;
            memset(usbSizeBuf, 0, sizeof(usbSizeBuf));
        }
        else
        {
            leveTracker = true;
        }
        //bufCopy = false;
    }
    else
    {
        dataStartIndex = DataStartatIndex((char*)buf, 9); // keep finding the next instance of comma until reached the record column

        if (strncmp("DATA", (char*)(buf + dataStartIndex + 3 * (buf[0] - 0x30)), strlen("DATA")) == 0) // check the Record column is DATA0/1
        {
            //buf = buf + 5; // reset to index column by +5 for level (0-5), comma, speed ("HS"/"FS"/"LS"), comma
            
            dataStartIndex = DataStartatIndex((char*)buf, 10); // keep finding the next instance of comma until reached the data column
            tempDataStorage = (char*)(buf + dataStartIndex + 3); // skip the usb related preamble byte

            dataStartIndex = DataStartatIndex((char*)buf, 4); // keep finding the next instance of comma until reached the timestamp column

            usbReadSize = std::strtol(usbSizeBuf, NULL, 10);
            
            if (dataDirIN == 1 && tempDataStorage.find("EF") > 0) // older board compatibility which uses FTDI
            {
                usbReadSize -= 2;
                snprintf((char*)(buf + dataStartIndex), numOfElements, "%d B,", usbReadSize); // copy the size data
            }
            else
            {
                snprintf((char*)(buf + dataStartIndex), numOfElements, "%s,", usbSizeBuf); // copy the size data
            }            
            snprintf((char*)(buf + strlen((char*)buf)), strlen((char*)buf) + 1, "%s,", CDC_string[dataDirIN]); // copy the CDC direction
            // copy the data from data column
            
            if (dataDirIN == 1 && tempDataStorage.find("EF") > 0) // older board compatibility which uses FTDI
            {
                snprintf((char*)(buf + strlen((char*)buf)), 3 * usbReadSize, "%s\n", tempDataStorage.c_str() + 6);
            }
            else
            {
                snprintf((char*)(buf + strlen((char*)buf)), 3 * usbReadSize, "%s\n", tempDataStorage.c_str());
            }
            
            dataStartIndex = DataStartatIndex((char*)buf, 2); // keep finding the next instance of comma until reached the timestamp column
            snprintf((char*)buf, strlen((char*)buf), "%s\n", (char*)(buf + dataStartIndex));

            bufCopy = true;
            leveTracker = false;
            dataDirIN = -1;
            memset(usbSizeBuf, 0, sizeof(usbSizeBuf));
        }
    }

    /*for (int i = 0; i < strlen((char*)buf); i++)
    {
        if (bufReadSize == true) // ready to read size column
        {
            if (buf[i] > '0' && buf[i] <= '9') // the size exists if there is USB packet
            {
                bufCopy = true;
            }

            break; // if the above condition satisfied the flag will be set otherwise it will take the default value.
        }
        if (buf[i] == ',')
        {
            commDelimiterCntr++;
            if (commDelimiterCntr == 2)// skip the first 2 columns
            {
                bufReadSize = true;
            }
        }
    }*/



    return bufCopy;
}

int main()
{
    errno_t err, err1=2;
    uint8_t buf[500] = { 0 };
    DWORD fileSize = 0;
    int32_t sizetoCopy_Process = 0;
    std::string inFileName, outFileName;

    FILE* in_file;
    FILE* out_file;
    //FILE* new_out_file;

    std::cout << "\t\t\t Data filter out\n\nEnter read file name with the csv file extension, example \"tip1 move up sniffing.csv\": ";
    std::getline(std::cin, inFileName);

    inFileName = FileDirectory + inFileName; 

    err = fopen_s(&in_file, inFileName.c_str(), "r+");

    while (err != 0) // unable to open the file request for correct file name and keep trying until correct file name
    {
        std::cout << "\nUnable to open" << inFileName;

        std::cout << "\nEnter read file name with the csv file extension, example \"z move up sniffing.csv\": ";
        std::getline(std::cin, inFileName);
        inFileName = FileDirectory + inFileName;

        err = fopen_s(&in_file, inFileName.c_str(), "r+");
    }

    err = 2; //reset the err code for the next file name open

    std::cout << "Enter write file name with the csv file extension, example \"tip1 move up sniffing.csv\": ";
    std::getline(std::cin, outFileName); // get the write filename, filename can have spaces

    outFileName = FileDirectory + outFileName;

    err = fopen_s(&out_file, outFileName.c_str(), "w");

    while (err != 0) // unable to open the file request for correct file name and keep trying until correct file name
    {
        std::cout << "\nUnable to open" << outFileName;

        std::cout << "\nEnter write file name with the csv file extension, example \"z move up sniffing.csv\": ";
        std::getline(std::cin, outFileName);
        outFileName = FileDirectory + outFileName;

        err = fopen_s(&out_file, outFileName.c_str(), "w");
    }

    err = 2; //reset the err code for the next file name operation

    err = stat(inFileName.c_str(), &file_status);

    if (err != 0)
    {
        std::cout << "Unable to read the file size!\n";

        return 0;
    }

    sizetoCopy_Process = file_status.st_size;

    if (err == 0)
    {
        if (freopen_s(&out_file, outFileName.c_str(), "w", out_file) != 0) // reopen the file to clear all previous entries
        {
            std::cout << "\nUnable to reopen the" << outFileName;

            return 0;
        }
        while (sizetoCopy_Process)
        {
            if (fgets((char*)buf, sizeof(buf), in_file) != NULL)
            {
                if (checkTheDatatoCopy(buf)) // check whether the data is non-USB handshaking data
                {
                    fprintf_s(out_file, "%s", buf); // if a valid data copy/write it to the new file
                }

                sizetoCopy_Process -= (uint32_t)strlen((char*)buf);
            }
            else
            {
                break;
            }
        }

        std::cout << "the USB data was filtered successfully in to " << outFileName << "\n";
    }

    _fcloseall();
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
