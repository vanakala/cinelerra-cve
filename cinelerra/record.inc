#ifndef RECORD_INC
#define RECORD_INC

class RecordMenuItem;
class Record;

// Context for start_recording
// Skip start delay
#define CONTEXT_INTERACTIVE  0
// Include start delay
#define CONTEXT_BATCH        1
// Only 1 frame omitting start delay
#define CONTEXT_SINGLEFRAME  2

// States for record
#define RECORD_NOTHING 0
#define RECORD_INTRO 1
#define RECORD_CAPTURING 2

// Operations for the CAPTURING state
#define IS_DONE       0
#define IS_MONITORING 1
#define IS_RECORDING  2
#define IS_DUPLEXING  3
#define IS_PREVIEWING 4

// Record modes
#define RECORD_INFINITE 0
#define RECORD_TIMED 1
#define RECORD_LOOP 2
#define RECORD_SCENETOSCENE 3

// Record start time
#define RECORD_START 0
#define RECORD_DELAY 1





#define BATCH_DELAY 500000

#endif
