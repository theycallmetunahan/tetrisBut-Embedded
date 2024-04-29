// ============================ //
// Do not edit this part!!!!    //
// ============================ //
// 0x300001 - CONFIG1H
#pragma config OSC = HSPLL // Oscillator Selection bits (HS oscillator,
                           // PLL enabled (Clock Frequency = 4 x FOSC1))
#pragma config FCMEN = OFF // Fail-Safe Clock Monitor Enable bit
                           // (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF  // Internal/External Oscillator Switchover bit
                           // (Oscillator Switchover mode disabled)
// 0x300002 - CONFIG2L
#pragma config PWRT = OFF  // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF // Brown-out Reset Enable bits (Brown-out
                           // Reset disabled in hardware and software)
// 0x300003 - CONFIG1H
#pragma config WDT = OFF // Watchdog Timer Enable bit
                         // (WDT disabled (control is placed on the SWDTEN bit))
// 0x300004 - CONFIG3L
// 0x300005 - CONFIG3H
#pragma config LPT1OSC = OFF // Low-Power Timer1 Oscillator Enable bit
                             // (Timer1 configured for higher power operation)
#pragma config MCLRE = ON    // MCLR Pin Enable bit (MCLR pin enabled;
                             // RE3 input pin disabled)
// 0x300006 - CONFIG4L
#pragma config LVP = OFF   // Single-Supply ICSP Enable bit (Single-Supply
                           // ICSP disabled)
#pragma config XINST = OFF // Extended Instruction Set Enable bit
                           // (Instruction set extension and Indexed
                           // Addressing mode disabled (Legacy mode))

#pragma config DEBUG = OFF // Disable In-Circuit Debugger

#define KHZ 1000UL
#define MHZ (KHZ * KHZ)
#define _XTAL_FREQ (40UL * MHZ)
#define COUNTER_MAX 32

// ============================ //
//             End              //
// ============================ //

// FINAL VERSION

// PASSED ALL TEST CASES

// v5.0 Nis 22 2024
// shadow functionality implemented
// up, down, left, right, submit, rotate implemented

// TODO: 7-segment display
// TODO: physical testing
// W.C.

// counter display first digit correct
// counter display second digit not shown

#include <xc.h>
#include <stdint.h>

#define T_PRELOAD_HIGH 0x67
#define T_PRELOAD_LOW 0x69

uint8_t prevB;
uint8_t increment_update = 0;
int counter = 0;

static const uint8_t hexnums[10] = {
    0x3F,
    0x06,
    0x5B,
    0x4F,
    0x66,
    0x6D,
    0x7D,
    0x07,
    0x7F,
    0x6F};

struct dot_piece
{
    uint8_t position;
    uint8_t pattern[16];
    uint8_t index;
};

struct square_piece
{
    uint8_t position;
    uint8_t pattern_first[16];
    uint8_t pattern_second[16];
    uint8_t index;
};

struct l_piece
{
    uint8_t position;
    volatile uint8_t pattern_first[16];
    volatile uint8_t pattern_second[16];
    uint8_t index;
    uint8_t empty_cell;
};

struct submission_state
{
    volatile uint8_t pattern_LATC;
    volatile uint8_t pattern_LATD;
    volatile uint8_t pattern_LATE;
    volatile uint8_t pattern_LATF;
};

uint8_t original_pattern[16] = {0x03, 0x00, 0x06, 0x00, 0x0C, 0x00, 0x18, 0x00, 0x30, 0x00, 0x60, 0x00, 0xC0, 0x00, 0x80, 0x00};

void shadow_cell(uint8_t *pattern, int i)
{
    if (*pattern == 0)
        return;

    uint8_t mask = 0x80;
    uint8_t first_found = 0;
    uint8_t second_found = 0;

    while (mask != 0)
    {
        if ((*pattern & mask) != 0)
        {
            if (first_found == 0)
            {
                first_found = mask;
            }
            else if (second_found == 0)
            {
                second_found = mask;
                break;
            }
        }
        mask >>= 1;
    }

    if (i == 1)
    {

        if (first_found != 0)
        {
            *pattern &= ~first_found;
        }
    }
    else
    {

        if (second_found != 0)
        {
            *pattern &= ~second_found;
        }
        else if (first_found != 0)
        {

            *pattern &= ~first_found;
        }
    }
}

uint8_t piece = 0;

struct dot_piece my_dot = {
    0,
    {0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80, 0x00},
    0};

struct square_piece my_square = {
    0,
    {0x03, 0x00, 0x06, 0x00, 0x0C, 0x00, 0x18, 0x00, 0x30, 0x00, 0x60, 0x00, 0xC0, 0x00, 0x80, 0x00},
    {0x03, 0x00, 0x06, 0x00, 0x0C, 0x00, 0x18, 0x00, 0x30, 0x00, 0x60, 0x00, 0xC0, 0x00, 0x80, 0x00},
    0};

struct l_piece my_l = {
    0,
    {0x03, 0x00, 0x06, 0x00, 0x0C, 0x00, 0x18, 0x00, 0x30, 0x00, 0x60, 0x00, 0xC0, 0x00, 0x80, 0x00},
    {0x03, 0x00, 0x06, 0x00, 0x0C, 0x00, 0x18, 0x00, 0x30, 0x00, 0x60, 0x00, 0xC0, 0x00, 0x80, 0x00},
    0,
    3};

struct submission_state my_submission_state = {

    0x00,
    0x00,
    0x00,
    0x00};

struct submission_state my_piece_state = {

    0x00,
    0x00,
    0x00,
    0x00};

void originalize_pattern()
{

    for (int i = 0; i < 16; i++)
    {
        my_l.pattern_first[i] = original_pattern[i];
        my_l.pattern_second[i] = original_pattern[i];
    }
}
void clear_displays()
{
    PORTH = 0x00; // Clear all display enables
}

void display7seg(int digit, int position)
{
    PORTH = 0x00;

    PORTJ = hexnums[digit];

    if (position == 0)
    {
        PORTH |= (1 << 2);
    }
    else if (position == 1)
    {
        PORTH |= (1 << 3);
    }
}

void send7seg(int number)
{
    int tens = number / 10;
    int ones = number % 10;

    display7seg(tens, 0);
    display7seg(ones, 1);
}

void send7seg0(int number)
{
    int ones = number % 10;

    display7seg(ones, 1);
}

void send7seg1(int number)
{
    int tens = number / 10;

    display7seg(tens, 0);
}

void reinitialize_my_l()
{
    my_l.position = 0;
    my_l.index = 0;
    my_l.empty_cell = 3;
    originalize_pattern();
}

void reinitialize_my_square()
{
    my_square.position = 0;
    my_square.index = 0;
}

void reinitialize_my_dot()
{
    my_dot.position = 0;
    my_dot.index = 0;
}

volatile uint8_t prevRB5 = 0;
volatile uint8_t prevRB6 = 0;

static uint8_t prevStatePORTA = 0x00;

void Init()
{

    LATA = 0x00;
    PORTA = 0x00;
    TRISA = 0x0F;

    LATB = 0x00;
    PORTB = 0x00;
    TRISB = 0xFF;

    LATC = 0x00;
    PORTC = 0x00;
    TRISC = 0x00;

    LATD = 0x00;
    PORTD = 0x00;
    TRISD = 0x00;

    LATE = 0x00;
    PORTE = 0x00;
    TRISE = 0x00;

    LATF = 0x00;
    PORTF = 0x00;
    TRISF = 0x00;

    ADCON1 = 0x0F;

    TRISBbits.TRISB6 = 1;

    TRISBbits.TRISB5 = 1;

    prevRB5 = PORTBbits.RB5;
    prevRB6 = PORTBbits.RB6;
}

void InitializeTimerAndInterrupts()
{

    T0CON = 0x00;
    T0CONbits.TMR0ON = 1;
    T0CONbits.T0PS2 = 1;
    T0CONbits.T0PS1 = 0;
    T0CONbits.T0PS0 = 1;

    TMR0H = T_PRELOAD_HIGH;
    TMR0L = T_PRELOAD_LOW;

    RCONbits.IPEN = 0;
    INTCON = 0x00;
    INTCONbits.TMR0IE = 1;
    INTCONbits.RBIE = 1;
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.RBIF = 0;
}

void handle_index(int increment)
{

    if (piece == 0)
    {
        my_dot.index += increment;
        my_dot.index += increment;
        if (my_dot.index < 0)
        {
            my_dot.index = 0;
        }
        else if (my_dot.index > 15)
        {
            my_dot.index = 15;
        }
    }
    else if (piece == 1)
    {
        my_square.index += increment;
        my_square.index += increment;
        if (my_square.index < 0)
        {
            my_square.index = 0;
        }
        else if (my_square.index > 15)
        {
            my_square.index = 15;
        }
    }
    else if (piece == 2)
    {
        my_l.index += increment;
        my_l.index += increment;
        if (my_l.index < 0)
        {
            my_l.index = 0;
        }
        else if (my_l.index > 15)
        {
            my_l.index = 15;
        }
    }
}

void clear_LATS()
{

    my_piece_state.pattern_LATC = 0x00;
    my_piece_state.pattern_LATD = 0x00;
    my_piece_state.pattern_LATE = 0x00;
    my_piece_state.pattern_LATF = 0x00;
}
void handle_position(int increment)
{

    clear_LATS();

    if (piece == 0)
    {
        my_dot.position += increment;
        if (my_dot.position < 0)
        {
            my_dot.position = 0;
        }
        else if (my_dot.position > 3)
        {
            my_dot.position = 3;
        }
    }
    else if (piece == 1)
    {
        my_square.position += increment;
        if (my_square.position < 0)
        {
            my_square.position = 0;
        }
        else if (my_square.position > 2)
        {
            my_square.position = 2;
        }
    }
    else if (piece == 2)
    {
        my_l.position += increment;
        if (my_l.position < 0)
        {
            my_l.position = 0;
        }
        else if (my_l.position > 2)
        {
            my_l.position = 2;
        }
    }
}

void move_piece_right()
{
    handle_position(1);
}

void move_piece_left()
{
    handle_position(-1);
}

void move_piece_up()
{
    handle_index(-1);
}

void move_piece_down()
{
    handle_index(1);
}

int countSetBits(uint8_t n)
{
    int count = 0;
    while (n)
    {
        count += n & 1;
        n >>= 1;
    }
    return count;
}

void setLatBasedOnPieceAndSubmission()
{

    unsigned char computed_LATC = 0;
    unsigned char computed_LATD = 0;
    unsigned char computed_LATE = 0;
    unsigned char computed_LATF = 0;

    for (int i = 0; i < 8; i++)
    {
        unsigned char mask = 1 << i;
        if ((my_piece_state.pattern_LATC & mask) || (my_submission_state.pattern_LATC & mask))
        {
            computed_LATC |= mask;
        }
    }

    for (int i = 0; i < 8; i++)
    {
        unsigned char mask = 1 << i;
        if ((my_piece_state.pattern_LATD & mask) || (my_submission_state.pattern_LATD & mask))
        {
            computed_LATD |= mask;
        }
    }

    for (int i = 0; i < 8; i++)
    {
        unsigned char mask = 1 << i;
        if ((my_piece_state.pattern_LATE & mask) || (my_submission_state.pattern_LATE & mask))
        {
            computed_LATE |= mask;
        }
    }

    for (int i = 0; i < 8; i++)
    {
        unsigned char mask = 1 << i;
        if ((my_piece_state.pattern_LATF & mask) || (my_submission_state.pattern_LATF & mask))
        {
            computed_LATF |= mask;
        }
    }

    LATC = computed_LATC;
    LATD = computed_LATD;
    LATE = computed_LATE;
    LATF = computed_LATF;
    counter = countSetBits(my_submission_state.pattern_LATC) + countSetBits(my_submission_state.pattern_LATD) + countSetBits(my_submission_state.pattern_LATE) + countSetBits(my_submission_state.pattern_LATF);
}

void checkPORTAChanges(void)
{
    uint8_t currentStatePORTA = PORTA;
    if (currentStatePORTA != prevStatePORTA)
    {

        if ((currentStatePORTA & 0x01) != (prevStatePORTA & 0x01))
        {

            move_piece_right();
        }
        if ((currentStatePORTA & 0x02) != (prevStatePORTA & 0x02))
        {

            move_piece_up();
        }

        if ((currentStatePORTA & 0x04) != (prevStatePORTA & 0x04))
        {

            move_piece_down();
        }
        if ((currentStatePORTA & 0x08) != (prevStatePORTA & 0x08))
        {

            move_piece_left();
        }

        prevStatePORTA = currentStatePORTA;
        setLatBasedOnPieceAndSubmission();
    }
}

void shadowLPiece()
{
    if (my_l.empty_cell == 0)
    {

        shadow_cell(&my_l.pattern_first[my_l.index], 0);
    }
    else if (my_l.empty_cell == 1)
    {

        shadow_cell(&my_l.pattern_second[my_l.index], 0);
    }
    else if (my_l.empty_cell == 2)
    {

        shadow_cell(&my_l.pattern_first[my_l.index], 1);
    }
    else if (my_l.empty_cell == 3)
    {

        shadow_cell(&my_l.pattern_second[my_l.index], 1);
    }
}

void HandleUserSubmission()
{

    if (piece == 0)
    {
        if (my_dot.position == 0)
        {
            if ((my_dot.pattern[my_dot.index] & my_submission_state.pattern_LATC) == 0)
            {
                my_submission_state.pattern_LATC |= my_dot.pattern[my_dot.index];

                increment_update += 1;
            }
        }
        else if (my_dot.position == 1)
        {
            if ((my_dot.pattern[my_dot.index] & my_submission_state.pattern_LATD) == 0)
            {
                my_submission_state.pattern_LATD |= my_dot.pattern[my_dot.index];

                increment_update += 1;
            }
        }
        else if (my_dot.position == 2)
        {
            if ((my_dot.pattern[my_dot.index] & my_submission_state.pattern_LATE) == 0)
            {
                my_submission_state.pattern_LATE |= my_dot.pattern[my_dot.index];

                increment_update += 1;
            }
        }
        else if (my_dot.position == 3)
        {

            if ((my_dot.pattern[my_dot.index] & my_submission_state.pattern_LATF) == 0)
            {
                my_submission_state.pattern_LATF |= my_dot.pattern[my_dot.index];

                increment_update += 1;
            }
        }
    }
    else if (piece == 1)
    {
        if (my_square.position == 0)
        {
            if ((my_square.pattern_first[my_square.index] & my_submission_state.pattern_LATC) == 0 &&
                (my_square.pattern_second[my_square.index] & my_submission_state.pattern_LATD) == 0)
            {
                my_submission_state.pattern_LATC |= my_square.pattern_first[my_square.index];
                my_submission_state.pattern_LATD |= my_square.pattern_second[my_square.index];

                increment_update += 4;
            }
        }
        else if (my_square.position == 1)
        {
            if ((my_square.pattern_first[my_square.index] & my_submission_state.pattern_LATD) == 0 &&
                (my_square.pattern_second[my_square.index] & my_submission_state.pattern_LATE) == 0)
            {
                my_submission_state.pattern_LATD |= my_square.pattern_first[my_square.index];
                my_submission_state.pattern_LATE |= my_square.pattern_second[my_square.index];

                increment_update += 4;
            }
        }
        else if (my_square.position == 2)
        {
            if ((my_square.pattern_first[my_square.index] & my_submission_state.pattern_LATE) == 0 &&
                (my_square.pattern_second[my_square.index] & my_submission_state.pattern_LATF) == 0)
            {
                my_submission_state.pattern_LATE |= my_square.pattern_first[my_square.index];
                my_submission_state.pattern_LATF |= my_square.pattern_second[my_square.index];

                increment_update += 4;
            }
        }
    }
    else if (piece == 2)
    {
        shadowLPiece();
        if (my_l.position == 0)
        {
            if ((my_l.pattern_first[my_l.index] & my_submission_state.pattern_LATC) == 0 &&
                (my_l.pattern_second[my_l.index] & my_submission_state.pattern_LATD) == 0)
            {
                my_submission_state.pattern_LATC |= my_l.pattern_first[my_l.index];
                my_submission_state.pattern_LATD |= my_l.pattern_second[my_l.index];

                increment_update += 3;
            }
        }
        else if (my_l.position == 1)
        {
            if ((my_l.pattern_first[my_l.index] & my_submission_state.pattern_LATD) == 0 &&
                (my_l.pattern_second[my_l.index] & my_submission_state.pattern_LATE) == 0)
            {
                my_submission_state.pattern_LATD |= my_l.pattern_first[my_l.index];
                my_submission_state.pattern_LATE |= my_l.pattern_second[my_l.index];

                increment_update += 3;
            }
        }
        else if (my_l.position == 2)
        {
            if ((my_l.pattern_first[my_l.index] & my_submission_state.pattern_LATE) == 0 &&
                (my_l.pattern_second[my_l.index] & my_submission_state.pattern_LATF) == 0)
            {
                my_submission_state.pattern_LATE |= my_l.pattern_first[my_l.index];
                my_submission_state.pattern_LATF |= my_l.pattern_second[my_l.index];

                increment_update += 3;
            }
        }

        originalize_pattern();
    }
}

__interrupt(high_priority) void HandleInterrupt()
{

    if (INTCONbits.TMR0IF)
    /*
    Check the Timer0 Overflow Flag (INTCONbits.TMR0IF):
     The if statement checks
    if the Timer0 Overflow Interrupt Flag (TMR0IF) is set. This flag is set by
     hardware when Timer0 overflows, i.e., when it counts beyond its maximum value
      and rolls over back to zero. An overflow indicates that a specific period has
       elapsed, determined by the timer's start value and its configuration.
    */
    {
        INTCONbits.TMR0IF = 0;
        /*
        Clear the Timer0 Overflow Flag:
        Inside the if block, INTCONbits.TMR0IF = 0;
         clears the TMR0IF flag. This is a crucial step because the flag must be manually
          cleared by software to allow subsequent overflow events to trigger the interrupt
          again. If this flag is not cleared, the interrupt will not be triggered again,
           effectively disabling the Timer0 overflow interrupt functionality.
        */

        TMR0H = T_PRELOAD_HIGH;
        TMR0L = T_PRELOAD_LOW;
        /*
        Pre-load Timer0 with a Start Value:
        The next two lines, TMR0H = T_PRELOAD_HIGH; and TMR0L = T_PRELOAD_LOW;, preload Timer0
         with a specific start value. Timer0 is a 16-bit timer, consisting of two 8-bit registers
         (TMR0H for the high byte and TMR0L for the low byte). Preloading the timer with a specific
         value shortens the count to overflow, allowing precise control over the timer's period.
         The actual period of the timer before it overflows again depends on the preloaded value,
         the timer's prescaler setting, and the microcontroller's clock frequency.
        */

        if (piece == 0)
        {
            if (my_dot.position == 0)
            {
                my_piece_state.pattern_LATC = my_dot.pattern[my_dot.index];
            }
            else if (my_dot.position == 1)
            {
                my_piece_state.pattern_LATD = my_dot.pattern[my_dot.index];
            }
            else if (my_dot.position == 2)
            {
                my_piece_state.pattern_LATE = my_dot.pattern[my_dot.index];
            }
            else if (my_dot.position == 3)
            {
                my_piece_state.pattern_LATF = my_dot.pattern[my_dot.index];
            }

            my_dot.index += 1;
        }
        else if (piece == 1)
        {
            if (my_square.position == 0)
            {
                my_piece_state.pattern_LATC = my_square.pattern_first[my_square.index];
                my_piece_state.pattern_LATD = my_square.pattern_second[my_square.index];
            }
            else if (my_square.position == 1)
            {
                my_piece_state.pattern_LATD = my_square.pattern_first[my_square.index];
                my_piece_state.pattern_LATE = my_square.pattern_second[my_square.index];
            }
            else if (my_square.position == 2)
            {
                my_piece_state.pattern_LATE = my_square.pattern_first[my_square.index];
                my_piece_state.pattern_LATF = my_square.pattern_second[my_square.index];
            }

            my_square.index += 1;
        }
        else if (piece == 2)
        {
            shadowLPiece();

            if (my_l.position == 0)
            {
                my_piece_state.pattern_LATC = my_l.pattern_first[my_l.index];
                my_piece_state.pattern_LATD = my_l.pattern_second[my_l.index];
            }
            else if (my_l.position == 1)
            {
                my_piece_state.pattern_LATD = my_l.pattern_first[my_l.index];
                my_piece_state.pattern_LATE = my_l.pattern_second[my_l.index];
            }
            else if (my_l.position == 2)
            {
                my_piece_state.pattern_LATE = my_l.pattern_first[my_l.index];
                my_piece_state.pattern_LATF = my_l.pattern_second[my_l.index];
            }

            originalize_pattern();
            my_l.index += 1;
        }
        else
        {

            ;
        }

        if (my_dot.index == 16)
        {
            reinitialize_my_dot();
            piece = 1;
        }
        else if (my_square.index == 16)
        {
            reinitialize_my_square();
            piece = 2;
        }
        else if (my_l.index == 16)
        {
            reinitialize_my_l();

            piece = 0;
        }
        else
        {

            ;
        }
    }
    if (INTCONbits.RBIF)
    {

        uint8_t currentRB5 = PORTBbits.RB5;
        uint8_t currentRB6 = PORTBbits.RB6;

        if (currentRB5 != prevRB5)
        {

            if (my_l.empty_cell == 0)
            {
                my_l.empty_cell = 1;
            }
            else if (my_l.empty_cell == 1)
            {
                my_l.empty_cell = 2;
            }
            else if (my_l.empty_cell == 2)
            {
                my_l.empty_cell = 3;
            }
            else if (my_l.empty_cell == 3)
            {
                my_l.empty_cell = 0;
            }
            else
            {

                ;
            }
            prevRB5 = currentRB5;
        }

        if (currentRB6 != prevRB6)
        {

            HandleUserSubmission();
            prevRB6 = currentRB6;
        }

        INTCONbits.RBIF = 0;
    }
    else
    {

        ;
    }
    setLatBasedOnPieceAndSubmission();
}

void InitializeTheObjects()
{

    ;
}

__interrupt(low_priority) void HandleInterrupt2()
{
}

void InitSevenSeg()
{

    LATH = 0x00;
    PORTH = 0x00;
    TRISH = 0x00;

    LATJ = 0x00;
    PORTJ = 0x00;
    TRISJ = 0x00;
}

void main()
{
    Init();

    __delay_ms(1000);

    InitializeTimerAndInterrupts();
    InitializeTheObjects();
    InitSevenSeg();

    while (1)
    {

        checkPORTAChanges();
        send7seg0(counter);
        send7seg1(counter);
    }
}
