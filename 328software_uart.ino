/*
  Software UART learning project

  Currently Universal Asynchronous Transmitter supporting only 9600 baudrate.
  Does not have a receiver.
*/

uint16_t counter = 0;

// For baudrate 9600, we want to send one bit per 104.1667us.
// A 832 clock cycles long sleep results in 104us sleep on 8MHz:
// 832 / 8000000 = 104.
// Error compared to 104.1667 is
// (.1667 / 104.1667) * 100% = 0.16% which is fine

// hard coded busy time sleep for 832 cycles:
void delay832()
{
  uint16_t start = TCNT1;

  while((uint16_t)(TCNT1 - start) < 832);
}

// after TCNT1 was set to 0, this function busy waits until TCNT1 == 832
void wait_until_832()
{
  while((uint16_t)(TCNT1) < 832);
}

void sendletter(char letter)
{
  // PB0 acts as a TX pin

  PORTB &= 0b11111110; // PB0 -> 0, start bit
  delay832();
  for(uint8_t i = 0; i < 8; i++)
  {
    TCNT1 = 0;
    uint8_t bit = (letter >> i) & 1;
    
    PORTB = (PORTB & ~1) | bit;
    wait_until_832();
  }
  PORTB |= 0b00000001;
  delay832();
}

/*
Some bit operations to learn:
  PORTB = (PORTB & ~1) | bit;
  PORTB = (PORTB & 0xFE) | bit;

  // Set bit n
  reg |= (1 << n);

  // Clear bit n
  reg &= ~(1 << n);

  // Toggle bit n
  reg ^= (1 << n);

  reg = (reg & ~(1 << n)) | (value << n);
  PORTB = (PORTB & ~1) | bit;
*/

void sendnumber(uint16_t number)
{
  uint8_t dig10000s = '0',
          dig1000s  = '0',
          dig100s   = '0',
          dig10s    = '0';
  
  while (number >= 10000)
  {
    number -= 10000;
    dig10000s++;
  }

  while (number >= 1000)
  {
    number -= 1000;
    dig1000s++;
  }

  while (number >= 100)
  {
    number -= 100;
    dig100s++;
  }

  while (number >= 10)
  {
    number -= 10;
    dig10s++;
  }

  sendletter(dig10000s); sendletter(dig1000s); sendletter(dig100s); sendletter(dig10s);
  sendletter(number + '0'); sendletter('\r'); sendletter('\n');
}

void delayms(uint16_t ms)
{
  while (ms > 0)
  {
    delayus(1000);
    ms--;
  }
}

void delayus(uint16_t us)
{
  uint16_t start = TCNT1;

  uint16_t cycles = 8 * us;

  while((uint16_t)(TCNT1 - start) < cycles)
  {}
}

void setup() {
  // put your setup code here, to run once:
  
  // Timer/Counter 1 settings
  TCCR1A = 0; // Use timer in normal mode
  TCCR1B = 1; // Timer/Counter register (TCNT1) increments with no prescaler (8MHz)

  DDRB = 0b11; // PORTB bit 0 is output, pin 14
  PORTB = 0b11; // set B1-B0 -> 1
}

void loop() {
  // put your main code here, to run repeatedly:
  PORTB &= 0b11111101; // this makes B1 -> 0
  delayms(1000);
  PORTB |= 0b00000010; // this makes B1 -> 1
  delayms(1000);
  sendnumber(counter);
  counter++;
}
