// specific to each detector
// field1 --> black sensor
// field2 --> white sensor
// filed3 --> gold sensor
// filed4 --> orange sensor
// filed5 --> silver sensor
// filed6 --> grey sensor

#define DETEC_NAME        "Detecteur CO2 black"
#define DETEC_FIELD       "field1"
#define DETEC_NUM         1

// time slot for screen on (0:0 - 24:0 always on)
#define ON_HOUR  6
#define ON_MIN   50
#define OFF_HOUR 23
#define OFF_MIN  30

// alternatve dysplay to improved by nOfOkUz
#define RINOLFI   false // override with original design only
#define ALTERN    true // for alterning with AirScore
#define AIR_SCORE false // for AirScore display only

// contain all common code

#include "capteurCO2.h"

// ***********************
