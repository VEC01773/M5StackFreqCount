// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
	Name:       M5StackFreqCount.ino
	Created:	2020/06/20 15:25:43
	Author:     ITEC91\hide_
*/

// Define User Types below here or use a .h file
//
#include <M5Stack.h>
#include "driver/pcnt.h"

// Define Function Prototypes that use User Types below here or use a .h file
//


// Define Functions below here or use other .ino or cpp files
//
#define LEDC_CHANNEL_0    0
#define LEDC_TIMER_BIT    4
#define LEDC_BASE_FREQ    10000000  //10MHz Bit��1�ōő�40MHz
#define SIG_OUT_PIN       5         //�e�X�g�M���o�͒[�q

#define PULSE_INPUT_PIN 22          //�p���X�̓��̓s��

//�O���[�o���ϐ�
volatile int32_t PulsCounter;       //�p���X�J�E���g
volatile int32_t PrePulsCounter;    //�O��ǂݍ��݂����p���X�J�E���g

volatile uint32_t CurTime;          //usec
volatile uint32_t PreTime;          //�O��ǂݍ��݂���usec

volatile uint16_t GateTime;         //�Q�[�g�^�C��
volatile bool GateStartFlg;         //�X�^�[�g�t���O
volatile uint32_t OverCount;

//�^�C�}�[�����ݗp
hw_timer_t* timer1 = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

int int_cycle = 1000;	//usec
//----------------------------
//�^�C�}�[�����݊֐�
// ���݂̃p���X�J�E���g�����擾
//----------------------------
void IRAM_ATTR onTimer1()
{
	static int16_t count;
	static int16_t precount;
	portENTER_CRITICAL_ISR(&timerMux);
	pcnt_get_counter_value(PCNT_UNIT_0, &count);

	if (GateStartFlg)//�Q�[�g�X�^�[�g
	{
		PreTime = micros();
		PrePulsCounter = count;
		PulsCounter = PrePulsCounter;
		GateStartFlg = false;
		OverCount = 0;
	}
	else
	{
		CurTime = micros();
		if (count < precount)
			OverCount++;
		PulsCounter = count + 0x7fff * OverCount;
		GateTime--;
	}
	precount = count;
	portEXIT_CRITICAL_ISR(&timerMux);
}

void SetOutputFreq(double freq)
{
	int basef;
	int timerbit;

	if (freq <= 0)
	{
		ledcDetachPin(SIG_OUT_PIN);
		return;
	}
	else if (freq >= 40e6)
	{
		basef = 40e6;
		timerbit = 1;
	}
	else
	{
		basef = freq;
		timerbit = 1;
	}
	//PWM�Ŏw����g�����o��
	ledcSetup(LEDC_CHANNEL_0, basef, timerbit);
	ledcAttachPin(SIG_OUT_PIN, LEDC_CHANNEL_0);
	ledcWrite(LEDC_CHANNEL_0, 1 << (timerbit - 1));

}

//----------------------------
// setup
//----------------------------
void setup()
{
	Serial.begin(115200);
	M5.begin();
	M5.Lcd.fillScreen(WHITE);
	delay(500);
	// text print
	M5.Lcd.fillScreen(BLACK);
	M5.Lcd.setCursor(10, 10);
	M5.Lcd.setTextColor(WHITE);
	M5.Lcd.setTextSize(2);
	M5.Lcd.printf("Frequecy Counter!");

	//�p���X�J�E���^�ݒ�
	pcnt_config_t pcnt_config;//�ݒ�p�̍\���̂̐錾
	pcnt_config.pulse_gpio_num = PULSE_INPUT_PIN;   //����Pin
	pcnt_config.ctrl_gpio_num = PCNT_PIN_NOT_USED;  //����Pin�͖��g�p
	pcnt_config.lctrl_mode = PCNT_MODE_KEEP;        //����s����Low�̓���
	pcnt_config.hctrl_mode = PCNT_MODE_KEEP;        //����s����High�̓���
	pcnt_config.channel = PCNT_CHANNEL_0;
	pcnt_config.unit = PCNT_UNIT_0;
	pcnt_config.pos_mode = PCNT_COUNT_INC;
	pcnt_config.neg_mode = PCNT_COUNT_DIS;
	pcnt_config.counter_h_lim = 0x7fff;
	pcnt_config.counter_l_lim = -0x8000;

	pcnt_unit_config(&pcnt_config);//���j�b�g������
	pcnt_counter_pause(PCNT_UNIT_0);//�J�E���^�ꎞ��~
	pcnt_counter_clear(PCNT_UNIT_0);//�J�E���^������
	pcnt_counter_resume(PCNT_UNIT_0);//�J�E���g�J�n

	GateTime = 2;
	GateStartFlg = true;

	//�^�C�}�[0�A�v���X�P�[���[,
	timer1 = timerBegin(0, 80, true);
	//�����݊֐�
	timerAttachInterrupt(timer1, &onTimer1, true);
	//�^�C�}�[����Ԋu�̎w�� 1usec * INT_CYCLE msec
	timerAlarmWrite(timer1, int_cycle, true);
	timerAlarmEnable(timer1);

	Serial.println("Start!");

	//�e�X�g�M���o��
	SetOutputFreq(1e6);
}

//----------------------------
// loop
//----------------------------
void loop()
{
	if (GateTime == 0)
	{
		portENTER_CRITICAL_ISR(&timerMux);
		GateTime = 500;
		GateStartFlg = true;

		uint32_t countdiff = PulsCounter - PrePulsCounter;
		uint32_t timediff;
		if (CurTime < PreTime)
			timediff = 0xffffffff - (PreTime - CurTime);
		else
			timediff = (CurTime - PreTime);

		float f = (float)countdiff / (float)timediff * 1000.0;
		portEXIT_CRITICAL_ISR(&timerMux);

		M5.Lcd.fillRect(0, 0, 360, 26, 0);

		Serial.printf("frequency :%0.3fkHz %d %d %d %u %u %u\n", f, PulsCounter, PrePulsCounter, countdiff, CurTime, PreTime, timediff);

		//  M5.Lcd.clear();
		M5.Lcd.fillRect(0, 50, 360, 75, 0);
		delay(1);

		M5.Lcd.setCursor(0, 50);
		M5.Lcd.printf("frequency :%8.3fkHz\n", f);

		M5.Lcd.setCursor(0, 70);
		M5.Lcd.printf("countdiff :%d\n", countdiff);

		M5.Lcd.setCursor(0, 90);
		M5.Lcd.printf("Gate Time :%dusec\n", int_cycle);

		M5.Lcd.fillRect(0, 220, 240, 20, 0);
		M5.Lcd.progressBar(0, 220, 240, 20, 20);


	}
	/*
		  if(countdiff < 1000)
		  {
			int_cycle += 1000 ;
			timerEnd(timer1);
			timer1 = timerBegin(0, 80, true);
			//�����݊֐�
			timerAttachInterrupt(timer1, &onTimer1, true);
			timerAlarmWrite(timer1, int_cycle, true);
			timerAlarmEnable(timer1);
			delay(int_cycle/1000);
		  }
		*/
}