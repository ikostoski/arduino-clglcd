#define BIT_(p,b)     (b)
#define BIT(cfg)      BIT_(cfg)
#define PORT_(p,b)    (PORT ## p)
#define PORT(cfg)     PORT_(cfg)
#define PIN_(p,b)     (PIN ## p)
#define PIN(cfg)      PIN_(cfg)
#define DDR_(p,b)     (DDR ## p)
#define DDR(cfg)      DDR_(cfg)

// Explicitly use atomic SBI/CBI to avoid disabling interrupts
// in main loop. Works only on IO addressable registers
#define SET_(p_, b_)    asm volatile ("sbi %[p], %[b]\n\t" :: [p] "I" _SFR_IO_ADDR(p_), [b] "I" (b_))
#define SET(cfg)        SET_(PORT_(cfg), BIT_(cfg))
#define CLEAR_(p_,b_)   asm volatile ("cbi %[p], %[b]\n\t" :: [p] "I" _SFR_IO_ADDR(p_), [b] "I" (b_))
#define CLEAR(cfg)      CLEAR_(PORT_(cfg), BIT_(cfg))
#define TOGGLE(cfg)     PIN_(cfg) = _BV(BIT_(cfg))
#define OUTPUT_PIN(cfg) SET_(DDR_(cfg), BIT_(cfg))
#define INPUT_PIN(cfg)  CLEAR_(DDR_(cfg), BIT_(cfg))

// Timers

#define TM_LINE_A      1
#define TM_LINE_B      2
#define TM_LINE_C      3
#define TM_LINE_D      4
#define TM_LINE_(n,x)  (TM_LINE_ ## x)
#define TM_LINE(cfg)   TM_LINE_(cfg)
#define TM_BASE_(n,x)  (n)
#define TM_BASE(cfg)   TM_BASE_(cfg)
#define TM_CR_(n,x,l)  (TCCR ## n ## l)
#define TM_CR(cfg,l)   TM_CR_(cfg,l)
#define TM_OCR_(n,x)   (OCR ## n ## x)
#define TM_OCR(cfg)    TM_OCR_(cfg)
#define TM_CNT_(n,x)   (TCNT ## n)
#define TM_CNT(cfg)    TM_CNT_(cfg)
#define TM_CNT_L_(n,x) (TCNT ## n ## L)
#define TM_CNT_L(cfg)  TM_CNT_L_(cfg)
#define TM_ICR_(n,x)   (ICR ## n)
#define TM_ICR(cfg)    TM_ICR_(cfg)
#define TM_IMSK_(n,x)  (TIMSK ## n)
#define TM_IMSK(cfg)   TM_IMSK_(cfg)

#define TM_PSC1_(n,x)          (1 << CS ## n ## 0)
#define TM_PSC1(cfg)           TM_PSC1_(cfg)
#define TM_WGM_(n,x,b)         (1<<WGM ## n ## b) 
#define TM_WGM(cfg,b)          TM_WGM_(cfg,b)

#define TM_COM_TOGGLE_(n,x)    (1<<COM ## n ## x ## 0)
#define TM_COM_TOGGLE(cfg)     TM_COM_TOGGLE_(cfg)
#define TM_COM_CLEAR_(n,x)     (1<<COM ## n ## x ## 1)
#define TM_COM_CLEAR(cfg )     TM_COM_CLEAR_(cfg)
#define TM_COM_SET_(n,x)       (1<<COM ## n ## x ## 1) | (1<<COM ## n ## x ## 0)
#define TM_COM_SET(cfg)        TM_COM_SET_(cfg)
