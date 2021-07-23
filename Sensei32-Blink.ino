/* 
  SENSEI32 - Blink
  
  ####################################################################################################################################
  This software, the ideas and concepts is Copyright (c) Davide Raggini & Davide Samori 2021. All rights to this software are reserved.

  Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
  1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
  2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author Davide Raggini & Davide Samori as the source of the material.
  3. You may not, except with our express written permission, distribute or commercially exploit the content.
  4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

  The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
  software use is visible to an end-user.

  THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY
  OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  Contact us:   davide.raggini@gmail.com
                davide.samori@gmail.com
*/

/*******************************************************************************************
 *
 * Dependencies
 *
 *******************************************************************************************/

/*******************************************************************************************
 *
 * Private Definitions
 *
 *******************************************************************************************/
 
// Define to ignore the battery voltage - DEBUG ONLY
//#define FAKE_BATTERY 

// Define the amount delay between LED ON and OFF time [ms]
#define DELAY_ms  1000

/*******************************************************************************************
 *
 * Global Variables
 *
 *******************************************************************************************/

/*******************************************************************************************
 *
 * Private Variables
 *
 *******************************************************************************************/

// Sensei32 Board Led
static const uint8_t BOARD_LED        = 2; 

// Sensei32 Analog Input for Li-Ion Battery Voltage
static const uint8_t ANALOG_VBAT      = 35;

// Sensei32 Analog Input for VBUS and InCharge Status Read
static const uint8_t ANALOG_VBUS      = 36;
static const uint8_t ANALOG_INCHARGE  = 39;

// Sensei32 battery voltage and battery safety margin
float BATT_voltage;
float BATT_MinimumVoltage               = 2.90;

// Miscellanea and strings/variable to present time and date
bool  VBUS_Status = false, InCharge_Status = false;

/*******************************************************************************************
 *
 * RTC Stored Variables (retained in deep-sleep)
 *
 *******************************************************************************************/

/*******************************************************************************************
 *
 * Arduino Setup
 *
 ******************************************************************************************/

/**
  * @brief	Arduino Environment Setup Callback
  *
  * @param  None
  * @retval None
  */
void setup() {
 
  // Slowdown the cpu
  setCpuFrequencyMhz(80); //Set CPU clock to 80MHz to low the power consumption
  getCpuFrequencyMhz();   //Get CPU clock
  
  // Setup stuff
  Serial.begin(115200);
  while(!Serial);

  // Init BOARD LED
  pinMode(BOARD_LED, OUTPUT);
  
}

/*******************************************************************************************
 *
 * Arduino Loop
 *
 ******************************************************************************************/

/**
  * @brief	Arduino Environment Loop Callback
  *
  * @param  None
  * @retval None
  */
void loop() {

  // New Line, pretty serial output
  Serial.println("");
  
  // Read battery voltage
  ReadBatteryOCV();
  // Read Sensei32 status
  ReadVBUS();
  ReadInCharge();
  
  // Execute only if there is enaugh voltage (this should preserve battery if not used for long time)
  // > Exception, if USB VBUS is detected, execute even if the battery is under UVLO.
  if( (BATT_voltage > BATT_MinimumVoltage) | (VBUS_Status == true) ){

    // Blink LED, ON state
    digitalWrite(BOARD_LED, HIGH);
    Serial.println("[" + String(millis()) + "] LED ON");
    delay(DELAY_ms);

    // Blink LED, OFF state
    digitalWrite(BOARD_LED, LOW);
    Serial.println("[" + String(millis()) + "] LED OFF");
    delay(DELAY_ms);
        
  } else {
    
    // Must go to deepsleep without a timeout to preserve battery
    // > When USB is plugged back in, execution will be resumed
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_36,1); //1 = High, Hardware pull-down on VBUS
    esp_deep_sleep_start();
     
  }
  
}

/*******************************************************************************************
 *
 * Private Functions
 *
 *******************************************************************************************/

/**
  * @brief	Function to get the Li-Ion battery voltage
  *
  * @param  None
  * @retval None
  */
void ReadBatteryOCV() {
  #ifdef FAKE_BATTERY
    BATT_voltage = 3.6f;
  #else
    // Battery should be read as first thing (so it will be similar to OCV value...)
    BATT_voltage = analogRead(ANALOG_VBAT) / 4096.0 * 6.77395927613 * 0.8843421307;  //(1/0.48715970461) * 3.3 - R1= 470K, R2=(680K//1.3M_leak)
                                                                                     //fine_tuning offset was added: 0.8843421307
  #endif
  Serial.println("[Sensei32 - Core] BATT Voltage = " + String(BATT_voltage));
}

/**
  * @brief	Function to get USB BUS Voltage
  *
  * @param  None
  * @retval None
  */
void ReadVBUS() {
  float VBUS_voltage = analogRead(ANALOG_VBUS) / 4096.0 * 3.3 * 2.076923077;  // R1 = 100K, R2 = (100K//1.3M_leak)
  Serial.println("[Sensei32 - Core] VBUS Voltage = " + String(VBUS_voltage));
  // On Sensei32 VBUS is read with a 1/2 divider from USB VBUS
  // Digital status is computed by evaluating the pin voltage
  if( VBUS_voltage >= 3.3 ){
    VBUS_Status = true;
  } else {
    VBUS_Status = false;
  }
  Serial.println("[Sensei32 - Core] VBUS State Digital State = " + String(VBUS_Status) );
}

/**
  * @brief	Function to get the charge status of the Li-Ion Battery
  *
  * @param  None
  * @retval None
  */
void ReadInCharge() {
  float InCharge_voltage = analogRead(ANALOG_INCHARGE) / 4096.0 * 3.3 * 2.098461538;  // R1 = 102K, R2 = (100K//1.3M_leak)
  Serial.println("[Sensei32 - Core] InCharge Voltage = " + String(InCharge_voltage));
  // On Sensei32 InCharge_Status is read with a 1/2 divider from LiIon Charge Pin
  // Digital status is computed by evaluating the pin voltage, with inverted logic
  if( InCharge_voltage >= 3.3 ){
    InCharge_Status = false;
  } else {
    InCharge_Status = true;
  }
  Serial.println("[Sensei32 - Core] InCharge Digital State = " + String(InCharge_Status) );
}
