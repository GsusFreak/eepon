
#ifndef EPONSIM_H
#define EPONSIM_H

#include <csim.h>

/*
 * Macro definitions
 */
double      maximumGrantCycle;
#define MAX(A,B) ((A) > (B) ? (A):(B))
#define MIN(A,B) ((A) < (B) ? (A):(B))

#ifdef DEBUG_TRC_HI
#define DEBUG_TRC
#define DEBUG_TRC_LO
#endif

#ifdef DEBUG_TRC
#define DEBUG_TRC_LO
#endif

#ifdef DEBUG_ONU
#define DEBUG_ONU
#endif

#define MAX_SS_SIM_TIME 4000

#define CI_HALFWIDTH_PERCENTAGE 0.8

/* Preamble/IPG in bytes */
#define PREAMBLE_IPG_BYTES  20     /* 7-byte preamble, 1-byte SFD, and 12-byte IPG */

/* Maximum ONU buffer size before halting simulation */
#define MAX_PKT_BUF_SIZE  2e9
// #define MAX_PKT_BUF_SIZE 2e15

/* Maximum FATAL Error allowed before halting simulation */
#define MAX_FATAL_ERRORS  1

/* FATAL Error Codes */
#define FATAL_CAUSE_BUFFER_OVR    0
#define FATAL_CAUSE_MAC_CONTENTION  1
#define FATAL_CAUSE_STRAY_PKT   2
#define FATAL_CAUSE_INV_WA    3
#define FATAL_CAUSE_GATE_TOO_SMALL    4
#define FATAL_CAUSE_NO_GRANT    5
#define FATAL_CAUSE_NO_MEM    6
#define FATAL_CAUSE_MISC    10
#define FATAL_CAUSE_LENGTH_VIDEO_BUFFER_OVR   20
#define FATAL_CAUSE_LENGTH_DATA_BUFFER_OVR    21

#define MAX_ONU      64
#define MAX_NUM_RUN  40
#define MAX_NUM_LOAD 4

/* Maximum size of trace capability */
#define MAX_TRACE_VALUES    1500

/* constants for NULL values */
#define ONU_NULL  -1
#define LAMBDA_NULL -1

/* Lambda Map constants */
#define LAMBDA_TRUE     1
#define LAMBDA_FALSE    0

/* Fixed Propagation Delay definitions */
#define FIXED_PROP_DELAY_OFF  0
#define FIXED_PROP_DELAY_ON     1

/* Simulation Trace definitions */
#define SIM_TRACE_OFF 0
#define SIM_TRACE_ON  1

/* Video Queue Service definitions */
#define NOT_SERVICED  0
#define SERVICED  1

/* Get Tail definitions */
#define GET_TAIL_OFF  0
#define GET_TAIL_ON     1

/* Traffic Type definitions */
#define TRAFFIC_POISSON       0
#define TRAFFIC_SELF_SIMILAR    1

#define AVG_PKT_SIZE  493.7

/* ONU Scheduling Pool States */
#define ONU_SCHED_INACTIVE  0
#define ONU_SCHED_ACTIVE    1
#define ONU_SCHED_IMMINENT  2

/* Optimization Scheduler definitions */
#define COST_INFINITE     (MAXINT/2)
#define COST_WAVELENGTH_SWITCH    100
#define COST_TSLOT_PENALTY    10
#define COST_DUMMY      ((COST_INFINITE) - 1)

#define MAX_SCHEDULER_NUM_SLOT    1000

#define BOBB 5000

/*
 * Some useful macros
 */
/* Setup lambda free temp data structure */
#define mSETUP_TEMP_LAMBDA_FREE(loopVar) \
  do { \
  for(loopVar = 0; loopVar < simParams.NUM_LAMBDAS; loopVar++) \
  { \
    lambdaFreeTemp[loopVar] = lambdaFree[loopVar]; \
  } } while(0)
/* Update lambda free array to current sim time */
#define mUPDATE_LAMBDA_FREE(loopVar,lambdaFreeArray) \
  do { \
  for(loopVar = 0; loopVar < simParams.NUM_LAMBDAS; loopVar++) \
  { \
    if(lambdaFreeArray[loopVar] < simtime()) \
    { \
      lambdaFreeArray[loopVar] = simtime(); \
    } \
  } } while(0)
    
typedef enum
{
  PILOT_RUN,
  ACTUAL_RUN,
  TAIL_RUN,
  TIME_TRACE
} eSIM_TYPE;

typedef enum
{
  END_TYPE_TIME,
  END_TYPE_TRAFFIC
} eEND_TYPE;

typedef enum {SLEEP_SCHEDULER_JOURNAL, SLEEP_SCHEDULER_HEAVY_TRAFFIC, SLEEP_SCHEDULER_HEAVY_TRAFFIC_HYBRID} eSLEEP_SCHEDULER;

/*
 * Type definitions
 */

typedef struct
{
  /* parameters directly set */
  int       TRAFFIC_TYPE;
  int       NUM_RUNS;         /* Number of independent runs */
  int       NUM_ONU;    /* Number of ONUs on PON */
  int       NUM_HEAVY_ONU;    /* Number of heavily loaded ONUs */
  int       NUM_SS_STREAMS;   /* Number of Self Similar Traffic Generator Streams */
  double    SS_HURST_PARAM;   /* Hurst Parameter for Self-Similar Traffic Generator */
  double    LINK_SPEED;   /* link speed in bps */
  double    DESIRED_LOAD;   /* desired traffic load */
  double    PIPG_LOAD;    /* traffic load with Preamble/IPG */
  int       HEAVY_LOAD;       /* heavy load (measured in number of times greater than a lightly loaded ONU) */
  double    TUNING_TIME;    /* tuning time for tunable lasers */
  double    SIM_TIME;         /* total simulation time */
  double    START_LOAD;
  double    END_LOAD;
  double    LOAD_INCR;
  long      RAND_SEED;
  double    SIM_TRACE_TIMESCALE;
  double    SIM_TRACE_TIME;
  int       SIM_TRACE;
  int       GET_TAIL;   /* Get tails for histogram (each simulation will run twice!!!) */
  double    OLT_FRAME_TIME;
  
  eSIM_TYPE simType;
  double    ACTIVE_POWER_CONSUMPTION;
  double    IDLE_POWER_CONSUMPTION;
  double    SLEEP_POWER_CONSUMPTION;
  double    PROBE_POWER_CONSUMPTION;
  double    WAKEUP_POWER_CONSUMPTION;

  double    ONU_TIME_SLEEP;
  double    ONU_TIME_TRIGGER;
  double    ONU_TIME_WAKEUP;
  double    ONU_TIME_PROBE;

  int       ONU_GRANTED;
  double    TIME_PER_GRANT;
  int       boolRecordStateTime;
  eEND_TYPE endType;
  eSLEEP_SCHEDULER sleepScheduler;

  int     NUM_PARTS;    /* Number of bandwidth partitions */
  double  LINK_SPEED_PER_PART;  /* in bps */
  double  AVG_PKT_INTER_ARVL_TIME;
  double  AVG_PKT_INTER_ARVL_TIME_HEAVY;
  double  TIME_PER_BYTE;
  double  PREAMBLE_IPG_TIME;
  /* Self Similar Traffic Generator Parameters */
  double  SS_PARETO_LOC_PARAM;
  double  SS_OFF_LOC_PARAM;
  double  SS_OFF_LOC_PARAM_HEAVY;
  double  SS_PARETO_SHAPE_PARAM;
  
  //double  LINK_SPEED_PER_SS_STREAM;
  double    AVG_BURST_SIZE;
  double    AVG_T_ON;
  double    AVG_T_OFF;
  double    AVG_T_OFF_HEAVY;
  int       FIXED_PROP_DELAY;
  double    FIXED_PROP_DELAY_VALUE;
  double    MAX_PROP_DELAY;
  double    ACTUAL_MAX_PROP_DELAY;
  double    ACTUAL_MIN_PROP_DELAY;
  double    ONU_PROP[MAX_ONU];    /* ONU to OLT propagation delay values */

  int       ONUTrafficScalar[MAX_ONU];
  double    ONUTrafficScalar_link_speed[MAX_ONU];
  int       ONUTrafficScalar_length;
  double    AVG_PKT_INTER_ARVL_TIME_ONU[MAX_ONU];
} sSIM_PARAMS;

/* Packet entity data structure */
typedef struct entity_pkt
{
  double  creationTime;
  double  transmissionTime;
  double  arrivalTime;
  int     size;
  char    frameType;
  int     forecastSize;
  double  frameTimeStamp;
  int     forecastPktNumber;
  int     onuNum;
  struct  entity_pkt *next;
  struct  entity_pkt *prev;
} sENTITY_PKT;

/* OLT attribute structure */
typedef struct
{
  sENTITY_PKT   *packetsHead;
  sENTITY_PKT   *packetsTail;
  double        packetQueueSize;  /* Packet Queue Size (in bytes) */
  unsigned long packetQueueNum;   /* # of Packets in Queue */
  double        minArrivalTime;   /* Minimum arrival time in Queue */
  double        avgArrivalTime;   /* Average arrival time in Queue */
  MBOX          pktMailbox;
  int           tslotStart;
  TABLE         transmitTimeTable;
  TABLE         queueTimeTable;
  TABLE         queueLengthTable;
  STREAM        pktInterArrivalStream;
  STREAM        pktSizeStream;
  STREAM        burstSizeStream;
  double        transmitByteCnt;
  TABLE         transmitThroughput;
  int           lastONUServiced;
} sOLT;


/* ONU attribute structure */
typedef enum {ONU_ST_ACTIVE, ONU_ST_IDLE, ONU_ST_SLEEP, ONU_ST_PROBE, ONU_ST_WAKEUP, FINAL_eONU_STATE_ENTRY} eONU_STATE;

typedef struct
{
  double      minArrivalTime;   /* Minimum arrival time in Queue */
  double      avgArrivalTime;   /* Average arrival time in Queue */
  double      latency;
  double      rtt;
  int         tslotStart;
  STREAM      burstSizeStream;
  double      transmitByteCnt;
  TABLE       transmitThroughput;
  
  eONU_STATE  state;
  double      timeInState[FINAL_eONU_STATE_ENTRY];
  double      timeStateStarted;
  int         cntState[FINAL_eONU_STATE_ENTRY]; 
  double      heavy_traffic_sleep_duration;
  
  int         queuesize;
  int         disableQueueTracking;
} sONU;

/* Schedueing Pool Data Structure */
typedef struct
{
  int     state;      /* ONU activity state (2 - imminent, 1 - in scheduling pool, 0 - inactive) */
  double  poolTime;   /* simulation time entered pool */
  int     gateLambda; /* scheduled lambda */
  double  gateStart;  /* scheduled start */
  double  gateLength; /* scheduled length */
  int     rounds;     /* age in scheduling rounds */
} sSCHED_POOL;

typedef struct
{
  double      estimatedMean;
  double      intervalLower;
  double      intervalUpper;
  unsigned long   insideIntervalCount;
  unsigned long   outsideIntervalCount;
  double      confidenceLevel;
} sSS_STAT;

/* Statistic Estimation Structure */
typedef struct
{
  double  meanEst;
  double  minEst;
  double  maxEst;
} sSTAT_EST;


eSIM_TYPE simType;

typedef struct
{
  int     onuNum;
  double  startTime;
  double  length;
} sGRANT_TRC;

/* 
 * Integer Linked List structure
 */
struct onu_list
{
  int            onuNum;
  int            posNum;
  unsigned long  grantLen;  /* in bytes */
  unsigned long  numFrames; /* in frames */
  double         grantTime;  /* in seconds */
  double         poolTime; /* in seconds */
  double         minArrivalTime; /* in seconds */
  double         avgArrivalTime; /* in seconds */
  double         latency;  /* in seconds */
  struct         onu_list *next;
};
typedef struct onu_list sONU_LIST;

typedef enum {SORT_ASCENDING_ORDER, SORT_DESCENDING_ORDER} eSORT_METHOD;

typedef enum {SORT_ONU_NUM, SORT_POS_NUM, SORT_GRANT_LEN, SORT_NUM_FRAMES, SORT_GRANT_TIME, SORT_POOL_TIME, SORT_MIN_ARRIVAL, SORT_AVG_ARRIVAL, SORT_PROP_DELAY} eSORT_CRITERIA;

typedef enum {SORT_EQ, SORT_GT, SORT_LT} eSORT_EQUAL;

/*
 * Global variable declarations
 */
extern char procId[20];

/* simulation parameters data structure */
extern sSIM_PARAMS simParams;

/* OLT attributes structure array */
extern sOLT oltAttrs;
extern sONU onuAttrs[MAX_ONU];

/* a flag to signal a graceful termination of the simulation */
extern int terminateSim;
extern int simInitPhase;

/* count of fatal errors in the simulation */
extern int fatalErrorCount;
extern int fatalErrorCode;

/* Trace messaging buffers */
extern char sim_msg_buf[10000];
extern char dump_msg_buf[1000];


/* Empirical Packet Size Distribution */
#define EMPIRICAL_SIZE  4
extern double   EMPIRICAL_PROB[5];
extern double   EMPIRICAL_VALUE[5];
extern double   EMPIRICAL_CUTOFF[5];
extern long     EMPIRICAL_ALIAS[5];

/* CSIM Data Collection Variables */
extern TABLE    overallQueueDelay;
//extern TABLE    ONUQueueDelay[MAX_ONU];
extern TABLE    cycleQueueDelay;
extern TABLE    heavyQueueDelay;
extern TABLE    lightQueueDelay;

extern TABLE    overallQueueLength;
//extern TABLE    ONUQueueLength[MAX_ONU];
extern TABLE    heavyQueueLength;
extern TABLE    lightQueueLength;

extern sSTAT_EST  overallQueueDelayEst;
//extern sSTAT_EST  ONUQueueDelayEst[MAX_ONU];
extern sSTAT_EST  heavyQueueDelayEst;
extern sSTAT_EST  lightQueueDelayEst;

extern sSTAT_EST  overallQueueLengthEst;
//extern sSTAT_EST  ONUQueueLengthEst[MAX_ONU];
extern sSTAT_EST  heavyQueueLengthEst;
extern sSTAT_EST  lightQueueLengthEst;

extern sSS_STAT   overallQueueDelayStat;
//extern sSS_STAT   ONUQueueDelayStat[MAX_ONU];
extern sSS_STAT   heavyQueueDelayStat;
extern sSS_STAT   lightQueueDelayStat;

extern sSS_STAT   overallQueueLengthStat;
//extern sSS_STAT   ONUQueueLengthStat[MAX_ONU];
extern sSS_STAT   heavyQueueLengthStat;
extern sSS_STAT   lightQueueLengthStat;

extern EVENT SERVICE_OLT;
extern EVENT PACKET_ARRIVED[MAX_ONU];
extern EVENT ONU_HAS_NO_QUEUED_PACKETS[MAX_ONU];
extern EVENT HEAVY_TRAFFIC_SLEEP_TRIGGERED[MAX_ONU];

/* Reset throughput flag */
extern int    reset_throughput_flag;
/* Throughput Fairness data structures */
extern double   actual_tput[MAX_ONU];
extern double   actual_tput_olt;
extern double   ideal_tput[MAX_ONU];
extern double   ideal_tput_olt;
extern TABLE    throughputFairness;

/* Structures used for sorting ONUs */
/* This array is used for ordering ONUs for scheduling in LFJ order or in wavelength assignment order */
extern sONU_LIST *scheduleList;

/* Scheduling Pool structure array */
extern      sSCHED_POOL schedPool[MAX_ONU];
extern int  schedPoolCount;

/* Array of weights for changing the traffic loads between ONUs */
//extern int  ONUTrafficScalar[MAX_ONU];

/*
 * Function declarations
 */
/* Sim core dump */
void dump_sim_core();

/*----------------David's Troubleshooting----------------*/

// Enable printing to a structured file
#define EnableTroubleshooting_v1  1

// Enable printing to a sequential file
#define EnableTroubleshooting_v2  1

// Enable printing of process statuses
// #define EnableTroubleshooting_v3 1

// Enable ONU printing
// #define EnableTroubleshooting_v4 1

// Define Test Variable Types
typedef struct
{
  double  main_start,
          main_finish,
          read_sim_cfg_file_start,
          read_sim_cfg_file_finish,
          heartbeat_process,
          // That's [run# (probably only 1)][load level (probably 0.1 to 0.9)][PilotRun == 0, ActualRun == 1]
          main_test[MAX_NUM_RUN][MAX_NUM_LOAD],
          main_begin_load[MAX_NUM_RUN][MAX_NUM_LOAD],
          main_end_load[MAX_NUM_RUN][MAX_NUM_LOAD],
          sim_time_per_load[MAX_NUM_RUN][MAX_NUM_LOAD],
          sim_time_per_load_start[MAX_NUM_RUN][MAX_NUM_LOAD],
          sim_start[MAX_NUM_RUN][MAX_NUM_LOAD][2],
          sim_process[MAX_NUM_RUN][MAX_NUM_LOAD][2],
          sim_before_ONU_processes[MAX_NUM_RUN][MAX_NUM_LOAD][2],
          sim_finish[MAX_NUM_RUN][MAX_NUM_LOAD][2],
          sim_finish2[MAX_NUM_RUN][MAX_NUM_LOAD][2],
          sim_ctrl_simType[MAX_NUM_RUN][MAX_NUM_LOAD][2],
          sim_ctrl_testProbe[MAX_NUM_RUN][MAX_NUM_LOAD][2],
          data_pkt_created[MAX_NUM_RUN][MAX_NUM_LOAD][MAX_ONU],
          data_pkt_destroyed[MAX_NUM_RUN][MAX_NUM_LOAD][MAX_ONU],
          data_pkt_created_olt[MAX_NUM_RUN][MAX_NUM_LOAD],
          data_pkt_destroyed_olt[MAX_NUM_RUN][MAX_NUM_LOAD];
  int     loadOrderCounter,
          runNum;
} sIndicators;

// Declare Global Test Variables
sIndicators test_vars;
FILE  *indicatorFile;

#ifdef  EnableTroubleshooting_v2
FILE  *TSstream;
#endif

#ifdef  EnableTroubleshooting_v3
FILE  *status_processes_temp1;
#endif

#ifdef  EnableTroubleshooting_v4
FILE  *ONU_files[32];
#endif

FILE  *PDHstream;

FILE  *droppedScalPackets;

#define DATA_QUEUE_MAX_LENGTH   400000
#define VIDEO_QUEUE_MAX_LENGTH  400000

// Temporarily hard-code the moving average window size
#define Length_overallVideoQueueDelay_MovingAverage   5
#define LowerBound_scalableDropping   0.0005
#define UpperBound_scalableDropping   0.003

// Declare Troubleshooting Function Prototypes
void test_var_print(void);
void test_var_init(void);
void status_processes_print(void);
int  file_exists(const char*);
void find_num_layers();
void find_num_frames();
void trace_on(void);
void trace_off(void);
void set_trace_file(FILE* f);
void set_output_file(FILE* f);
void status_processes(void);
int  TSprint(const char*, ...);
int  PDHprint(const char*, ...);
int  ONUprint(int, const char*, ...);

/*----------------End David's Troubleshooting----------------*/

#endif



