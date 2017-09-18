#include <ControlChain.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <Bounce2.h>
#include <Encoder.h>
#include <SPI.h>
#include <Wire.h>

#define ENC_MIN -200.0
#define ENC_MAX 200.0

ControlChain cc;
U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, 13, 11, 10, 9);

float valFSW1, valFSW2, valFSW3;
float valEncButton1, valEncButton2;
float valEncA, valEncB;

// int switchFlag = 0;

Bounce debounceFSW1 = Bounce();
Bounce debounceFSW2 = Bounce();
Bounce debounceFSW3 = Bounce();
Bounce debounceEncA = Bounce();
Bounce debounceEncB = Bounce();
unsigned long debounceDelay = 1;    // the debounce time; increase if the output flickers

Encoder encoderA(3, 4);
Encoder encoderB(5, 6);
float valueA, valueB;

int FSW1 = A1, FSW2 = A2, FSW3 = A3;
int LED1 = 7, LED2 = 8, LED3 = 12;
int EncSW1 = A5, EncSW2 = A4;
int RotEnc1A = 4, RotEnc1B = 3;
int RotEnc2A = 6, RotEnc2B = 5;

void setup() {

  //#################  define pins and activate pullup-resistors   #####################
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED2, LOW);

  pinMode(FSW1, INPUT_PULLUP);
  pinMode(FSW2, INPUT_PULLUP);
  pinMode(FSW3, INPUT_PULLUP);

  pinMode(EncSW1, INPUT_PULLUP);
  pinMode(EncSW2, INPUT_PULLUP);

  pinMode(RotEnc1A, INPUT_PULLUP);
  pinMode(RotEnc1B, INPUT_PULLUP);
  pinMode(RotEnc2A, INPUT_PULLUP);
  pinMode(RotEnc2B, INPUT_PULLUP);

  //###############################  set switch debouncers   ##########################
  debounceFSW1.attach(FSW1);
  debounceFSW1.interval(debounceDelay);
  
  debounceFSW2.attach(FSW2);
  debounceFSW2.interval(debounceDelay);
  
  debounceFSW3.attach(FSW3);
  debounceFSW3.interval(debounceDelay);
  
  debounceEncA.attach(EncSW1);
  debounceEncA.interval(debounceDelay);
  
  debounceEncB.attach(EncSW2);
  debounceEncB.interval(debounceDelay);

  //############################### create ControlChain device  #########################
  cc.begin();
  const char *uri = "https://github.com/Charly-R/TrippleCPedal";
  cc_device_t *device = cc.newDevice("TrippleCPedal", uri);

    //#############################   create switches     ###############################
    // create footswitch 1
    cc_actuator_config_t FSW1_config;
    FSW1_config.type = CC_ACTUATOR_MOMENTARY;
    FSW1_config.name = "FootSwitch1";
    FSW1_config.value = &valFSW1;
    FSW1_config.min = 0.0;
    FSW1_config.max = 1.0;
    FSW1_config.supported_modes = CC_MODE_TOGGLE | CC_MODE_TRIGGER;
    FSW1_config.max_assignments = 1;

    // create footswitch 2
    cc_actuator_config_t FSW2_config = FSW1_config;
    FSW2_config.name = "FootSwitch2";
    FSW2_config.value = &valFSW2;

    // create footswitch 3
    cc_actuator_config_t FSW3_config = FSW1_config;
    FSW3_config.name = "FootSwitch3";
    FSW3_config.value = &valFSW3;

    // add switches to device
    cc_actuator_t* actuator_FSW1;
    cc_actuator_t* actuator_FSW2;
    cc_actuator_t* actuator_FSW3;
    actuator_FSW1 = cc.newActuator(&FSW1_config);
    actuator_FSW2 = cc.newActuator(&FSW2_config);
    actuator_FSW3 = cc.newActuator(&FSW3_config);
    cc.addActuator(device, actuator_FSW1);
    cc.addActuator(device, actuator_FSW2);
    cc.addActuator(device, actuator_FSW3);

    //########################### create inc-encoders ###################################

    cc_actuator_config_t EncoderA_config;
    EncoderA_config.type = CC_ACTUATOR_CONTINUOUS;
    EncoderA_config.name = "EncoderA";
    EncoderA_config.value = &valEncA;
    EncoderA_config.min = ENC_MIN;
    EncoderA_config.max = ENC_MAX;
    EncoderA_config.supported_modes = CC_MODE_REAL | CC_MODE_INTEGER;
    EncoderA_config.max_assignments = 1;

	/*cc_actuator_config_t EncoderB_config = EncoderA_config;
	EncoderB_config.name = "EncoderB";
	EncoderB_config.value = &valEncB;*/

    // add encoders to device
    cc_actuator_t* actuator_EncA;
    //cc_actuator_t* actuator_EncB;
	actuator_EncA = cc.newActuator(&EncoderA_config);
	//actuator_EncB = cc.newActuator(&EncoderB_config);
    cc.addActuator(device, actuator_EncA);
    //cc.addActuator(device, actuator_EncB);

    //############################# activate callback messages ###########################

    cc.setEventCallback(CC_EV_UPDATE, (void(*)(void* arg)) assignment_update);
    cc.setEventCallback(CC_EV_ASSIGNMENT, (void(*)(void* arg)) assignment_add);
    //cc.setEventCallback(CC_EV_UNASSIGNMENT, (void(*)(void* arg)) assignment_remove);

	//############################# start display  #######################################
	u8g2.begin();
}

void toggleLED(int chosenLED, float assignmentVal) {
	if (assignmentVal > 0.0)
		digitalWrite(chosenLED, HIGH);
	else
		digitalWrite(chosenLED, LOW);
}

float readAndCheckEncoder(Encoder enc, float min, float max) {

	if (enc.read() > max) enc.write(max);
	if (enc.read() < min) enc.write(min);

	return enc.read();
}

void assignment_update(cc_assignment_t *assignment) {

	if (assignment->mode & CC_MODE_TOGGLE) {
		//toggle LED based on assignment update
		switch (assignment->actuator_id) {
		case 0:
			toggleLED(LED1, assignment->value);
			break;
		case 1:
			toggleLED(LED2, assignment->value);
			break;
		case 2:
			toggleLED(LED3, assignment->value);
		default:
			break;
		}
	}
}

void assignment_add(cc_assignment_t *assignment) {

	// make sure everything starts at the saved position
	switch (assignment->actuator_id) {
	case 0:
		valFSW1 = assignment->value;
		toggleLED(LED1, assignment->value);
		//switchFlag = 1;
		break;
	case 1:
		valFSW2 = assignment->value;
		toggleLED(LED2, assignment->value);
		//switchFlag = 2;
		break;
	case 2:
		valFSW3 = assignment->value;
		toggleLED(LED3, assignment->value);
		//switchFlag = 3;
	case 3:
		encoderA.write(-(ENC_MIN + (assignment->value * ((ENC_MAX - ENC_MIN) / (assignment->max - assignment->min)))));
		break;
	case 4:
		encoderB.write(-(ENC_MIN + (assignment->value * ((ENC_MAX - ENC_MIN) / (assignment->max - assignment->min)))));
		break;
	default:
		break;
	}
}

void assignment_remove(cc_assignment_t *assignment) {
}

void loop() {

  debounceFSW1.update();
  debounceFSW2.update();
  debounceFSW3.update();
  //debounceEncA.update();
  //debounceEncB.update();

  valFSW1 = (float) debounceFSW1.read();
  valFSW2 = (float) debounceFSW2.read();
  valFSW3 = (float) debounceFSW3.read();
  //valEncButton1 = (float) debounceEncA.read();
  //valEncButton2 = (float) debounceEncB.read();

  valEncA = -readAndCheckEncoder(encoderA, ENC_MIN, ENC_MAX);
  //valEncB = readAndCheckEncoder(encoderB, ENC_MIN, ENC_MAX);

  /*char test[8];
  String encWert = String(valFSW1);
  encWert.toCharArray(test, 8);
  u8g2.firstPage();
  do {
	  u8g2.setFont(u8g2_font_8x13_t_symbols);
	  u8g2.drawStr(0, 64, "This is a test");
  } while (u8g2.nextPage());*/

  cc.run();
}