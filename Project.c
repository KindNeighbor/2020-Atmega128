#define F_CPU    16000000
#define STOP		 0 // 스탑워치를 위한 설정
#define START		 1
#define INIT		 2
#define MODE1		 3 // 작동모드를 나누기 위한 설정
#define MODE2		 4
#define ONE		 5 // 자릿수 변경을 위한 설정
#define TEN		 6
#define HUND		 7
#define THOUS		 8

#include <avr/io.h>
#include <util/delay.h> // 딜레이 함수를 사용하기 위한 선언
#include <avr/interrupt.h> // 인터럽트를 사용하기 위한 선언

void Show4Digit(int number); 
void ShowDigit(int i, int digit); // 스톱워치의 분,초를 나누는 점을 표시
void Show4Digit1(int number);
void ShowDigit1(int i, int digit); // 점을 빼고 4자리 숫자만 표시

const unsigned char Segment_Data[] =
{0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x27,0x7F,0x6F}; // 0~9 숫자를 간단히 쓰기위해 세그먼트에 0~9를 표시할 수 있는 헥스코드
char COLUMN[4]={0,0,0,0};

int digit1=ONE; // 초기 설정을 해 놓았다. 모드는 1, 스톱워치는 0부터, 자릿수 표현도 왼쪽에서 제일 첫번째 자리부터, 모드2에서의 초기 숫자도 0으로 놓았다.
int MODE=MODE1;
int SHOW_NUMBER=0, SHOW_NUMBER12=0, SHOW_NUMBER34=0; // 각 표현 하고자 설정 해 놓은 값들을 설정하고 0으로 설정해 놓는다.
int SHOW_NUMBER1=0, SHOW_NUMBER2=0, SHOW_NUMBER3=0, SHOW_NUMBER4=0;
int SHOW_NUMBER_1=0;
int state=STOP;
unsigned char count_int=0;

ISR(TIMER0_OVF_vect) { // 오버 플로우 타이머를 이용해서 숫자가 카운팅 되도록 한다.  
	if(MODE==MODE1) {
		switch(state) { // 외부 인터럽트에 따라서 동작을 나눔
			case STOP : break;
			case START :
			count_int++;
			if(count_int == 244) { // 244번에 한번씩 SHOW_NUMBER34를 증가시킨다. 밑의 코드에서 오버 플로우 타이머 레지스터를 설정해서 약 244Hz를 만들 것이므로, 244번에 한번 올라감 = 1초를 만든다.
				SHOW_NUMBER34++;
				count_int=0; // SHOW_NUMBER34가 1증가하면 다시 count_int가 0이 된다.
				if(SHOW_NUMBER34>59) { // 시계 처럼 60진법 이므로, 59보다 커지면 분 부분의 값을 늘리고, 초 부분은 0으로 만든다.
					SHOW_NUMBER34=0;
					SHOW_NUMBER12++;
					if(SHOW_NUMBER12>59) {
						SHOW_NUMBER12=0, SHOW_NUMBER34=0;
					}
				}
			}
			
			break;
			case INIT : SHOW_NUMBER34=0, SHOW_NUMBER12=0, state=STOP; // 리셋을 하기 위한 코드
			break;
		}
	}
}

ISR(INT4_vect) { // 모드 설정을 위한 외부 인터럽트. 모드1인경우 누르면 불이 2개 들어오고 모드2 로 변경된다.
	if(MODE==MODE1) {PORTG=0x03, MODE=MODE2;}
	else {PORTG=0x01, MODE=MODE1;}
}
	

ISR(INT5_vect) { // 모드1에서 스톱워치의 시작,정지를 담당하고, 모드2에서 숫자를 증가시킨다.
	if(MODE==MODE1) { // 모드 1일때만 작동
		if(state==STOP) state=START;
		else state=STOP;
	}
	else {	// 모드 2일때만 작동. 모드가 2개뿐이라서 else 사용
		switch(digit1) { // 각 경우에 맞는 자릿수의 숫자만을 증가. 9가 넘으면 0이 됨.
			case ONE :
			SHOW_NUMBER1++;
			if(SHOW_NUMBER1>9) SHOW_NUMBER1=0;
			break;
			case TEN :
			SHOW_NUMBER2++;
			if(SHOW_NUMBER2>9) SHOW_NUMBER2=0;
			break;
			case HUND :
			SHOW_NUMBER3++;
			if(SHOW_NUMBER3>9) SHOW_NUMBER3=0;
			break;
			case THOUS :
			SHOW_NUMBER4++;
			if(SHOW_NUMBER4>9) SHOW_NUMBER4=0;
			break;
		}
	}
}



ISR(INT6_vect) { // 모드 1에서는 리셋을, 모드2에서는 자릿수를 변경해준다.

		if(MODE==MODE1) { // 모드 1에서 작동, 리셋 담당
		state=INIT;
		}
		else { // 모드 2에서 작동. 한번 누를 때마다 자릿수를 변경.
		if(digit1==ONE) digit1=TEN;
		else if(digit1==TEN) digit1=HUND;
		else if(digit1==HUND) digit1=THOUS;
		else digit1=ONE; // ONE으로 돌아가면 다시 처음부터 반복
		}	
}

int main(void) {
	DDRC = 0xff; DDRA = 0xff; // 세그먼트에 출력되는 포트의 출력 설정
	DDRB = 0x01; // 부저를 포트B에 연결했으므로, 마찬가지로 출력 설정
	DDRG = 0x03; // 모드를 구분하는 LED를 위한 출력 설정
	EICRB = 0x3f; // INT4~INT6을 사용했으므로 EICRB를 사용. 모두 Rising Edge로 설정했다.
	EIMSK = 0x70; //INT4~INT6을 Enable
	TCCR0 = 0x06; // 오버플로우 타이머를 위한 레지스터. 0x06인 경우 256 scale의 노말 모드 이다. 위에서 16000000으로 설정했으므로 16000000/256 = 62500Hz가 된다.
	TCNT0 = 0x00; // 0부터 시작해서 255까지 카운트 되고, 256이 되면 0이 되면서 오버플로우 인터럽트가 발생한다. 256번을 카운트 하므로, 오버 플로우가 발생하는 
	// 간격은 62500Hz/256 = 244.14Hz로 약 244Hz가 된다. 위에서 count_int를 가지고 시간의 속도를 정할 때 244란 숫자가 나오는 이유이다. 
	TIMSK = 0x01; // 0 오버플로우 인터럽트를 Enable
	SREG |= 0x80; // 외부 인터럽트를 전역 Enable
	PORTC = 0x00; // 세그먼트 초기 값 0으로 설정
	PORTG = 0x01; // 모드1일 때 초기에 LED 한 개만 켜 놓기 위해서 설정
	while(1) {
		switch(MODE) { // 모드에 따른 작동을 구분하기 위해 스위치 함수 사용
			case MODE1 :
				state=STOP;
				while(MODE==MODE1) { // 모드 1일때만 작동
				SHOW_NUMBER=SHOW_NUMBER12*100+SHOW_NUMBER34; // 초 부분과 분 부분을 합쳐서 표시되는 값을 만듦
				Show4Digit(SHOW_NUMBER); // 세그먼트에 4자리 숫자 표시
				}
			break;
			case MODE2 : 
				digit1=ONE;
				while(MODE==MODE2) { // 모드 2일때만 작동
				SHOW_NUMBER_1 = SHOW_NUMBER1*1000+SHOW_NUMBER2*100+SHOW_NUMBER3*10+SHOW_NUMBER4; // 표시되는 숫자를 각 자리 숫자에 10~1000을 각각 곱해서 표현
				Show4Digit1(SHOW_NUMBER_1); // 세그먼트에 4자리 숫자 표시
				if(SHOW_NUMBER_1==5198) PORTB=0x01; // 표시되는 숫자가 5198 일시에 포트B의 비트 값이 1오르고, 연결된 부저에서 소리가 남
				else PORTB=0x00; // 설정한 숫자와 같지 않으면 소리가 나지 않게 한다.
				}
			break;
			}
	}
}


void Show4Digit(int number) { // 입력된 숫자를 표시하는 함수
	COLUMN[0] = number/1000; COLUMN[1] = (number%1000)/100; // 각각의 자릿수를 표현.
/연산과 %연산을 통해 COLUMN[0] 부터 [4]까지 세그먼트의 왼쪽부터 차례대로 표현한다.
	COLUMN[2] = (number%100)/10; COLUMN[3] = (number%10);
	for(int i=0;i<4;i++) { // 4칸의 숫자를 빠르게 반복해서 동시에 뜨는 것처럼 보이게 한다.
		ShowDigit(COLUMN[i],i); // ShowDigit함수로 각 위치에 맞는 자리와 세그먼트에 원하는 숫자를 표시
		_delay_ms(2); // 빠르게 반복
	}
}

void ShowDigit(int i, int digit) {
	PORTC=~(0x01<<digit); // 원하는 자릿수만 켜지게 함
	if(digit==1)
		PORTA = Segment_Data[i]|0x80; // 위에서 언급했듯이 분과 초 사이에 점을 표시 하기 위해서 설정. 비트 연산자를 통해서 digit=1일 때, 즉 세그먼트의 
	// 왼쪽에서 두번째 자리의 숫자 옆의 점을 8번째 비트를 1로 할당 시키고 | 연산자, 즉 or연산을 통해서 점을 항상 켜 놓도록 한다.
	else
		PORTA = Segment_Data[i]; // 나머지 자릿수에서는 숫자 옆의 점에 불이 들어오지 않게 한다.
}

void Show4Digit1(int number) { // 위의 Show4Digit과 같은 함수. 점이 표시 되지 않는 경우를 표현 하기 위해서 따로 설정
	COLUMN[0] = number/1000; COLUMN[1] = (number%1000)/100;
	COLUMN[2] = (number%100)/10; COLUMN[3] = (number%10);
	for(int i=0;i<4;i++) {
		ShowDigit1(COLUMN[i],i); // 밑의 점을 표시하지 않는 ShowDigit1함수를 사용
		_delay_ms(2);
	}
}

void ShowDigit1(int i, int digit) { // 점이 표시 되지 않는다.
	PORTC=~(0x01<<digit);
	PORTA = Segment_Data[i];
}
