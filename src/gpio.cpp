#include <SPIFFS.h>
#include "gpio.h"
#include "MQTTServices.h"
#include "FileSystem.h"
#include "ServoEasing.hpp"
#include "sound.h"


// GPIO control Class definitions and helper functions
// Creates an array of objects (gpio[]) to manage control of the 16 GPIO bits
// Local Applications should write to Outputs using 'localWrite()'
// Remote (ie MQTT) Interface should use 'remoteWrite()'
// Global flags 'enableLocal' and 'enableRemote' allow control of each output to be disabled
//
// User application must
//  * call 'setupGPIO();        early in the powerup sequence (before GPIO assignments or configuration is loaded)
//  * use gpio[n].setType(x)   to define the IO type for any used GPIO (see list of defined types in gpio.h)
//  * use gpio[n].localWrite()  to control an output
//  * use gpio[n].localRead()   to read any GPIO
//  * use gpio[n].read()        to read the value of any GPIO
//
//  Normally the following configuration would be made via the web interface (see WiFI.ino)
//  * Presets 0,1,2
//  * local / remote enable
//

// Create Queue handles
QueueHandle_t servoQueue;

// Task handles for GPIO helper tasks
TaskHandle_t gpioTask0;
TaskHandle_t gpioTask1;
TaskHandle_t gpioTask2;
TaskHandle_t gpioTask3;
TaskHandle_t gpioTask4;
TaskHandle_t gpioTask5;
TaskHandle_t gpioTask6;
TaskHandle_t gpioTask7;
TaskHandle_t gpioTask8;
TaskHandle_t gpioTask9;
TaskHandle_t gpioTask10;
TaskHandle_t gpioTask11;
TaskHandle_t gpioTask12;
TaskHandle_t gpioTask13;
TaskHandle_t gpioTask14;
TaskHandle_t gpioTask15;
TaskHandle_t servoSaverTask;
gpioPin gpio[16];                // Create an array of GPIO Classes which can be indexed [0] to [15] to control GPIO


void  setupGPIO()
{
  // Call this early at startup -before loading the I/O configuration,  to instantiate objects and initialise the control structures
  #ifdef debugGPIO
  Serial.println("setupGPIO");
  #endif



  // Map the I/O pins to match the physical PCB layout
  gpio[0].pin = GPIO_BIT00;
  gpio[0].bitNo = 0;
  gpio[1].pin = GPIO_BIT01;
  gpio[1].bitNo = 1;
  gpio[2].pin = GPIO_BIT02;
  gpio[2].bitNo = 2;
  gpio[3].pin = GPIO_BIT03;
  gpio[3].bitNo = 3;
  gpio[4].pin = GPIO_BIT04;
  gpio[4].bitNo = 4;
  gpio[5].pin = GPIO_BIT05;
  gpio[5].bitNo = 5;
  gpio[6].pin = GPIO_BIT06;
  gpio[6].bitNo = 6;
  gpio[7].pin = GPIO_BIT07;
  gpio[7].bitNo = 7;
  gpio[8].pin = GPIO_BIT08;
  gpio[8].bitNo = 8;
  gpio[9].pin = GPIO_BIT09;
  gpio[9].bitNo = 9;
  gpio[10].pin = GPIO_BIT0A;
  gpio[10].bitNo = 10;
  gpio[11].pin = GPIO_BIT0B;
  gpio[11].bitNo = 11;
  gpio[12].pin = GPIO_BIT0C;
  gpio[12].bitNo = 12;
  gpio[13].pin = GPIO_BIT0D;
  gpio[13].bitNo = 13;
  gpio[14].pin = GPIO_BIT0E;
  gpio[14].bitNo = 14;
  gpio[15].pin = GPIO_BIT0F;
  gpio[15].bitNo = 15;

  // Read the last saved gpio configuration
  for(int bit=0; bit<16; bit++)
  {
    readConfigFile(SPIFFS,bit);
  }
  pinMode(RUN1,INPUT_PULLUP);
  pinMode(RUN2,INPUT_PULLUP);
  pinMode(POWER_GPIO,OUTPUT);
  analogReadResolution(12);       // set the resolution to 12 bits (0-4096)

    // Create servo queue
  servoQueue = xQueueCreate(50, sizeof(servoData));
  if (servoQueue == NULL) Serial.println("Error creating the servo queue");
  else xTaskCreatePinnedToCore(servoSaver, "ServoSaver", 3000, NULL, 1, &servoSaverTask, 0); // Core 0

  // Create helper tasks for each GPIO pin
  xTaskCreatePinnedToCore(helper0, "GPIO0", 2000, NULL, 1, &gpioTask0, 0); // Core 0
  xTaskCreatePinnedToCore(helper1, "GPIO1", 2000, NULL, 1, &gpioTask1, 0); // Core 0
  xTaskCreatePinnedToCore(helper2, "GPIO2", 2000, NULL, 1, &gpioTask2, 0); // Core 0
  xTaskCreatePinnedToCore(helper3, "GPIO3", 2000, NULL, 1, &gpioTask3, 0); // Core 0
  xTaskCreatePinnedToCore(helper4, "GPIO4", 2000, NULL, 1, &gpioTask4, 0); // Core 0
  xTaskCreatePinnedToCore(helper5, "GPIO5", 2000, NULL, 1, &gpioTask5, 0); // Core 0
  xTaskCreatePinnedToCore(helper6, "GPIO6", 2000, NULL, 1, &gpioTask6, 0); // Core 0
  xTaskCreatePinnedToCore(helper7, "GPIO7", 2000, NULL, 1, &gpioTask7, 0); // Core 0
  xTaskCreatePinnedToCore(helper8, "GPIO8", 2000, NULL, 1, &gpioTask8, 0); // Core 0
  xTaskCreatePinnedToCore(helper9, "GPIO9", 2000, NULL, 1, &gpioTask9, 0); // Core 0
  xTaskCreatePinnedToCore(helper10, "GPIOA", 2000, NULL, 1, &gpioTask10, 0); // Core 0
  xTaskCreatePinnedToCore(helper11, "GPIOB", 2000, NULL, 1, &gpioTask11, 0); // Core 0
  xTaskCreatePinnedToCore(helper12, "GPIOC", 2000, NULL, 1, &gpioTask12, 0); // Core 0
  xTaskCreatePinnedToCore(helper13, "GPIOD", 2000, NULL, 1, &gpioTask13, 0); // Core 0
  xTaskCreatePinnedToCore(helper14, "GPIOE", 2000, NULL, 1, &gpioTask14, 0); // Core 0
  xTaskCreatePinnedToCore(helper15, "GPIOF", 2000, NULL, 1, &gpioTask15, 0); // Core 0

  Serial.println("done setupGPIO");
}

void  powerGPIO(bool powerUp)
{
  // Powers up electrical feed to thw switch 0V line (powers servos etc)
  Serial.println("powerup GPIO ");
pinMode(POWER_GPIO,OUTPUT);
digitalWrite(POWER_GPIO, powerUp);
}

bool  GPIOState()
{
  // Returns true if th GPIO system is running (pwered up)
  return digitalRead(POWER_GPIO);
}

bool run1Switch()
{
  // Returns true if the RUN1 switch is closed (connected to GND)
  return digitalRead(RUN1) == LOW;
}

bool run2Switch()
{
  // Returns true if the RUN2 switch is closed (connected to GND)
  return digitalRead(RUN2) == LOW;
}

void loadServoPositions()
{
  Serial.println("Loading servo positions from SPIFFS");
  // Load the last saved servo positions from the SPIFFS file system
  for(int bit=0; bit<16; bit++)
  {
    int position = readServoPosition(SPIFFS, bit);
    Serial.print("Servo bit:");
    Serial.print(bit);  
    Serial.print(" Position:");
    Serial.println(position);
    if((position >= 0) && (position <= 180) && ((gpio[bit].type == GPIO_SERVO)||(gpio[bit].type == GPIO_SERVO_ACTUATOR)))
    {
      Serial.printf("Loaded position for servo %d: %d\n", bit, position);
      gpio[bit].initValue(position);
    }
  }
}

void helper0(void * pvParameters)
{
  // helper task for GPIO 0 servo control
  gpio[0].helper();
}

void helper1(void * pvParameters)
{
  // helper task for GPIO 1 servo control
  gpio[1].helper();
}

void helper2(void * pvParameters)
{
  // helper task for GPIO 2 servo control
  gpio[2].helper();
}

void helper3(void * pvParameters)
{
  // helper task for GPIO 3 servo control
  gpio[3].helper();
}

void helper4(void * pvParameters)
{
  // helper task for GPIO 4 servo control
  gpio[4].helper();
}

void helper5(void * pvParameters)
{
  // helper task for GPIO 5 servo control
  gpio[5].helper();
}

void helper6(void * pvParameters)
{
  // helper task for GPIO 6 servo control
  gpio[6].helper();
}

void helper7(void * pvParameters)
{
  // helper task for GPIO 7 servo control
  gpio[7].helper();
}

void helper8(void * pvParameters)
{
  // helper task for GPIO 8 servo control
  gpio[8].helper();
}

void helper9(void * pvParameters)
{
  // helper task for GPIO 9 servo control
  gpio[9].helper();
}

void helper10(void * pvParameters)
{
  // helper task for GPIO 10 servo control
  gpio[10].helper();
}

void helper11(void * pvParameters)
{
  // helper task for GPIO 11 servo control
  gpio[11].helper();
}

void helper12(void * pvParameters)
{
  // helper task for GPIO 12 servo control
  gpio[12].helper();
}

void helper13(void * pvParameters)
{
  // helper task for GPIO 13 servo control
  gpio[13].helper();
}

void helper14(void * pvParameters)
{
  // helper task for GPIO 14 servo control
  gpio[14].helper();
}

void helper15(void * pvParameters)
{
  // helper task for GPIO 15 servo control
  gpio[15].helper();
}

gpioPin::gpioPin()
{
  #ifdef debugGPIO
  Serial.println("(gpioPin)");
  #endif
  strcpy(name,"-");
  pin = 0;
  bitNo = 0;
  type = GPIO_NONE;
  preset0 = 0;
  preset1 = 1;
  preset2 = 2;
  rate = 1000;
  easingType = 0;
  enableRemote = true;
  enableLocal = true;
  value = 0;
  target = 0;
  publishRate = 0;
  nextPublish = 0;
  lock = xSemaphoreCreateMutex();
}

void gpioPin::setType(uint8_t newType)
{

  Serial.print("(setType) Pin:");
  Serial.print(pin);
  Serial.print(" Bit:");
  Serial.print(bitNo);
  Serial.print(" NewType:");
  Serial.println(newType);

  xSemaphoreTake(lock,portMAX_DELAY);
  type = newType;
  if(type == GPIO_DIGOUT)pinMode(pin,OUTPUT);
  if(type == GPIO_DIGOUT_PULSE)pinMode(pin,OUTPUT);
  if(type == GPIO_DIGIN)pinMode(pin,INPUT_PULLUP);
  if(type == GPIO_AIN)pinMode(pin,INPUT);
  if((type == GPIO_SERVO)||(type == GPIO_SERVO_ACTUATOR))
  {
    servo = new ServoEasing;
    servo->attach(pin,preset0);           // preset0 is the home position of the servo
//    setEasingType(EASE_LINEAR);
  }
  vTaskDelay(10 / portTICK_PERIOD_MS);          // in case we get called in a tight loop
  value = target;
// Publish
  xSemaphoreGive(lock);
}

void gpioPin::initValue(int initValue)
{
  // This is called after the GPIO type has been set to initialise the value of the GPIO (eg to set the initial position of a servo)
  xSemaphoreTake(lock,portMAX_DELAY);
  if((type == GPIO_SERVO)||(type == GPIO_SERVO_ACTUATOR))
  {
    target = initValue;
    servo->write(initValue);
    value = initValue;
  }
  xSemaphoreGive(lock);
}

int gpioPin::getValue()
{
  return(value);
}

void gpioPin::alwaysWrite(int demand)
{
  {
    Serial.println("alwaysWrite");
    xSemaphoreTake(lock,portMAX_DELAY);
    Serial.println("gotSemaphore");
    write(demand);
    Serial.println("writted");
    xSemaphoreGive(lock);
    Serial.println("done");
  }
}

void gpioPin::localWrite(int demand)
{
  if(enableLocal == true)
  {
    xSemaphoreTake(lock,portMAX_DELAY);
    write(demand);
    xSemaphoreGive(lock);
  }
}

void gpioPin::remoteWrite(int demand)
{
  if(enableRemote == true)
  {
    xSemaphoreTake(lock,portMAX_DELAY);
    write(demand);
    xSemaphoreGive(lock);
  }
}

void gpioPin::write(int demand)
{
  //#ifdef debugGPIO
  Serial.print("(GPIO write) pin:");
  Serial.print(pin);
  Serial.print(" type:");
  Serial.print(type);  
  Serial.print(" demand:");
  Serial.print(demand);
  Serial.print(" value:");
  Serial.println(value);
  //#endif
  if(demand != value)
  {
    if((type == GPIO_DIGOUT)||(type == GPIO_DIGOUT_PULSE))
    {
      Serial.println(demand);
      digitalWrite(pin,demand);
      value = demand;
      publish(bitNo,value);
    }
    else if((type == GPIO_PWM) || (type == GPIO_PWM_PULSE))
    {
      value = demand;
      if(value == 0)analogWrite(pin,preset1);
      else analogWrite(pin,preset2);
      Serial.print("preset1:");
      Serial.println(preset1);
      Serial.print("preset2:");
      Serial.println(preset2);
      publish(bitNo,value);
    }
    else if((type == GPIO_SERVO)||(type == GPIO_SERVO_ACTUATOR))
    {
      target = demand;
    }
    else 
    {
      value = demand;
      publish(bitNo,value);
    }
  }     
}

int  gpioPin::localRead()
{
  if(enableLocal == true)
  {
    xSemaphoreTake(lock,portMAX_DELAY);
    int val = read();
    xSemaphoreGive(lock);
    return(val);
  }
  return(-1);
}

int  gpioPin::remoteRead()
{
  if(enableRemote == true)
  {
    xSemaphoreTake(lock,portMAX_DELAY);
    int val = read();
    xSemaphoreGive(lock);
    return(val);
  }
  return(-1);
}

int  gpioPin::read()
{
  #ifdef debugGPIO
//  Serial.print("(GPIO read) pin:");
//  Serial.print(pin);
//  Serial.print(" type:");
//  Serial.print(type);  
  #endif

  int val;
  val = value;
  switch(type)
  {
    case GPIO_DIGIN:
      val = digitalRead(pin);
      if(val != value)
      {
        // digital input has changed
        value = val;
        publish(bitNo,value);
      }
      break;
    case GPIO_AIN:
      val = analogRead(pin);
      if(val != value)
      {
        // analog input has changed
        // Apply deadband (preset1)
        int diff = value - val;
        diff = abs(diff);
        if(diff > preset1)
        {
          value = val;
          publish(bitNo,value);
        }
      }
      break;
    case  GPIO_SERVO:
    case  GPIO_SERVO_ACTUATOR:
      val = servo->read();
      break;
  }
  value = val;
  #ifdef debugGPIO
  //Serial.print(" value:");
  //Serial.println(value);
  #endif
  return(value);
}

void  gpioPin::setPublishRate(int publishRate_mS)
{
  // background periodic publish via MQTT (even if GPIO has not changed)
  xSemaphoreTake(lock,portMAX_DELAY);
  publishRate = publishRate_mS;
  nextPublish = millis();
  xSemaphoreGive(lock);
} 

int  gpioPin::getPublishRate()
{
  // background periodic publish via MQTT (even if GPIO has not changed)
  return(publishRate);
} 

void  gpioPin::setEasingType(uint_fast8_t aEasingType)
{
  xSemaphoreTake(lock,portMAX_DELAY);
  if((type == GPIO_SERVO)||(type == GPIO_SERVO_ACTUATOR))
  {
    easingType = aEasingType;
    servo->setEasingType(easingType);
  }
  xSemaphoreGive(lock);
}

int  gpioPin::getEasingType()
{
  if((type == GPIO_SERVO)||(type == GPIO_SERVO_ACTUATOR))return(servo->getEasingType());
  else return(0);
}

void gpioPin::helper()
{
  while(true)
  {
    if((type == GPIO_SERVO)||(type == GPIO_SERVO_ACTUATOR))servoController();
    if((type == GPIO_AIN) || (type == GPIO_DIGIN))inputPoller();
    if((type == GPIO_DIGOUT_PULSE) || (type == GPIO_PWM_PULSE))outputPulser();
    if((type == GPIO_DIGOUT)||(type == GPIO_PWM)||(type == GPIO_NONE))outputHelper();
    vTaskDelay(10);
  }
}

void gpioPin::servoController()
{
  if((type == GPIO_SERVO)||(type == GPIO_SERVO_ACTUATOR))
  {
    while(value != target)
    {
      servo->easeTo(target, rate);
      value = read();
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    queServoPosition(bitNo, value, SERVO_SETTLING_DELAY);         // queue the current servo position for saving to SPIFFS after a delay to allow the servo to settle
    publish(bitNo,value);                                     //  publish (MQTT) the final position after the easeTo has completed
    while(value == target)
    {
      if(publishRate != 0)
      {
        if(millis()>nextPublish)
        {
          nextPublish = millis()+publishRate;
          publish(bitNo,value);
        }
      }
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}


void gpioPin::inputPoller()
{
  // Input helper function called by the gpioTask assigned to this pin
  // Polls a GPIO_DIGIN or GPIO_AIN
  // rate = polling rate (mS)
  // publishRate = background polling rate which will report value even if it hasnt changed 
  if(rate == 0)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);                // Rate set to 0 so nothing to do
    return;
  }
  if((type == GPIO_DIGIN)||(type == GPIO_AIN))
  {
    remoteRead();            // check and publish if value has changed
    if(publishRate == 0)return;
    // Background publishing rate will publish value even if it has not changed
    if(millis() > nextPublish)
    {
      Serial.println("Background publish");
      nextPublish = millis()+publishRate;
      int val = remoteRead();
      publish(bitNo,value);
    }
  }
  vTaskDelay(rate / portTICK_PERIOD_MS);
}

void gpioPin::outputHelper()
{
  // Digital Output helper function (applies to GPIO_DIGOUT, GPIO_PWM only and GPIO_NONE)
  // Check if background publishing has been requested
  if(publishRate != 0)
  {
    if(millis()>nextPublish)
    {
      nextPublish = millis()+publishRate;
      publish(bitNo,value);
    }
  }
}

void gpioPin::outputPulser()
{
  // Digital Output helper function which operates pulsed Outputs
  // rate = overall pulse cycle in mS
  // preset0 = Mark duration (<= rate)
  // preset1 = Mark PWM rate (applies to GPIO_PWM_PULSE)
  // preset2 = Space PWM rate (applies to GPIO_PWM_PULSE)
  // turn on/off using Local/Remote Wrrite functions
  if(rate == 0)
  {
    if(value > 0)
    {
      if(type == GPIO_DIGOUT_PULSE)digitalWrite(pin,false);
      if(type == GPIO_PWM_PULSE)analogWrite(pin,0);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);                // Rate set to 0 so nothing to do
  }
  else
  {
    if(value == true)
    {
      if(type == GPIO_DIGOUT_PULSE)
      {
        if(rate < preset0)return;                           // Mark period is greater than the total period
        digitalWrite(pin,true);
        vTaskDelay(preset0 / portTICK_PERIOD_MS);           // Mark Period
        digitalWrite(pin,false);
        vTaskDelay((rate-preset0) / portTICK_PERIOD_MS);    // Space Period
      }
      else if(type == GPIO_PWM_PULSE)
      {
        if(rate < preset0)return;                           // Mark period is greater than the total period
        analogWrite(pin,preset1);                           // Mark PWM setting  (0 to 255)
        vTaskDelay(preset0 / portTICK_PERIOD_MS);           // Mark Period
        analogWrite(pin,preset2);                           // Space PWM setting (0 to 255)
        vTaskDelay((rate-preset0) / portTICK_PERIOD_MS);    // Space Period
      }
      else vTaskDelay(1000 / portTICK_PERIOD_MS);          // nothing to do here
    }
    else
    {
      // demand is false, turn off flashing
      if(type == GPIO_DIGOUT_PULSE)digitalWrite(pin,false);
      if(type == GPIO_PWM_PULSE)analogWrite(pin,0);
      vTaskDelay(1000);
    }
  }
  // Check if background publishing has been requested
  if(publishRate != 0)
  {
    if(millis()>nextPublish)
    {
      nextPublish = millis()+publishRate;
      publish(bitNo,value);
    }
  }
}

void queServoPosition(uint8_t bitNo, int position, unsigned int delay)
{
  // This function is called by the gpioPin helper task when a servo position is updated
  // It queues the new position for the servo saver task to save to SPIFFS
  servoData data;
  data.bitNo = bitNo;
  data.position = position;
  data.timestamp = millis() + delay;   // add a delay to allow the servo to settle before saving the position
  if(xQueueSend(servoQueue, &data, 0) != pdPASS)
  {
    Serial.println("Error queuing servo position");
  }
}

void servoSaver(void * pvParameters)
{
  // This helper task periodically saves the current servo position to the SPIFFS file system
  // This allows the position to be restored at powerup and also allows the web interface to display the current position
  servoData receivedServo;
  while(true)
  {
    // Wait indefinitely for servo data
    if (xQueueReceive(servoQueue, &receivedServo, portMAX_DELAY) == pdPASS) 
    {
      Serial.printf("[Servo] Received position for servo: %d, position: %d\n", receivedServo.bitNo, receivedServo.position);
      while(millis() < receivedServo.timestamp)
      {
        // Save the servo position to SPIFFS or perform other actions
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      // Save the position to SPIFFS
      if(GPIOState() == true)
      {
        // Check if servo is still at the same position before saving (ie it has not been moved again since we queued the save)
        if(gpio[receivedServo.bitNo].getValue() == receivedServo.position)
        {
          writeServoPosition(SPIFFS, receivedServo.bitNo, receivedServo.position);
          Serial.printf("[Servo] Saved position for servo: %d\n", receivedServo.bitNo);
        }
      }
    }
  }
}

void gpioPin::publish(uint8_t bit, int aValue)
{
  if(GPIOState() == true)   // only publish if power to the GPIOs is on
  {
    Serial.println("Publishing GPIO change");
    MQTTSensor payload;
    payload.bitNo = bit;
    payload.value = aValue;
    MQTTPublishSensor(payload); 
  }
  else Serial.println("GPIO power off - not publishing");
}
