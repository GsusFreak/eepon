
#include <values.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <csim.h>
#include <float.h>
#include "eponsim.h"
#include "eponsim_util.h"
#include "eponsim_stats.h"
#include "eponsim_traffic.h"
#include "eponsim_onu.h"
#include "eponsim_olt.h"
#include "eponsim_prop.h"



/* Declare all of the event objects */
EVENT SIM_END_EVENT;
EVENT SERVICE_OLT;
EVENT PACKET_ARRIVED[MAX_ONU];
EVENT ONU_HAS_NO_QUEUED_PACKETS[MAX_ONU];
EVENT HEAVY_TRAFFIC_SLEEP_TRIGGERED[MAX_ONU];

/* simulation parameters data structure */
sSIM_PARAMS simParams;

/* ONU attributes structure array */
sOLT oltAttrs;
sONU onuAttrs[MAX_ONU];

/* Scheduling Pool structure array */
sSCHED_POOL schedPool[MAX_ONU];
int schedPoolCount;

/* Throughput Fairness data structures */
double actual_tput[MAX_ONU];
double actual_tput_olt;
double ideal_tput[MAX_ONU];
double ideal_tput_olt;

TABLE   throughputFairness;

TABLE   overallQueueDelay;
TABLE   cycleQueueDelay;
TABLE   heavyQueueDelay;
TABLE   lightQueueDelay;

TABLE   overallQueueLength;
TABLE   heavyQueueLength;
TABLE   lightQueueLength;

sSTAT_EST overallQueueDelayEst;
sSTAT_EST heavyQueueDelayEst;
sSTAT_EST lightQueueDelayEst;

sSTAT_EST overallQueueLengthEst;
sSTAT_EST heavyQueueLengthEst;
sSTAT_EST lightQueueLengthEst;

sSS_STAT  overallQueueDelayStat;
sSS_STAT  heavyQueueDelayStat;
sSS_STAT  lightQueueDelayStat;

sSS_STAT  overallQueueLengthStat;
sSS_STAT  heavyQueueLengthStat;
sSS_STAT  lightQueueLengthStat;

double    overallQueueDelayTrace[MAX_TRACE_VALUES];
double    simTimeTrace[MAX_TRACE_VALUES];

/* Parameters for self-similar traffic. */

int  i, j;     /* global loop variables */
char procId[20];

/* count of fatal errors in the simulation */
int fatalErrorCount;
int fatalErrorCode;

/* a flag to signal a graceful termination of the simulation */
int terminateSim;
int simInitPhase;

/* Reset throughput flag */
int reset_throughput_flag;

/* Trace messaging buffers */
char sim_msg_buf[10000];
char dump_msg_buf[1000];

double currScheduleTime[20];
double currScheduleTimeMax;
double clockTime1, clockTime2;

/*
 * Grant Trace Data Structures
 */
sGRANT_TRC grantTrace[MAX_LAMBDAS][GRANT_TRACE_SIZE];
int grantTracePtr[MAX_LAMBDAS];

/* This array is used for ordering ONUs for scheduling in LFJ order or in wavelength assignment order */
sONU_LIST *scheduleList[MAX_LAMBDAS];
sONU_LIST *scheduleListLfjSpt[MAX_LAMBDAS];
sONU_LIST *scheduleListLfjLpt[MAX_LAMBDAS];

// Declare eponsim.c Troubleshooting Variables
int   numRuns       = 1,
      numLoadLevels = 9,
      numONUs       = 32;
// FILE *traceFileDave;

int TSprint(const char *text, ...)
{
#ifdef EnableTroubleshooting_v2
  va_list args;
  int out;
  
  TSstream = fopen("B_TS_Out.txt", "a");
  va_start(args, text);
  out = vfprintf(TSstream, text, args);
  va_end(args);
  fclose(TSstream);
  return out;
#else
  return 0;
#endif
}

int ONUprint(int ONUids, const char *text, ...)
{
#ifdef EnableTroubleshooting_v4
  va_list args;
  int out;
  
  int hello = 30;
  hello += 10;
  va_start(args, text);
  out = vfprintf(ONU_files[ONUids], text, args);
  va_end(args);
  return out;
#else
  ONUids += 1;
  return 0;
#endif
}


// Print All Test Variables to File
void test_var_print()
{
#ifdef EnableTroubleshooting_v1
  indicatorFile = fopen("A_Test_Variables.txt", "w");
  
  fprintf(indicatorFile, "last_updated_at: %10.5e\n", simtime());
  fprintf(indicatorFile, "heartbeat_process: %.0f\n", test_vars.heartbeat_process);
  if(overallQueueDelay != NULL)
    fprintf(indicatorFile, "Num Queues Serviced: %ld\n", table_cnt(overallQueueDelay));
  fprintf(indicatorFile, "\n");
  fprintf(indicatorFile, "main_start  = %.0f\n", test_vars.main_start);
  fprintf(indicatorFile, "main_finish = %.0f\n", test_vars.main_finish);
  fprintf(indicatorFile, "read_sim_cfg_file_start  = %.0f\n", test_vars.read_sim_cfg_file_start);
  fprintf(indicatorFile, "read_sim_cfg_file_finish = %.0f\n", test_vars.read_sim_cfg_file_finish);
  fprintf(indicatorFile, "\n");
  int iaa;
  for (iaa = 0; iaa < numRuns; iaa++) {
    fprintf(indicatorFile, "Run #%d\n", iaa + 1);
    fprintf(indicatorFile, "\n");
    int ibb;
    for (ibb = 0; ibb < numLoadLevels; ibb++) {
      fprintf(indicatorFile, "Load #%d\n", ibb + 1);
      fprintf(indicatorFile, "sim_time        = %.0f\n", test_vars.sim_time_per_load[iaa][ibb]);
      fprintf(indicatorFile, "main_begin_load = %.0f\n", test_vars.main_begin_load[iaa][ibb]);
      fprintf(indicatorFile, "main_end_load   = %.0f\n", test_vars.main_end_load[iaa][ibb]);
      fprintf(indicatorFile, "\n");
      int icc;
      for (icc = 0; icc < 2; icc++) {
        if (icc == 0)
          fprintf(indicatorFile, "Pilot Run\n");
        if (icc == 1)
          fprintf(indicatorFile, "Actual Run\n");
        fprintf(indicatorFile, "sim_start  = %.0f\n", test_vars.sim_start[iaa][ibb][icc]);
        fprintf(indicatorFile, "sim_finish = %.0f\n", test_vars.sim_finish[iaa][ibb][icc]);
        fprintf(indicatorFile, "\n");
      }
      for (icc = 0; icc < numONUs; icc++) {
        fprintf(indicatorFile, "ONU  #%02d: ", icc+1);
        fprintf(indicatorFile, "data_pkt_created   = %.0f\n", test_vars.data_pkt_created[iaa][ibb][icc]);
        fprintf(indicatorFile, "          data_pkt_destroyed = %.0f\n", test_vars.data_pkt_destroyed[iaa][ibb][icc]);
        fprintf(indicatorFile, "\n");
      }
      fprintf(indicatorFile, "OLT:      ");
      fprintf(indicatorFile, "data_pkt_created   = %.0f\n", test_vars.data_pkt_created_olt[iaa][ibb]);
      fprintf(indicatorFile, "          data_pkt_destroyed = %.0f\n", test_vars.data_pkt_destroyed_olt[iaa][ibb]);
      fprintf(indicatorFile, "sim_time = %.0f\n", test_vars.sim_time_per_load[iaa][ibb]);
      fprintf(indicatorFile, "\n");
    }
    fprintf(indicatorFile, "\n\n");
  }

  // Print a summary of the interesting information at the bottom
  for (iaa = 0; iaa < numRuns; iaa++) {
    fprintf(indicatorFile, "-- Run #%d --", iaa + 1);
    fprintf(indicatorFile, "\n");
    int ibb;
    for (ibb = 0; ibb < numLoadLevels; ibb++) {
      fprintf(indicatorFile, "Load #%d - ", ibb + 1);
      fprintf(indicatorFile, "sim_time = %.0f\n", test_vars.sim_time_per_load[iaa][ibb]);
    }
  }
  fprintf(indicatorFile, "\n\n");
  
  fflush(indicatorFile);
  fclose(indicatorFile);
  
#endif
  return;
}

// Initialize Test Variables
void test_var_init()
{
#ifdef EnableTroubleshooting_v1
  test_vars.main_start = 0;
  test_vars.main_finish = 0;
  test_vars.read_sim_cfg_file_start = 0;
  test_vars.read_sim_cfg_file_finish = 0;
  test_vars.heartbeat_process = 0;
  int iaa; // Once for each run
  for (iaa = 0; iaa < 10; iaa++) {
    int ibb; // Once for each load level
    for (ibb = 0; ibb < 20; ibb++) {
      test_vars.sim_time_per_load[iaa][ibb] = 0;
      test_vars.sim_time_per_load_start[iaa][ibb] = 0;
      test_vars.main_begin_load[iaa][ibb] = 0;
      test_vars.main_end_load[iaa][ibb] = 0;
      test_vars.main_test[iaa][ibb] = 0;
      test_vars.data_pkt_created_olt[iaa][ibb] = 0;
      test_vars.data_pkt_destroyed_olt[iaa][ibb] = 0;
      int icc; // Once for each ONU
      for (icc = 0; icc < 64; icc++)
        test_vars.data_pkt_created[iaa][ibb][icc] = 0;
        test_vars.data_pkt_destroyed[iaa][ibb][icc] = 0;
      for (icc = 0; icc < 2; icc++) {
        test_vars.sim_start[iaa][ibb][icc] = 0;
        test_vars.sim_process[iaa][ibb][icc] = 0;
        test_vars.sim_finish[iaa][ibb][icc] = 0;
        test_vars.sim_finish2[iaa][ibb][icc] = 0;
        test_vars.sim_before_ONU_processes[iaa][ibb][icc] = 0;
        test_vars.sim_ctrl_simType[iaa][ibb][icc] = 0;
      }
    }
  }
  
  test_vars.loadOrderCounter = 0;
#endif
  return;
}

// Print the Last Three status_processes Outputs
void status_processes_print() {
#ifdef EnableTroubleshooting_v3
  // This keeps trace of the three most recent status_processes outputs.
  // temp1 is the newest, temp2 is the second newest, and temp3
  // is the oldest. Thus, temp3 must be update first.

  if(file_exists("Z_status_processes_temp3"))
    remove("Z_status_processes_temp3");
    
  if(file_exists("Z_status_processes_temp2"))
    rename("Z_status_processes_temp2","Z_status_processes_temp3");
  
  if(file_exists("Z_status_processes_temp1"))
    rename("Z_status_processes_temp1","Z_status_processes_temp2");
  
  // Update temp1
  status_processes_temp1 = fopen("Z_status_processes_temp1", "w");
  set_output_file(status_processes_temp1);
  status_processes();
  printf("------------------------------------\nlast_updated_at: %10.5e", simtime());
  set_output_file(stdout);
  fclose(status_processes_temp1);
#endif
  return;
}

void open_TS_pointers() {
#ifdef EnableTroubleshooting_v4
  char  ONU_file_name[20];
  int   ONUid;
  for (ONUid = 0; ONUid < 32; ONUid++) 
  {
    sprintf(ONU_file_name, "B_ONU_output_file_%d.txt", ONUid);
    ONU_files[ONUid] = fopen(ONU_file_name, "a");
    ONU_file_name[0] = 0;
  }
#endif
  return;
}

void close_TS_pointers() {
//#ifdef EnableTroubleshooting_v2
  //fclose(TSstream);
//#endif
#ifdef EnableTroubleshooting_v4
  int ONUid;
  for (ONUid = 0; ONUid < 32; ONUid++) {
    fclose(ONU_files[ONUid]);
  }
#endif
  return;
}

int file_exists(const char *fname) {
  FILE *file;
  file = fopen(fname, "r");
  if(file != NULL)
  {
    fclose(file);
    return 1;
  }
  return 0;
}

/* Calculate certain simulation parameters */
void calc_sim_params()
{
  double dutyCycle;
  int loopIndex;

  /* parameters calculated from specified parameters */
  simParams.NUM_PARTS     = (simParams.NUM_ONU - simParams.NUM_HEAVY_ONU) + (simParams.NUM_HEAVY_ONU*simParams.HEAVY_LOAD);
  simParams.LINK_SPEED_PER_PART   = (simParams.LINK_SPEED*simParams.DESIRED_LOAD)/((double)simParams.NUM_PARTS);  /* in bps */
  simParams.AVG_PKT_INTER_ARVL_TIME = (double)((AVG_PKT_SIZE+PREAMBLE_IPG_BYTES)*8)/simParams.LINK_SPEED_PER_PART;
  simParams.AVG_PKT_INTER_ARVL_TIME_HEAVY = (double)((AVG_PKT_SIZE+PREAMBLE_IPG_BYTES)*8)/(simParams.LINK_SPEED_PER_PART*simParams.HEAVY_LOAD);
  simParams.TIME_PER_BYTE     = (8)/simParams.LINK_SPEED; /* at 1 Mbps link speed, byte time is 8 microseconds */
  simParams.PREAMBLE_IPG_TIME   = PREAMBLE_IPG_BYTES*simParams.TIME_PER_BYTE;
  
  /* 
   * Self-Similar parameters 
   */
  /* a - shape parameter, b - location parameter */
  /* a is fixed according to hurst parameter, b is adjusted for load */
  simParams.SS_PARETO_LOC_PARAM   = (double) 1;
  simParams.SS_PARETO_SHAPE_PARAM   = 3-2*simParams.SS_HURST_PARAM;
  /* Calculate Average Burst Size, i.e., the mean of the Pareto distribution with the Shape and Location Params above */
  simParams.AVG_BURST_SIZE  = (simParams.SS_PARETO_LOC_PARAM*simParams.SS_PARETO_SHAPE_PARAM)/(simParams.SS_PARETO_SHAPE_PARAM - 1);
  //simParams.LINK_SPEED_PER_SS_STREAM  = simParams.LINK_SPEED_PER_PART/simParams.NUM_SS_STREAMS;
  /* Calculate AVG ON Period value (in seconds) */
  simParams.AVG_T_ON      = (AVG_PKT_SIZE+PREAMBLE_IPG_BYTES)*simParams.AVG_BURST_SIZE*simParams.TIME_PER_BYTE;

  /* Calculate AVG Off Period values (in seconds), this is how we control the offered load */
  /* Duty cycle should equal the desired load per SS stream */
  // Potential bug in next line, fixed in line after
  //loadPart = simParams.LINK_SPEED_PER_SS_STREAM*simParams.DESIRED_LOAD*(simParams.TIME_PER_BYTE/8);
  dutyCycle = simParams.DESIRED_LOAD/(simParams.NUM_SS_STREAMS*simParams.NUM_PARTS);

  simParams.AVG_T_OFF   = (simParams.AVG_T_ON / dutyCycle) - simParams.AVG_T_ON;
  simParams.AVG_T_OFF_HEAVY = (simParams.AVG_T_ON / (simParams.HEAVY_LOAD*dutyCycle)) - simParams.AVG_T_ON;
  /* Compute Location parameter (b) for OFF time distribution */
  simParams.SS_OFF_LOC_PARAM        = simParams.AVG_T_OFF * ((simParams.SS_PARETO_SHAPE_PARAM - 1)/simParams.SS_PARETO_SHAPE_PARAM);
  simParams.SS_OFF_LOC_PARAM_HEAVY  = simParams.AVG_T_OFF_HEAVY * ((simParams.SS_PARETO_SHAPE_PARAM - 1)/simParams.SS_PARETO_SHAPE_PARAM);

  //printf("AVG_T_ON = %g, AVG_T_OFF = %g, Duty Cycle = %g\n",simParams.AVG_T_ON, simParams.AVG_T_OFF, 
  //       simParams.AVG_T_ON/(simParams.AVG_T_ON+simParams.AVG_T_OFF));

  /* Set ideal throughput values */
  printf("\nIdeal throughput values\n");
  for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
  {
    if(loopIndex < simParams.NUM_HEAVY_ONU)
    {
      /* Heavy loaded ONU */
      ideal_tput[loopIndex] = simParams.HEAVY_LOAD*simParams.LINK_SPEED_PER_PART;
    }
    else
    {
      /* Light loaded ONU */
      ideal_tput[loopIndex] = simParams.LINK_SPEED_PER_PART;
    }
    printf("ONU #%d = %g\n",loopIndex,ideal_tput[loopIndex]);
  }
  printf("\n\n");

}

/* Initialize simulation data structures */
void init_data_structures()
{
  fatalErrorCount = 0;
  terminateSim = 0;
  schedPoolCount = 0;
  reset_throughput_flag = 0;
  oltAttrs.packetQueueSize  = 0;
  oltAttrs.packetQueueNum = 0;
  oltAttrs.transmitByteCnt  = 0;
  TSprint("Setting all onuAttrs state to IDLE\n");
  oltAttrs.packetsHead = NULL;
  oltAttrs.packetsTail = NULL;
  for(i=0; i < simParams.NUM_ONU; i++)
  {
    onuAttrs[i].latency = 0;
    onuAttrs[i].transmitByteCnt = 0;
    onuAttrs[i].state = ONU_ST_IDLE;
    onuAttrs[i].timeStateStarted = simtime();
    for(int ibb=0; ibb < FINAL_eONU_STATE_ENTRY; ibb++)
      onuAttrs[i].timeInState[ibb] = 0;
  }
  
  /* initialize time trace data structures */
  for(i=0; i < MAX_TRACE_VALUES; i++)
  {
    overallQueueDelayTrace[i] = 0;
    simTimeTrace[i]     = 0;
  }
}

/* Simulation error handler */
void sim_err_handler(long err_msg_num)
{
  FILE *simcoreFile, *pidFile;
  
  printf("Simulation Error\n");
  
  simcoreFile = fopen("sim_core","a");
  
  fprintf(simcoreFile, "SIM ERROR #%ld: %s\n\n", err_msg_num, (char *) csim_error_msg(err_msg_num));
  
  fclose(simcoreFile);

  /* Report failure in pid */
  pidFile = fopen("pid","w");
  fprintf(pidFile, "Failed\n");
  fclose(pidFile);
}

/* Dump Simulation failure data */
void dump_sim_core()
{
  FILE *simcoreFile;
  int loopIndex;
  
  fatalErrorCount++;
  
  printf("Dumping Sim Core, fatal error count = %d\n",fatalErrorCount);
  
  simcoreFile = fopen("sim_core","a");
  hold(1);

  /* Output Simulation Time */
  fprintf(simcoreFile,"simtime=%e\n", simtime());
  fflush(NULL);

  /* Report Error Type */
  switch(fatalErrorCode)
  {
    case FATAL_CAUSE_BUFFER_OVR:
      fprintf(simcoreFile,"FATAL ERROR: Buffer limit exceeded!\n");
      break;
    case FATAL_CAUSE_MAC_CONTENTION:
      fprintf(simcoreFile,"FATAL ERROR: MAC Contention!\n");
      break;
    case FATAL_CAUSE_STRAY_PKT:
      fprintf(simcoreFile,"FATAL ERROR: Stray Packet!\n");
      break;
    case FATAL_CAUSE_INV_WA:
      fprintf(simcoreFile,"FATAL ERROR: Invalid Wavelength Assignment!\n");
      break;
    case FATAL_CAUSE_GATE_TOO_SMALL:
      fprintf(simcoreFile,"FATAL ERROR: GATE too small!\n");
      break;
    case FATAL_CAUSE_NO_GRANT:
      fprintf(simcoreFile,"FATAL ERROR: No Grant Received!\n");
      break;
    case FATAL_CAUSE_LENGTH_VIDEO_BUFFER_OVR:
      fprintf(simcoreFile,"FATAL ERROR: Too Many Packets in Video Buffer; Memory at Risk\n");
      break;
    case FATAL_CAUSE_LENGTH_DATA_BUFFER_OVR:
      fprintf(simcoreFile,"FATAL ERROR: Too Many Packets in Data Buffer; Memory at Risk\n");
      break;
    case FATAL_CAUSE_NO_MEM:
      fprintf(simcoreFile,"FATAL ERROR: Out of Memory!\n");
      break;
  }
  
  fflush(NULL);

  fprintf(simcoreFile,"Context Information:\n");
  fprintf(simcoreFile,"%s",dump_msg_buf);
  fprintf(simcoreFile,"\n\n");
  fflush(NULL);
      
  /* Collect stats in files */
  fprintf(simcoreFile, "Statistics\n");
  fprintf(simcoreFile,"overallQueueDelay = %e +/- %e, heavyQueueDelay = %e +/- %e, lightQueueDelay = %e +/- %e\n", 
  table_mean(overallQueueDelay), table_conf_halfwidth(overallQueueDelay, 0.95), 
  table_mean(heavyQueueDelay), table_conf_halfwidth(heavyQueueDelay, 0.95), 
  table_mean(lightQueueDelay), table_conf_halfwidth(lightQueueDelay, 0.95));
  fflush(NULL);
  fprintf(simcoreFile,"qTime = %e +/- %e, qLen = %e +/- %e,  ", table_mean(oltAttrs.queueTimeTable), 
  table_conf_halfwidth(oltAttrs.queueTimeTable, 0.98), 
  table_mean(oltAttrs.queueLengthTable), 
  table_conf_halfwidth(oltAttrs.queueLengthTable, 0.98));
  fprintf(simcoreFile,"\n");
  fflush(NULL);
  fprintf(simcoreFile, "State Information\n");
  fprintf(simcoreFile, "OLT\n");
  fprintf(simcoreFile, "packetQueueSize = %.0f, ", oltAttrs.packetQueueSize);
  fprintf(simcoreFile, "packetQueueNum = %ld, ", oltAttrs.packetQueueNum);
  for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
  {
    fprintf(simcoreFile, "latency[%d] = %e, ", loopIndex, onuAttrs[loopIndex].latency);
    fprintf(simcoreFile, "rtt[%d] = %e, ", loopIndex, onuAttrs[loopIndex].rtt);
  }
  fprintf(simcoreFile, "tslotStart = %d\n", oltAttrs.tslotStart);
  fflush(NULL);
  
  fprintf(simcoreFile, "\n\n");
  fprintf(simcoreFile, "Latest message buffer\n");
  fprintf(simcoreFile, "%s", sim_msg_buf);
  fprintf(simcoreFile, "\n\n");
  fclose(simcoreFile);
  fflush(NULL);

  if(fatalErrorCount >= MAX_FATAL_ERRORS)
  {
    /* halt the simulation */
    printf("Halt simulation\n");
    terminateSim = 1;
    set(SIM_END_EVENT);
  }
}

/* Simulation memory cleanup routine */
void sim_cleanup()
{
  remove_all_packets();

  // Set all table pointers to NULL temporarily
  overallQueueDelay = NULL;
  heavyQueueDelay = NULL;
  lightQueueDelay = NULL;
  overallQueueLength = NULL;
  heavyQueueLength = NULL;
  lightQueueLength = NULL;
  throughputFairness = NULL;
}


/* heartbeat process with stats time trace, so we know the simulation is still alive and can trace values of statistics */
void heartbeat_with_timetrace()
{
  FILE *simCtrlFile;
  char  currToken[150];
  int   beat, loopIndex;
  int   beatCnt;

  create("heartbeat");
  
  beat = 0;
  beatCnt = 0;

  while(!terminateSim)
  {
    hold(simParams.SIM_TRACE_TIMESCALE);
    /* Check for excessive buffer sizes */
    if(oltAttrs.packetQueueSize > MAX_PKT_BUF_SIZE)
    {
      /* Fill out some context information */
      dump_msg_buf[0] = '\0';
      sprintf(dump_msg_buf,"Detected by heart beat process\n");
      sprintf(dump_msg_buf,"%sOLT has overflowed it's buffer.\n",dump_msg_buf);
      fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
      dump_sim_core();
    }
    /* Update the hearbeat 10-counter */
    beatCnt++;
    if(beatCnt == 10)
    {
      beatCnt = 0; /* reset heart beat count */
    }
    /* Check external sim control */
    if((simCtrlFile = fopen("sim_ctrl","r")) != NULL)
    {
      fscanf(simCtrlFile, "%s", currToken);
      fclose(simCtrlFile);
      if(strcmp(currToken, "stop") == 0)
      {
        simCtrlFile = fopen("sim_ctrl","w");
        fprintf(simCtrlFile,"run");
        fclose(simCtrlFile);
        /* stop the simulation */
        set(SIM_END_EVENT);
      }
    }
    /* 
     * Trace statistics 
     */
    /* queueing delay */
    for(loopIndex=0; loopIndex < MAX_TRACE_VALUES-1; loopIndex++)
    {
      overallQueueDelayTrace[loopIndex] = overallQueueDelayTrace[loopIndex+1];
      simTimeTrace[loopIndex] = simTimeTrace[loopIndex+1];
    }
    overallQueueDelayTrace[MAX_TRACE_VALUES-1] = table_conf_mean(overallQueueDelay);
    simTimeTrace[MAX_TRACE_VALUES-1] = simtime();
    simCtrlFile = fopen("od_trc","w");
    for(loopIndex=0; loopIndex < MAX_TRACE_VALUES; loopIndex++)
    {
      fprintf(simCtrlFile,"%e %e\n",simTimeTrace[loopIndex],overallQueueDelayTrace[loopIndex]);
    }
    fclose(simCtrlFile);
    /* Print heartbeat */
    if(beat == 0)
    {
      beat = 1;
      printf("*");
      fflush(NULL);
    }
    else
    {
      beat = 0;
      printf("/");
      fflush(NULL);
    }
  }
}

/* heartbeat process, so we know the simulation is still alive */
void heartbeat()
{
  FILE  *simCtrlFile;
  char  currToken[150];
  int   beat;
  int   beatCnt;

  create("heartbeat");
  
  beat = 0;
  beatCnt = 0;

  while(!terminateSim)
  {
    hold(simParams.SIM_TRACE_TIMESCALE);
    /* Check for excessive buffer sizes */
    if(oltAttrs.packetQueueSize > MAX_PKT_BUF_SIZE)
    {
      /* Fill out some context information */
      dump_msg_buf[0] = '\0';
      sprintf(dump_msg_buf,"Detected by heart beat process\n");
      sprintf(dump_msg_buf,"%sOLT has overflowed it's buffer.\n",dump_msg_buf);
      fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
      dump_sim_core();
    }
    /* Update the hearbeat 10-counter */
    beatCnt++;
    if(beatCnt == 10)
    {
      beatCnt = 0; /* reset heart beat count */
    }
    /* Check external sim control */
    if((simCtrlFile = fopen("sim_ctrl","r")) != NULL)
    {
      fscanf(simCtrlFile, "%s", currToken);
      fclose(simCtrlFile);
      if(strcmp(currToken, "stop") == 0)
      {
        simCtrlFile = fopen("sim_ctrl","w");
        fprintf(simCtrlFile,"run");
        fclose(simCtrlFile);
        /* stop the simulation */
        set(SIM_END_EVENT);
      }
    }
            
    // Test Variables heartbeat_process
    test_vars.heartbeat_process++;
    test_var_print();
        
    // Test Variables sim_time_per_load
    test_vars.sim_time_per_load[test_vars.runNum][test_vars.loadOrderCounter] = simtime() - test_vars.sim_time_per_load_start[test_vars.runNum][test_vars.loadOrderCounter];
    test_var_print();
    
    /* Print heartbeat */
    if(beat == 0)
    {
      beat = 1;
      printf("*");
      fflush(NULL);
    }
    else
    {
      beat = 0;
      printf("/");
      fflush(NULL);
    }
  }
}

/* 
 * FUNCTION: sim_ctrl()
 * DESCRIPTION: Simulation control flow, controls simulation termination by setting
 *              SIM_END_EVENT event, at appropriate time
 *
 */
void sim_ctrl()
{
  create("sim_ctrl");
  if(simType == PILOT_RUN)
  {
    hold(0.1);
    while(table_cnt(overallQueueDelay) < 2000)
    {
      hold(0.1);
    }
    reset();

    /* Estimation period */
    hold(1);
    while(table_cnt(overallQueueDelay) < 20000)
    {
      hold(1);
    }
  }

  if((simType == ACTUAL_RUN) || (simType == TAIL_RUN))
  {
    printf("Actual run!!!\n");
    /* Remove initialization bias, wait for 20000 frames to record queueing delay */
    hold(0.1);
    while(table_cnt(overallQueueDelay) < 20000)
    {
      hold(0.1);
    }
    reset();
    //if(simParams.TRAFFIC_TYPE == TRAFFIC_POISSON)
    //{
    //  /* Wait for CI convergence */
    //  wait(converged);    /* Overall Queue delay */
    //}
    //else
    //{
    //  //hold(simParams.SIM_TIME);
    //  while(table_cnt(overallQueueDelay) < (simParams.SIM_TIME*1e6))
    //  {
    //    hold(60);
    //  }
    //}
    
    switch(simParams.endType)
    {
      case END_TYPE_TRAFFIC:
        while(table_cnt(overallQueueDelay) < (simParams.SIM_TIME*1e5))
          hold(1);
        break;
      case END_TYPE_TIME:
        while(simtime() < simParams.SIM_TIME)
          hold(1);
        break;
      default:
        while(simtime() < simParams.SIM_TIME)
          hold(1);
    }
  }

  /* Simulation is completed, set SIM_END global event */
  set(SIM_END_EVENT);
}


void sim()
{
  // Test Variables sim_start
  if (simType == PILOT_RUN) {
    test_vars.sim_start[test_vars.runNum][test_vars.loadOrderCounter][0]++;
    test_var_print();
  }
  if (simType == ACTUAL_RUN) {
    test_vars.sim_start[test_vars.runNum][test_vars.loadOrderCounter][1]++;
    test_var_print();
  }
  
  char tempStr[100];
  char tempStr_2[100];
  long rand_seed;
    
  /* initialize the simulation */
  create("sim");
  
  // Test Variables sim_process
  if (simType == PILOT_RUN) {
    test_vars.sim_process[test_vars.runNum][test_vars.loadOrderCounter][0]++;
    test_var_print();
  }
  if (simType == ACTUAL_RUN) {
    test_vars.sim_process[test_vars.runNum][test_vars.loadOrderCounter][1]++;
    test_var_print();
  }
  
  TSprint("sim_start\n");
  
  /* Set CSIM maximums */
  max_mailboxes(20000);
  max_messages(1000000000);
  max_events(1000000000);
  max_processes(1000000000);
  
  /* setup simulation parameters */
  calc_sim_params();
  
  /* initialize data structures */
  init_data_structures();
  
  /* Setup error handler */
  set_err_handler((*sim_err_handler));
  
  /* Start heartbeat */
  if(simParams.SIM_TRACE == SIM_TRACE_ON)
  {
    heartbeat_with_timetrace();
  }
  else
  {
    heartbeat();
  }
  
  rand_seed = simParams.RAND_SEED;
   
  /* Initialize overall queueing delay table */
  overallQueueDelay = table("Overall_Queue_Delay");
  
  if(simType == ACTUAL_RUN)
  {
    table_histogram(overallQueueDelay, 500, 0.0, overallQueueDelayEst.maxEst);
  }
  else if(simType == TAIL_RUN)
  {
    table_histogram(overallQueueDelay, 500, overallQueueDelayEst.minEst, overallQueueDelayEst.maxEst);
  }
  table_confidence(overallQueueDelay);
  
  if (simParams.TRAFFIC_TYPE != TRAFFIC_SELF_SIMILAR) table_run_length(overallQueueDelay, 0.01, 0.95, 10000);
  else table_run_length(overallQueueDelay, 0.05, 0.90, 10000);
  
  /* Initialize Heavy ONU queueing delay table */
  heavyQueueDelay = table("Heavy_ONU_Queue_Delay");
  table_confidence(heavyQueueDelay);
  
  /* Initialize Light ONU queueing delay table */
  lightQueueDelay = table("Light_ONU_Queue_Delay");
  table_confidence(lightQueueDelay);
  
  overallQueueLength = table("Overall_Queue_Length");
  table_confidence(overallQueueLength);
  
  heavyQueueLength = table("Heavy_ONU_Queue_Length");
  table_confidence(heavyQueueLength);
  
  lightQueueLength = table("Light_ONU_Queue_Length");
  table_confidence(lightQueueLength);
  
  throughputFairness = table("Throughput_Fairness");
  
  /* setup empirical distribution for packet sizes */
  setup_empirical(EMPIRICAL_SIZE, EMPIRICAL_PROB, EMPIRICAL_CUTOFF, EMPIRICAL_ALIAS);
  
  // Test Variables sim_before_ONU_processes
  if (simType == PILOT_RUN) {
    test_vars.sim_before_ONU_processes[test_vars.runNum][test_vars.loadOrderCounter][0]++;
    test_var_print();
  }
  if (simType == ACTUAL_RUN) {
    test_vars.sim_before_ONU_processes[test_vars.runNum][test_vars.loadOrderCounter][1]++;
    test_var_print();
  }
      
  tempStr[0] = '\0'; 
  sprintf(tempStr, "Tx Time for OLT");
  oltAttrs.transmitTimeTable = table(tempStr);
  table_confidence(oltAttrs.transmitTimeTable);
  tempStr[0] = '\0'; 
  sprintf(tempStr, "Queue Time for OLT");
  oltAttrs.queueTimeTable = table(tempStr);
  table_confidence(oltAttrs.queueTimeTable);
  tempStr[0] = '\0'; 
  sprintf(tempStr, "Queue Length for OLT");
  oltAttrs.queueLengthTable = table(tempStr);
  table_confidence(oltAttrs.queueLengthTable);
  tempStr[0] = '\0';
  sprintf(tempStr, "Throughput for OLT");
  oltAttrs.transmitThroughput = table(tempStr);
  table_confidence(oltAttrs.transmitThroughput);
  /* setup ONU latency */
  for(i=0; i < simParams.NUM_ONU; i++)
  {
    onuAttrs[i].latency = simParams.ONU_PROP[i]; /* ONU distance of 10 to 20 km */
    onuAttrs[i].rtt = onuAttrs[i].latency*2;
    tempStr[0] = '\0';
    sprintf(tempStr, "Throughput for ONU %d", i);
    onuAttrs[i].transmitThroughput = table(tempStr);
    table_confidence(onuAttrs[i].transmitThroughput);
  } 

  /* Setup random number streams */
  oltAttrs.pktInterArrivalStream = create_stream();
  reseed(oltAttrs.pktInterArrivalStream,rand_seed++);
  oltAttrs.pktSizeStream = create_stream();
  reseed(oltAttrs.pktSizeStream,rand_seed++);
  oltAttrs.burstSizeStream = create_stream();
  reseed(oltAttrs.burstSizeStream,rand_seed++);
  /* Setup packet arrival mailboxes */
  tempStr[0] = '\0'; 
  sprintf(tempStr, "OLT pkt mb");
  oltAttrs.pktMailbox = mailbox(tempStr);
  
  /* Initialize the Simulation End Event */
  SIM_END_EVENT = event("Sim End");
  
  /* Initialize the Olt Service Event */
  SERVICE_OLT = event("OLT Needs to be Serviced");

  /* Spawn Traffic generator(s) for ONU */
  /* Start the ONU processes */
  for(i=0; i < simParams.NUM_ONU; i++)
  {
    // Initalize the Packet Arrived Event for Each ONU
    sprintf(tempStr_2, "A Packet Arrived for ONU #%d", i);
    PACKET_ARRIVED[i] = event(tempStr_2);
    sprintf(tempStr_2, "ONU #%d Has No Queued Packets", i);
    ONU_HAS_NO_QUEUED_PACKETS[i] = event(tempStr_2);

    switch(simParams.TRAFFIC_TYPE)
    {
      case TRAFFIC_POISSON:
        traffic_src_poisson(i); /* spawn poisson traffic src for each ONU */
        TSprint("traffic_src_poisson #%d has started\n", i);
        break;
      case TRAFFIC_SELF_SIMILAR:
        traffic_agg_self_similar(i);  /* spawn the aggregator */
        for(j=0;j < simParams.NUM_SS_STREAMS; j++)
        {
          traffic_src_self_similar(i,j); /* spawn the individual streams */
        }
        TSprint("traffic_src_self_similar #%d has started\n", i);
        break;
      default:
        break;
    }
  }

  /* spawn the OLT process */
  olt(); 
 
  /* spawn the process which keeps track of upstream grant permission */ 
  grantCycle();
  
  for(i=0; i < simParams.NUM_ONU; i++)
  {
    /* Start the ONU process */
    onu(i);
  }
 
  /* Start the throughput calculation process for both onu and olt*/
  onu_throughput_calc();
  
  // Test Variabels sim_finish
  if (simType == PILOT_RUN) {
    test_vars.sim_finish[test_vars.runNum][test_vars.loadOrderCounter][0]++;
    test_var_print();
  }
  if (simType == ACTUAL_RUN) {
    test_vars.sim_finish[test_vars.runNum][test_vars.loadOrderCounter][1]++;
    test_var_print();
  }
  TSprint("sim_finish\n");
  
  /* Spawn process that handles simulation execution */
  sim_ctrl();
  wait(SIM_END_EVENT);
  
  // Test Variables sim_finish2
  if (simType == PILOT_RUN) {
    test_vars.sim_finish2[test_vars.runNum][test_vars.loadOrderCounter][0]++;
    test_var_print();
  }
  if (simType == ACTUAL_RUN) {
    test_vars.sim_finish2[test_vars.runNum][test_vars.loadOrderCounter][1]++;
    test_var_print();
  }
  TSprint("sim_finish2\n");
  
  if(!terminateSim)
  {
    /* If simulation terminated normally, then dump stat report */
    report_facilities();
    report_classes();
    report_tables();
    report_storages();
    report_buffers();
    report_events();
    report_mailboxes();
    report_qtables();
    report_meters();
    report_boxes();
    
    report();
  }
  TSprint("sim_finish3\n");
}

/* 
 * FUNCTION: read_sim_cfg_file()
 * DESCRIPTION: Reads simulation parameters from a file
 *
 */
void read_sim_cfg_file()
{
  // Test Variables read_sim_cfg_file_start
  test_vars.read_sim_cfg_file_start++;
  test_var_print();
  TSprint("read_sim_cfg_file_start\n");
  
  FILE *cfgFile;
  char currToken[200];
    
  /* set defaults */
  simParams.TRAFFIC_TYPE    = TRAFFIC_POISSON;  /* Traffic Type */
  simParams.TUNING_TIME     = 5e-3;
  simParams.SIM_TIME        = 300;
  simParams.NUM_RUNS        = 1;
  simParams.NUM_ONU         = 32;     /* Number of ONUs on PON */
  simParams.NUM_HEAVY_ONU   = 5;      /* Number of heavily loaded ONUs */
  simParams.LINK_SPEED      = 10e9;   /* link speed in bps */
  
  simParams.HEAVY_LOAD      = 2;      /* heavy load multiple */
  
  simParams.SS_HURST_PARAM  = 0.75;   /* Self Similar Traffic Source Hurst Parameter */
  simParams.NUM_SS_STREAMS  = 32;     /* Number of Self Similar Traffic Streams */
  
  simParams.START_LOAD      = 0.1;
  simParams.END_LOAD        = 0.9;
  simParams.LOAD_INCR       = 0.1;

  /* Set the values for the Cyclic Sleep Mechanism */
  simParams.ACTIVE_POWER_CONSUMPTION = 6.5;
  simParams.IDLE_POWER_CONSUMPTION   = 4.1;
  simParams.SLEEP_POWER_CONSUMPTION  = 2.5;
  simParams.PROBE_POWER_CONSUMPTION  = 3.5;
  simParams.WAKEUP_POWER_CONSUMPTION = (simParams.PROBE_POWER_CONSUMPTION + simParams.ACTIVE_POWER_CONSUMPTION)/2.0;  

  simParams.ONU_TIME_SLEEP   = 0.005;
  simParams.ONU_TIME_TRIGGER = 0.0001;
  simParams.ONU_TIME_WAKEUP  = 0.001;
  simParams.ONU_TIME_PROBE   = 0.00025;
  
  simParams.ONU_GRANTED      = 0; 
  simParams.TIME_PER_GRANT   = simParams.ONU_TIME_PROBE/2.0/simParams.NUM_ONU;
  simParams.simType          = PILOT_RUN;
  simParams.boolRecordStateTime = 0;
  
  // Set the simulation to end once a certain amount of traffic passes through it
  simParams.endType          = END_TYPE_TRAFFIC;

  /* Generate Random Number Seed */
  simParams.RAND_SEED = 200100;

  /* Fixed Prop Delay */
  simParams.FIXED_PROP_DELAY = FIXED_PROP_DELAY_OFF;
  simParams.FIXED_PROP_DELAY_VALUE = 50e-6;

  /* Maximum Prop Delay */
  simParams.MAX_PROP_DELAY = 50e-6;

  /* Get Tails of Distributions */
  simParams.GET_TAIL = GET_TAIL_OFF;

  /* Simulation Trace */
  simParams.SIM_TRACE = SIM_TRACE_OFF;
  
  /* Simulation Trace Time */
  simParams.SIM_TRACE_TIME = 1000;  /* sim trace time in seconds */
  simParams.SIM_TRACE_TIMESCALE = 0.5;   /* sim trace timescale in seconds */

  /* open the config file */
  if((cfgFile = fopen("sim_cfg","r")) == NULL)
  {
    printf("ERROR: Can't open simulation configuration file [sim_cfg]\n");
    exit(-1);
  }
  else
  {
    while((fscanf(cfgFile, "%s", currToken)) != EOF)
    {
      //printf("currToken = %s, ", currToken);
      if(strcmp(currToken, "TRAFFIC_TYPE") == 0)
      {
        currToken[0] = '\0';
        fscanf(cfgFile, "%s", currToken);
        if(strcmp(currToken, "TRAFFIC_SELF_SIMILAR") == 0)
        {
          simParams.TRAFFIC_TYPE = TRAFFIC_SELF_SIMILAR;
        }
        else if(strcmp(currToken, "TRAFFIC_POISSON") == 0)
        {
          simParams.TRAFFIC_TYPE = TRAFFIC_POISSON;
        }
      }
      else if(strcmp(currToken, "ONU_TIME_SLEEP") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.ONU_TIME_SLEEP = atof(currToken);
      }
      else if(strcmp(currToken, "ONU_TIME_TRIGGER") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.ONU_TIME_TRIGGER = atof(currToken);
      }
      else if(strcmp(currToken, "ONU_TIME_WAKEUP") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.ONU_TIME_WAKEUP = atof(currToken);
      }
      else if(strcmp(currToken, "ONU_TIME_PROBE") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.ONU_TIME_PROBE = atof(currToken);
      }
      else if(strcmp(currToken, "ONU_TIME_SLEEP") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.ONU_TIME_SLEEP = atof(currToken);
      }
      else if(strcmp(currToken, "ACTIVE_POWER_CONSUMPTION") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.ACTIVE_POWER_CONSUMPTION = atof(currToken);
      }
      else if(strcmp(currToken, "IDLE_POWER_CONSUMPTION") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.IDLE_POWER_CONSUMPTION = atof(currToken);
      }
      else if(strcmp(currToken, "SLEEP_POWER_CONSUMPTION") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.SLEEP_POWER_CONSUMPTION = atof(currToken);
      }
      else if(strcmp(currToken, "PROBE_POWER_CONSUMPTION") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.PROBE_POWER_CONSUMPTION = atof(currToken);
      }
      else if(strcmp(currToken, "SIM_TIME") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.SIM_TIME = atof(currToken);
        //printf("SIM_TIME = %e\n", simParams.SIM_TIME);
      }
      else if(strcmp(currToken, "NUM_RUNS") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.NUM_RUNS = atoi(currToken);
        //printf("NUM_RUNS = %d\n", simParams.NUM_RUNS);
      }
      else if(strcmp(currToken, "NUM_ONU") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.NUM_ONU = atoi(currToken);
        //printf("NUM_ONU = %d\n", simParams.NUM_ONU);
      }
      else if(strcmp(currToken, "LINK_SPEED") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.LINK_SPEED = atof(currToken);
        //printf("LINK_SPEED = %e\n", simParams.LINK_SPEED);
      }
      else if(strcmp(currToken, "SS_HURST_PARAM") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.SS_HURST_PARAM = atof(currToken);
        //printf("SS_HURST_PARAM = %e\n", simParams.SS_HURST_PARAM);
      }
      else if(strcmp(currToken, "NUM_SS_STREAMS") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.NUM_SS_STREAMS = atoi(currToken);
        //printf("NUM_SS_STREAMS = %d\n", simParams.NUM_SS_STREAMS);
      }
      else if(strcmp(currToken, "START_LOAD") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.START_LOAD = atof(currToken);
        //printf("START_LOAD = %e\n", simParams.START_LOAD);
      }
      else if(strcmp(currToken, "END_LOAD") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.END_LOAD = atof(currToken);
        //printf("END_LOAD = %e\n", simParams.END_LOAD);
      }
      else if(strcmp(currToken, "LOAD_INCR") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.LOAD_INCR = atof(currToken);
        //printf("LOAD_INCR = %e\n", simParams.LOAD_INCR);
      }
      else if(strcmp(currToken, "RAND_SEED") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.RAND_SEED = atol(currToken);
        //printf("RAND_SEED = %ld\n", simParams.RAND_SEED);
      }
      else if(strcmp(currToken, "FIXED_PROP_DELAY") == 0)
      {
        currToken[0] = '\0';
        fscanf(cfgFile, "%s", currToken);
        if(strcmp(currToken, "FIXED_PROP_DELAY_OFF") == 0)
        {
          simParams.FIXED_PROP_DELAY = FIXED_PROP_DELAY_OFF;
        }
        else if(strcmp(currToken, "FIXED_PROP_DELAY_ON") == 0)
        {
          simParams.FIXED_PROP_DELAY = FIXED_PROP_DELAY_ON;
        }
      }
      else if(strcmp(currToken, "END_TYPE") == 0)
      {
        currToken[0] = '\0';
        fscanf(cfgFile, "%s", currToken);
        if(strcmp(currToken, "END_TYPE_TRAFFIC") == 0)
        {
          simParams.endType = END_TYPE_TRAFFIC;
        }
        else if(strcmp(currToken, "END_TYPE_TIME") == 0)
        {
          simParams.endType = END_TYPE_TIME;
        }
      }
      else if(strcmp(currToken, "SLEEP_SCHEDULER") == 0)
      {
        currToken[0] = '\0';
        fscanf(cfgFile, "%s", currToken);
        if(strcmp(currToken, "JOURNAL") == 0)
        {
          simParams.sleepScheduler = SLEEP_SCHEDULER_JOURNAL;
        }
        else if(strcmp(currToken, "HEAVY_TRAFFIC") == 0)
        {
          simParams.sleepScheduler = SLEEP_SCHEDULER_HEAVY_TRAFFIC;
        }
        else if(strcmp(currToken, "HEAVY_TRAFFIC_HYBRID") == 0)
        {
          simParams.sleepScheduler = SLEEP_SCHEDULER_HEAVY_TRAFFIC_HYBRID;
        }
      }
      else if(strcmp(currToken, "GET_TAIL") == 0)
      {
        currToken[0] = '\0';
        fscanf(cfgFile, "%s", currToken);
        if(strcmp(currToken, "GET_TAIL_OFF") == 0)
        {
          simParams.GET_TAIL = GET_TAIL_OFF;
        }
        else if(strcmp(currToken, "GET_TAIL_ON") == 0)
        {
          simParams.GET_TAIL = GET_TAIL_ON;
        }
      }
      else if(strcmp(currToken, "SIM_TRACE") == 0)
      {
        currToken[0] = '\0';
        fscanf(cfgFile, "%s", currToken);
        if(strcmp(currToken, "SIM_TRACE_OFF") == 0)
        {
          simParams.SIM_TRACE = SIM_TRACE_OFF;
        }
        else if(strcmp(currToken, "SIM_TRACE_ON") == 0)
        {
          simParams.SIM_TRACE = SIM_TRACE_ON;
        }
      }
      else if(strcmp(currToken, "SIM_TRACE_TIME") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.SIM_TRACE_TIME = atof(currToken);
        //printf("SIM_TRACE_TIME = %e\n", simParams.SIM_TRACE_TIME);
      }
      else if(strcmp(currToken, "SIM_TRACE_TIMESCALE") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.SIM_TRACE_TIMESCALE = atof(currToken);
        //printf("SIM_TRACE_TIMESCALE = %e\n", simParams.SIM_TRACE_TIMESCALE);
      }
      else if(strcmp(currToken, "FIXED_PROP_DELAY_VALUE") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.FIXED_PROP_DELAY_VALUE = atof(currToken);
        //printf("FIXED_PROP_DELAY_VALUE = %e\n", simParams.FIXED_PROP_DELAY_VALUE);
      }
      else if(strcmp(currToken, "MAX_PROP_DELAY") == 0)
      {
        fscanf(cfgFile, "%s", currToken);
        simParams.MAX_PROP_DELAY = atof(currToken);
        //printf("MAX_PROP_DELAY = %e\n", simParams.MAX_PROP_DELAY);
      }
    }
    //printf("Sim configuration loaded\n");
  }
  
  // Update the TIME_PER_GRANT variable with the new value of 
  // NUM_ONU.  
  simParams.TIME_PER_GRANT   = simParams.ONU_TIME_PROBE/2.0/simParams.NUM_ONU;
  
  // Test Variables read_sim_cfg_file_finish
  test_vars.read_sim_cfg_file_finish++;
  test_var_print();
  TSprint("read_sim_cfg_file_finish\n\n");
}


void estimate_hist_max()
{
  overallQueueDelayEst.maxEst = table_max(overallQueueDelay)*1.2;
  heavyQueueDelayEst.maxEst = table_max(heavyQueueDelay);
  lightQueueDelayEst.maxEst = table_max(lightQueueDelay);
  overallQueueLengthEst.maxEst = table_max(overallQueueLength);
  heavyQueueLengthEst.maxEst = table_max(heavyQueueLength);
  lightQueueLengthEst.maxEst = table_max(lightQueueLength);
}

void setup_hist_tail()
{
  overallQueueDelayEst.minEst = overallQueueDelayEst.maxEst;
  heavyQueueDelayEst.minEst = heavyQueueDelayEst.maxEst;
  lightQueueDelayEst.minEst = lightQueueDelayEst.maxEst;
  overallQueueLengthEst.minEst = overallQueueLengthEst.maxEst;
  heavyQueueLengthEst.minEst = heavyQueueLengthEst.maxEst;
  lightQueueLengthEst.minEst = lightQueueLengthEst.maxEst;
  overallQueueDelayEst.maxEst = table_max(overallQueueDelay);
  heavyQueueDelayEst.maxEst = table_max(heavyQueueDelay);
  lightQueueDelayEst.maxEst = table_max(lightQueueDelay);
  overallQueueLengthEst.maxEst = table_max(overallQueueLength);
  heavyQueueLengthEst.maxEst = table_max(heavyQueueLength);
  lightQueueLengthEst.maxEst = table_max(lightQueueLength);
}

void write_sim_data(int runNumber, double trafficLoad)
{
  int     loopIndex;
  //char    filename_suffix[100];
  char    filename_str[150];
  char    double_str[15];
  char    *charPtr;

  FILE *odFile, *hdFile, *ldFile, *olFile, *hlFile, *llFile, *statsFile;
  FILE *odxlFile, *hdxlFile, *ldxlFile, *olxlFile, *hlxlFile, *llxlFile;
  FILE *clFile, *rsFile, *gsFile, *gspFile, *noFile;
  FILE *cllFile, *clhFile, *rslFile, *rshFile, *gslFile, *gshFile;
  FILE *nzFile, *nzlFile, *nzhFile;
  FILE *rgFile, *rglFile, *rghFile;
  FILE *odoFile, *odmnFile, *odmxFile, *clmnFile, *clmxFile;
  FILE *nomxFile;
  FILE *lbFile;
  FILE *srFile;
  FILE *srmxFile;
  FILE *tpoFile[MAX_ONU];
  FILE *tfsFile, *tfFile;
  FILE *cr1File, *cr2File, *mcrFile;
  FILE *odHistFile, *gspHistFile;
  FILE *pdFile, *plFile, *clpFile;
  FILE *pcFile, *tpcFile, *ppcFile, *cpcFile;

  /* Determine file names */
  //filename_suffix[0] = '\0';
  /* To support multiple runs, record the run number for run number greater than 0 */
  if(runNumber > 0)
  {
    double_str[0] = '\0';
    sprintf(double_str, "%d", runNumber);
    //sprintf(filename_suffix, "%srun%s_", filename_suffix, double_str);
  }
  //switch(simParams.TRAFFIC_TYPE)
  //{
  //  case TRAFFIC_POISSON:
  //    strcat(filename_suffix, "pn_");
  //    break;
  //  case TRAFFIC_SELF_SIMILAR:
  //    strcat(filename_suffix, "ss_");
  //    break;
  //}

  //sprintf(filename_suffix, "%s%do_", filename_suffix, simParams.NUM_ONU);
  
  //sprintf(filename_suffix, ".txt%s", filename_suffix);
  
  /*
   * Open files
   */
  filename_str[0] = '\0';
  sprintf(filename_str, "od.txt");
  odFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "odo.txt");
  odoFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "odmn.txt");
  odmnFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "odmx.txt");
  odmxFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "hd.txt");
  hdFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "ld.txt");
  ldFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "ol.txt");
  olFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "hl.txt");
  hlFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "ll.txt");
  llFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "cl.txt");
  clFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "clmn.txt");
  clmnFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "clmx.txt");
  clmxFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "cll.txt");
  cllFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "clh.txt");
  clhFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "rs.txt");
  rsFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "rsl.txt");
  rslFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "rsh.txt");
  rshFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "rg.txt");
  rgFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "rgl.txt");
  rglFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "rgh.txt");
  rghFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "gsp.txt");
  gspFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "gs.txt");
  gsFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "gsl.txt");
  gslFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "gsh.txt");
  gshFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "nz.txt");
  nzFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "nzl.txt");
  nzlFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "nzh.txt");
  nzhFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "no.txt");
  noFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "nomx.txt");
  nomxFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "lb.txt");
  lbFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "sr.txt");
  srFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "srmx.txt");
  srmxFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "pd.txt");
  pdFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "pl.txt");
  plFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "clp.txt");
  clpFile = fopen(filename_str,"a");

  double_str[0] = '\0';
  sprintf(double_str, "%g", trafficLoad);
  charPtr = strchr(double_str, '.');
  if(charPtr != NULL)
  {
    *charPtr = '_';
  }

  filename_str[0] = '\0';
  sprintf(filename_str, "od_hist_%s.txt", double_str);
  odHistFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "gsp_hist_%s.txt", double_str);
  gspHistFile = fopen(filename_str,"a");

  filename_str[0] = '\0';
  sprintf(filename_str, "od_xl.txt");
  odxlFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "hd_xl.txt");
  hdxlFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "ld_xl.txt");
  ldxlFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "ol_xl.txt");
  olxlFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "hl_xl.txt");
  hlxlFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "ll_xl.txt");
  llxlFile  = fopen(filename_str,"a");
  statsFile = fopen("stats","a");
  
  /* Open files for throughput per onu (tpo) calculations */
  for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
  {
    filename_str[0] = '\0';
    sprintf(filename_str, "tpo%d.txt", loopIndex+1);
    tpoFile[loopIndex] = fopen(filename_str,"a");
  }
  /* Open files for throughput fairness */
  filename_str[0] = '\0';
  sprintf(filename_str, "tfs.txt");
  tfsFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "tf.txt");
  tfFile = fopen(filename_str,"a");
  /* Open files for competitive ratio */
  filename_str[0] = '\0';
  sprintf(filename_str, "cr1.txt");
  cr1File = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "cr2.txt");
  cr2File = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "mcr.txt");
  mcrFile = fopen(filename_str,"a");
  
  // Open files to record the ONU State power consumption
  // data
  filename_str[0] = '\0';
  sprintf(filename_str, "pc.txt");
  pcFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "tpc.txt");
  tpcFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "ppc.txt");
  ppcFile = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "cpc.txt");
  cpcFile = fopen(filename_str,"a");
  
  fprintf(statsFile,"rand_seed_base=%ld\n", simParams.RAND_SEED);
  fprintf(statsFile,"sim_time=%e\n", simtime());
  fprintf(statsFile,"cpu_time=%e\n", cputime());
  fprintf(statsFile,"sim_time/cpu_time=%e\n", simtime()/cputime());

  if(simParams.simType == ACTUAL_RUN) 
  {
    double timeActive = 0, timeIdle = 0, timeProbe = 0, timeSleep = 0, timeWakeup = 0;
    double pwrActive = 0, pwrIdle = 0, pwrProbe = 0, pwrSleep = 0, pwrWakeup = 0, pwrAvg = 0;
    double pctActive = 0, pctIdle = 0, pctProbe = 0, pctSleep = 0, pctWakeup = 0;
    int    cntActive = 0, cntIdle = 0, cntProbe = 0, cntSleep = 0, cntWakeup = 0;
    for(int iaa = 0; iaa < simParams.NUM_ONU; iaa++)
    {
      // Please see the emun eONU_STATE in eponsim.h to find the
      // appropriate position of each state in timeInState.
      timeActive += onuAttrs[iaa].timeInState[0];
      timeIdle += onuAttrs[iaa].timeInState[1];
      timeSleep += onuAttrs[iaa].timeInState[2];
      timeProbe += onuAttrs[iaa].timeInState[3];
      timeWakeup += onuAttrs[iaa].timeInState[4];

      cntActive += onuAttrs[iaa].cntState[0]; 
      cntIdle += onuAttrs[iaa].cntState[1]; 
      cntProbe += onuAttrs[iaa].cntState[2]; 
      cntSleep += onuAttrs[iaa].cntState[3]; 
      cntWakeup += onuAttrs[iaa].cntState[4]; 
    }
    timeActive /= simParams.NUM_ONU;
    timeIdle /= simParams.NUM_ONU;
    timeSleep /= simParams.NUM_ONU;
    timeProbe /= simParams.NUM_ONU;
    timeWakeup /= simParams.NUM_ONU;

    cntActive /= simParams.NUM_ONU;
    cntIdle /= simParams.NUM_ONU;
    cntSleep /= simParams.NUM_ONU;
    cntProbe /= simParams.NUM_ONU;
    cntWakeup /= simParams.NUM_ONU;

    pctActive = timeActive/(timeActive + timeIdle + timeSleep + timeProbe + timeWakeup)*100.0;
    pctIdle = timeIdle/(timeActive + timeIdle + timeSleep + timeProbe + timeWakeup)*100.0;
    pctSleep = timeSleep/(timeActive + timeIdle + timeSleep + timeProbe + timeWakeup)*100.0;
    pctProbe = timeProbe/(timeActive + timeIdle + timeSleep + timeProbe + timeWakeup)*100.0;
    pctWakeup = timeWakeup/(timeActive + timeIdle + timeSleep + timeProbe + timeWakeup)*100.0;

    pwrActive = timeActive*simParams.ACTIVE_POWER_CONSUMPTION;
    pwrIdle =  timeIdle*simParams.IDLE_POWER_CONSUMPTION;
    pwrSleep = timeSleep*simParams.SLEEP_POWER_CONSUMPTION;
    pwrProbe = timeProbe*simParams.PROBE_POWER_CONSUMPTION;
    pwrWakeup = timeWakeup*simParams.WAKEUP_POWER_CONSUMPTION;
    pwrAvg = pwrActive + pwrIdle + pwrSleep + pwrProbe + pwrWakeup;
    fprintf(tpcFile, "%f %f %f %f %f %f\n", trafficLoad, timeActive, timeIdle, timeSleep, timeProbe, timeWakeup);
    //for(int iaa = 0; iaa < simParams.NUM_ONU; iaa++)
    //{
    //  fprintf(tpcFile, "%f ", onuAttrs[iaa].timeInState[4]);
    //}
    //fprintf(tpcFile, "\n");
    fprintf(ppcFile, "%f %f %f %f %f %f\n", trafficLoad, pctActive, pctIdle, pctSleep, pctProbe, pctWakeup);
    fprintf(cpcFile, "%f %d %d %d %d %d\n", trafficLoad, cntActive, cntIdle, cntSleep, cntProbe, cntWakeup);
    fprintf(pcFile, "%f %f %f %f %f %f %f\n", trafficLoad, pwrAvg, pwrActive, pwrIdle, pwrSleep, pwrProbe, pwrWakeup);
  }
  if((simParams.TRAFFIC_TYPE != TRAFFIC_SELF_SIMILAR) /*&& (simParams.OLT_TYPE != OLT_APS)*/)
  {
    /* Collect stats in files */
    fprintf(odoFile,"%f %e\n", trafficLoad, table_mean(overallQueueDelay));
    fprintf(odmnFile,"%f %e\n", trafficLoad, table_min(overallQueueDelay));
    fprintf(odmxFile,"%f %e\n", trafficLoad, table_max(overallQueueDelay));
    fprintf(odFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallQueueDelay), table_conf_lower(overallQueueDelay, 0.95), table_conf_upper(overallQueueDelay, 0.95), 0.95);
    fprintf(hdFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyQueueDelay), table_conf_lower(heavyQueueDelay, 0.95), table_conf_upper(heavyQueueDelay, 0.95), 0.95);
    fprintf(ldFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightQueueDelay), table_conf_lower(lightQueueDelay, 0.95), table_conf_upper(lightQueueDelay, 0.95), 0.95);
    fprintf(olFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallQueueLength), table_conf_lower(overallQueueLength, 0.95), table_conf_upper(overallQueueLength, 0.95), 0.95);
    fprintf(hlFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyQueueLength), table_conf_lower(heavyQueueLength, 0.95), table_conf_upper(heavyQueueLength, 0.95), 0.95);
    fprintf(llFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightQueueLength), table_conf_lower(lightQueueLength, 0.95), table_conf_upper(lightQueueLength, 0.95), 0.95);
  
    fprintf(odxlFile,"%e, ", table_conf_mean(overallQueueDelay));
    fprintf(hdxlFile,"%e, ", table_conf_mean(heavyQueueDelay));
    fprintf(ldxlFile,"%e, ", table_conf_mean(lightQueueDelay));
    fprintf(olxlFile,"%e, ", table_conf_mean(overallQueueLength));
    fprintf(hlxlFile,"%e, ", table_conf_mean(heavyQueueLength));
    fprintf(llxlFile,"%e, ", table_conf_mean(lightQueueLength));

    fprintf(statsFile,"overallQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e,cnt=%ld), heavyQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e), lightQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e)\n", table_conf_mean(overallQueueDelay), table_conf_halfwidth(overallQueueDelay, 0.95), table_mean(overallQueueDelay), table_var(overallQueueDelay), table_min(overallQueueDelay), table_max(overallQueueDelay), table_cnt(overallQueueDelay), table_conf_mean(heavyQueueDelay), table_conf_halfwidth(heavyQueueDelay, 0.95), table_mean(heavyQueueDelay), table_var(heavyQueueDelay), table_min(heavyQueueDelay), table_max(heavyQueueDelay), table_conf_mean(lightQueueDelay), table_conf_halfwidth(lightQueueDelay, 0.95), table_mean(lightQueueDelay), table_var(lightQueueDelay), table_min(lightQueueDelay), table_max(lightQueueDelay));
    fprintf(statsFile,"qTime = %e +/- %e (mean=%e,var=%e,min=%e,max=%e), qLen = %e +/- %e (mean=%e,var=%e,min=%e,max=%e)\n", table_conf_mean(oltAttrs.queueTimeTable),  table_conf_halfwidth(oltAttrs.queueTimeTable, 0.95), table_mean(oltAttrs.queueTimeTable), table_var(oltAttrs.queueTimeTable), table_min(oltAttrs.queueTimeTable), table_max(oltAttrs.queueTimeTable), table_conf_mean(oltAttrs.queueLengthTable), table_conf_halfwidth(oltAttrs.queueLengthTable, 0.95), table_mean(oltAttrs.queueLengthTable), table_var(oltAttrs.queueLengthTable), table_min(oltAttrs.queueLengthTable), table_max(oltAttrs.queueLengthTable));
  }
  else
  {
    /* Collect stats in files */
    fprintf(odoFile,"%f %e\n", trafficLoad, table_mean(overallQueueDelay));
    fprintf(odmnFile,"%f %e\n", trafficLoad, table_min(overallQueueDelay));
    fprintf(odmxFile,"%f %e\n", trafficLoad, table_max(overallQueueDelay));
    fprintf(odFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallQueueDelay), table_conf_lower(overallQueueDelay, 0.90), table_conf_upper(overallQueueDelay, 0.90), 0.90);
    
    fprintf(hdFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyQueueDelay), table_conf_lower(heavyQueueDelay, 0.90), table_conf_upper(heavyQueueDelay, 0.90), 0.90);
    fprintf(ldFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightQueueDelay), table_conf_lower(lightQueueDelay, 0.90), table_conf_upper(lightQueueDelay, 0.90), 0.90);
    fprintf(olFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallQueueLength), table_conf_lower(overallQueueLength, 0.90), table_conf_upper(overallQueueLength, 0.90), 0.90);
    fprintf(hlFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyQueueLength), table_conf_lower(heavyQueueLength, 0.90), table_conf_upper(heavyQueueLength, 0.90), 0.90);
    fprintf(llFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightQueueLength), table_conf_lower(lightQueueLength, 0.90), table_conf_upper(lightQueueLength, 0.90), 0.90);
    
    fprintf(odxlFile,"%e, ", table_conf_mean(overallQueueDelay));
    fprintf(hdxlFile,"%e, ", table_conf_mean(heavyQueueDelay));
    fprintf(ldxlFile,"%e, ", table_conf_mean(lightQueueDelay));
    fprintf(olxlFile,"%e, ", table_conf_mean(overallQueueLength));
    fprintf(hlxlFile,"%e, ", table_conf_mean(heavyQueueLength));
    fprintf(llxlFile,"%e, ", table_conf_mean(lightQueueLength));
    
    fprintf(statsFile,"overallQueueDelay (batch_size=%lu,num_batches=%lu,converged=%d,acc=%g)\n",table_batch_size(overallQueueDelay),table_batch_count(overallQueueDelay),table_converged(overallQueueDelay),table_conf_accuracy(overallQueueDelay,0.90));
    fprintf(statsFile,"overallQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e,cnt=%lu), heavyQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e), lightQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e)\n", table_conf_mean(overallQueueDelay), table_conf_halfwidth(overallQueueDelay, 0.90), table_mean(overallQueueDelay), table_var(overallQueueDelay), table_min(overallQueueDelay), table_max(overallQueueDelay), table_cnt(overallQueueDelay), table_conf_mean(heavyQueueDelay), table_conf_halfwidth(heavyQueueDelay, 0.90), table_mean(heavyQueueDelay), table_var(heavyQueueDelay), table_min(heavyQueueDelay), table_max(heavyQueueDelay),  table_conf_mean(lightQueueDelay), table_conf_halfwidth(lightQueueDelay, 0.90), table_mean(lightQueueDelay), table_var(lightQueueDelay), table_min(lightQueueDelay), table_max(lightQueueDelay));
    fprintf(statsFile,"qTime = %e +/- %e (mean=%e,var=%e,min=%e,max=%e), qLen = %e +/- %e (mean=%e,var=%e,min=%e,max=%e)\n", table_conf_mean(oltAttrs.queueTimeTable),  table_conf_halfwidth(oltAttrs.queueTimeTable, 0.90), table_mean(oltAttrs.queueTimeTable), table_var(oltAttrs.queueTimeTable), table_min(oltAttrs.queueTimeTable), table_max(oltAttrs.queueTimeTable), table_conf_mean(oltAttrs.queueLengthTable), table_conf_halfwidth(oltAttrs.queueLengthTable, 0.90), table_mean(oltAttrs.queueLengthTable), table_var(oltAttrs.queueLengthTable), table_min(oltAttrs.queueLengthTable), table_max(oltAttrs.queueLengthTable));
  }

  /* Report Histograms */
  for(loopIndex=1; loopIndex <= table_histogram_num(overallQueueDelay); loopIndex++)
  {
    /* Print frequency for each bucket */
    fprintf(odHistFile,"%e-%e %e\n",table_histogram_width(overallQueueDelay)*(loopIndex-1),table_histogram_width(overallQueueDelay)*loopIndex,(double)((double)((unsigned long)table_histogram_bucket(overallQueueDelay,loopIndex))/(double)((unsigned long)table_histogram_total(overallQueueDelay))));
  }
  
  /* Report ONU throughput */
  for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
  {
    fprintf(tpoFile[loopIndex],"%f %f %f %f\n", trafficLoad, table_mean(onuAttrs[loopIndex].transmitThroughput),table_min(onuAttrs[loopIndex].transmitThroughput),table_max(onuAttrs[loopIndex].transmitThroughput));
    actual_tput[loopIndex] = table_mean(onuAttrs[loopIndex].transmitThroughput);
  }

  /* Record throughput fairness from the fairness samples */
  fprintf(tfsFile,"%f %g %g %g\n", trafficLoad, table_mean(throughputFairness),table_min(throughputFairness), table_max(throughputFairness));

  /* Record throughput fairness for the whole simulation run */
  fprintf(tfFile,"%f %g\n", trafficLoad, rj_fairness_index(actual_tput,ideal_tput,simParams.NUM_ONU));

  fprintf(statsFile, "od_hist  (num=%lu,total=%lu,low=%e,high=%e)\n",table_histogram_num(overallQueueDelay),table_histogram_total(overallQueueDelay),table_histogram_low(overallQueueDelay),table_histogram_high(overallQueueDelay));

  fprintf(statsFile, "\n\n");

  //trafficLoad = simParams.DESIRED_LOAD;

  /* Close files for reporting ONU throughput */
  for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++) fclose(tpoFile[loopIndex]);
  
  fclose(odoFile);
  fclose(odFile);
  
  fclose(odmnFile);
  fclose(odmxFile);
  fclose(hdFile);
  fclose(ldFile);
  fclose(olFile);
  fclose(hlFile);
  fclose(llFile);
  fclose(clFile);
  fclose(clmnFile);
  fclose(clmxFile);
  fclose(cllFile);
  fclose(clhFile);
  fclose(rsFile);
  fclose(rslFile);
  fclose(rshFile);
  fclose(rgFile);
  fclose(rglFile);
  fclose(rghFile);
  fclose(gspFile);
  fclose(gsFile);
  fclose(gslFile);
  fclose(gshFile);
  fclose(noFile);
  fclose(nomxFile);
  fclose(srFile);
  fclose(srmxFile);
  fclose(lbFile);
  fclose(nzFile);
  fclose(nzlFile);
  fclose(nzhFile);
  fclose(odHistFile);
  fclose(gspHistFile);
  fclose(odxlFile);
  fclose(hdxlFile);
  fclose(ldxlFile);
  fclose(olxlFile);
  fclose(hlxlFile);
  fclose(llxlFile);
  fclose(statsFile);
  fclose(pdFile);
  fclose(plFile);
  fclose(clpFile);
  fclose(tfsFile);
  fclose(tfFile);
  fclose(cr1File);
  fclose(cr2File);
  fclose(mcrFile);
  fclose(pcFile);
  fclose(tpcFile);
  fclose(ppcFile);
}

void write_sim_hist_tail_data(double trafficLoad)
{
  int loopIndex;
  char  filename_suffix[100];
  char  filename_str[150];
  FILE *odHistFile, *gspHistFile;

  filename_str[0] = '\0';
  sprintf(filename_str, "od_hist_tail_%d_%d_%s", (int)floor(trafficLoad+0.0001), (int)((trafficLoad+0.0001-(int)floor(trafficLoad+0.0001))*10), filename_suffix);
  odHistFile  = fopen(filename_str,"a");
  filename_str[0] = '\0';
  sprintf(filename_str, "gsp_hist_tail_%d_%d_%s", (int)floor(trafficLoad+0.0001), (int)((trafficLoad+0.0001-(int)floor(trafficLoad+0.0001))*10), filename_suffix);
  gspHistFile = fopen(filename_str,"a");

  /* Report Histograms */
  for(loopIndex=1; loopIndex <= table_histogram_num(overallQueueDelay); loopIndex++)
  {
    /* Print frequency for each bucket */
    fprintf(odHistFile,"%e-%e %e\n",table_histogram_width(overallQueueDelay)*(loopIndex-1)+overallQueueDelayEst.minEst,table_histogram_width(overallQueueDelay)*loopIndex+overallQueueDelayEst.minEst,(double)((double)((unsigned long)table_histogram_bucket(overallQueueDelay,loopIndex))/(double)((unsigned long)table_histogram_total(overallQueueDelay))));
  }

  fclose(odHistFile);
  fclose(gspHistFile);
}

/* 
 * FUNCTION: main()
 * DESCRIPTION: Controls simulation runs and dumps all data to files
 *
 */
int main()
{
  open_TS_pointers();

  // Test Variables main_start
  test_var_init();
  test_vars.main_start++;
  test_var_print();
  TSprint("main started with PID: %d\n\n", getpid());
  

  int runNum, sim_aborted;
  double trafficLoad;
  FILE *pidFile;
  
  /* Report process ID */
  pidFile = fopen("pid","w");
  fprintf(pidFile, "%d\n",getpid());
  fclose(pidFile);
    
  // Read sim parameters from config file [sim_cfg]
  read_sim_cfg_file();

  for(runNum=0; runNum < simParams.NUM_RUNS; runNum++)
  {
    test_vars.runNum = runNum;
    // Assign propagation delays to ONUs (One propagation delay setting per parameter sweep)
    onu_prop_delay_distr(runNum);
    
    test_vars.loadOrderCounter = 0;
    for(trafficLoad=simParams.START_LOAD; trafficLoad <= (simParams.END_LOAD + 0.0001); trafficLoad += simParams.LOAD_INCR)
    {
      // Test Variables main_begin_load
      test_vars.main_begin_load[test_vars.runNum][test_vars.loadOrderCounter]++;
      test_var_print();
      TSprint("main_begin_load\t\t\trunNum: %d\t\tutilization: %.1f\n", test_vars.runNum + 1, 0.1*(test_vars.loadOrderCounter + 1));
    
      sim_aborted = 0;
      printf("LOAD = %f\n", trafficLoad);
      simParams.DESIRED_LOAD = trafficLoad;
    
      /* Do a pilot run of the simulation to estimate statistics */
      simType = PILOT_RUN;
      simParams.simType = PILOT_RUN;
    
      TSprint("Pilot Run\n");
    
      // Test Variables main_test
      test_vars.main_test[test_vars.runNum][test_vars.loadOrderCounter]++;
      test_var_print();

      sim();

      if(terminateSim != 0)
      {
        terminateSim = 0;
        fatalErrorCount = 0;
        rerun();
        sim_cleanup();
        fflush(NULL);
        continue;
      }
      
      /* Estimate histogram maximums */
      estimate_hist_max();
    
      /* Setup the model for next run */
      rerun();
      sim_cleanup(); /* perform a memory clean up */
    
      /* Run the actual simulation */
      simType = ACTUAL_RUN;
      simParams.simType = ACTUAL_RUN;

      // Reset the timers in the onuAttrs.stateTimes
      for(int iaa = 0; iaa < simParams.NUM_ONU; iaa++)
      {
        for(int ibb = 0; ibb < FINAL_eONU_STATE_ENTRY; ibb++)
        {
          onuAttrs[iaa].timeInState[ibb] = 0;
          onuAttrs[iaa].cntState[ibb] = 0; 
        }
        onuAttrs[iaa].state = ONU_ST_IDLE;
        onuAttrs[iaa].timeStateStarted = simtime();
      }

      // Test Variables sim_time_per_load_start
      test_vars.sim_time_per_load_start[test_vars.runNum][test_vars.loadOrderCounter] = simtime();
      test_var_print();

      // Turn on the state time recording.
      simParams.boolRecordStateTime = 1; 

      TSprint("Actual Run\n");
    
      sim();
      fflush(NULL);
     
      // Test Variables sim_time_per_load
      test_vars.sim_time_per_load[test_vars.runNum][test_vars.loadOrderCounter] = simtime() - test_vars.sim_time_per_load_start[test_vars.runNum][test_vars.loadOrderCounter];
      test_var_print();

      // Turn on the state time recording.
      simParams.boolRecordStateTime = 0; 
      
      /* if simulation completes produce output */
      if(terminateSim == 0)
      {
        write_sim_data(runNum,trafficLoad);
      }
      else
      {
        printf("Simulation terminated, moving to next\n");
        terminateSim = 0;
        fatalErrorCount = 0;
        rerun();
        sim_cleanup();
        continue;
      }
      
      /* Setup the model for next run */
      rerun();
      sim_cleanup(); /* perform a memory clean up */
      
      // Test Variables main_end_load
      test_vars.main_end_load[test_vars.runNum][test_vars.loadOrderCounter]++;
      test_var_print();
      TSprint("main_end_load\n\n\n");
    
      test_vars.loadOrderCounter++;
    }
    test_var_print();
  }

  if(!sim_aborted) conclude_csim();

  /* Report simulation complete */
  pidFile = fopen("pid","w");
  fprintf(pidFile, "done\n");
  fclose(pidFile);
  
  // Test Variables main_finish
  test_vars.main_finish++;
  test_var_print();
  TSprint("main_finish\n");
  
  close_TS_pointers();
  
  return 0;
}



