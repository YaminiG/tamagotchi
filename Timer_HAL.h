//------------------------------------------
// TIMER API
// Also known as TIMER HAL (Hardware Abstraction Layer)
// HAL is a specific form of API that designs the interface with a certain hardware



// Functions related to 32-bit timer. Notice how we change the name to a generic name.
// Initializes the 32-bit timer, named Timer 1
void InitTimer1();
void InitTimer2();

// Starts the 32-bit timer for 100ms in one-shot mode
void Timer2StartOneShot100ms();

// Checks to see the 32-bit timer is expired (finished counting) or not.
int Timer2Expired();

void Init10sTimer();
void StartOneShot10sTimer();
int OneShot10sTimerExpired();


void Init3sTimer();
void StartOneShot3sTimer();
int OneShot3sTimerExpired();

