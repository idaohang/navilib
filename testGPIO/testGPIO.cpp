/// Test GPIO liines of car unit

#include <string>
#include <iostream>

#include "CGpio.h"
using namespace std;

// Construct object
static CGpio GPIO(1 /* poll time (sec)*/);

// print GIO line states
static void print_gio()
{
   cout << "Relay = " << GPIO.getRelay() << endl;
   cout << "IN1 = " << GPIO.getINline(1) << endl;
   cout << "IN2 = " << GPIO.getINline(2) << endl;
   cout << "OUT1 = " << GPIO.getOUTline(1) << endl;
   cout << "OUT2 = " << GPIO.getOUTline(2) << endl;
}

// --------------------------------------------
// Main program
int main(int argc, char* argv[])
{
    char input[256];
    
    // Init object
    GPIO.Init();
    
    cout << "Press <enter> to close RELAY"; cin.getline(input, sizeof(input));
    GPIO.setRelayClosed(true);
    print_gio();

    cout << "Press <enter> to open RELAY"; cin.getline(input, sizeof(input));
    GPIO.setRelayClosed(false);
    print_gio();

    cout << "Press <enter> to turn on LED (gpio 147)"; cin.getline(input, sizeof(input));
    GPIO.setLedLight(true);
    print_gio();

    cout << "Press <enter> to turn off LED (gpio 147)"; cin.getline(input, sizeof(input));
    GPIO.setLedLight(false);
    print_gio();

    cout << "Press <enter> to switch GSM on (gpio 23)"; cin.getline(input, sizeof(input));
    GPIO.setGSM(true);
    print_gio();

    cout << "Press <enter> to switch GSM off (gpio 23)"; cin.getline(input, sizeof(input));
    GPIO.setGSM(false);
    print_gio();

    cout << "Press <enter> to switch ON OUT1"; cin.getline(input, sizeof(input));
    GPIO.setOUTline(1, true);
    print_gio();

    cout << "Press <enter> to switch OFF OUT1"; cin.getline(input, sizeof(input));
    GPIO.setOUTline(1, false);
    print_gio();

    cout << "Press <enter> to switch ON OUT2"; cin.getline(input, sizeof(input));
    GPIO.setOUTline(2, true);
    print_gio();

    cout << "Press <enter> to switch OFF OUT2"; cin.getline(input, sizeof(input));
    GPIO.setOUTline(2, false);
    print_gio();
}
