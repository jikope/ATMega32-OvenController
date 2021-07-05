/* *
 * Oven control for ATMega32
 */
#include <mega32.h>
#include <lcd.h>
#include <delay.h>
#include <stdlib.h>

#asm
    .EQU __lcd_port=0x15   
#endasm

#define TIME 1
#define TEMP 2
#define EVENT 3
#define ADC_VREF_TYPE 0x40

volatile unsigned int overflow;
char str_sec[5], str_min[5], str_hour[5], str_time[6], str_temp[4];
volatile int sec, minute, hour;
int loop, count, temperature, input_temp;

void Init();
void UpdateTime();
void ScanKeypad(char menu);
void ToVar(char value, int cond);
int StringToInt(const char *pszBuffer);
int ADC_Read(char adc_input);

void main(void)
{
// LCD INIT
DDRC = 0xF0;
PORTC = 0xFF; 
while (1)
      {         
        Init();  
        
        // WELCOME SCREEN 
        lcd_gotoxy(0, 0);
        lcd_putsf("WELCOME TO OVEN");    
        delay_ms(300);
        lcd_clear();
        lcd_putsf("Press any Button");
        lcd_gotoxy(4, 1);
        lcd_putsf("to Start ");
        while (loop == 1){    
            ScanKeypad(EVENT);
        }    
        lcd_clear();  
        delay_ms(100);
        
        // INPUT TEMPERATURE
        count = 0;
        loop = 1;
        lcd_gotoxy(0, 0);
        lcd_putsf("TEMP: ");
        while (loop == 1){ 
            ScanKeypad(TEMP);                 
        }     
        input_temp = StringToInt(str_temp);        
        count = 0;
        loop = 1;
        delay_ms(300);
            
        // INPUT TIME
        lcd_clear(); 
        lcd_gotoxy(0, 0);
        lcd_putsf("TIME HH:MM:SS");
        lcd_gotoxy(7, 1);
        lcd_putsf(":");
        lcd_gotoxy(10, 1);
        lcd_putsf(":");       
        while (loop == 1){ 
            ScanKeypad(TIME);                 
        }  
        lcd_clear();
        
        // TIME CONVERSION
        hour = StringToInt(str_hour);
        minute = StringToInt(str_min);
        sec = StringToInt(str_sec);
        
        itoa(hour, str_hour);
        itoa(minute, str_min);
        itoa(sec, str_sec);

        lcd_gotoxy(0, 0);
        lcd_putsf("TEMP: ");
        lcd_puts(str_temp);         
        lcd_gotoxy(0, 1);
        lcd_putsf("TIME: ");
        UpdateTime();
        delay_ms(300);
        TCCR0 |= (0 << CS02)|(1 << CS01)|(1 << CS00);
        
        // BAKING LOOP        
        while(1){
            // CHECK IF TIME EQUALS TO ZERO
            if(hour == 0 && minute == 0 && sec == 0){
                TCCR0 &= ~(1 << CS01);  // OFF
                TCCR0 &= ~(1 << CS00);
                lcd_clear();
                lcd_putsf("Selesai ");
                delay_ms(300);
                break;
            }         
            // READ TEMPERATURE FROM PORTA.0
            temperature = ADC_Read(0);
            // CHECK IF TEMP EXCEEDS DESIRED TEMP
            if(temperature < input_temp){
                PORTD.7 = 1;
            }else if (temperature > input_temp){
                PORTD.7 = 0 ;
            }  
        }                     
             
        continue;        
      }
}

void Init(){
    // VARIABLES INIT
    overflow = 0;
    sec = 99, minute = 99, hour = 99;
    loop = 1, count = 0, temperature = 0;
    
    // STRING INIT
    str_time[0] = '\0';
    str_time[1] = '\0';
    str_time[2] = '\0';
    str_time[3] = '\0';
    str_time[4] = '\0';
    str_time[5] = '\0';             
    str_hour[0] = '\0';
    str_hour[1] = '\0';
    str_min[0] = '\0';
    str_min[1] = '\0';
    str_sec[0] = '\0';
    str_sec[1] = '\0'; 
               
    // ADC INIT
    DDRA = 0x00;
    ADCSRA = 0x87;
    ACSR = 0x80;
    
    // LCD INIT
    //DDRC = 0xF0;
    //PORTC = 0xFF;         
               
    // KEYPAD INIT
    DDRB = 0xF0;
    PORTB = 0xFF;  
          
    // DRIVER INIT
    DDRD = 0xF0;
    PORTD = 0x0F;
    PORTD.7 = 0;
    PORTD.0 = 1;
    PORTD.3 = 1;
    
    // TIMER INIT
    TCCR0 |= (1 << WGM01); // CTC Mode 
    TCCR0 &= ~(1 << CS01);  // OFF
    TCCR0 &= ~(1 << CS00);
    //TCCR0 |= (0 << CS02)|(1 << CS01)|(1 << CS00); // Pre-scaler 64
    TCNT0 = 0;                    
    OCR0 = 156; // Comparator
    TIMSK |= (1 << OCIE0); // Enable Interrupt
    // Enable Global Interrupt
    #asm("sei")
    
    // LCD INIT     
    lcd_init(16);
    lcd_clear();      
}

void UpdateTime(){
    // HOUR
    if(hour < 10){         
        lcd_gotoxy(5, 1);
        lcd_putsf("0");
        lcd_gotoxy(6, 1);
    }else{          
        lcd_gotoxy(5, 1);        
    }   
    lcd_puts(str_hour);
                  
    lcd_gotoxy(7, 1);
    lcd_putsf(":");
    
    // MINUTE
    if(minute < 10){
        lcd_gotoxy(8, 1);
        lcd_putsf("0");      
        lcd_gotoxy(9, 1);
    }else{
        lcd_gotoxy(8, 1);
    }               
    lcd_puts(str_min);
    
    lcd_gotoxy(10, 1);
    lcd_putsf(":");
    
    // SECOND
    if(sec < 10){        
        lcd_gotoxy(11, 1);
        lcd_putsf("0");
        lcd_gotoxy(12, 1);
    }else{
        lcd_gotoxy(11, 1);
    }                    
    lcd_puts(str_sec);
}

void ScanKeypad(char menu){
        PORTB = 0b11101111;
        if(PINB.0 == 0){ ToVar('1', menu); }
        if(PINB.1 == 0){ ToVar('4', menu); }
        if(PINB.2 == 0){ ToVar('7', menu); }
        if(PINB.3 == 0){ ToVar('*', menu); }
        
        PORTB = 0b11011111;
        if(PINB.0 == 0){ ToVar('2', menu); }
        if(PINB.1 == 0){ ToVar('5', menu); }
        if(PINB.2 == 0){ ToVar('8', menu); }
        if(PINB.3 == 0){ ToVar('0', menu); }
        
        PORTB = 0b10111111;
        if(PINB.0 == 0){ ToVar('3', menu); }
        if(PINB.1 == 0){ ToVar('6', menu); }
        if(PINB.2 == 0){ ToVar('9', menu); }
        if(PINB.3 == 0){ ToVar('#', menu); }      
}

void ToVar(char value, int cond){
    if(cond == TIME){         
        if(value == '*'){
            count = 0;    
            str_time[0] = '\0';
            str_time[1] = '\0';
            str_time[2] = '\0';
            str_time[3] = '\0';
            str_time[4] = '\0';
            str_time[5] = '\0'; 
                 
            str_hour[0] = '\0';
            str_hour[1] = '\0';
            str_min[0] = '\0';
            str_min[1] = '\0';
            str_sec[0] = '\0';
            str_sec[1] = '\0';  
            lcd_clear(); 
            lcd_gotoxy(0, 0);
            lcd_putsf("TIME HH:MM:SS");
            lcd_gotoxy(7, 1);
            lcd_putsf(":");
            lcd_gotoxy(10, 1);
            lcd_putsf(":"); 
            delay_ms(50); 
            
        }   
        else if(value == '#'){
             loop = 0;
            }
        else{
            if (count < 5 ){
                str_time[count] = value;
                count = count + 1;
             UpdateTime(); 
                delay_ms(50);  
            }else if( count == 5 ){
                str_time[count] = value;
                //UpdateTime();
                delay_ms(50);
                count = 7;
            }
                str_hour[0] = str_time[0];
                str_hour[1] = str_time[1];
                str_min[0] = str_time[2];
                str_min[1] = str_time[3];
                str_sec[0] = str_time[4];
                str_sec[1] = str_time[5];            
            }
        UpdateTime();
    }else if(cond == TEMP){
        if(value == '*'){
        count = 0;
        str_temp[0] = '\0';
        str_temp[1] = '\0';
        str_temp[2] = '\0';
        str_temp[3] = '\0';
        lcd_clear();
        lcd_gotoxy(0, 0);
        lcd_putsf("TEMP: ");
        }else if(value == '#'){
                loop = 0;
            }else{
                if (count < 3 ){
                str_temp[count] = value;
                count = count + 1;
                delay_ms(50);  
            }else if( count == 3 ){
                str_temp[count] = value;
                delay_ms(50);
                count = 7;
            }               
        }
        lcd_gotoxy(5, 0);
        lcd_puts(str_temp); 
    }else if(cond == EVENT){
        loop = 0;
    }      
}

interrupt[TIM0_COMP] void timer0_comp_isr(void){
    overflow++;
    if(overflow >= 100){
        sec--;       
        if(sec < 0){
            sec = 59;
            minute--;
            itoa(minute, str_min);
            //UpdateTime();
            if(minute < 0){
                minute = 59;
                hour--;              
                itoa(minute, str_min);
                itoa(hour, str_hour);
                //UpdateTime();
            }
        }
        itoa(sec, str_sec);
        itoa(temperature, str_temp);
        lcd_gotoxy(6, 0);
        lcd_puts(str_temp);
        lcd_putsf("C");
        UpdateTime();         
        overflow = 0;
    }          
}

int ADC_Read(char adc_input){
    ADMUX = adc_input | (ADC_VREF_TYPE & 0xFF);
    ADCSRA |= (1 << ADSC);
    while((ADCSRA &(1 << ADIF)) == 0);
        ADCSRA |= (1 << ADIF);
    return ADCW;
}

#define Is_NUMERIC_STRING(d) (*(char*)d >= 48) && (*(char*)d<= 57)
int StringToInt(const char *pszBuffer)
{
    int result=0; // variable to store the result
    int sign = 1; //Initialize sign as positive
    if(pszBuffer == NULL) //If pointer is null
        return 0;
    //If number is negative, then update sign
    if((*pszBuffer) == '-')
    {
        sign = -1;
        ++pszBuffer; //Increment the pointer
    }
    while( Is_NUMERIC_STRING(pszBuffer)) //check string validity
    {
        result = (result*10)+ (*pszBuffer-48);
        pszBuffer++; //Increment the pointer
    }
    return (sign * result);
}
