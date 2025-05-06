//=====[Libraries]=============================================================
// Include necessary libraries for the Nucleo-F439ZI board and ARM book functions
#include "mbed.h"
#include "arm_book_lib.h"

//=====[Declaration and initialization of public global objects]===============
// Define input pins for sensors and buttons
DigitalIn enterButton(BUTTON1);         // Enter button for potential future use (not relevant to Task 3)
DigitalIn gasDetector(D2);              // Gas detector input pin
DigitalIn overTempDetector(D3);         // Over-temperature detector input pin
DigitalIn aButton(D4);                  // Code entry buttons (not relevant to Task 3)
DigitalIn bButton(D5);
DigitalIn cButton(D6);
DigitalIn dButton(D7);

// Define output pins for LEDs
DigitalOut alarmLed(LED1);              // LED to indicate alarm state
DigitalOut incorrectCodeLed(LED3);      // LED for incorrect code indication (not relevant to Task 3)
DigitalOut systemBlockedLed(LED2);      // LED for lockout indication (not relevant to Task 3)

// UART object for serial communication with PC at 115200 baud
UnbufferedSerial uartUsb(USBTX, USBRX, 115200);  // [Requirement (i), (ii), (iii), (iv)]: Sets up UART for all communication tasks

// Timer to track periodic report interval
Timer reportTimer;  // [Requirement (ii), (iii)]: Used to manage periodic status reports every 5 seconds

//=====[Declaration and initialization of public global variables]=============+
bool alarmState = OFF;                  // Tracks alarm state (relevant for periodic and continuous reporting)
int numberOfIncorrectCodes = 0;         // Tracks incorrect code attempts (not relevant to Task 3)

//=====[Declarations (prototypes) of public functions]=========================
void inputsInit();
void outputsInit();

void alarmActivationUpdate();
void alarmDeactivationUpdate();

void uartTask();                   // [Requirement (i)]: Handles user input to report sensor states
void availableCommands();
void sendStatusReport();          // [Requirement (ii), (iii)]: Sends periodic and continuous status updates
void sendWarningIfNeeded();       // [Requirement (iv)]: Triggers warnings for unsafe conditions

//=====[Main function, the program entry point after power on or reset]========
int main()
{
    inputsInit();                   // Initialize input pins
    outputsInit();                  // Initialize output pins
    reportTimer.start();            // [Requirement (ii), (iii)]: Start timer for periodic status reporting

    while (true) {
        alarmActivationUpdate();    // Update alarm state (affects reporting)
        alarmDeactivationUpdate();  // Handle code entry (not relevant to Task 3)
        uartTask();                 // [Requirement (i)]: Process UART input for sensor state requests

        // [Requirement (ii), (iii)]: Periodically send status report every 5 seconds
        if (reportTimer.read() >= 5.0f) {
            sendStatusReport();     // Send alarm, gas, and temperature statuses
            reportTimer.reset();    // Reset timer for next interval
        }

        sendWarningIfNeeded();      // [Requirement (iv)]: Continuously check and send warnings if needed
        thread_sleep_for(100);      // Reduce CPU usage to prevent busy-waiting
    }
}

//=====[Implementations of public functions]===================================

void inputsInit()
{
    gasDetector.mode(PullDown);     // Set gas detector pin to pull-down mode for stable input
    overTempDetector.mode(PullDown); // Set temperature detector pin to pull-down mode for stable input
    aButton.mode(PullDown);         // Set code buttons to pull-down (not relevant to Task 3)
    bButton.mode(PullDown);
    cButton.mode(PullDown);
    dButton.mode(PullDown);
}

void outputsInit()
{
    alarmLed = OFF;                 // Initialize alarm LED to OFF
    incorrectCodeLed = OFF;         // Initialize incorrect code LED to OFF (not relevant to Task 3)
    systemBlockedLed = OFF;         // Initialize lockout LED to OFF (not relevant to Task 3)
}

void alarmActivationUpdate()
{
    if (gasDetector || overTempDetector) {  // Check if gas or temperature sensor is triggered
        alarmState = ON;                    // Set alarm state to ON (affects periodic/continuous reporting)
    }
    alarmLed = alarmState;                  // Reflect alarm state on LED (visual indicator)
}

void alarmDeactivationUpdate()
{
    if (numberOfIncorrectCodes < 5) {
        if (aButton && bButton && cButton && dButton && !enterButton) {
            incorrectCodeLed = OFF;         // Reset incorrect code LED (not relevant to Task 3)
        }
        if (enterButton && !incorrectCodeLed && alarmState) {
            if (aButton && bButton && !cButton && !dButton) {
                alarmState = OFF;           // Deactivate alarm (affects reporting)
                numberOfIncorrectCodes = 0; // Reset counter (not relevant to Task 3)
            } else {
                incorrectCodeLed = ON;       // Indicate incorrect code (not relevant to Task 3)
                numberOfIncorrectCodes++;    // Increment counter (not relevant to Task 3)
            }
        }
    } else {
        systemBlockedLed = ON;              // Indicate lockout (not relevant to Task 3)
    }
}

// [Requirement (i)]: Handles user input to report sensor states via UART
void uartTask()
{
    char receivedChar = '\0';  // Variable to store the incoming character from the PC

    if (uartUsb.readable()) {  // Check if there is data to read from the USB serial port
        uartUsb.read(&receivedChar, 1);  // Read one character from the serial port

        switch (receivedChar) {
            case '1':  // Optional command for alarm state (not in requirements)
                uartUsb.write(alarmState ? "The alarm is activated\r\n" : "The alarm is not activated\r\n",
                              alarmState ? 24 : 28);
                break;
            case '2':  // [Requirement (i)]: Report gas detector state when '2' is pressed
                uartUsb.write(gasDetector ? "Gas detected!\r\n" : "No gas detected\r\n",
                              gasDetector ? 15 : 18);  // Send gas state to PC
                break;
            case '3':  // [Requirement (i)]: Report temperature detector state when '3' is pressed
                uartUsb.write(overTempDetector ? "Over temperature detected!\r\n" : "Temperature normal\r\n",
                              overTempDetector ? 28 : 20);  // Send temperature state to PC
                break;
            default:
                availableCommands();  // Display available commands if invalid key is pressed
                break;
        }
    }
}

// Prints list of available UART commands
void availableCommands()
{
    uartUsb.write("Available commands:\r\n", 21);
    uartUsb.write("Press '1' to get the alarm state\r\n", 33);
    uartUsb.write("Press '2' to check gas status\r\n", 32);
    uartUsb.write("Press '3' to check temperature status\r\n\r\n", 41);
}

// [Requirement (ii), (iii)]: Sends periodic and continuous status updates every 5 seconds
void sendStatusReport()
{
    char buffer[128];
    int len = snprintf(buffer, sizeof(buffer),
                       "\r\n[STATUS REPORT]\r\nAlarm: %s\r\nGas: %s\r\nTemperature: %s\r\n\r\n",
                       alarmState ? "ON" : "OFF",          // Current alarm state
                       gasDetector ? "Detected" : "Normal", // Gas detection status
                       overTempDetector ? "High" : "Normal"); // Temperature status
    uartUsb.write(buffer, len);  // Send the formatted report to the PC
}

// [Requirement (iv)]: Continuously sends warning messages if dangerous conditions are detected
void sendWarningIfNeeded()
{
    if (gasDetector) {  // Check if gas detector indicates unsafe levels
        uartUsb.write("[WARNING] Gas levels unsafe!\r\n", 29);  // Send warning to PC
    }
    if (overTempDetector) {  // Check if temperature detector indicates unsafe levels
        uartUsb.write("[WARNING] Temperature too high!\r\n", 33);  // Send warning to PC
    }
}