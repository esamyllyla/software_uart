/*
  Software UART learning project

  Currently Universal Asynchronous Transmitter supporting only 9600 baudrate.
  Does not have a receiver.

  Pins:
  B0: transmit
  B1: debug led
  B2: receive
*/

#define RECEIVE_BUFFER_SIZE 128

volatile static bool bit_coming = 0;

volatile static uint8_t receive_buffer[RECEIVE_BUFFER_SIZE];
volatile static uint8_t head = 0;
volatile static bool eof = 0;

volatile static uint8_t last_received_byte = 0;

uint16_t counter = 0;

// For baudrate 9600, we want to send one bit per 104.1667us.
// A 832 clock cycles long sleep results in 104us sleep on 8MHz:
// 832 / 8000000 = 104u.
// Error compared to 104.1667 is
// (.1667 / 104.1667) * 100% = 0.16% which is fine

// For receiving at 9600, one 52us time sleep is needed in 
// the beginning of a receive, to read bits in the middle
// 416 / 8000000 = 52u

// hard coded busy time sleep for 832 cycles:
void delay832()
{
  uint16_t start = TCNT1;

  while((uint16_t)(TCNT1 - start) < 832);
}

void delay416()
{
  uint16_t start = TCNT1;

  while((uint16_t)(TCNT1 - start) < 416);
}

// after TCNT1 was set to 0, this function busy waits until TCNT1 == 832
void wait_until_832()
{
  while((uint16_t)(TCNT1) < 832);
}

void sendletter(char letter)
{
  // PB0 acts as a TX pin

  PORTB &= ~(1 << PINB0); // 0b11111110; // PB0 -> 0, start bit
  delay832();
  for(uint8_t i = 0; i < 8; i++)
  {
    TCNT1 = 0;
    uint8_t bit = (letter >> i) & 1;
    
    PORTB = (PORTB & ~1) | bit;
    wait_until_832();
  }
  PORTB |= (1 << PINB0);
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

void sendstring(uint8_t head)
{
  for(uint8_t i = 0; i < head; i++)
  {
    sendletter(receive_buffer[i]);
  }
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

uint8_t receive_byte()
{
  uint8_t byte = 0;

  delay416();
  delay832();
  for(uint8_t i = 0; i < 8; i++)
  {
    TCNT1 = 0;
    uint8_t bit = (PINB >> PINB2) & 1;

    byte |= bit << i;

    wait_until_832();
  }
  delay832();
  //delay832();

  return byte;
}

void read_byte_stream()
{
  bit_coming = 1;
  head = 0;

  while(bit_coming)
  {
    delay416(); // initial delay to read from the middle of each bit
    delay832(); // skip first start bit

    uint8_t byte = 0;

    for(uint8_t i = 0; i < 8; i++)
    {
      TCNT1 = 0;
      uint8_t bit = (PINB >> PINB2) & 1;

      byte |= bit << i;

      wait_until_832();
    }

    // at this point, transmitter is sending stop bit
    TCNT1 = 0;

    receive_buffer[head] = byte;
    head++;

    // wait until stop bit ends
    // if new byte is to come, the pin will go to 0 for new start bit
    while((PINB >> PINB2) & 1)
    {
      if((uint16_t)(TCNT1) > 832) // if pin doesnt go to 0 within 1 byte time,
                                  // it has stopped sending bytes
      {
        bit_coming = 0;
        break;
      }
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  
  // Timer/Counter 1 settings
  TCCR1A = 0; // Use timer in normal mode
  TCCR1B = 1; // Timer/Counter register (TCNT1) increments with no prescaler (8MHz)

  DDRB  = (1 << PINB0) | (1 << PINB1); // 0b11; // PORTB bit 0 is output, pin 14
  DDRB &= ~(1 << PINB2); // PINB2 is input, pin 16
}

void loop() {
  if(((PINB >> PINB2) & 1) == 0)
  {
    PORTB |= (1 << PINB1); // debug led for indicating ongoing byte stream receive
    read_byte_stream();
    PORTB &= ~(1 << PINB1); // debug led off
    delayms(500);
    sendstring(head); // debug print received data
  }
}
