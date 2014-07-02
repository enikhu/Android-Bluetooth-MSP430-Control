/****************************************************************
*  �sim    : Android Bluetooth Uygulamas1                       *
*  Yazar   : Resul Aglar                                        *
*  E-posta : resulaglar@gmail.com                               *
*  Tarih   : 15-04-2014                                	  	*
*  S�r�m   : 1.0                                                *
****************************************************************/
#include <io430g2553.h>
#include "stdbool.h"

 
#define LED1      0x01
#define LED2      0x40 
#define BUTON     0x08 
#define TXD 0x02 // TXD pini P1.1
#define RXD 0x04 // RXD pini P1.2
#define Bit_time 104 // 9600 Baud, SMCLK=1MHz (1MHz/9600)=104
#define Bit_time_5 52 // yar�m bit zaman�

unsigned char BitCnt; // Bit saya� de�i�keni
unsigned int alinan;  // Al�nan karakter
unsigned int gonderilen;  // G�nderilen karakter
unsigned char CharCnt; // Karakter saya�
unsigned int RXByte; // veri al�m de�i�keni
unsigned int TXByte; // veri g�nderim de�i�keni
bool isReceiving; // veri alma durum biti
bool hasReceived; // veri al�nd� durum biti
void Transmit(void);
long ham,derece;

//gecikme fonksiyonu
void delay(unsigned long int d)
{
  d*=100;
  for(;d>0;d--);
}


void main (void)
{   WDTCTL = WDTPW + WDTHOLD; // Watchdog timeri durdur
 	BCSCTL1 = CALBC1_1MHZ; // 1mhz dahili osilat�r
	DCOCTL = CALDCO_1MHZ; // SMCLK = DCO = 1MHz
    // AD �evirici s�cakl�k �l�mek i�in ayarlan�r.
    ADC10CTL1 = INCH_10 + ADC10DIV_3;
    ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON ;
	P1SEL |= TXD; // g�nderme pini ayarlar�
	P1IES |= RXD; // Alma pini ayarlar�
	P1IFG &= ~RXD; // Clear RXD (flag) before enabling interrupt
	P1IE |= RXD; // Enable RXD interrupt
    P1DIR=0;	//Port1 tamam giri�
  	P1DIR |= TXD+LED1+LED2; // Gerekli pinler ��k�� olarak ayarlan�r.
	isReceiving = false; // Set initial values
	hasReceived = false;
    gonderilen=0;
    P1OUT=0;			//Port1 ��k��� temizle.
	__bis_SR_register(GIE); // interrupts enabled
     
        
               
// Ana d�ng�
  while(1){
	delay(100);
	//LED1'in durumuna g�re g�nderilen verinin 1. biti d�zenlenir.
   if(P1OUT & LED1)		
     gonderilen |= 0x01;
   else
     gonderilen &= ~0x01;
   //LED2'nin durumuna g�re g�nderilen verinin 2. biti d�zenlenir.
   if(P1OUT & LED2)
     gonderilen |= 0x02;
   else
     gonderilen &= ~0x02;
   //Butonun durumuna g�re g�nderilen verinin 3. biti d�zenlenir.
   if(!(P1IN & BUTON))
     gonderilen |= 0x04;
   else
     gonderilen &= ~0x04;
	//G�nderilen verinin 8. biti set edilerek buton ve LEd durum bilgileri g�nderilir.
   TXByte = (gonderilen | 0x80); Transmit();
   
   ADC10CTL0 |= ENC + ADC10SC;            // AD �evrimi ba�lat
     if(ADC10CTL0 & ADC10IFG)		        // AD �evrim bitti mi?
      {
        ADC10CTL0 &= !ADC10IFG;		        // �evrim bayar��n� temizle
        ham = ADC10MEM;		                // s�cakl��� oku
        derece = ((ham - 673) * 423) / 1024;	//S�cakl��� dereceye �evir.
	//G�nderilen verinin 8.biti s�f�rlanarak s�cakl�k bilgisi g�nderilir.
     TXByte = (int)derece & 0x7f; Transmit();
      }

   //Al�nan veri de�erlendirilir.
   if(hasReceived){
   alinan=RXByte;
   hasReceived = false;
   //Gelen verinin 1. bitine g�re LED1 durumu d�zenlenir.
      if(alinan & 0x01)      
        P1OUT |= LED1;
      else
        P1OUT &= ~LED1;
	//Gelen verinin 2. bitine g�re LED2 durumu d�zenlenir.
      if(alinan & 0x02)
        P1OUT |= LED2;
        else
        P1OUT &= ~LED2;
    }
     }
      }     
        


//Bu fonksiyon TXByte degi�kenindeki veriyi TXD pininden g�nderir.
void Transmit(void)
{
	while(isReceiving); // Wait for RX completion
	TXByte |= 0x100; // Add stop bit to TXByte (which is logical 1)
	TXByte = TXByte << 1; // Add start bit (which is logical 0)
	BitCnt = 0xA; // Load Bit counter, 8 bits + ST/SP
	CCTL0 = OUT; // TXD Idle as Mark
	TACTL = TASSEL_2 + MC_2; // SMCLK, continuous mode
	CCR0 = TAR; // Initialize compare register
	CCR0 += Bit_time; // Set time till first bit
	CCTL0 = CCIS0 + OUTMOD0 + CCIE; // Set signal, intial value, enable interrupts
	while ( CCTL0 & CCIE ); // Wait for previous TX completion
}
 
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
	isReceiving = true;
	P1IE &= ~RXD; // Disable RXD interrupt
	P1IFG &= ~RXD; // Clear RXD IFG (interrupt flag)
	TACTL = TASSEL_2 + MC_2; // SMCLK, continuous mode
	CCR0 = TAR; // Initialize compare register
	CCR0 += Bit_time_5; // Set time till first bit
	CCTL0 = OUTMOD1 + CCIE; // Dissable TX and enable interrupts
	RXByte = 0; // Initialize RXByte
	BitCnt = 0x9; // Load Bit counter, 8 bits + ST
}
// Timer interrupt routine. This handles transmiting and receiving bytes.
#pragma vector = TIMER0_A1_VECTOR 
__interrupt void TA1_ISR (void)
{
if(!isReceiving)
{
	CCR0 += Bit_time; // Add Offset to CCR0
if ( BitCnt == 0) // If all bits TXed
{
	TACTL = TASSEL_2; // SMCLK, timer off (for power consumption)
	CCTL0 &= ~ CCIE ; // Disable interrupt
}
	else
{
	CCTL0 |= OUTMOD2; // Set TX bit to 0
if (TXByte & 0x01)
	CCTL0 &= ~ OUTMOD2; // If it should be 1, set it to 1
	TXByte = TXByte >> 1;
	BitCnt --;
 }}
	else
{
	CCR0 += Bit_time; // Add Offset to CCR0
if ( BitCnt == 0)
{
	TACTL = TASSEL_2; // SMCLK, timer off (for power consumption)
	CCTL0 &= ~ CCIE ; // Disable interrupt
	isReceiving = false;
	P1IFG &= ~RXD; // clear RXD IFG (interrupt flag)
	P1IE |= RXD; // enabled RXD interrupt
if ( (RXByte & 0x201) == 0x200) // Validate the start and stop bits are correct
{
	RXByte = RXByte >> 1; // Remove start bit
	RXByte &= 0xFF; // Remove stop bit
	hasReceived = true;
}
__bic_SR_register_on_exit(CPUOFF); // Enable CPU so the main while loop continues
}
else
{
	if ( (P1IN & RXD) == RXD) // If bit is set?
	RXByte |= 0x400; // Set the value in the RXByte
	RXByte = RXByte >> 1; // Shift the bits down
	BitCnt --;
	}}}
