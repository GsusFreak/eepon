
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



/* Simulation End Event */
EVENT SIM_END_EVENT;

/* simulation parameters data structure */
sSIM_PARAMS simParams;

/* ONU attributes structure array */
sONU onuAttrs[MAX_ONU];

/* Scheduling Pool structure array */
sSCHED_POOL schedPool[MAX_ONU];
int schedPoolCount;

FACILITY lambda[MAX_LAMBDAS];		/* each wavelength is modeled as a facility */

double	lambdaFree[MAX_LAMBDAS];	/* array to keep track of lambda free times */
double	lambdaFreeTemp[MAX_LAMBDAS];/* array to keep track of lambda free times (temp version) */

double downstreamFree;              /* keeps track of availability of downstream channel (assumes only GATE messages) */

int		lambdaAssign[MAX_LAMBDAS];	/* array to keep track of lambda ONU assignments */

/* Throughput Fairness data structures */
double actual_tput[MAX_ONU];
double ideal_tput[MAX_ONU];

TABLE 		throughputFairness;
TABLE		overallQueueDelay;
TABLE		overallVideoQueueDelay;
TABLE		overallVideoQueueDelay_MovingAverage;
TABLE		unusedVideoGrant;
TABLE		videoReport;
TABLE		cycleVideoQueueDelay;
TABLE		queueDelay;
TABLE		cycleQueueDelay;
TABLE		heavyQueueDelay;
TABLE		lightQueueDelay;
TABLE		preferQueueDelay;
TABLE		overallQueueLength;
TABLE		heavyQueueLength;
TABLE		lightQueueLength;
TABLE		preferQueueLength;
TABLE		overallGrantSize;
TABLE		overallGrantSizePkt;
TABLE		heavyGrantSize;
TABLE		lightGrantSize;
TABLE		overallCycleLength;
TABLE		heavyCycleLength;
TABLE		lightCycleLength;
TABLE		preferCycleLength;
TABLE		overallRptToSchedTime;
TABLE		heavyRptToSchedTime;
TABLE		lightRptToSchedTime;
TABLE		overallRptToGateTime;
TABLE		heavyRptToGateTime;
TABLE		lightRptToGateTime;
TABLE 	numONUSched;
TABLE 	loadBalanceMeasure;
TABLE 	schedRounds;

TABLE		compRatio1;
TABLE		compRatio2;
TABLE		minCompRatio;

TABLE		excessBandwidth;
TABLE		excessBandwidthONU;

sSTAT_EST	overallQueueDelayEst;
sSTAT_EST	heavyQueueDelayEst;
sSTAT_EST	lightQueueDelayEst;
sSTAT_EST	overallQueueLengthEst;
sSTAT_EST	heavyQueueLengthEst;
sSTAT_EST	lightQueueLengthEst;
sSTAT_EST	overallGrantSizeEst;
sSTAT_EST	overallGrantSizePktEst;
sSTAT_EST	heavyGrantSizeEst;
sSTAT_EST	lightGrantSizeEst;
sSTAT_EST	overallCycleLengthEst;
sSTAT_EST	heavyCycleLengthEst;
sSTAT_EST	lightCycleLengthEst;
sSTAT_EST	overallRptToSchedTimeEst;
sSTAT_EST	heavyRptToSchedTimeEst;
sSTAT_EST	lightRptToSchedTimeEst;
sSTAT_EST	overallRptToGateTimeEst;
sSTAT_EST	heavyRptToGateTimeEst;
sSTAT_EST	lightRptToGateTimeEst;
sSTAT_EST	numONUSchedEst;


sSS_STAT	overallQueueDelayStat;
sSS_STAT	heavyQueueDelayStat;
sSS_STAT	lightQueueDelayStat;
sSS_STAT	overallQueueLengthStat;
sSS_STAT	heavyQueueLengthStat;
sSS_STAT	lightQueueLengthStat;

METER		overallGrantRate;
METER		heavyGrantRate;
METER		lightGrantRate;
METER 	overallZeroReqRate;
METER 	heavyZeroReqRate;
METER 	lightZeroReqRate;
METER 	overallNonzeroReqRate;
METER 	heavyNonzeroReqRate;
METER 	lightNonzeroReqRate;

double      overallQueueDelayTrace[MAX_TRACE_VALUES];
double      overallCycleLengthTrace[MAX_TRACE_VALUES];
double      overallGrantSizeTrace[MAX_TRACE_VALUES];
double      simTimeTrace[MAX_TRACE_VALUES];

// Global variables for the Controls-based Packet Dropping
// Variables
double		output_MFAC	=		0;
double		outputPrev_MFAC =	0;
double		input_MFAC =		0;
double		inputPrev_MFAC =	0;
double		psi_MFAC = 			-1;
//Constants
//double		epsilon =			DBL_MIN*10;
double		epsilon =			0.001;
double		psiInitial_MFAC = 	-1;
double		lambda_MFAC = 		1;
double		eta_MFAC = 			100000;
double		mu_MFAC = 			1;
double		rho_MFAC = 			0.1;
int		flag_firstRun_MFAC =	0;
// END - Global variables for the Controls-based Packet Dropping


/* Parameters for self-similar traffic. */

int i, j;			/* global loop variables */
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
double clockTime1,clockTime2;

/*
 * Grant Trace Data Structures
 */
sGRANT_TRC grantTrace[MAX_LAMBDAS][GRANT_TRACE_SIZE];
int grantTracePtr[MAX_LAMBDAS];

/* This array is used for ordering ONUs for scheduling in LFJ order or in wavelength assignment order */
sONU_LIST *scheduleList[MAX_LAMBDAS];
sONU_LIST *scheduleListLfjSpt[MAX_LAMBDAS];
sONU_LIST *scheduleListLfjLpt[MAX_LAMBDAS];
#ifdef DEBUG_TRC_LO
sONU_LIST *trcList[MAX_LAMBDAS];
#endif

// Declare eponsim.c Troubleshooting Variables
int 	numRuns = 1,
		numLoadLevels = 9,
		numONUs = 32;
// FILE	*traceFileDave;

int TSprint(const char *text, ...)
{
#ifdef EnableTroubleshooting_v2
	va_list args;
	int out;
	
	TSstream = fopen("A_TS_Out", "a");
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
	return 0;
#endif
}


// Print All Test Variables to File
void test_var_print()
{
#ifdef EnableTroubleshooting_v1
	indicatorFile = fopen("A_Test_Variables", "w");
	
	fprintf(indicatorFile, "last_updated_at: %10.5e\n", simtime());
	fprintf(indicatorFile, "heartbeat_process: %.0f\n", test_vars.heartbeat_process);
	fprintf(indicatorFile, "\n");
	fprintf(indicatorFile, "main_start  = %.0f\n", test_vars.main_start);
	fprintf(indicatorFile, "main_finish = %.0f\n", test_vars.main_finish);
	fprintf(indicatorFile, "read_sim_cfg_file_start  = %.0f\n", test_vars.read_sim_cfg_file_start);
	fprintf(indicatorFile, "read_sim_cfg_file_finish = %.0f\n", test_vars.read_sim_cfg_file_finish);
	fprintf(indicatorFile, "\n");
	int iaa;
	for (iaa = 0; iaa < numRuns; iaa++) {
		fprintf(indicatorFile, "Run #%d\n", iaa + 1);
		fprintf(indicatorFile, "main_begin_lambda = %.0f\n", test_vars.main_begin_lambda[iaa]);
		fprintf(indicatorFile, "main_end_lambda   = %.0f\n", test_vars.main_end_lambda[iaa]);
		fprintf(indicatorFile, "\n");
		int ibb;
		for (ibb = 0; ibb < numLoadLevels; ibb++) {
			fprintf(indicatorFile, "Load #%d\n", ibb + 1);
			fprintf(indicatorFile, "main_begin_load = %.0f\n", test_vars.main_begin_load[iaa][ibb]);
			fprintf(indicatorFile, "main_end_load   = %.0f\n", test_vars.main_end_load[iaa][ibb]);
			// fprintf(indicatorFile, "main_test = %.0f\n", test_vars.main_test[iaa][ibb]);
			fprintf(indicatorFile, "\n");
			int icc;
			for (icc = 0; icc < 2; icc++) {
				if (icc == 0)
					fprintf(indicatorFile, "Pilot Run\n");
				if (icc == 1)
					fprintf(indicatorFile, "Actual Run\n");
				fprintf(indicatorFile, "sim_start  = %.0f\n", test_vars.sim_start[iaa][ibb][icc]);
				// fprintf(indicatorFile, "sim_process = %.0f\n", test_vars.sim_process[iaa][ibb][icc]);
				// fprintf(indicatorFile, "sim_before_ONU_processes = %.0f\n", test_vars.sim_before_ONU_processes[iaa][ibb][icc]);
				// fprintf(indicatorFile, "sim_ctrl_videoTraffic = %.0f\n", test_vars.sim_ctrl_videoTraffic[iaa][ibb][icc]);
				// fprintf(indicatorFile, "sim_ctrl_simType = %.0f\n", test_vars.sim_ctrl_simType[iaa][ibb][icc]);
				fprintf(indicatorFile, "sim_finish = %.0f\n", test_vars.sim_finish[iaa][ibb][icc]);
				// fprintf(indicatorFile, "sim_finish2 = %.0f\n", test_vars.sim_finish2[iaa][ibb][icc]);
				fprintf(indicatorFile, "\n");
			}
			for (icc = 0; icc < numONUs; icc++) {
				fprintf(indicatorFile, "ONU  #%d:\t", icc+1);
				if (simParams.SCALABLE_VIDEO_TRAFFIC == SCALABLE_VIDEO_OFF)
				{
					fprintf(indicatorFile, "traffic_src_video_start   = %.0f\n", test_vars.traffic_src_video_start[iaa][ibb][icc]);
					fprintf(indicatorFile, "\t\t\ttraffic_src_video_process = %.0f\n", test_vars.traffic_src_video_process[iaa][ibb][icc]);
					fprintf(indicatorFile, "\t\t\ttraffic_src_video_finish  = %.0f\n", test_vars.traffic_src_video_finish[iaa][ibb][icc]);
				}
				else if (simParams.SCALABLE_VIDEO_TRAFFIC == SCALABLE_VIDEO_ON)
				{
					fprintf(indicatorFile, "traffic_scalable_video_start   = %.0f\n", test_vars.traffic_scalable_video_start[iaa][ibb][icc]);
					fprintf(indicatorFile, "\t\t\ttraffic_scalable_video_process = %.0f\n", test_vars.traffic_scalable_video_process[iaa][ibb][icc]);
					fprintf(indicatorFile, "\t\t\ttraffic_scalable_video_finish  = %.0f\n", test_vars.traffic_scalable_video_finish[iaa][ibb][icc]);
				}
				fprintf(indicatorFile, "\t\t\tvid_pkt_created    = %.0f\n", test_vars.vid_pkt_created[iaa][ibb][icc]);
				fprintf(indicatorFile, "\t\t\tvid_pkt_destroyed  = %.0f\n", test_vars.vid_pkt_destroyed[iaa][ibb][icc]);
				fprintf(indicatorFile, "\t\t\tdata_pkt_created   = %.0f\n", test_vars.data_pkt_created[iaa][ibb][icc]);
				fprintf(indicatorFile, "\t\t\tdata_pkt_destroyed = %.0f\n", test_vars.data_pkt_destroyed[iaa][ibb][icc]);
				fprintf(indicatorFile, "\n");
			}
			fprintf(indicatorFile, "\t\t\tgate_created   = %.0f\n", test_vars.gate_created[iaa][ibb]);
			fprintf(indicatorFile, "\t\t\tgate_destroyed = %.0f\n", test_vars.gate_destroyed[iaa][ibb]);
			fprintf(indicatorFile, "\n");
		}
		fprintf(indicatorFile, "\n");
	}
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
		test_vars.main_begin_lambda[iaa] = 0;
		test_vars.main_end_lambda[iaa] = 0;
		int ibb; // Once for each load level
		for (ibb = 0; ibb < 20; ibb++) {
			test_vars.main_begin_load[iaa][ibb] = 0;
			test_vars.main_end_load[iaa][ibb] = 0;
			test_vars.main_test[iaa][ibb] = 0;
			test_vars.gate_created[iaa][ibb] = 0;
			test_vars.gate_destroyed[iaa][ibb] = 0;
			int icc; // Once for each ONU
			for (icc = 0; icc < 64; icc++)
				test_vars.traffic_src_video_start[iaa][ibb][icc] = 0;
				test_vars.traffic_src_video_process[iaa][ibb][icc] = 0;
				test_vars.traffic_src_video_finish[iaa][ibb][icc] = 0;
				test_vars.traffic_scalable_video_start[iaa][ibb][icc] = 0;
				test_vars.traffic_scalable_video_process[iaa][ibb][icc] = 0;
				test_vars.traffic_scalable_video_finish[iaa][ibb][icc] = 0;
				test_vars.vid_pkt_created[iaa][ibb][icc] = 0;
				test_vars.vid_pkt_destroyed[iaa][ibb][icc] = 0;
				test_vars.data_pkt_created[iaa][ibb][icc] = 0;
				test_vars.data_pkt_destroyed[iaa][ibb][icc] = 0;
			for (icc = 0; icc < 2; icc++) {
				test_vars.sim_start[iaa][ibb][icc] = 0;
				test_vars.sim_process[iaa][ibb][icc] = 0;
				test_vars.sim_finish[iaa][ibb][icc] = 0;
				test_vars.sim_finish2[iaa][ibb][icc] = 0;
				test_vars.sim_before_ONU_processes[iaa][ibb][icc] = 0;
				test_vars.sim_ctrl_simType[iaa][ibb][icc] = 0;
				test_vars.sim_ctrl_simType[iaa][ibb][icc] = 0;
			}
		}
	}
	
	test_vars.loadOrderCounter = 0;
	// traceFileDave = fopen("A_trace_file", "w");
	// set_trace_file(traceFileDave);
	// trace_on();
	
#endif

// Quick sloppy init for a SV dropping algorithm adjustment variable
simParams.SV_DROP_EXP_ADJUST = 10.0;
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
	
	//// Write Consolidated Output File
	//status_processes_temp1 = fopen("Z_status_processes_temp1", "r");
	//status_processes_temp2 = fopen("Z_status_processes_temp2", "r");
	//status_processes_temp3 = fopen("Z_status_processes_temp3", "r");
	//status_processes_output = fopen("Z_status_processes_output", "w");
	
	//fclose(status_processes_output);
#endif
	return;
}

void open_TS_pointers() {
//#ifdef EnableTroubleshooting_v2
	//TSstream = fopen("A_TS_Out", "a");
//#endif
#ifdef EnableTroubleshooting_v4
	char	ONU_file_name[20];
	int 	ONUid;
	for (ONUid = 0; ONUid < 32; ONUid++) 
	{
		sprintf(ONU_file_name, "B_ONU_output_file_%d", ONUid);
		ONU_files[ONUid] = fopen(ONU_file_name, "a");
		ONU_file_name[0] = 0;
	}
#endif
	droppedScalPackets = fopen("A_Dropped_Scalable_Packets", "a");
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
	fclose(droppedScalPackets);
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

//##########################################################################################################################
/* Calculate certain simulation parameters */
void calc_sim_params()
{
    double dutyCycle;
    int loopIndex;

	/* parameters calculated from specified parameters */
	simParams.NUM_PARTS			= (simParams.NUM_ONU - simParams.NUM_HEAVY_ONU) + (simParams.NUM_HEAVY_ONU*simParams.HEAVY_LOAD);
	simParams.LINK_SPEED_PER_PART		= (simParams.LINK_SPEED*simParams.DESIRED_LOAD)/((double)simParams.NUM_PARTS);	/* in bps */
	simParams.AVG_PKT_INTER_ARVL_TIME	= (double)((AVG_PKT_SIZE+PREAMBLE_IPG_BYTES)*8)/simParams.LINK_SPEED_PER_PART;
	simParams.AVG_PKT_INTER_ARVL_TIME_HEAVY	= (double)((AVG_PKT_SIZE+PREAMBLE_IPG_BYTES)*8)/(simParams.LINK_SPEED_PER_PART*simParams.HEAVY_LOAD);
	simParams.TIME_PER_BYTE			= (8)/simParams.LINK_SPEED;	/* at 1 Mbps link speed, byte time is 8 microseconds */
	simParams.PREAMBLE_IPG_TIME		= PREAMBLE_IPG_BYTES*simParams.TIME_PER_BYTE;
	
     /* 
     * Self-Similar parameters 
     */
    /* a - shape parameter, b - location parameter */
    /* a is fixed according to hurst parameter, b is adjusted for load */
	simParams.SS_PARETO_LOC_PARAM		= (double) 1;
	simParams.SS_PARETO_SHAPE_PARAM		= 3-2*simParams.SS_HURST_PARAM;
    /* Calculate Average Burst Size, i.e., the mean of the Pareto distribution with the Shape and Location Params above */
	simParams.AVG_BURST_SIZE	= (simParams.SS_PARETO_LOC_PARAM*simParams.SS_PARETO_SHAPE_PARAM)/(simParams.SS_PARETO_SHAPE_PARAM - 1);
	//simParams.LINK_SPEED_PER_SS_STREAM	= simParams.LINK_SPEED_PER_PART/simParams.NUM_SS_STREAMS;
    /* Calculate AVG ON Period value (in seconds) */
	simParams.AVG_T_ON			= (AVG_PKT_SIZE+PREAMBLE_IPG_BYTES)*simParams.AVG_BURST_SIZE*simParams.TIME_PER_BYTE;

    /* Calculate AVG Off Period values (in seconds), this is how we control the offered load */
    /* Duty cycle should equal the desired load per SS stream */
    // Potential bug in next line, fixed in line after
    //loadPart = simParams.LINK_SPEED_PER_SS_STREAM*simParams.DESIRED_LOAD*(simParams.TIME_PER_BYTE/8);
    dutyCycle = simParams.DESIRED_LOAD/(simParams.NUM_SS_STREAMS*simParams.NUM_PARTS);

	simParams.AVG_T_OFF		= (simParams.AVG_T_ON / dutyCycle) - simParams.AVG_T_ON;
	simParams.AVG_T_OFF_HEAVY	= (simParams.AVG_T_ON / (simParams.HEAVY_LOAD*dutyCycle)) - simParams.AVG_T_ON;
    /* Compute Location parameter (b) for OFF time distribution */
	simParams.SS_OFF_LOC_PARAM		    = simParams.AVG_T_OFF * ((simParams.SS_PARETO_SHAPE_PARAM - 1)/simParams.SS_PARETO_SHAPE_PARAM);
	simParams.SS_OFF_LOC_PARAM_HEAVY	= simParams.AVG_T_OFF_HEAVY * ((simParams.SS_PARETO_SHAPE_PARAM - 1)/simParams.SS_PARETO_SHAPE_PARAM);

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

//##########################################################################################################################
/* Initialize simulation data structures */
void init_data_structures()
{
	fatalErrorCount = 0;
	terminateSim = 0;
	schedPoolCount = 0;
	reset_throughput_flag = 0;
	for(i=0; i < simParams.NUM_ONU; i++)
	{
		onuAttrs[i].packetsHead		= NULL;
		onuAttrs[i].packetsTail		= NULL;
		onuAttrs[i].packetQueueSize	= 0;
		onuAttrs[i].rptQueueSize	= 0;
		onuAttrs[i].packetQueueNum	= 0;
		onuAttrs[i].rptQueueNum	   	= 0;
		onuAttrs[i].latency		= 0;
		onuAttrs[i].readyTime      	= 0;
		onuAttrs[i].step2Sched      	= 0;
        	onuAttrs[i].grantLen        	= 0;
		onuAttrs[i].prevGateTime	= 0;
		onuAttrs[i].transmitByteCnt	= 0;
		schedPool[i].gateLambda		= LAMBDA_NULL;
		schedPool[i].gateLength		= 0;
 		schedPool[i].gateStart		= 1e200;
 		schedPool[i].poolTime		= 1e200;
		schedPool[i].state		= ONU_SCHED_INACTIVE;
		schedPool[i].rounds		= 0;
	}
	downstreamFree = 0;
	for(i=0; i < simParams.NUM_LAMBDAS; i++)
	{
		lambdaFree[i] = 0;
		lambdaAssign[i] = 0;
		grantTracePtr[i] = 0;
	}
	/* initialize time trace data structures */
	for(i=0; i < MAX_TRACE_VALUES; i++)
	{
		overallQueueDelayTrace[i]	= 0;
		simTimeTrace[i]			= 0;
	}
}

//##########################################################################################################################
/* Simulation error handler */
void sim_err_handler(long err_msg_num)
{
	FILE *simcoreFile, *pidFile;
	
	printf("Simulation Error\n");
	
	simcoreFile = fopen("sim_core","a");
	
	fprintf(simcoreFile, "SIM ERROR #%ld: %s\n\n", err_msg_num, (char *)csim_error_msg(err_msg_num));
	
	fclose(simcoreFile);

	/* Report failure in pid */
	pidFile = fopen("pid","w");
	fprintf(pidFile, "Failed\n");
	fclose(pidFile);
}

//##########################################################################################################################
/* Dump Simulation failure data */
void dump_sim_core()
{
	FILE *simcoreFile;
	int loopIndex,loopIndex2;
	
	fatalErrorCount++;
	
	printf("Dumping Sim Core, fatal error count = %d\n",fatalErrorCount);
	
	simcoreFile = fopen("sim_core","a");
	hold(1);

	/* Output date and time of dump */
	
	fprintf(simcoreFile,"wdm_onu=%d,load=%f,lambdas=%d\n", simParams.NUM_WDM_ONU, simParams.DESIRED_LOAD, simParams.NUM_LAMBDAS);

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
	for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
	{
		fprintf(simcoreFile,"qTime[%d] = %e +/- %e, qLen[%d] = %e +/- %e,  ", loopIndex, table_mean(onuAttrs[loopIndex].queueTimeTable), 
		table_conf_halfwidth(onuAttrs[loopIndex].queueTimeTable, 0.98), loopIndex, 
		table_mean(onuAttrs[loopIndex].queueLengthTable), 
		table_conf_halfwidth(onuAttrs[loopIndex].queueLengthTable, 0.98));
	}
    fprintf(simcoreFile,"\n");
	fflush(NULL);
	for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
	{
		fprintf(simcoreFile,"Utilization of Wavelength #%d: %f\n", loopIndex, util(lambda[loopIndex]));
	}
	fflush(NULL);
	fprintf(simcoreFile, "State Information\n");
	for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
	{
		fprintf(simcoreFile, "ONU #%d\n", loopIndex);
		fprintf(simcoreFile, "packetQueueSize = %.0f, ", onuAttrs[loopIndex].packetQueueSize);
		fprintf(simcoreFile, "rptQueueSize = %.0f, ", onuAttrs[loopIndex].rptQueueSize);
		fprintf(simcoreFile, "packetQueueNum = %ld, ", onuAttrs[loopIndex].packetQueueNum);
		fprintf(simcoreFile, "rptQueueNum = %ld, ", onuAttrs[loopIndex].rptQueueNum);
		fprintf(simcoreFile, "grantLen = %.0f, ", onuAttrs[loopIndex].grantLen);
		fprintf(simcoreFile, "rptTime = %e, ", onuAttrs[loopIndex].rptTime);
		fprintf(simcoreFile, "prevGateTime = %e, ", onuAttrs[loopIndex].prevGateTime);
		fprintf(simcoreFile, "readyTime = %e, ", onuAttrs[loopIndex].readyTime);
		fprintf(simcoreFile, "latency = %e, ", onuAttrs[loopIndex].latency);
		fprintf(simcoreFile, "rtt = %e, ", onuAttrs[loopIndex].rtt);
		fprintf(simcoreFile, "numSupportedLambdas = %d, ", onuAttrs[loopIndex].numSupportedLambdas);
        for(loopIndex2=0; loopIndex2 < simParams.NUM_LAMBDAS; loopIndex2++)
        {
            fprintf(simcoreFile, "supportedLambdas[%d] = %d, ", loopIndex2, onuAttrs[loopIndex].supportedLambdas[loopIndex2]);
        }
		fprintf(simcoreFile, "tunedLambda = %d, ", onuAttrs[loopIndex].tunedLambda);
		fprintf(simcoreFile, "wdmType = %d, ", onuAttrs[loopIndex].wdmType);
		fprintf(simcoreFile, "tuningTime = %e, ", onuAttrs[loopIndex].tuningTime);
		fprintf(simcoreFile, "priority = %d, ", onuAttrs[loopIndex].priority);
		fprintf(simcoreFile, "tslotStart = %d\n", onuAttrs[loopIndex].tslotStart);
	}
	fflush(NULL);
	
	for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
	{
		fprintf(simcoreFile, "lambdaFree[%d] = %f, ", loopIndex, lambdaFree[loopIndex]);
	}
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

//##########################################################################################################################
/* Simulation memory cleanup routine */
void sim_cleanup()
{
	int loopIndex;
	/* Free all memory allocated during simulation */
	for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
	{
		/* free all packet memory */
		remove_all_packets(loopIndex);
	}

}

//##########################################################################################################################
/* ONU lambda assignment function */
void lambda_assignment()
{
	int search, loopIndex, lambdaNum, optimal, assignedLambda;
	
	for(loopIndex=0;loopIndex < simParams.NUM_ONU;loopIndex++)
	{
		search = 1;
		lambdaNum = 0;
		optimal = 32000;
		while(search)
		{
			if(onuAttrs[loopIndex].supportedLambdas[lambdaNum] != LAMBDA_NULL)
			{
				if(lambdaAssign[lambdaNum] < optimal)
				{
					optimal = lambdaAssign[lambdaNum];
					assignedLambda = lambdaNum;
					if(optimal == 0)
					{
						search = 0; /* can't do any better */
					}
				}
			}
			else
			{
				/* we have reached the end of this ONUs list of supported wavelengths */
				search = 0;
			}
			lambdaNum++;
			if(lambdaNum >= simParams.NUM_LAMBDAS)
			{
				search = 0; /* we have reached the end of the wavelength list */
			}
		}
		onuAttrs[loopIndex].tunedLambda = assignedLambda;
		lambdaAssign[assignedLambda]++;
	}

}

//##########################################################################################################################
/* heartbeat process with stats time trace, so we know the simulation is still alive and can trace values of statistics */
void heartbeat_with_timetrace()
{
	FILE *simCtrlFile;
	char	currToken[150];
	int beat,loopIndex;
	int beatCnt;

	create("heartbeat");
	
	beat = 0;
    beatCnt = 0;

	while(!terminateSim)
	{
		if(simParams.OLT_TYPE == OLT_APS)
		{
			hold(0.01);
		}
		else
		{
			hold(simParams.SIM_TRACE_TIMESCALE);
		}
		/* Check for excessive buffer sizes */
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
    		if(onuAttrs[loopIndex].packetQueueSize > MAX_PKT_BUF_SIZE)
    		{
                /* Fill out some context information */
                dump_msg_buf[0] = '\0';
                sprintf(dump_msg_buf,"Detected by heart beat process\n");
                sprintf(dump_msg_buf,"%sONU #%d has overflowed it's buffer.\n",dump_msg_buf,loopIndex);
    			fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
    			dump_sim_core();
    		}
        }
        /* Update the hearbeat 10-counter */
        beatCnt++;
        /* Every 10 heart beats check to see that each ONU is receiving a grant */
        if(beatCnt == 10)
        {
            beatCnt = 0; /* reset heart beat count */
            for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
            {
                /* If last grant to an ONU has been more than 10 seconds ago, it is likely not receiving grants */
                if((simtime() - onuAttrs[loopIndex].prevGateTime) > 10)
                {
                    /* Fill out some context information */
                    dump_msg_buf[0] = '\0';
                    sprintf(dump_msg_buf,"Detected by heart beat process\n");
                    sprintf(dump_msg_buf,"%sONU #%d has not received grant in more than 10 seconds.\n",dump_msg_buf,loopIndex);
                    fatalErrorCode = FATAL_CAUSE_NO_GRANT;
                    dump_sim_core();
                }
            }
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
		 /* cycle length */
        for(loopIndex=0; loopIndex < MAX_TRACE_VALUES-1; loopIndex++)
        {
            overallCycleLengthTrace[loopIndex] = overallCycleLengthTrace[loopIndex+1];
        }
        overallCycleLengthTrace[MAX_TRACE_VALUES-1] = table_conf_mean(overallCycleLength);
        simCtrlFile = fopen("cl_trc","w");
        for(loopIndex=0; loopIndex < MAX_TRACE_VALUES; loopIndex++)
        {
            fprintf(simCtrlFile,"%e %e\n",simTimeTrace[loopIndex],overallCycleLengthTrace[loopIndex]);
        }
        fclose(simCtrlFile);
		 /* grant size */
        for(loopIndex=0; loopIndex < MAX_TRACE_VALUES-1; loopIndex++)
        {
            overallGrantSizeTrace[loopIndex] = overallGrantSizeTrace[loopIndex+1];
        }
        overallGrantSizeTrace[MAX_TRACE_VALUES-1] = table_conf_mean(overallGrantSize);
        simCtrlFile = fopen("gs_trc","w");
        for(loopIndex=0; loopIndex < MAX_TRACE_VALUES; loopIndex++)
        {
            fprintf(simCtrlFile,"%e %e\n",simTimeTrace[loopIndex],overallGrantSizeTrace[loopIndex]);
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

//##########################################################################################################################
/* heartbeat process, so we know the simulation is still alive */
void heartbeat()
{
	FILE *simCtrlFile;
	char	currToken[150];
	int beat;
	int beatCnt;
    int loopIndex;

	create("heartbeat");
	
	
	
	beat = 0;
	beatCnt = 0;

	while(!terminateSim)
	{
		if(simParams.OLT_TYPE == OLT_APS)
		{
			hold(0.01);
		}
		else
		{
			hold(simParams.SIM_TRACE_TIMESCALE);
		}
		/* Check for excessive buffer sizes */
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
    		if(onuAttrs[loopIndex].packetQueueSize > MAX_PKT_BUF_SIZE)
    		{
                /* Fill out some context information */
                dump_msg_buf[0] = '\0';
                sprintf(dump_msg_buf,"Detected by heart beat process\n");
                sprintf(dump_msg_buf,"%sONU #%d has overflowed it's buffer.\n",dump_msg_buf,loopIndex);
    			fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
    			dump_sim_core();
    		}
        }
        /* Update the hearbeat 10-counter */
        beatCnt++;
        /* Every 10 heart beats check to see that each ONU is receiving a grant */
        if(beatCnt == 10)
        {
            beatCnt = 0; /* reset heart beat count */
            for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
            {
                /* If last grant to an ONU has been more than 10 seconds ago, it is likely not receiving grants */
                if((simtime() - onuAttrs[loopIndex].prevGateTime) > 10)
                {
                    /* Fill out some context information */
                    dump_msg_buf[0] = '\0';
                    sprintf(dump_msg_buf,"Detected by heart beat process\n");
                    sprintf(dump_msg_buf,"%sONU #%d has not received grant in more than 10 seconds.\n",dump_msg_buf,loopIndex);
                    fatalErrorCode = FATAL_CAUSE_NO_GRANT;
                    dump_sim_core();
                }
            }
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

//##########################################################################################################################
/* 
 * FUNCTION: sim_ctrl()
 * DESCRIPTION: Simulation control flow, controls simulation termination by setting
 *              SIM_END_EVENT event, at appropriate time
 *
 */
void sim_ctrl()
{
	create("sim_ctrl");

#ifdef DEBUG_TRC_LO
    hold(0.03);
#else
    if(simType == PILOT_RUN)
    {
        /* Remove initialization bias, wait for 2000 frames to record queueing delay */
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
        if(simParams.TRAFFIC_TYPE == TRAFFIC_POISSON)
        {
            /* Wait for CI convergence */
            wait(converged);    /* Overall Queue delay */
            wait(converged);    /* Overall Cycle Length */
#if 0
            if((simParams.OLT_TYPE != OLT_ONLINE_NASC) && (simParams.OLT_TYPE != OLT_LEAST_ASSIGNED))
            {
                wait(converged);    /* Overall REPORT-to-Schedule */
            }
            wait(converged);    /* Overall REPORT-to-GATE */
#endif
        }
        else
        {
            //hold(simParams.SIM_TIME);
            while(table_cnt(overallQueueDelay) < (simParams.SIM_TIME*1e6))
            {
                hold(60);
            }
        }
    }

#endif

	if((simParams.VIDEO_TRAFFIC != VIDEO_TRAFFIC_ON) || (simType != ACTUAL_RUN))
	{
		/* Simulation is completed, set SIM_END global event */
		set(SIM_END_EVENT);
	}
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
  	long rand_seed;
  		
  	/* initialize the simulation */
  	create("sim");
  	
  	// Test Variables sim_process
  	// Please note: This flag is also useless. Sim finishes. Bummer.
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
  	
  	///* Start heartbeat */
  	//if(simParams.SIM_TRACE == SIM_TRACE_ON)
  	//{
  	//	heartbeat_with_timetrace();
  	//}
  	//else
  	//{
  	//	heartbeat();
  	//}
  
  	/* 
  	 * Setup Random Number Stream Seeds, start with a base simParams.RAND_SEED
  	 * then increment by 1, every time the seed is used so the next one is the
  	 * previous + 1
  	 */
  	rand_seed = simParams.RAND_SEED;
  		
  	/* initialize the wavelength facilities */
  	for(i=0; i < simParams.NUM_LAMBDAS; i++)
  	{
  	   	tempStr[0] = '\0';
  	   	sprintf(tempStr, "Lambda #%d", i);
  	   	lambda[i] = facility(tempStr);
  	   	lambdaFree[i] = 0;					/* record that this lambda is free at time 0 */
  	}
  		
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
  
  	queueDelay = table("Queue_Delay");
  	cycleQueueDelay = table("Cycle_Queue_Delay");
  	
  	/* Initialize Heavy ONU queueing delay table */
  	heavyQueueDelay = table("Heavy_ONU_Queue_Delay");
   	table_confidence(heavyQueueDelay);
  	
  	/* Initialize Light ONU queueing delay table */
  	lightQueueDelay = table("Light_ONU_Queue_Delay");
   	table_confidence(lightQueueDelay);
  	
  	/* Initialize Preferred ONU queueing delay table */
  	preferQueueDelay = table("Preferred_ONU_Queue_Delay");
   	table_confidence(preferQueueDelay);
  	
  	overallQueueLength = table("Overall_Queue_Length");
   	table_confidence(overallQueueLength);
  
   	heavyQueueLength = table("Heavy_ONU_Queue_Length");
  	table_confidence(heavyQueueLength);
  
   	lightQueueLength = table("Light_ONU_Queue_Length");
  	table_confidence(lightQueueLength);
  
   	preferQueueLength = table("Preferred_ONU_Queue_Length");
  	table_confidence(preferQueueLength);
  	
  	overallGrantSize = table("Overall_Grant_Size");
  	table_confidence(overallGrantSize);
  
  	overallGrantSizePkt = table("Overall_Grant_Size_Pkt");
    if(simType == ACTUAL_RUN)
    {
      	table_histogram(overallGrantSizePkt, 500, 0, overallGrantSizePktEst.maxEst);
    }
    else if(simType == TAIL_RUN)
    {
      	table_histogram(overallGrantSizePkt, 500, overallGrantSizePktEst.minEst, overallGrantSizePktEst.maxEst);
    }
  	table_confidence(overallGrantSizePkt);
  
  	heavyGrantSize = table("Heavy_Grant_Size");
  	table_confidence(heavyGrantSize);
  
  	lightGrantSize = table("Light_Grant_Size");
  	table_confidence(lightGrantSize);
  
    overallCycleLength = table("Overall_Grant_Cycle_Len");
    if(simType == ACTUAL_RUN)
    {
      	table_histogram(overallCycleLength, 500, 0.0, overallQueueDelayEst.maxEst);
    }
    else if(simType == TAIL_RUN)
    {
      	table_histogram(overallCycleLength, 500, overallQueueDelayEst.minEst, overallQueueDelayEst.maxEst);
    }
  	table_confidence(overallCycleLength);
  	if(simParams.TRAFFIC_TYPE != TRAFFIC_SELF_SIMILAR)
  	{
  	  	table_run_length(overallCycleLength, 0.01, 0.95, 10000);
  	}
  	else
  	{
  	    table_run_length(overallCycleLength, 0.05, 0.90, 10000);
  	}
  
    heavyCycleLength = table("Heavy_Grant_Cycle_Len");
  	table_confidence(heavyCycleLength);
  
   	lightCycleLength = table("Light_Grant_Cycle_Len");
  	table_confidence(lightCycleLength);
  
   	preferCycleLength = table("Preferred_Grant_Cycle_Len");
  	table_confidence(preferCycleLength);
  
   	overallRptToSchedTime = table("REPORT_to_Sched_Time");
  	table_confidence(overallRptToSchedTime);
    if(simParams.OLT_TYPE != OLT_ONLINE_NASC && simParams.OLT_TYPE != OLT_IPACT_PSF)
    {
      	if (simParams.TRAFFIC_TYPE != TRAFFIC_SELF_SIMILAR) table_run_length(overallRptToSchedTime, 0.01, 0.95, 10000);
      	else table_run_length(overallRptToSchedTime, 0.05, 0.90, 10000);
    }
  
    heavyRptToSchedTime = table("Heavy_REPORT_to_Sched_Time");
  	table_confidence(heavyRptToSchedTime);
  
  	lightRptToSchedTime = table("Light_REPORT_to_Sched_Time");
  	table_confidence(lightRptToSchedTime);
  
    overallRptToGateTime = table("REPORT_to_GATE_Time");
  	table_confidence(overallRptToGateTime);
  	if (simParams.TRAFFIC_TYPE != TRAFFIC_SELF_SIMILAR) table_run_length(overallRptToGateTime, 0.01, 0.95, 10000);
    else table_run_length(overallRptToGateTime, 0.01, 0.90, 10000);
  
  	heavyRptToGateTime = table("Heavy_REPORT_to_GATE_Time");
  	table_confidence(heavyRptToGateTime);
  
    lightRptToGateTime = table("Light_REPORT_to_GATE_Time");
  	table_confidence(lightRptToGateTime);
  
    numONUSched = table("Number_of_ONUs_Scheduled");
  
    loadBalanceMeasure = table("Load_Balance");
  
    schedRounds = table("Scheduling_Rounds");
  
    throughputFairness = table("Throughput_Fairness");
  
    compRatio1 = table("Competitive_Ratio_1");
    compRatio2 = table("Competitive_Ratio_2");
    minCompRatio = table("Min_Competitive_Ratio");
  
    excessBandwidth = table("Excess_Bandwidth");
    excessBandwidthONU = table("Excess_Bandwidth_ONU");
  
    overallGrantRate = meter("Grant_Rate");
  	meter_confidence(overallGrantRate);
  
    heavyGrantRate = meter("Heavy_Grant_Rate");
  	meter_confidence(heavyGrantRate);
  
    lightGrantRate = meter("Light_Grant_Rate");
  	meter_confidence(lightGrantRate);
  
    overallZeroReqRate = meter("Zero_Request_Rate");
  	meter_confidence(overallZeroReqRate);
  
    heavyZeroReqRate = meter("Heavy_Zero_Request_Rate");
  	meter_confidence(heavyZeroReqRate);
  
    lightZeroReqRate = meter("Light_Zero_Request_Rate");
  	meter_confidence(lightZeroReqRate);
  
    overallNonzeroReqRate = meter("NonZero_Request_Rate");
  	meter_confidence(overallNonzeroReqRate);
  	
    heavyNonzeroReqRate = meter("Heavy_NonZero_Request_Rate");
  	meter_confidence(heavyNonzeroReqRate);
  	
    lightNonzeroReqRate = meter("Light_NonZero_Request_Rate");
  	meter_confidence(lightNonzeroReqRate);
  	
  	/* setup empirical distribution for packet sizes */
  	setup_empirical(EMPIRICAL_SIZE, EMPIRICAL_PROB, EMPIRICAL_CUTOFF, EMPIRICAL_ALIAS);
  	
  	// Test Variables sim_before_ONU_processes
  	// Please note: This flag is actually useless. Bummer.
  	if (simType == PILOT_RUN) {
  	    test_vars.sim_before_ONU_processes[test_vars.runNum][test_vars.loadOrderCounter][0]++;
  	    test_var_print();
  	}
  	if (simType == ACTUAL_RUN) {
  	    test_vars.sim_before_ONU_processes[test_vars.runNum][test_vars.loadOrderCounter][1]++;
  	    test_var_print();
  	}
  
  
  	/* Start the ONU processes */
  	for(i=0; i < simParams.NUM_ONU; i++)
  	{
    	  /* 
  		   * initialize ONU structures 
  		   */
  		  
  		  tempStr[0] = '\0'; 
  		  sprintf(tempStr, "Tx Time for ONU #%d", i);
  		  onuAttrs[i].transmitTimeTable = table(tempStr);
  		  table_confidence(onuAttrs[i].transmitTimeTable);
  		  tempStr[0] = '\0'; 
  		  sprintf(tempStr, "Queue Time for ONU #%d", i);
  		  onuAttrs[i].queueTimeTable = table(tempStr);
  		  table_confidence(onuAttrs[i].queueTimeTable);
  		  tempStr[0] = '\0'; 
  		  sprintf(tempStr, "Queue Length for ONU #%d", i);
  		  onuAttrs[i].queueLengthTable = table(tempStr);
  		  table_confidence(onuAttrs[i].queueLengthTable);
  		  tempStr[0] = '\0';
  		  sprintf(tempStr, "Throughput for ONU #%d", i);
  		  onuAttrs[i].transmitThroughput = table(tempStr);
  		  table_confidence(onuAttrs[i].transmitThroughput);
        onuAttrs[i].zeroReqRate = meter("Zero Request Rate");
        meter_confidence(onuAttrs[i].zeroReqRate);
        onuAttrs[i].nonzeroReqRate = meter("Non-Zero Request Rate");
        meter_confidence(onuAttrs[i].nonzeroReqRate);
  		  /* setup ONU latency */
  		  onuAttrs[i].latency = simParams.ONU_PROP[i]; /* ONU distance of 10 to 20 km */
  		  onuAttrs[i].rtt	= onuAttrs[i].latency*2;
  		  /*
  		   * Setup ONU WDM information
  		   */
        onuAttrs[i].numSupportedLambdas = 0;
  		  if(i < simParams.NUM_WDM_ONU)
  		  {
  		    	/* Setup simParams.NUM_WDM_ONU worth of ONUs as WDM supporting */
  		    	for(j=0;j < simParams.NUM_LAMBDAS; j++)
  		    	{
  		    		  onuAttrs[i].supportedLambdas[j] = j;
               	onuAttrs[i].supportedLambdasMap[j] = LAMBDA_TRUE;
  		    	}
  		    	onuAttrs[i].supportedLambdas[j] = LAMBDA_NULL;
            onuAttrs[i].numSupportedLambdas = j;
  		    	onuAttrs[i].tunedLambda = 0; /* If tunable, initially tuned to lambda #0 */
  		    	if(simParams.WDM_TYPE == WDM_TUNABLE)
  		    	{
  		    	  	onuAttrs[i].wdmType = WDM_TUNABLE;
  		    	  	onuAttrs[i].tuningTime = simParams.TUNING_TIME;
  		    	}
  		    	else
  		    	{
  		    	  	onuAttrs[i].wdmType = WDM_FIXED;
  		    	}
  		  }
  		  else if(i < (simParams.NUM_WDM_ONU + simParams.NUM_LH_WDM_ONU))
        {
  			    /* Setup simParams.NUM_WDM_ONU worth of ONUs as WDM supporting */
  			    for(j=0;j < (simParams.NUM_LAMBDAS)/2; j++)
  			    {
  			      	onuAttrs[i].supportedLambdas[j] = j;
  			      	onuAttrs[i].supportedLambdasMap[j] = LAMBDA_TRUE;
  			    }
  			    onuAttrs[i].supportedLambdas[j] = LAMBDA_NULL;
                  		onuAttrs[i].numSupportedLambdas = j;
  			    for(j=(simParams.NUM_LAMBDAS)/2;j < simParams.NUM_LAMBDAS; j++)
  			    {
  			    	  onuAttrs[i].supportedLambdasMap[j] = LAMBDA_FALSE;
  			    }
  			    onuAttrs[i].tunedLambda = 0; /* If tunable, initially tuned to lambda #0 */
  			    if(simParams.WDM_TYPE == WDM_TUNABLE)
  			    {
  			    	  onuAttrs[i].wdmType = WDM_TUNABLE;
  			    	  onuAttrs[i].tuningTime = simParams.TUNING_TIME;
  			    }
  			    else
  			    {
  			    	  onuAttrs[i].wdmType = WDM_FIXED;
  			    }
  		  }
  		  else if(i < (simParams.NUM_WDM_ONU + simParams.NUM_LH_WDM_ONU + simParams.NUM_UH_WDM_ONU))
  	    {
  		    	/* Setup simParams.NUM_WDM_ONU worth of ONUs as WDM supporting */
  		    	for(j=0;j < (simParams.NUM_LAMBDAS)/2; j++)
  		    	{
  		    		  onuAttrs[i].supportedLambdasMap[j] = LAMBDA_FALSE;
  		    	}
  		    	for(j=(simParams.NUM_LAMBDAS)/2;j < simParams.NUM_LAMBDAS; j++)
  		    	{
  		    		  onuAttrs[i].supportedLambdas[j-(simParams.NUM_LAMBDAS)/2] = j;
  		    		  onuAttrs[i].supportedLambdasMap[j] = LAMBDA_TRUE;
  		    	}
  		    	onuAttrs[i].supportedLambdas[j-(simParams.NUM_LAMBDAS)/2] = LAMBDA_NULL;
            onuAttrs[i].numSupportedLambdas = j-(simParams.NUM_LAMBDAS)/2;
  		    	onuAttrs[i].tunedLambda = 0; /* If tunable, initially tuned to lambda #0 */
  		    	if(simParams.WDM_TYPE == WDM_TUNABLE)
  		    	{
  		    		  onuAttrs[i].wdmType = WDM_TUNABLE;
  		    		  onuAttrs[i].tuningTime = simParams.TUNING_TIME;
  		    	}
  		    	else
  		    	{
  		    		  onuAttrs[i].wdmType = WDM_FIXED;
  		    	}
        }
        else
  		  {
  		    	onuAttrs[i].wdmType = WDM_NONE;
  		    	/* the rest only support a single wavelength */
  		    	onuAttrs[i].supportedLambdas[0] = 0;
  		    	onuAttrs[i].supportedLambdasMap[0] = LAMBDA_TRUE;
           	for(j=1;j < simParams.NUM_LAMBDAS+1; j++)
            {
                onuAttrs[i].supportedLambdas[j] = LAMBDA_NULL;
                onuAttrs[i].supportedLambdasMap[j] = LAMBDA_FALSE;
            }
            onuAttrs[i].numSupportedLambdas = 1;
        }
  		
  	  	/* Setup random number streams */
  	  	onuAttrs[i].pktInterArrivalStream = create_stream();
  	  	reseed(onuAttrs[i].pktInterArrivalStream,rand_seed++);
  	  	onuAttrs[i].pktSizeStream = create_stream();
  	  	reseed(onuAttrs[i].pktSizeStream,rand_seed++);
  	  	onuAttrs[i].burstSizeStream = create_stream();\
  	  	reseed(onuAttrs[i].burstSizeStream,rand_seed++);
  	  	/* Setup packet arrival mailboxes */
  	  	tempStr[0] = '\0'; 
  	  	sprintf(tempStr, "ONU #%d pkt mb", i);
  	  	onuAttrs[i].pktMailbox = mailbox(tempStr);
  	  	for(j=0;j < simParams.NUM_LAMBDAS; j++)
  	  	{
  	  		  if(onuAttrs[i].supportedLambdasMap[j] == LAMBDA_TRUE)
  	  		  {
  	  		    	/* Setup GATE msg mailboxes */
  	  		    	tempStr[0] = '\0'; 
  	  		    	sprintf(tempStr, "ONU #%d,L#%d grant mb", i,j);
  	  		    	onuAttrs[i].grantMailbox[j] = mailbox(tempStr);
  	  		    	olt(i,j); /* spawn an ONU process for each wavelength */
  	  		  }
  	  	}
  	  	/* Spawn Traffic generator(s) for ONU */
  	  	switch(simParams.TRAFFIC_TYPE)
  	  	{
  	  	  	case TRAFFIC_POISSON:
  	  	  		  traffic_src_poisson(i); /* spawn poisson traffic src for ONU */
  	  	  		  break;
  	  	  	case TRAFFIC_SELF_SIMILAR:
  	  	    		traffic_agg_self_similar(i);	/* spawn the aggregator */
  	  	    		for(j=0;j < simParams.NUM_SS_STREAMS; j++)
  	  	    		{
  	  	    			  traffic_src_self_similar(i,j); /* spawn the individual streams */
  	  	    		}
  	  	    		break;
  	  	  	default:
  	  	  		  break;
  	  	}
  	}
  	
  	if(simParams.OLT_TYPE == OLT_LEAST_ASSIGNED) lambda_assignment();
  	
  	for(i=0; i < simParams.NUM_ONU; i++) printf("ONU #%d assigned to lambda #%d\n", i, onuAttrs[i].tunedLambda);
  
  	/* Start the new ONU process */
  	onu();
  
    /* Start the throughput calculation process */
    onu_throughput_calc();
  	
  	/* Initialize the Simulation End Event */
  	SIM_END_EVENT = event("Sim End");
  
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
  	
  	// Test Variabels sim_finish2
  	if (simType == PILOT_RUN) {
  		  test_vars.sim_finish2[test_vars.runNum][test_vars.loadOrderCounter][0]++;
  		  test_var_print();
  	}
  	if (simType == ACTUAL_RUN) {
  		  test_vars.sim_finish2[test_vars.runNum][test_vars.loadOrderCounter][1]++;
  		  test_var_print();
  	}
  	
    /* Flush grant trace file if necessary */
    if(simParams.GRANT_TRACE == GRANT_TRACE_ON)
    {
      	grant_trace_flush();
    }
 		if(!terminateSim)
 		{
 			  /* If simulation terminated normally, then dump stat report */
 			  report();
 		}
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
	simParams.OLT_TYPE		= OLT_ONLINE_NASC;	/* OLT Type */
	simParams.DBA_TYPE		= DBA_GATED;		/* DBA Type */
	simParams.VIDEO_DBA_TYPE	= VIDEO_DBA_GATED;	/* Video DBA Type */
	simParams.TRAFFIC_TYPE		= TRAFFIC_POISSON;	/* Traffic Type */
	simParams.WDM_TYPE		= WDM_FIXED;		/* WDM Type */
	simParams.TIME_SHIFT		= 0.0;
	simParams.GUARD_TIME		= 5e-6;
	simParams.TUNING_TIME		= 5e-3;
	simParams.SIM_TIME		= 300;
	simParams.NUM_RUNS		= 1;
	simParams.NUM_ONU		= 10;			/* Number of ONUs on PON */
	simParams.NUM_HEAVY_ONU		= 5;			/* Number of heavily loaded ONUs */
	simParams.LINK_SPEED		= 1e9;			/* link speed in bps */
	
	simParams.NUM_WDM_ONU		= 5;			/* Number of WDM ONUs */
	simParams.NUM_UH_WDM_ONU	= 0;			/* Number of Upper Half WDM ONUs */
	simParams.NUM_LH_WDM_ONU	= 0;			/* Number of Lower Half WDM ONUs */
	simParams.NUM_PREFERRED_ONU	= 0;			/* Number of Preferrentially treated ONUs */

   	 simParams.HEAVY_LOAD		= 2;			/* heavy load multiple */
	
	simParams.SS_HURST_PARAM	= 0.75;			/* Self Similar Traffic Source Hurst Parameter */
	simParams.NUM_SS_STREAMS	= 32;			/* Number of Self Similar Traffic Streams */
	
	simParams.START_LOAD		= 0.3;
	simParams.END_LOAD		= 2.0;
	simParams.LOAD_INCR		= 0.1;

	simParams.START_LAMBDA		= 1;
	simParams.END_LAMBDA		= 6;
	
	simParams.FIXED_GRANT_SIZE 	= 1514+PREAMBLE_IPG_BYTES;
	simParams.MAX_GRANT_SIZE 	= 1514*5;
	simParams.SCHEDULE_INTERVAL 	= 0;
	simParams.SCHEDULER_SLOT_SIZE 	= 64;
	simParams.SCHEDULER_NUM_SLOT 	= 180;
	simParams.SCHEDULER_MAX_WINDOW 	= 0;
	simParams.STARVE_THRESH 	= simParams.NUM_ONU;
	
	simParams.SCALABLE_VIDEO_TRAFFIC				= 0;
	simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM		= 0;
	simParams.SCALABLE_VIDEO_NUM_LAYERS				= 1;
	simParams.SV_DROP_NUM_VALUES_FOR_MA				= 5;
	simParams.SV_DROP_MIN_BOUND						= 0.0005;
	simParams.SV_DROP_MAX_BOUND						= 0.005;

	/* Availability Cost Weight for WBM schedulers */
	simParams.AVAIL_COST_WEIGHT = 1.0;
	
	/* Generate Random Number Seed */
	simParams.RAND_SEED = 200100;

	/* Fixed Prop Delay */
	simParams.FIXED_PROP_DELAY = FIXED_PROP_DELAY_OFF;
	simParams.FIXED_PROP_DELAY_VALUE = 50e-6;

	/* Maximum Prop Delay */
	simParams.MAX_PROP_DELAY = 50e-6;

	/* Get Tails of Distributions */
	simParams.GET_TAIL = GET_TAIL_OFF;

	/* Grant Trace */
	simParams.GRANT_TRACE = GRANT_TRACE_OFF;
	
	/* Simulation Trace */
	simParams.SIM_TRACE = SIM_TRACE_OFF;
	
	/* Simulation Trace Time */
	simParams.SIM_TRACE_TIME = 1000;	/* sim trace time in seconds */
	simParams.SIM_TRACE_TIMESCALE = 10; 	/* sim trace timescale in seconds */

	
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
#ifdef DEBUG_TRC
			printf("currToken = %s, ", currToken);
#endif
			
			if(strcmp(currToken, "OLT_TYPE") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				if(strcmp(currToken, "OLT_WDM_IPACT") == 0)
				{
					simParams.OLT_TYPE = OLT_WDM_IPACT;
				}
				else if(strcmp(currToken, "OLT_IPACT_PSF") == 0)
				{
					simParams.OLT_TYPE = OLT_IPACT_PSF;
				}
				else if(strcmp(currToken, "OLT_IPACT") == 0)
				{
					simParams.OLT_TYPE = OLT_IPACT;
				}
				else if(strcmp(currToken, "OLT_LEAST_ASSIGNED") == 0)
				{
					simParams.OLT_TYPE = OLT_LEAST_ASSIGNED;
				}
				else if(strcmp(currToken, "OLT_APS") == 0)
				{
					simParams.OLT_TYPE = OLT_APS;
				}
				else if(strcmp(currToken, "OLT_LFJ") == 0)
				{
					simParams.OLT_TYPE = OLT_LFJ;
				}
				else if(strcmp(currToken, "OLT_LFJ_LPT") == 0)
				{
					simParams.OLT_TYPE = OLT_LFJ_LPT;
				}
				else if(strcmp(currToken, "OLT_LFJ_SPT") == 0)
				{
					simParams.OLT_TYPE = OLT_LFJ_SPT;
				}
				else if(strcmp(currToken, "OLT_LFJ_LNF") == 0)
				{
					simParams.OLT_TYPE = OLT_LFJ_LNF;
				}
				else if(strcmp(currToken, "OLT_EAF") == 0)
				{
					simParams.OLT_TYPE = OLT_EAF;
				}
				else if(strcmp(currToken, "OLT_EAAF") == 0)
				{
					simParams.OLT_TYPE = OLT_EAAF;
				}
				else if(strcmp(currToken, "OLT_WBM") == 0)
				{
					simParams.OLT_TYPE = OLT_WBM;
				}
				else if(strcmp(currToken, "OLT_WBM_LPT") == 0)
				{
					simParams.OLT_TYPE = OLT_WBM_LPT;
				}
				else if(strcmp(currToken, "OLT_WBM_LNF") == 0)
				{
					simParams.OLT_TYPE = OLT_WBM_LNF;
				}
				else if(strcmp(currToken, "OLT_WBM_EAAF") == 0)
				{
					simParams.OLT_TYPE = OLT_WBM_EAAF;
				}
				else if(strcmp(currToken, "OLT_SPD") == 0)
				{
					simParams.OLT_TYPE = OLT_SPD;
				}
				else if(strcmp(currToken, "OLT_LPD") == 0)
				{
					simParams.OLT_TYPE = OLT_LPD;
				}
				else if(strcmp(currToken, "OLT_SPD_LPT") == 0)
				{
					simParams.OLT_TYPE = OLT_SPD_LPT;
				}
				else if(strcmp(currToken, "OLT_ONLINE_NASC") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_NASC;
				}
				else if(strcmp(currToken, "OLT_ONLINE_INTERVAL_LFJ") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_INTERVAL_LFJ_LPT;
				}
				else if(strcmp(currToken, "OLT_ONLINE_INTERVAL_LFJ_LPT") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_INTERVAL_LFJ_LPT;
				}
				else if(strcmp(currToken, "OLT_ONLINE_INTERVAL_LFJ_SPT") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_INTERVAL_LFJ_SPT;
				}
				else if(strcmp(currToken, "OLT_ONLINE_INTERVAL_LFJ_LNF") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_INTERVAL_LFJ_LNF;
				}
				else if(strcmp(currToken, "OLT_ONLINE_INTERVAL_WBM") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_INTERVAL_WBM;
				}
				else if(strcmp(currToken, "OLT_ONLINE_INTERVAL_WBM_LPT") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_INTERVAL_WBM_LPT;
				}
				else if(strcmp(currToken, "OLT_ONLINE_INTERVAL_WBM_LNF") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_INTERVAL_WBM_LNF;
				}
				else if(strcmp(currToken, "OLT_ONLINE_INTERVAL_WBM_EAAF") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_INTERVAL_WBM_EAAF;
				}
				else if(strcmp(currToken, "OLT_ONLINE_INTERVAL_EAF") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_INTERVAL_EAF;
				}
				else if(strcmp(currToken, "OLT_ONLINE_INTERVAL_EAAF") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_INTERVAL_EAAF;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_LFJ_LPT") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_LFJ_LPT;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_LFJ_LNF") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_LFJ_LNF;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_LFJ_SPT") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_LFJ_SPT;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_WBM") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_WBM;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_WBM_LPT") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_WBM_LPT;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_WBM_LNF") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_WBM_LNF;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_WBM_EAAF") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_WBM_EAAF;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_EAF") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_EAF;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_EAAF") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_EAAF;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_SPD") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_SPD;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_LPD") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_LPD;
				}
				else if(strcmp(currToken, "OLT_ONLINE_JIT_TEST") == 0)
				{
					simParams.OLT_TYPE = OLT_ONLINE_JIT_TEST;
				}
			}
			else if(strcmp(currToken, "DBA_TYPE") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				if(strcmp(currToken, "DBA_FIXED") == 0)
				{
					simParams.DBA_TYPE = DBA_FIXED;
				}
				else if(strcmp(currToken, "DBA_LIMITED_GATE") == 0)
				{
					simParams.DBA_TYPE = DBA_LIMITED_GATE;
				}
				else if(strcmp(currToken, "DBA_LIMITED_CYCLE") == 0)
				{
					simParams.DBA_TYPE = DBA_LIMITED_CYCLE;
				}
				else if(strcmp(currToken, "DBA_GATED") == 0)
				{
					simParams.DBA_TYPE = DBA_GATED;
				}
				else if(strcmp(currToken, "DBA_CONSTANT_CREDIT") == 0)
				{
					simParams.DBA_TYPE = DBA_CONSTANT_CREDIT;
				}
				else if(strcmp(currToken, "DBA_LINEAR_CREDIT") == 0)
				{
					simParams.DBA_TYPE = DBA_LINEAR_CREDIT;
				}
				else if(strcmp(currToken, "DBA_ELASTIC") == 0)
				{
					simParams.DBA_TYPE = DBA_ELASTIC;
				}
				else if(strcmp(currToken, "DBA_EXCESS") == 0)
				{
					simParams.DBA_TYPE = DBA_EXCESS;
				}
			}
			else if(strcmp(currToken, "VIDEO_DBA_TYPE") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				if(strcmp(currToken, "VIDEO_DBA_FIXED") == 0)
				{
					simParams.VIDEO_DBA_TYPE = VIDEO_DBA_FIXED;
				}
				else if(strcmp(currToken, "VIDEO_DBA_LIMITED_GATE") == 0)
				{
					simParams.VIDEO_DBA_TYPE = VIDEO_DBA_LIMITED_GATE;
				}
				else if(strcmp(currToken, "VIDEO_DBA_GATED") == 0)
				{
					simParams.VIDEO_DBA_TYPE = VIDEO_DBA_GATED;
				}
			}
			else if(strcmp(currToken, "TRAFFIC_TYPE") == 0)
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
			else if(strcmp(currToken, "VIDEO_TRAFFIC") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				if(strcmp(currToken, "VIDEO_TRAFFIC_ON") == 0)
				{
					simParams.VIDEO_TRAFFIC = VIDEO_TRAFFIC_ON;
				}
				else if(strcmp(currToken, "VIDEO_TRAFFIC_OFF") == 0)
				{
					simParams.VIDEO_TRAFFIC = VIDEO_TRAFFIC_OFF;
				}
			}
			else if(strcmp(currToken, "SCALABLE_VIDEO_TRAFFIC") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				if(strcmp(currToken, "SCALABLE_VIDEO_ON") == 0)
				{
					simParams.SCALABLE_VIDEO_TRAFFIC = SCALABLE_VIDEO_ON;
				}
				else if(strcmp(currToken, "SCALABLE_VIDEO_OFF") == 0)
				{
					simParams.SCALABLE_VIDEO_TRAFFIC = SCALABLE_VIDEO_OFF;
				}
			}
			else if(strcmp(currToken, "SCALABLE_VIDEO_DROPPING_ALGORITHM") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				if(strcmp(currToken, "SCALABLE_VIDEO_DROPPING_OFF") == 0)
				{
					simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM = SCALABLE_VIDEO_DROPPING_OFF;
				}
				else if(strcmp(currToken, "SCALABLE_VIDEO_DROPPING_THRESHOLD") == 0)
				{
					simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM = SCALABLE_VIDEO_DROPPING_THRESHOLD;
				}
				else if(strcmp(currToken, "SCALABLE_VIDEO_DROPPING_DYNAMIC") == 0)
				{
					simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM = SCALABLE_VIDEO_DROPPING_DYNAMIC;
				}
				else if(strcmp(currToken, "SCALABLE_VIDEO_DROPPING_STEP_THRESHOLD") == 0)
				{
					simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM = SCALABLE_VIDEO_DROPPING_LINEAR_THRESHOLD;
				}
				else if(strcmp(currToken, "SCALABLE_VIDEO_DROPPING_CFDL_MFAC") == 0)
				{
					simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM = SCALABLE_VIDEO_DROPPING_CFDL_MFAC;
				}
				else if(strcmp(currToken, "SCALABLE_VIDEO_DROPPING_EXP_THRESHOLD") == 0)
				{
					simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM = SCALABLE_VIDEO_DROPPING_EXP_THRESHOLD;
				}
			}
			else if(strcmp(currToken, "VIDEO_TRACE_FILE") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				strcpy(simParams.VIDEO_TRACE_FILE, currToken);
				
			}
			else if(strcmp(currToken, "VIDEO_PREDICTION") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				if(strcmp(currToken, "VIDEO_PREDICTION_ON") == 0)
				{
					simParams.VIDEO_PREDICTION = VIDEO_PREDICTION_ON;
				}
				else if(strcmp(currToken, "VIDEO_PREDICTION_OFF") == 0)
				{
					simParams.VIDEO_PREDICTION = VIDEO_PREDICTION_OFF;
				}
			}
			else if(strcmp(currToken, "FRAME_RATE") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				simParams.FRAME_RATE = atof(currToken);
				
			}
			else if(strcmp(currToken, "TIME_SHIFT") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				simParams.TIME_SHIFT = atof(currToken);
				
			}
			else if(strcmp(currToken, "SV_DROP_LAYER_THRESHOLD") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				simParams.SV_DROP_LAYER_THRESHOLD = atof(currToken);
				
			}
			else if(strcmp(currToken, "SV_DROP_SENSITIVITY") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				simParams.SV_DROP_SENSITIVITY = atof(currToken);
				
			}
			else if(strcmp(currToken, "SV_MIN_BOUND") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				simParams.SV_DROP_MIN_BOUND = atof(currToken);
				
			}
			else if(strcmp(currToken, "SV_MAX_BOUND") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				simParams.SV_DROP_MAX_BOUND = atof(currToken);
				
			}
			else if(strcmp(currToken, "SV_NUM_MA_VALUES") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				simParams.SV_DROP_NUM_VALUES_FOR_MA = atof(currToken);
				
			}
			else if(strcmp(currToken, "PREDICTION_TYPE") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				strcpy(simParams.PREDICTION_TYPE, currToken);
			}
			else if(strcmp(currToken, "WDM_TYPE") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				if(strcmp(currToken, "WDM_FIXED") == 0)
				{
					simParams.WDM_TYPE = WDM_FIXED;
				}
				else if(strcmp(currToken, "WDM_TUNABLE") == 0)
				{
					simParams.WDM_TYPE = WDM_TUNABLE;
				}
			}
			else if(strcmp(currToken, "GUARD_TIME") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.GUARD_TIME = atof(currToken);
#ifdef DEBUG_TRC
				printf("GUARD_TIME = %e\n", simParams.GUARD_TIME);
#endif
			}
			else if(strcmp(currToken, "TUNING_TIME") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.TUNING_TIME = atof(currToken);
#ifdef DEBUG_TRC
				printf("TUNING_TIME = %e\n", simParams.TUNING_TIME);
#endif
			}
			else if(strcmp(currToken, "SIM_TIME") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.SIM_TIME = atof(currToken);
#ifdef DEBUG_TRC
				printf("SIM_TIME = %e\n", simParams.SIM_TIME);
#endif
			}
			else if(strcmp(currToken, "NUM_RUNS") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.NUM_RUNS = atoi(currToken);
#ifdef DEBUG_TRC
				printf("NUM_RUNS = %d\n", simParams.NUM_RUNS);
#endif
            }
			else if(strcmp(currToken, "NUM_ONU") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.NUM_ONU = atoi(currToken);
#ifdef DEBUG_TRC
				printf("NUM_ONU = %d\n", simParams.NUM_ONU);
#endif
			}
			else if(strcmp(currToken, "NUM_HEAVY_ONU") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.NUM_HEAVY_ONU = atoi(currToken);
#ifdef DEBUG_TRC
				printf("NUM_HEAVY_ONU = %d\n", simParams.NUM_HEAVY_ONU);
#endif
			}
			else if(strcmp(currToken, "NUM_WDM_ONU") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.NUM_WDM_ONU = atoi(currToken);
#ifdef DEBUG_TRC
				printf("NUM_WDM_ONU = %d\n", simParams.NUM_WDM_ONU);
#endif
			}
			else if(strcmp(currToken, "NUM_UH_WDM_ONU") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.NUM_UH_WDM_ONU = atoi(currToken);
#ifdef DEBUG_TRC
				printf("NUM_UH_WDM_ONU = %d\n", simParams.NUM_UH_WDM_ONU);
#endif
			}
			else if(strcmp(currToken, "NUM_LH_WDM_ONU") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.NUM_LH_WDM_ONU = atoi(currToken);
#ifdef DEBUG_TRC
				printf("NUM_LH_WDM_ONU = %d\n", simParams.NUM_LH_WDM_ONU);
#endif
			}
			else if(strcmp(currToken, "NUM_PREFERRED_ONU") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.NUM_PREFERRED_ONU = atoi(currToken);
#ifdef DEBUG_TRC
				printf("NUM_PREFERRED_ONU = %d\n", simParams.NUM_PREFERRED_ONU);
#endif
			}
			else if(strcmp(currToken, "HEAVY_LOAD") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.HEAVY_LOAD = atoi(currToken);
#ifdef DEBUG_TRC
				printf("HEAVY_LOAD = %d\n", simParams.HEAVY_LOAD);
#endif
			}
			else if(strcmp(currToken, "LINK_SPEED") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.LINK_SPEED = atof(currToken);
#ifdef DEBUG_TRC
				printf("LINK_SPEED = %e\n", simParams.LINK_SPEED);
#endif
			}
			else if(strcmp(currToken, "SS_HURST_PARAM") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.SS_HURST_PARAM = atof(currToken);
#ifdef DEBUG_TRC
				printf("SS_HURST_PARAM = %e\n", simParams.SS_HURST_PARAM);
#endif
			}
			else if(strcmp(currToken, "NUM_SS_STREAMS") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.NUM_SS_STREAMS = atoi(currToken);
#ifdef DEBUG_TRC
				printf("NUM_SS_STREAMS = %d\n", simParams.NUM_SS_STREAMS);
#endif
			}
			else if(strcmp(currToken, "START_LOAD") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.START_LOAD = atof(currToken);
#ifdef DEBUG_TRC
				printf("START_LOAD = %e\n", simParams.START_LOAD);
#endif
			}
			else if(strcmp(currToken, "END_LOAD") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.END_LOAD = atof(currToken);
#ifdef DEBUG_TRC
				printf("END_LOAD = %e\n", simParams.END_LOAD);
#endif
			}
			else if(strcmp(currToken, "LOAD_INCR") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.LOAD_INCR = atof(currToken);
#ifdef DEBUG_TRC
				printf("LOAD_INCR = %e\n", simParams.LOAD_INCR);
#endif
			}
			else if(strcmp(currToken, "START_LAMBDA") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.START_LAMBDA = atoi(currToken);
#ifdef DEBUG_TRC
				printf("START_LAMBDA = %d\n", simParams.START_LAMBDA);
#endif
			}
			else if(strcmp(currToken, "END_LAMBDA") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.END_LAMBDA = atoi(currToken);
#ifdef DEBUG_TRC
				printf("END_LAMBDA = %d\n", simParams.END_LAMBDA);
#endif
			}
			else if(strcmp(currToken, "FIXED_GRANT_SIZE") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.FIXED_GRANT_SIZE = atoi(currToken);
#ifdef DEBUG_TRC
				printf("FIXED_GRANT_SIZE = %d\n", simParams.FIXED_GRANT_SIZE);
#endif
			}
			else if(strcmp(currToken, "MAX_GRANT_SIZE") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.MAX_GRANT_SIZE = atoi(currToken);
#ifdef DEBUG_TRC
				printf("MAX_GRANT_SIZE = %d\n", simParams.MAX_GRANT_SIZE);
#endif
			}
			else if(strcmp(currToken, "SCHEDULE_INTERVAL") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.SCHEDULE_INTERVAL = atof(currToken);
#ifdef DEBUG_TRC
				printf("SCHEDULE_INTERVAL = %e\n", simParams.SCHEDULE_INTERVAL);
#endif
			}
			else if(strcmp(currToken, "SCHEDULER_SLOT_SIZE") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.SCHEDULER_SLOT_SIZE = atoi(currToken);
#ifdef DEBUG_TRC
				printf("SCHEDULER_SLOT_SIZE = %d\n", simParams.SCHEDULER_SLOT_SIZE);
#endif
			}
			else if(strcmp(currToken, "SCHEDULER_NUM_SLOT") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.SCHEDULER_NUM_SLOT = atoi(currToken);
#ifdef DEBUG_TRC
				printf("SCHEDULER_NUM_SLOT = %d\n", simParams.SCHEDULER_NUM_SLOT);
#endif
			}
			else if(strcmp(currToken, "SCHEDULER_MAX_WINDOW") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.SCHEDULER_MAX_WINDOW = atoi(currToken);
#ifdef DEBUG_TRC
				printf("SCHEDULER_MAX_WINDOW = %d\n", simParams.SCHEDULER_MAX_WINDOW);
#endif
			}
			else if(strcmp(currToken, "STARVE_THRESH") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.STARVE_THRESH = atoi(currToken);
#ifdef DEBUG_TRC
				printf("STARVE_THRESH = %d\n", simParams.STARVE_THRESH);
#endif
			}
			else if(strcmp(currToken, "RAND_SEED") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.RAND_SEED = atol(currToken);
#ifdef DEBUG_TRC
				printf("RAND_SEED = %ld\n", simParams.RAND_SEED);
#endif
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
			else if(strcmp(currToken, "GRANT_TRACE") == 0)
			{
				currToken[0] = '\0';
				fscanf(cfgFile, "%s", currToken);
				if(strcmp(currToken, "GRANT_TRACE_OFF") == 0)
				{
					simParams.GRANT_TRACE = GRANT_TRACE_OFF;
				}
				else if(strcmp(currToken, "GRANT_TRACE_ON") == 0)
				{
					simParams.GRANT_TRACE = GRANT_TRACE_ON;
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
#ifdef DEBUG_TRC
				printf("SIM_TRACE_TIME = %e\n", simParams.SIM_TRACE_TIME);
#endif
			}
			else if(strcmp(currToken, "SIM_TRACE_TIMESCALE") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.SIM_TRACE_TIMESCALE = atof(currToken);
#ifdef DEBUG_TRC
				printf("SIM_TRACE_TIMESCALE = %e\n", simParams.SIM_TRACE_TIMESCALE);
#endif
			}
			else if(strcmp(currToken, "AVAIL_COST_WEIGHT") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.AVAIL_COST_WEIGHT = atof(currToken);
#ifdef DEBUG_TRC
				printf("AVAIL_COST_WEIGHT = %e\n", simParams.AVAIL_COST_WEIGHT);
#endif
			}
			else if(strcmp(currToken, "FIXED_PROP_DELAY_VALUE") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.FIXED_PROP_DELAY_VALUE = atof(currToken);
#ifdef DEBUG_TRC
				printf("FIXED_PROP_DELAY_VALUE = %e\n", simParams.FIXED_PROP_DELAY_VALUE);
#endif
			}
			else if(strcmp(currToken, "MAX_PROP_DELAY") == 0)
			{
				fscanf(cfgFile, "%s", currToken);
				simParams.MAX_PROP_DELAY = atof(currToken);
#ifdef DEBUG_TRC
				printf("MAX_PROP_DELAY = %e\n", simParams.MAX_PROP_DELAY);
#endif
			}
		}
#ifdef DEBUG_TRC
		printf("Sim configuration loaded\n");
#endif
	}
	
	//find_num_layers();
	//find_num_frames();
	
	// Test Variables read_sim_cfg_file_finish
	test_vars.read_sim_cfg_file_finish++;
	test_var_print();
	TSprint("SCALABLE_VIDEO_ON: %d\n", simParams.SCALABLE_VIDEO_TRAFFIC);
	TSprint("SCALABLE_VIDEO_DROPPING: %d\n", simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM);
	TSprint("TOTAL_NUM_LAYERS: %d\n", simParams.SCALABLE_VIDEO_NUM_LAYERS);
	TSprint("SV_DROP_LAYER_THRESHOLD: %d\n", simParams.SV_DROP_LAYER_THRESHOLD);
	TSprint("SV_DROP_SENSITIVITY: %0.2f\n", simParams.SV_DROP_SENSITIVITY);
	TSprint("SV_DROP_NUM_MA_VALUES: %d\n", simParams.SV_DROP_NUM_VALUES_FOR_MA);
	TSprint("SV_MIN_BOUND: %f\n", simParams.SV_DROP_MIN_BOUND);
	TSprint("SV_MAX_BOUND: %f\n", simParams.SV_DROP_MAX_BOUND);
	TSprint("Max num of Frames: %d\n", (int)onuAttrs[0].maxFrameCount);
	TSprint("read_sim_cfg_file_finish\n\n");
}

void find_num_layers()
{
	// int		flag = -1;
	char 	prevChar[10],
			prevPrevChar[10],
			prevPrevPrevChar[10],
			mystring[LINE_LENGTH],
			c = 'A';
	int		ENHANCE_LAYERS_CHECK = 0;
	int 	layer,
			enhanceLayers = 0,
			lastFrame = 0,
			poundCheck = 0,
			temp = 0;
	FILE*	inFilePtr;
	
	if (simParams.SCALABLE_VIDEO_TRAFFIC == SCALABLE_VIDEO_ON)
	{
		// TSprint("Flag 1\n");
		inFilePtr = fopen(simParams.VIDEO_TRACE_FILE, "r");
		if (inFilePtr == NULL)
			TSprint("Error opening(1) %s \n", simParams.VIDEO_TRACE_FILE);
		// while (flag == -1)
			// flag = get_line2(inFilePtr, mystring, LINE_LENGTH);
		// TSprint("Flag 2\t c=%d\n", c);
		while (c != EOF)
		{
			c = fgetc(inFilePtr);
			if (c == '#') {
				poundCheck = 1;
			}
			if (c == '\n')
			{
				if (poundCheck == 0) 
				{
					// TSprint("Entered if Statement #1\n");
					mystring[0] = '\0';
					get_line(inFilePtr, mystring, LINE_LENGTH);
					// TSprint("Passed while loop\n");
					temp = atoi(mystring);
					// TSprint("Temp = %d\n", temp);
					if (temp > lastFrame)
						lastFrame = temp;
					//if (prevPrevChar[0] == -1)
						//layer = 0;
					//else 
					if ((prevChar[0] >= '0') && (prevChar[0] <= '9')) {
						layer = atoi(prevChar);
						// TSprint("Checkpoint_1\n");
					}
					else {
						if ((prevPrevChar[0] >= '0') && (prevPrevChar[0] <= '9')) {
						layer = atoi(prevPrevChar);
						// TSprint("Checkpoint_2\n");
						}
						else {
							if ((prevPrevPrevChar[0] >= '0') && (prevPrevPrevChar[0] <= '9')) {
								layer = atoi(prevPrevPrevChar);
								// TSprint("Checkpoint_3\n");
							}
						}
					}
					// else
						// TSprint("The layer data is not being reached.\n");
					if (ENHANCE_LAYERS_CHECK == 0) /* David (Set the number of enhancement layers.) */
					{
						// TSprint("layer=%d\ttemp=%d\tc=%c\tprev=%c\tprevPrev=%c\tprevPrevPrev=%c\n\n", layer, temp, c, prevChar[0], prevPrevChar[0], prevPrevPrevChar[0]);
						if (layer > enhanceLayers)
							enhanceLayers = layer;
						else if (layer < enhanceLayers)
						{
							simParams.SCALABLE_VIDEO_NUM_LAYERS = enhanceLayers + 1;
							ENHANCE_LAYERS_CHECK += 1; /* This ensures that the loop only sets the number of enhancement layers once. */
							// TSprint("Finished\n");
						}
					}
					else break;
				}
				if (poundCheck == 1)
					poundCheck = 0;
			}
			prevPrevPrevChar[0] = prevPrevChar[0];
			prevPrevChar[0] = prevChar[0];		// The correct layer data is two chars back from the '\n'
			//if (c == -1)
				//prevChar[0] = 0;
			//else 
			prevChar[0] = c;
		}
		fclose(inFilePtr);
	}
	return;
}

void find_num_frames()
{
	FILE*	inFilePtr;
	char	mystring[LINE_LENGTH];
	int		flag = 0,
			lastFlag = 0,
			last2Flag = 0,
			last3Flag = 0,
			Ah,
			iaa;
	double	tempFrameCount = -1;

	inFilePtr = fopen(simParams.VIDEO_TRACE_FILE, "r");
	if (inFilePtr == NULL)
		TSprint("Error opening(2) %s \n", simParams.VIDEO_TRACE_FILE);
	
	while (flag != 2000)
	{
		if ( (flag == 500 && lastFlag == 1000) || 
		(flag == 500 && lastFlag == 250 && last2Flag == 1000) || 
		(flag == 500 && lastFlag == 250 && last2Flag == 250 && last3Flag == 1000) )
		{
			tempFrameCount = (double)atoi(mystring);
		}
		last3Flag = last2Flag;
		last2Flag = lastFlag;
		lastFlag = flag;
		flag = get_line2(inFilePtr, mystring, LINE_LENGTH);
	}
	for (iaa = 0; iaa < simParams.NUM_ONU; iaa++)
		onuAttrs[iaa].maxFrameCount = tempFrameCount + 1;
	Ah = fclose(inFilePtr);
	if (Ah != 0)
		TSprint("Error closing %s \n", simParams.VIDEO_TRACE_FILE);
	return;
}

void estimate_hist_max()
{
#ifdef DEBUG_TRC
    printf("Calculate Histogram Maximum Estimators\n");
    fflush(NULL);
#endif

    overallQueueDelayEst.maxEst = table_max(overallQueueDelay)*1.2;
    heavyQueueDelayEst.maxEst = table_max(heavyQueueDelay);
    lightQueueDelayEst.maxEst = table_max(lightQueueDelay);
    overallQueueLengthEst.maxEst = table_max(overallQueueLength);
    heavyQueueLengthEst.maxEst = table_max(heavyQueueLength);
    lightQueueLengthEst.maxEst = table_max(lightQueueLength);
    overallGrantSizeEst.maxEst = table_max(overallGrantSize);
    overallGrantSizePktEst.maxEst = table_max(overallGrantSizePkt)*1.2;
    heavyGrantSizeEst.maxEst = table_max(heavyGrantSize);
    lightGrantSizeEst.maxEst = table_max(lightGrantSize);
    overallCycleLengthEst.maxEst = table_max(overallCycleLength)*1.2;
    heavyCycleLengthEst.maxEst = table_max(heavyCycleLength);
    lightCycleLengthEst.maxEst = table_max(lightCycleLength);
    overallRptToSchedTimeEst.maxEst = table_max(overallRptToSchedTime);
    heavyRptToSchedTimeEst.maxEst = table_max(heavyRptToSchedTime);
    lightRptToSchedTimeEst.maxEst = table_max(lightRptToSchedTime);
    overallRptToGateTimeEst.maxEst = table_max(overallRptToGateTime);
    heavyRptToGateTimeEst.maxEst = table_max(heavyRptToGateTime);
    lightRptToGateTimeEst.maxEst = table_max(lightRptToGateTime);
    numONUSchedEst.maxEst = table_max(numONUSched);

#ifdef DEBUG_TRC
    printf("overallQueueDelayEst.maxEst = %g\n",overallQueueDelayEst.maxEst);
    printf("overallQueueDelay table_cnt = %ld, table_min = %g, table_max = %g\n",table_cnt(overallQueueDelay),table_min(overallQueueDelay),table_max(overallQueueDelay));
    printf("heavyQueueDelayEst.maxEst = %g\n",heavyQueueDelayEst.maxEst);
    printf("lightQueueDelayEst.maxEst = %g\n",lightQueueDelayEst.maxEst);
    printf("overallQueueLengthEst.maxEst = %g\n",overallQueueLengthEst.maxEst);
    printf("heavyQueueLengthEst.maxEst = %g\n",heavyQueueLengthEst.maxEst);
    printf("lightQueueLengthEst.maxEst = %g\n",lightQueueLengthEst.maxEst);
    printf("overallGrantSizeEst.maxEst = %g\n",overallGrantSizeEst.maxEst);
    printf("overallGrantSizePktEst.maxEst = %g\n",overallGrantSizePktEst.maxEst);
    printf("heavyGrantSizeEst.maxEst = %g\n",heavyGrantSizeEst.maxEst);
    printf("lightGrantSizeEst.maxEst = %g\n",lightGrantSizeEst.maxEst);
    printf("overallCycleLengthEst.maxEst = %g\n",overallCycleLengthEst.maxEst);
    printf("heavyCycleLengthEst.maxEst = %g\n",heavyCycleLengthEst.maxEst);
    printf("lightCycleLengthEst.maxEst = %g\n",lightCycleLengthEst.maxEst);
    printf("overallRptToSchedTimeEst.maxEst = %g\n",overallRptToSchedTimeEst.maxEst);
    printf("heavyRptToSchedTimeEst.maxEst = %g\n",heavyRptToSchedTimeEst.maxEst);
    printf("lightRptToSchedTimeEst.maxEst = %g\n",lightRptToSchedTimeEst.maxEst);
    printf("overallRptToGateTimeEst.maxEst = %g\n",overallRptToGateTimeEst.maxEst);
    printf("heavyRptToGateTimeEst.maxEst = %g\n",heavyRptToGateTimeEst.maxEst);
    printf("lightRptToGateTimeEst.maxEst = %g\n",lightRptToGateTimeEst.maxEst);
    printf("numONUSchedEst.maxEst = %g\n",numONUSchedEst.maxEst);
    printf("Calculate Histogram Maximum Estimators\n");
    fflush(NULL);
#endif

#if 0
    overallQueueDelayEst.minEst = table_min(overallQueueDelay);
    heavyQueueDelayEst.minEst = table_min(heavyQueueDelay);
    lightQueueDelayEst.minEst = table_min(lightQueueDelay);
    overallQueueLengthEst.minEst = table_min(overallQueueLength);
    heavyQueueLengthEst.minEst = table_min(heavyQueueLength);
    lightQueueLengthEst.minEst = table_min(lightQueueLength);
    overallGrantSizeEst.minEst = table_min(overallGrantSize);
    overallGrantSizePktEst.minEst = table_min(overallGrantSizePkt);
    heavyGrantSizeEst.minEst = table_min(heavyGrantSize);
    lightGrantSizeEst.minEst = table_min(lightGrantSize);
    overallCycleLengthEst.minEst = table_min(overallCycleLength);
    heavyCycleLengthEst.minEst = table_min(heavyCycleLength);
    lightCycleLengthEst.minEst = table_min(lightCycleLength);
    overallRptToSchedTimeEst.minEst = table_min(overallRptToSchedTime);
    heavyRptToSchedTimeEst.minEst = table_min(heavyRptToSchedTime);
    lightRptToSchedTimeEst.minEst = table_min(lightRptToSchedTime);
    overallRptToGateTimeEst.minEst = table_min(overallRptToGateTime);
    heavyRptToGateTimeEst.minEst = table_min(heavyRptToGateTime);
    lightRptToGateTimeEst.minEst = table_min(lightRptToGateTime);
    numONUSchedEst.minEst = table_min(numONUSched);

    overallQueueDelayEst.meanEst = table_mean(overallQueueDelay);
    heavyQueueDelayEst.meanEst = table_mean(heavyQueueDelay);
    lightQueueDelayEst.meanEst = table_mean(lightQueueDelay);
    overallQueueLengthEst.meanEst = table_mean(overallQueueLength);
    heavyQueueLengthEst.meanEst = table_mean(heavyQueueLength);
    lightQueueLengthEst.meanEst = table_mean(lightQueueLength);
    overallGrantSizeEst.meanEst = table_mean(overallGrantSize);
    overallGrantSizePktEst.meanEst = table_mean(overallGrantSizePkt);
    heavyGrantSizeEst.meanEst = table_mean(heavyGrantSize);
    lightGrantSizeEst.meanEst = table_mean(lightGrantSize);
    overallCycleLengthEst.meanEst = table_mean(overallCycleLength);
    heavyCycleLengthEst.meanEst = table_mean(heavyCycleLength);
    lightCycleLengthEst.meanEst = table_mean(lightCycleLength);
    overallRptToSchedTimeEst.meanEst = table_mean(overallRptToSchedTime);
    heavyRptToSchedTimeEst.meanEst = table_mean(heavyRptToSchedTime);
    lightRptToSchedTimeEst.meanEst = table_mean(lightRptToSchedTime);
    overallRptToGateTimeEst.meanEst = table_mean(overallRptToGateTime);
    heavyRptToGateTimeEst.meanEst = table_mean(heavyRptToGateTime);
    lightRptToGateTimeEst.meanEst = table_mean(lightRptToGateTime);
    numONUSchedEst.meanEst = table_mean(numONUSched);
#endif

}

void setup_hist_tail()
{
    overallQueueDelayEst.minEst = overallQueueDelayEst.maxEst;
    heavyQueueDelayEst.minEst = heavyQueueDelayEst.maxEst;
    lightQueueDelayEst.minEst = lightQueueDelayEst.maxEst;
    overallQueueLengthEst.minEst = overallQueueLengthEst.maxEst;
    heavyQueueLengthEst.minEst = heavyQueueLengthEst.maxEst;
    lightQueueLengthEst.minEst = lightQueueLengthEst.maxEst;
    overallGrantSizeEst.minEst = overallGrantSizeEst.maxEst;
    overallGrantSizePktEst.minEst = overallGrantSizePktEst.maxEst;
    heavyGrantSizeEst.minEst = heavyGrantSizeEst.maxEst;
    lightGrantSizeEst.minEst = lightGrantSizeEst.maxEst;
    overallCycleLengthEst.minEst = overallCycleLengthEst.maxEst;
    heavyCycleLengthEst.minEst = heavyCycleLengthEst.maxEst;
    lightCycleLengthEst.minEst = lightCycleLengthEst.maxEst;
    overallRptToSchedTimeEst.minEst = overallRptToSchedTimeEst.maxEst;
    heavyRptToSchedTimeEst.minEst = heavyRptToSchedTimeEst.maxEst;
    lightRptToSchedTimeEst.minEst = lightRptToSchedTimeEst.maxEst;
    overallRptToGateTimeEst.minEst = overallRptToGateTimeEst.maxEst;
    heavyRptToGateTimeEst.minEst = heavyRptToGateTimeEst.maxEst;
    lightRptToGateTimeEst.minEst = lightRptToGateTimeEst.maxEst;
    numONUSchedEst.minEst = numONUSchedEst.maxEst;
    overallQueueDelayEst.maxEst = table_max(overallQueueDelay);
    heavyQueueDelayEst.maxEst = table_max(heavyQueueDelay);
    lightQueueDelayEst.maxEst = table_max(lightQueueDelay);
    overallQueueLengthEst.maxEst = table_max(overallQueueLength);
    heavyQueueLengthEst.maxEst = table_max(heavyQueueLength);
    lightQueueLengthEst.maxEst = table_max(lightQueueLength);
    overallGrantSizeEst.maxEst = table_max(overallGrantSize);
    overallGrantSizePktEst.maxEst = table_max(overallGrantSizePkt);
    heavyGrantSizeEst.maxEst = table_max(heavyGrantSize);
    lightGrantSizeEst.maxEst = table_max(lightGrantSize);
    overallCycleLengthEst.maxEst = table_max(overallCycleLength);
    heavyCycleLengthEst.maxEst = table_max(heavyCycleLength);
    lightCycleLengthEst.maxEst = table_max(lightCycleLength);
    overallRptToSchedTimeEst.maxEst = table_max(overallRptToSchedTime);
    heavyRptToSchedTimeEst.maxEst = table_max(heavyRptToSchedTime);
    lightRptToSchedTimeEst.maxEst = table_max(lightRptToSchedTime);
    overallRptToGateTimeEst.maxEst = table_max(overallRptToGateTime);
    heavyRptToGateTimeEst.maxEst = table_max(heavyRptToGateTime);
    lightRptToGateTimeEst.maxEst = table_max(lightRptToGateTime);
    numONUSchedEst.maxEst = table_max(numONUSched);

}

void write_sim_data(int runNumber, int numLambdas, double trafficLoad)
{
    int     loopIndex;
    char	filename_suffix[100];
	char	filename_str[150];
    char    double_str[15];
    char    *charPtr;

    FILE *odFile, *vodFile, *vodsFile, *vugFile, *vrFile, *vrmFile, *hdFile, *ldFile, *olFile, *hlFile, *llFile, *statsFile;
    FILE *qdFile;
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
	FILE *utFile[MAX_LAMBDAS];
	FILE *tpFile[MAX_LAMBDAS];
    FILE *tpoFile[MAX_ONU];
    FILE *tfsFile, *tfFile;
    FILE *cr1File, *cr2File, *mcrFile;
    FILE *ebFile, *ebpoFile;
	FILE *utxlFile[MAX_LAMBDAS];
    FILE *odHistFile, *clHistFile, *gspHistFile;
    FILE *pdFile, *plFile, *clpFile;


    /* Determine file names */
    filename_suffix[0] = '\0';
    /* To support multiple runs, record the run number for run number greater than 0 */
    if(runNumber > 0)
    {
        double_str[0] = '\0';
        sprintf(double_str, "%d", runNumber);
        sprintf(filename_suffix, "%srun%s_", filename_suffix, double_str);
    }
    switch(simParams.TRAFFIC_TYPE)
    {
        case TRAFFIC_POISSON:
            strcat(filename_suffix, "pn_");
            break;
        case TRAFFIC_SELF_SIMILAR:
            strcat(filename_suffix, "ss_");
            break;
    }

	if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
	{
		strcat(filename_suffix, "vid_");
		
		if(simParams.VIDEO_PREDICTION == VIDEO_PREDICTION_ON) sprintf(filename_suffix, "%s%s_", filename_suffix, simParams.PREDICTION_TYPE);
		else strcat(filename_suffix, "Poff_");
		
		if(simParams.TIME_SHIFT == 0) strcat(filename_suffix, "0_");
		else sprintf(filename_suffix, "%s%2.2lf_", filename_suffix, simParams.TIME_SHIFT); 
		
	}

    switch(simParams.OLT_TYPE)
    {
        case OLT_IPACT:
            strcat(filename_suffix, "ipt_");
            break;
        case OLT_IPACT_PSF:
            strcat(filename_suffix, "psf_");
            break;
        case OLT_WDM_IPACT:
            strcat(filename_suffix, "wipt_");
            break;
        case OLT_LEAST_ASSIGNED:
            strcat(filename_suffix, "la_");
            break;
        case OLT_APS:
            strcat(filename_suffix, "aps_");
            break;
        case OLT_LFJ:
            strcat(filename_suffix, "lfj_");
            break;
        case OLT_LFJ_LPT:
            strcat(filename_suffix, "lfjlpt_");
            break;
        case OLT_LFJ_SPT:
            strcat(filename_suffix, "lfjspt_");
            break;
        case OLT_LFJ_LNF:
            strcat(filename_suffix, "lfjlnf_");
            break;
        case OLT_SPD:
            strcat(filename_suffix, "spd_");
            break;
        case OLT_LPD:
            strcat(filename_suffix, "lpd_");
            break;
        case OLT_EAF:
            strcat(filename_suffix, "eaf_");
            break;
        case OLT_EAAF:
            strcat(filename_suffix, "eaaf_");
            break;
        case OLT_WBM:
            strcat(filename_suffix, "wbm_");
            break;
        case OLT_WBM_LPT:
            strcat(filename_suffix, "wbmlpt_");
            break;
        case OLT_WBM_LNF:
            strcat(filename_suffix, "wbmlnf_");
            break;
        case OLT_WBM_EAAF:
            strcat(filename_suffix, "wbmeaaf_");
            break;
        case OLT_ONLINE_NASC:
            strcat(filename_suffix, "nasc_");
            break;
        case OLT_ONLINE_INTERVAL_LFJ_LPT:
            strcat(filename_suffix, "intvl_lfjlpt_");
            break;
        case OLT_ONLINE_INTERVAL_LFJ_SPT:
            strcat(filename_suffix, "intvl_lfjspt_");
            break;
        case OLT_ONLINE_INTERVAL_LFJ_LNF:
            strcat(filename_suffix, "intvl_lfjlnf_");
            break;
        case OLT_ONLINE_INTERVAL_WBM:
            strcat(filename_suffix, "intvl_wbm_");
            break;
        case OLT_ONLINE_INTERVAL_WBM_LPT:
            strcat(filename_suffix, "intvl_wbmlpt_");
            break;
        case OLT_ONLINE_INTERVAL_WBM_LNF:
            strcat(filename_suffix, "intvl_wbmlnf_");
            break;
        case OLT_ONLINE_INTERVAL_WBM_EAAF:
            strcat(filename_suffix, "intvl_wbmeaaf_");
            break;
        case OLT_ONLINE_INTERVAL_EAF:
            strcat(filename_suffix, "intvl_eaf_");
            break;
        case OLT_ONLINE_INTERVAL_EAAF:
            strcat(filename_suffix, "intvl_eaaf_");
            break;
        case OLT_ONLINE_JIT_LFJ_LPT:
            strcat(filename_suffix, "jit_lfjlpt_");
            break;
        case OLT_ONLINE_JIT_LFJ_LNF:
            strcat(filename_suffix, "jit_lfjlnf_");
            break;
        case OLT_ONLINE_JIT_LFJ_SPT:
            strcat(filename_suffix, "jit_lfjspt_");
            break;
        case OLT_ONLINE_JIT_WBM:
            strcat(filename_suffix, "jit_wbm_");
            break;
        case OLT_ONLINE_JIT_WBM_LPT:
            strcat(filename_suffix, "jit_wbmlpt_");
            break;
        case OLT_ONLINE_JIT_WBM_LNF:
            strcat(filename_suffix, "jit_wbmlnf_");
            break;
        case OLT_ONLINE_JIT_WBM_EAAF:
            strcat(filename_suffix, "jit_wbmeaaf_");
            break;
        case OLT_ONLINE_JIT_EAF:
            strcat(filename_suffix, "jit_eaf_");
            break;
        case OLT_ONLINE_JIT_EAAF:
            strcat(filename_suffix, "jit_eaaf_");
            break;
        case OLT_ONLINE_JIT_TEST:
            strcat(filename_suffix, "jit_test_");
            break;
        case OLT_ONLINE_JIT_SPD:
            strcat(filename_suffix, "jit_spd_");
            break;
        case OLT_ONLINE_JIT_LPD:
            strcat(filename_suffix, "jit_lpd_");
            break;
    }
    switch(simParams.DBA_TYPE)
    {
        case DBA_FIXED:
            double_str[0] = '\0';
            sprintf(double_str, "%d", simParams.FIXED_GRANT_SIZE);
            sprintf(filename_suffix, "%sfixed%s_", filename_suffix, double_str);
            break;
        case DBA_GATED:
            strcat(filename_suffix, "gated_");
            break;
        case DBA_LIMITED_GATE:
            double_str[0] = '\0';
            sprintf(double_str, "%d", simParams.MAX_GRANT_SIZE);
            sprintf(filename_suffix, "%slimited%s_", filename_suffix, double_str);
            break;
        case DBA_EXCESS:
            double_str[0] = '\0';
            sprintf(double_str, "%d", simParams.MAX_GRANT_SIZE);
            sprintf(filename_suffix, "%sexcess%s_", filename_suffix, double_str);
            break;

    }

    switch(simParams.VIDEO_DBA_TYPE)
    {
        case VIDEO_DBA_FIXED:
            //double_str[0] = '\0';
            //sprintf(double_str, "%d", simParams.FIXED_GRANT_SIZE);
            //sprintf(filename_suffix, "%sVfixed%s_", filename_suffix, double_str);
            strcat(filename_suffix, "Vfixed_");
            break;
        case VIDEO_DBA_GATED:
            strcat(filename_suffix, "Vgated_");
            break;
        case VIDEO_DBA_LIMITED_GATE:
            //double_str[0] = '\0';
            //sprintf(double_str, "%d", simParams.MAX_GRANT_SIZE);
            //sprintf(filename_suffix, "%sVlimited%s_", filename_suffix, double_str);
            strcat(filename_suffix, "Vlimited_");
            break;
    }
    
    
    switch(simParams.OLT_TYPE)
    {
    case OLT_WBM:
    case OLT_WBM_LPT:
    case OLT_WBM_LNF:
    case OLT_WBM_EAAF:
    case OLT_ONLINE_JIT_WBM:
    case OLT_ONLINE_JIT_WBM_LPT:
    case OLT_ONLINE_JIT_WBM_LNF:
    case OLT_ONLINE_JIT_WBM_EAAF:
    case OLT_ONLINE_INTERVAL_WBM:
    case OLT_ONLINE_INTERVAL_WBM_LPT:
    case OLT_ONLINE_INTERVAL_WBM_LNF:
    case OLT_ONLINE_INTERVAL_WBM_EAAF:
        double_str[0] = '\0';
        sprintf(double_str, "%g", simParams.AVAIL_COST_WEIGHT);
        charPtr = strchr(double_str, '.');
        if(charPtr != NULL)
        {
            *charPtr = '_';
        }
        sprintf(filename_suffix, "%sacw_%s_", filename_suffix, double_str);
        break;
    }
    switch(simParams.OLT_TYPE)
    {
    case OLT_ONLINE_JIT_LFJ_LPT:
    case OLT_ONLINE_JIT_LFJ_LNF:
    case OLT_ONLINE_JIT_LFJ_SPT:
    case OLT_ONLINE_JIT_WBM:
    case OLT_ONLINE_JIT_WBM_LPT:
    case OLT_ONLINE_JIT_WBM_LNF:
    case OLT_ONLINE_JIT_WBM_EAAF:
    case OLT_ONLINE_JIT_EAF:
    case OLT_ONLINE_JIT_EAAF:
    case OLT_ONLINE_JIT_TEST:
    case OLT_ONLINE_JIT_SPD:
    case OLT_ONLINE_JIT_LPD:
        sprintf(filename_suffix, "%sst%d_", filename_suffix, simParams.STARVE_THRESH);
        break;
    }
    if(simParams.SCHEDULER_MAX_WINDOW == 1)
    {
            strcat(filename_suffix, "mw_");
    }
    switch(simParams.WDM_TYPE)
    {
        case WDM_FIXED:
            strcat(filename_suffix, "fx_");
            break;
        case WDM_TUNABLE:
            strcat(filename_suffix, "tn_");
            break;
    }
    sprintf(filename_suffix, "%s%do_", filename_suffix, simParams.NUM_ONU);
    sprintf(filename_suffix, "%s%dwo_", filename_suffix, simParams.NUM_WDM_ONU);
    sprintf(filename_suffix, "%s%dw", filename_suffix, numLambdas);
    
    /*
     * Open files
     */
    filename_str[0]	= '\0';
    sprintf(filename_str, "od_%s", filename_suffix);
    odFile = fopen(filename_str,"a");
    
    if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
    {
	filename_str[0]	= '\0';
	sprintf(filename_str, "vod_%s", filename_suffix);
	vodFile = fopen(filename_str,"a");
	filename_str[0]	= '\0';
	sprintf(filename_str, "vods_%s", filename_suffix);
	vodsFile = fopen(filename_str,"a");
	filename_str[0]	= '\0';	
	sprintf(filename_str, "vug_%s", filename_suffix);
	vugFile = fopen(filename_str,"a");
	filename_str[0]	= '\0';
	sprintf(filename_str, "vr_%s", filename_suffix);
	vrFile = fopen(filename_str,"a");
	filename_str[0]	= '\0';
	sprintf(filename_str, "vrmax_%s", filename_suffix);
	vrmFile = fopen(filename_str,"a");
    }
    
    filename_str[0]	= '\0';
    sprintf(filename_str, "qd_%s", filename_suffix);
    qdFile = fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "odo_%s", filename_suffix);
    odoFile = fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "odmn_%s", filename_suffix);
    odmnFile = fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "odmx_%s", filename_suffix);
    odmxFile = fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "hd_%s", filename_suffix);
    hdFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "ld_%s", filename_suffix);
    ldFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "ol_%s", filename_suffix);
    olFile = fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "hl_%s", filename_suffix);
    hlFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "ll_%s", filename_suffix);
    llFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "cl_%s", filename_suffix);
    clFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "clmn_%s", filename_suffix);
    clmnFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "clmx_%s", filename_suffix);
    clmxFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "cll_%s", filename_suffix);
    cllFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "clh_%s", filename_suffix);
    clhFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "rs_%s", filename_suffix);
    rsFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "rsl_%s", filename_suffix);
    rslFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "rsh_%s", filename_suffix);
    rshFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "rg_%s", filename_suffix);
    rgFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "rgl_%s", filename_suffix);
    rglFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "rgh_%s", filename_suffix);
    rghFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "gsp_%s", filename_suffix);
    gspFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "gs_%s", filename_suffix);
    gsFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "gsl_%s", filename_suffix);
    gslFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "gsh_%s", filename_suffix);
    gshFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "nz_%s", filename_suffix);
    nzFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "nzl_%s", filename_suffix);
    nzlFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "nzh_%s", filename_suffix);
    nzhFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "no_%s", filename_suffix);
    noFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "nomx_%s", filename_suffix);
    nomxFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "lb_%s", filename_suffix);
    lbFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "sr_%s", filename_suffix);
    srFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "srmx_%s", filename_suffix);
    srmxFile	= fopen(filename_str,"a");

    filename_str[0]	= '\0';
    sprintf(filename_str, "pd_%s", filename_suffix);
    pdFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "pl_%s", filename_suffix);
    plFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "clp_%s", filename_suffix);
    clpFile	= fopen(filename_str,"a");


    double_str[0] = '\0';
    sprintf(double_str, "%g", trafficLoad);
    charPtr = strchr(double_str, '.');
    if(charPtr != NULL)
    {
        *charPtr = '_';
    }

    filename_str[0]	= '\0';
    sprintf(filename_str, "od_hist_%s_%s", double_str, filename_suffix);
    odHistFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "cl_hist_%s_%s", double_str, filename_suffix);
    clHistFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "gsp_hist_%s_%s", double_str, filename_suffix);
    gspHistFile	= fopen(filename_str,"a");

    filename_str[0]	= '\0';
    sprintf(filename_str, "od_%s_xl", filename_suffix);
    odxlFile = fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "hd_%s_xl", filename_suffix);
    hdxlFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "ld_%s_xl", filename_suffix);
    ldxlFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "ol_%s_xl", filename_suffix);
    olxlFile = fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "hl_%s_xl", filename_suffix);
    hlxlFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "ll_%s_xl", filename_suffix);
    llxlFile	= fopen(filename_str,"a");
    statsFile	= fopen("stats","a");
    
    /* Open files for reporting wavelength utilization */
    for(loopIndex=0; loopIndex < numLambdas; loopIndex++)
    {
        filename_str[0]	= '\0';
        sprintf(filename_str, "ut%d_%s", loopIndex+1, filename_suffix);
        utFile[loopIndex] = fopen(filename_str,"a");
        filename_str[0]	= '\0';
        sprintf(filename_str, "tp%d_%s", loopIndex+1, filename_suffix);
        tpFile[loopIndex] = fopen(filename_str,"a");
        filename_str[0]	= '\0';
        sprintf(filename_str, "ut%d_%s_xl", loopIndex+1, filename_suffix);
        utxlFile[loopIndex] = fopen(filename_str,"a");
    }				
    
    /* Open files for throughput per onu (tpo) calculations */
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        filename_str[0]	= '\0';
        sprintf(filename_str, "tpo%d_%s", loopIndex+1, filename_suffix);
        tpoFile[loopIndex] = fopen(filename_str,"a");
    }
    /* Open files for throughput fairness */
    filename_str[0]	= '\0';
    sprintf(filename_str, "tfs_%s", filename_suffix);
    tfsFile = fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "tf_%s", filename_suffix);
    tfFile = fopen(filename_str,"a");
    /* Open files for competitive ratio */
    filename_str[0]	= '\0';
    sprintf(filename_str, "cr1_%s", filename_suffix);
    cr1File = fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "cr2_%s", filename_suffix);
    cr2File = fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "mcr_%s", filename_suffix);
    mcrFile = fopen(filename_str,"a");
    
    /* Open files for excess bandwidth */
    filename_str[0]	= '\0';
    sprintf(filename_str, "eb_%s", filename_suffix);
    ebFile = fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "ebpo_%s", filename_suffix);
    ebpoFile = fopen(filename_str,"a");


    /* Hack */
    //trafficLoad = simParams.PIPG_LOAD;

    fprintf(statsFile,"wdm_onu=%d,load=%f,lambdas=%d\n", simParams.NUM_WDM_ONU, trafficLoad, numLambdas);

    fprintf(statsFile,"rand_seed_base=%ld\n", simParams.RAND_SEED);
    
    fprintf(statsFile,"sim_time=%e\n", simtime());
    fprintf(statsFile,"cpu_time=%e\n", cputime());
    fprintf(statsFile,"sim_time/cpu_time=%e\n", simtime()/cputime());

    fprintf(statsFile,"grant_rate=%e,heavy_grant_rate=%e,light_grant_rate=%e\n",meter_rate(overallGrantRate),
            meter_rate(heavyGrantRate),meter_rate(lightGrantRate));
    fprintf(statsFile,"percent_nonzero_req=%e,heavy_percent_nonzero_req=%e,light_percent_nonzero_req=%e\n",meter_rate(overallNonzeroReqRate)/(meter_rate(overallZeroReqRate)+meter_rate(overallNonzeroReqRate)),
            meter_rate(heavyNonzeroReqRate)/(meter_rate(heavyZeroReqRate)+meter_rate(heavyNonzeroReqRate)),
            meter_rate(lightNonzeroReqRate)/(meter_rate(lightZeroReqRate)+meter_rate(lightNonzeroReqRate)));
    fprintf(statsFile,"req_rate=%e,heavy_reg_rate=%e,light_reg_rate=%e\n",(meter_rate(overallZeroReqRate)+meter_rate(overallNonzeroReqRate)),
            (meter_rate(heavyZeroReqRate)+meter_rate(heavyNonzeroReqRate)),
            (meter_rate(lightZeroReqRate)+meter_rate(lightNonzeroReqRate)));


    if((simParams.TRAFFIC_TYPE != TRAFFIC_SELF_SIMILAR) /*&& (simParams.OLT_TYPE != OLT_APS)*/)
    {
        /* Collect stats in files */
        fprintf(odoFile,"%f %e\n", trafficLoad, table_mean(overallQueueDelay));
        fprintf(odmnFile,"%f %e\n", trafficLoad, table_min(overallQueueDelay));
        fprintf(odmxFile,"%f %e\n", trafficLoad, table_max(overallQueueDelay));
        fprintf(odFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallQueueDelay), table_conf_lower(overallQueueDelay, 0.95), table_conf_upper(overallQueueDelay, 0.95), 0.95);
        
        fprintf(qdFile,"%f %e\n", trafficLoad, table_mean(queueDelay));
        fprintf(hdFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyQueueDelay), table_conf_lower(heavyQueueDelay, 0.95), table_conf_upper(heavyQueueDelay, 0.95), 0.95);
        fprintf(ldFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightQueueDelay), table_conf_lower(lightQueueDelay, 0.95), table_conf_upper(lightQueueDelay, 0.95), 0.95);
        fprintf(olFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallQueueLength), table_conf_lower(overallQueueLength, 0.95), table_conf_upper(overallQueueLength, 0.95), 0.95);
        fprintf(hlFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyQueueLength), table_conf_lower(heavyQueueLength, 0.95), table_conf_upper(heavyQueueLength, 0.95), 0.95);
        fprintf(llFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightQueueLength), table_conf_lower(lightQueueLength, 0.95), table_conf_upper(lightQueueLength, 0.95), 0.95);
        fprintf(clFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallCycleLength), table_conf_lower(overallCycleLength, 0.95), table_conf_upper(overallCycleLength, 0.95), 0.95);
        fprintf(clmnFile,"%f %e\n", trafficLoad, table_min(overallCycleLength));
        fprintf(clmxFile,"%f %e\n", trafficLoad, table_max(overallCycleLength));
        fprintf(cllFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightCycleLength), table_conf_lower(lightCycleLength, 0.95), table_conf_upper(lightCycleLength, 0.95), 0.95);
        fprintf(clhFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyCycleLength), table_conf_lower(heavyCycleLength, 0.95), table_conf_upper(heavyCycleLength, 0.95), 0.95);
        fprintf(rsFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallRptToSchedTime), table_conf_lower(overallRptToSchedTime, 0.95), table_conf_upper(overallRptToSchedTime, 0.95), 0.95);
        fprintf(rslFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightRptToSchedTime), table_conf_lower(lightRptToSchedTime, 0.95), table_conf_upper(lightRptToSchedTime, 0.95), 0.95);
        fprintf(rshFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyRptToSchedTime), table_conf_lower(heavyRptToSchedTime, 0.95), table_conf_upper(heavyRptToSchedTime, 0.95), 0.95);
        fprintf(rgFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallRptToGateTime), table_conf_lower(overallRptToGateTime, 0.95), table_conf_upper(overallRptToGateTime, 0.95), 0.95);
        fprintf(rglFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightRptToGateTime), table_conf_lower(lightRptToGateTime, 0.95), table_conf_upper(lightRptToGateTime, 0.95), 0.95);
        fprintf(rghFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyRptToGateTime), table_conf_lower(heavyRptToGateTime, 0.95), table_conf_upper(heavyRptToGateTime, 0.95), 0.95);
        fprintf(gspFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallGrantSizePkt), table_conf_lower(overallGrantSizePkt, 0.95), table_conf_upper(overallGrantSizePkt, 0.95), 0.95);
        fprintf(gsFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallGrantSize), table_conf_lower(overallGrantSize, 0.95), table_conf_upper(overallGrantSize, 0.95), 0.95);
        fprintf(gslFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightGrantSize), table_conf_lower(lightGrantSize, 0.95), table_conf_upper(lightGrantSize, 0.95), 0.95);
        fprintf(gshFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyGrantSize), table_conf_lower(heavyGrantSize, 0.95), table_conf_upper(heavyGrantSize, 0.95), 0.95);
        fprintf(noFile,"%f %e\n", trafficLoad, table_mean(numONUSched));
        fprintf(nomxFile,"%f %e\n", trafficLoad, table_max(numONUSched));
        fprintf(srFile,"%f %e\n", trafficLoad, table_mean(schedRounds));
        fprintf(srmxFile,"%f %e\n", trafficLoad, table_max(schedRounds));
        fprintf(lbFile,"%f %e\n", trafficLoad, table_mean(loadBalanceMeasure));
        fprintf(nzFile,"%f %e\n", trafficLoad, meter_rate(overallNonzeroReqRate)/(meter_rate(overallZeroReqRate)+meter_rate(overallNonzeroReqRate)));
        fprintf(nzlFile,"%f %e\n", trafficLoad, meter_rate(lightNonzeroReqRate)/(meter_rate(lightZeroReqRate)+meter_rate(lightNonzeroReqRate)));
        fprintf(nzhFile,"%f %e\n", trafficLoad, meter_rate(heavyNonzeroReqRate)/(meter_rate(heavyZeroReqRate)+meter_rate(heavyNonzeroReqRate)));
        fprintf(pdFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(preferQueueDelay), table_conf_lower(preferQueueDelay, 0.95), table_conf_upper(preferQueueDelay, 0.95), 0.95);
        fprintf(plFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(preferQueueLength), table_conf_lower(preferQueueLength, 0.95), table_conf_upper(preferQueueLength, 0.95), 0.95);
        fprintf(clpFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(preferCycleLength), table_conf_lower(preferCycleLength, 0.95), table_conf_upper(preferCycleLength, 0.95), 0.95);
    
        fprintf(odxlFile,"%e, ", table_conf_mean(overallQueueDelay));
        fprintf(hdxlFile,"%e, ", table_conf_mean(heavyQueueDelay));
        fprintf(ldxlFile,"%e, ", table_conf_mean(lightQueueDelay));
        fprintf(olxlFile,"%e, ", table_conf_mean(overallQueueLength));
        fprintf(hlxlFile,"%e, ", table_conf_mean(heavyQueueLength));
        fprintf(llxlFile,"%e, ", table_conf_mean(lightQueueLength));

        fprintf(statsFile,"num_onu_sched = (mean=%e,var=%e,min=%e,max=%e,cnt=%ld)\n",table_mean(numONUSched),table_var(numONUSched),table_min(numONUSched),table_max(numONUSched),table_cnt(numONUSched));
        fprintf(statsFile,"avg_grant_size_pkt = %e +/- %e, (mean=%e,var=%e,min=%e,max=%e)\n",table_conf_mean(overallGrantSizePkt),table_conf_halfwidth(overallGrantSizePkt, 0.95),table_mean(overallGrantSizePkt),table_var(overallGrantSizePkt),table_min(overallGrantSizePkt),table_max(overallGrantSizePkt));
        fprintf(statsFile,"avg_grant_size = %e +/- %e, (mean=%e,var=%e,min=%e,max=%e)\n",table_conf_mean(overallGrantSize),table_conf_halfwidth(overallGrantSize, 0.95),table_mean(overallGrantSize),table_var(overallGrantSize),table_min(overallGrantSize),table_max(overallGrantSize));
        fprintf(statsFile,"avg_cycle_time = %e +/- %e, (mean=%e,var=%e,min=%e,max=%e,cnt=%ld)\n",table_conf_mean(overallCycleLength),table_conf_halfwidth(overallCycleLength, 0.95),table_mean(overallCycleLength),table_var(overallCycleLength),table_min(overallCycleLength),table_max(overallCycleLength),table_cnt(overallCycleLength));
        fprintf(statsFile,"rpt_to_sched_time = %e +/- %e, (mean=%e,var=%e,min=%e,max=%e)\n",table_conf_mean(overallRptToSchedTime),table_conf_halfwidth(overallRptToSchedTime, 0.95),table_mean(overallRptToSchedTime),table_var(overallRptToSchedTime),table_min(overallRptToSchedTime),table_max(overallRptToSchedTime));
        fprintf(statsFile,"rpt_to_gate_time = %e +/- %e, (mean=%e,var=%e,min=%e,max=%e)\n",table_conf_mean(overallRptToGateTime),table_conf_halfwidth(overallRptToGateTime, 0.95),table_mean(overallRptToGateTime),table_var(overallRptToGateTime),table_min(overallRptToGateTime),table_max(overallRptToGateTime));
        fprintf(statsFile,"overallQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e,cnt=%ld), heavyQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e), lightQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e)\n", 
                table_conf_mean(overallQueueDelay), table_conf_halfwidth(overallQueueDelay, 0.95), table_mean(overallQueueDelay), table_var(overallQueueDelay), table_min(overallQueueDelay), table_max(overallQueueDelay), table_cnt(overallQueueDelay),
                table_conf_mean(heavyQueueDelay), table_conf_halfwidth(heavyQueueDelay, 0.95), table_mean(heavyQueueDelay), table_var(heavyQueueDelay), table_min(heavyQueueDelay), table_max(heavyQueueDelay), 
                table_conf_mean(lightQueueDelay), table_conf_halfwidth(lightQueueDelay, 0.95), table_mean(lightQueueDelay), table_var(lightQueueDelay), table_min(lightQueueDelay), table_max(lightQueueDelay));
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            fprintf(statsFile,"qTime[%d] = %e +/- %e (mean=%e,var=%e,min=%e,max=%e), qLen[%d] = %e +/- %e (mean=%e,var=%e,min=%e,max=%e), rtt = %e\n", loopIndex, table_conf_mean(onuAttrs[loopIndex].queueTimeTable), 
                    table_conf_halfwidth(onuAttrs[loopIndex].queueTimeTable, 0.95), table_mean(onuAttrs[loopIndex].queueTimeTable), table_var(onuAttrs[loopIndex].queueTimeTable), table_min(onuAttrs[loopIndex].queueTimeTable), table_max(onuAttrs[loopIndex].queueTimeTable), loopIndex, 
                    table_conf_mean(onuAttrs[loopIndex].queueLengthTable), 
                    table_conf_halfwidth(onuAttrs[loopIndex].queueLengthTable, 0.95), table_mean(onuAttrs[loopIndex].queueLengthTable), table_var(onuAttrs[loopIndex].queueLengthTable), table_min(onuAttrs[loopIndex].queueLengthTable), table_max(onuAttrs[loopIndex].queueLengthTable), onuAttrs[loopIndex].rtt);
        }
    }
    else
    {
        /* Collect stats in files */
        fprintf(odoFile,"%f %e\n", trafficLoad, table_mean(overallQueueDelay));
        fprintf(odmnFile,"%f %e\n", trafficLoad, table_min(overallQueueDelay));
        fprintf(odmxFile,"%f %e\n", trafficLoad, table_max(overallQueueDelay));
        fprintf(odFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallQueueDelay), table_conf_lower(overallQueueDelay, 0.90), table_conf_upper(overallQueueDelay, 0.90), 0.90);
        
	fprintf(qdFile,"%f %e\n", trafficLoad, table_mean(queueDelay));
        fprintf(hdFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyQueueDelay), table_conf_lower(heavyQueueDelay, 0.90), table_conf_upper(heavyQueueDelay, 0.90), 0.90);
        fprintf(ldFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightQueueDelay), table_conf_lower(lightQueueDelay, 0.90), table_conf_upper(lightQueueDelay, 0.90), 0.90);
        fprintf(olFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallQueueLength), table_conf_lower(overallQueueLength, 0.90), table_conf_upper(overallQueueLength, 0.90), 0.90);
        fprintf(hlFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyQueueLength), table_conf_lower(heavyQueueLength, 0.90), table_conf_upper(heavyQueueLength, 0.90), 0.90);
        fprintf(llFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightQueueLength), table_conf_lower(lightQueueLength, 0.90), table_conf_upper(lightQueueLength, 0.90), 0.90);
        fprintf(clFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallCycleLength), table_conf_lower(overallCycleLength, 0.90), table_conf_upper(overallCycleLength, 0.90), 0.90);
        fprintf(clmnFile,"%f %e\n", trafficLoad, table_min(overallCycleLength));
        fprintf(clmxFile,"%f %e\n", trafficLoad, table_max(overallCycleLength));
        fprintf(cllFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightCycleLength), table_conf_lower(lightCycleLength, 0.90), table_conf_upper(lightCycleLength, 0.90), 0.90);
        fprintf(clhFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyCycleLength), table_conf_lower(heavyCycleLength, 0.90), table_conf_upper(heavyCycleLength, 0.90), 0.90);
        fprintf(rsFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallRptToSchedTime), table_conf_lower(overallRptToSchedTime, 0.90), table_conf_upper(overallRptToSchedTime, 0.90), 0.90);
        fprintf(rslFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightRptToSchedTime), table_conf_lower(lightRptToSchedTime, 0.90), table_conf_upper(lightRptToSchedTime, 0.90), 0.90);
        fprintf(rshFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyRptToSchedTime), table_conf_lower(heavyRptToSchedTime, 0.90), table_conf_upper(heavyRptToSchedTime, 0.90), 0.90);
        fprintf(rgFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallRptToGateTime), table_conf_lower(overallRptToGateTime, 0.90), table_conf_upper(overallRptToGateTime, 0.90), 0.90);
        fprintf(rglFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightRptToGateTime), table_conf_lower(lightRptToGateTime, 0.90), table_conf_upper(lightRptToGateTime, 0.90), 0.90);
        fprintf(rghFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyRptToGateTime), table_conf_lower(heavyRptToGateTime, 0.90), table_conf_upper(heavyRptToGateTime, 0.90), 0.90);
        fprintf(gspFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallGrantSizePkt), table_conf_lower(overallGrantSizePkt, 0.90), table_conf_upper(overallGrantSizePkt, 0.90), 0.90);
        fprintf(gsFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallGrantSize), table_conf_lower(overallGrantSize, 0.90), table_conf_upper(overallGrantSize, 0.90), 0.90);
        fprintf(gslFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(lightGrantSize), table_conf_lower(lightGrantSize, 0.90), table_conf_upper(lightGrantSize, 0.90), 0.90);
        fprintf(gshFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(heavyGrantSize), table_conf_lower(heavyGrantSize, 0.90), table_conf_upper(heavyGrantSize, 0.90), 0.90);
        fprintf(noFile,"%f %e\n", trafficLoad, table_mean(numONUSched));
        fprintf(nomxFile,"%f %e\n", trafficLoad, table_max(numONUSched));
        fprintf(srFile,"%f %e\n", trafficLoad, table_mean(schedRounds));
        fprintf(srmxFile,"%f %e\n", trafficLoad, table_max(schedRounds));
        fprintf(lbFile,"%f %e\n", trafficLoad, table_mean(loadBalanceMeasure));
        fprintf(nzFile,"%f %e\n", trafficLoad, meter_rate(overallNonzeroReqRate)/(meter_rate(overallZeroReqRate)+meter_rate(overallNonzeroReqRate)));
        fprintf(nzlFile,"%f %e\n", trafficLoad, meter_rate(lightNonzeroReqRate)/(meter_rate(lightZeroReqRate)+meter_rate(lightNonzeroReqRate)));
        fprintf(nzhFile,"%f %e\n", trafficLoad, meter_rate(heavyNonzeroReqRate)/(meter_rate(heavyZeroReqRate)+meter_rate(heavyNonzeroReqRate)));
        fprintf(pdFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(preferQueueDelay), table_conf_lower(preferQueueDelay, 0.90), table_conf_upper(preferQueueDelay, 0.90), 0.90);
        fprintf(plFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(preferQueueLength), table_conf_lower(preferQueueLength, 0.90), table_conf_upper(preferQueueLength, 0.90), 0.90);
        fprintf(clpFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(preferCycleLength), table_conf_lower(preferCycleLength, 0.90), table_conf_upper(preferCycleLength, 0.90), 0.90);
        
        fprintf(odxlFile,"%e, ", table_conf_mean(overallQueueDelay));
        fprintf(hdxlFile,"%e, ", table_conf_mean(heavyQueueDelay));
        fprintf(ldxlFile,"%e, ", table_conf_mean(lightQueueDelay));
        fprintf(olxlFile,"%e, ", table_conf_mean(overallQueueLength));
        fprintf(hlxlFile,"%e, ", table_conf_mean(heavyQueueLength));
        fprintf(llxlFile,"%e, ", table_conf_mean(lightQueueLength));
        
        fprintf(statsFile,"num_onu_sched = (mean=%e,var=%e,min=%e,max=%e,cnt=%ld)\n",table_mean(numONUSched),table_var(numONUSched),table_min(numONUSched),table_max(numONUSched),table_cnt(numONUSched));
        fprintf(statsFile,"avg_grant_size_pkt = %e +/- %e, (mean=%e,var=%e,min=%e,max=%e)\n",table_conf_mean(overallGrantSizePkt),table_conf_halfwidth(overallGrantSizePkt, 0.90),table_mean(overallGrantSizePkt),table_var(overallGrantSizePkt),table_min(overallGrantSizePkt),table_max(overallGrantSizePkt));
        fprintf(statsFile,"avg_grant_size = %e +/- %e, (mean=%e,var=%e,min=%e,max=%e)\n",table_conf_mean(overallGrantSize),table_conf_halfwidth(overallGrantSize, 0.90),table_mean(overallGrantSize),table_var(overallGrantSize),table_min(overallGrantSize),table_max(overallGrantSize));
        fprintf(statsFile,"avg_cycle_time (batch_size=%lu,num_batches=%lu,converged=%d,acc=%g)\n",table_batch_size(overallCycleLength),table_batch_count(overallCycleLength),table_converged(overallCycleLength),table_conf_accuracy(overallCycleLength,0.90));
        fprintf(statsFile,"avg_cycle_time = %e +/- %e, (mean=%e,var=%e,min=%e,max=%e,cnt=%ld)\n",table_conf_mean(overallCycleLength),table_conf_halfwidth(overallCycleLength, 0.90),table_mean(overallCycleLength),table_var(overallCycleLength),table_min(overallCycleLength),table_max(overallCycleLength),table_cnt(overallCycleLength));
        fprintf(statsFile,"rpt_to_sched_time = %e +/- %e, (mean=%e,var=%e,min=%e,max=%e)\n",table_conf_mean(overallRptToSchedTime),table_conf_halfwidth(overallRptToSchedTime, 0.90),table_mean(overallRptToSchedTime),table_var(overallRptToSchedTime),table_min(overallRptToSchedTime),table_max(overallRptToSchedTime));
        fprintf(statsFile,"rpt_to_gate_time = %e +/- %e, (mean=%e,var=%e,min=%e,max=%e)\n",table_conf_mean(overallRptToGateTime),table_conf_halfwidth(overallRptToGateTime, 0.90),table_mean(overallRptToGateTime),table_var(overallRptToGateTime),table_min(overallRptToGateTime),table_max(overallRptToGateTime));
        fprintf(statsFile,"overallQueueDelay (batch_size=%lu,num_batches=%lu,converged=%d,acc=%g)\n",table_batch_size(overallQueueDelay),table_batch_count(overallQueueDelay),table_converged(overallQueueDelay),table_conf_accuracy(overallQueueDelay,0.90));
        fprintf(statsFile,"overallQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e,cnt=%lu), heavyQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e), lightQueueDelay = %e +/- %e (mean=%e,var=%e,min=%e,max=%e)\n", 
                table_conf_mean(overallQueueDelay), table_conf_halfwidth(overallQueueDelay, 0.90), table_mean(overallQueueDelay), table_var(overallQueueDelay), table_min(overallQueueDelay), table_max(overallQueueDelay), table_cnt(overallQueueDelay), 
                table_conf_mean(heavyQueueDelay), table_conf_halfwidth(heavyQueueDelay, 0.90), table_mean(heavyQueueDelay), table_var(heavyQueueDelay), table_min(heavyQueueDelay), table_max(heavyQueueDelay), 
                table_conf_mean(lightQueueDelay), table_conf_halfwidth(lightQueueDelay, 0.90), table_mean(lightQueueDelay), table_var(lightQueueDelay), table_min(lightQueueDelay), table_max(lightQueueDelay));
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            fprintf(statsFile,"qTime[%d] = %e +/- %e (mean=%e,var=%e,min=%e,max=%e), qLen[%d] = %e +/- %e (mean=%e,var=%e,min=%e,max=%e), rtt = %e\n", loopIndex, table_conf_mean(onuAttrs[loopIndex].queueTimeTable), 
                    table_conf_halfwidth(onuAttrs[loopIndex].queueTimeTable, 0.90), table_mean(onuAttrs[loopIndex].queueTimeTable), table_var(onuAttrs[loopIndex].queueTimeTable), table_min(onuAttrs[loopIndex].queueTimeTable), table_max(onuAttrs[loopIndex].queueTimeTable), loopIndex, 
                    table_conf_mean(onuAttrs[loopIndex].queueLengthTable), 
                    table_conf_halfwidth(onuAttrs[loopIndex].queueLengthTable, 0.90), table_mean(onuAttrs[loopIndex].queueLengthTable), table_var(onuAttrs[loopIndex].queueLengthTable), table_min(onuAttrs[loopIndex].queueLengthTable), table_max(onuAttrs[loopIndex].queueLengthTable), onuAttrs[loopIndex].rtt);
        }
    }

	if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
	{
		fprintf(vodFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(overallVideoQueueDelay), table_conf_lower(overallVideoQueueDelay, 0.90), table_conf_upper(overallVideoQueueDelay, 0.90), 0.90);
		fprintf(vodsFile,"%f %e %e %e\n", trafficLoad, table_var(overallVideoQueueDelay), table_min(overallVideoQueueDelay), table_max(overallVideoQueueDelay));
		fprintf(vugFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(unusedVideoGrant), table_conf_lower(unusedVideoGrant, 0.90), table_conf_upper(unusedVideoGrant, 0.90), 0.90);
		/* To output a report file */
		fprintf(vrFile,"%f %e %e %e %f\n", trafficLoad, table_conf_mean(videoReport), table_conf_lower(videoReport, 0.90), table_conf_upper(videoReport, 0.90), 0.90);
		fprintf(vrmFile,"%f %e\n", trafficLoad, table_max(videoReport));
	}

    /* Report Histograms */
    for(loopIndex=1; loopIndex <= table_histogram_num(overallQueueDelay); loopIndex++)
    {
        /* Print frequency for each bucket */
        fprintf(odHistFile,"%e-%e %e\n",table_histogram_width(overallQueueDelay)*(loopIndex-1),table_histogram_width(overallQueueDelay)*loopIndex,(double)((double)((unsigned long)table_histogram_bucket(overallQueueDelay,loopIndex))/(double)((unsigned long)table_histogram_total(overallQueueDelay))));
    }
    for(loopIndex=1; loopIndex <= table_histogram_num(overallCycleLength); loopIndex++)
    {
        /* Print frequency for each bucket */
        fprintf(clHistFile,"%e-%e %e\n",table_histogram_width(overallCycleLength)*(loopIndex-1),table_histogram_width(overallCycleLength)*loopIndex,(double)((double)((unsigned long)table_histogram_bucket(overallCycleLength,loopIndex))/(double)((unsigned long)table_histogram_total(overallCycleLength))));
    }
    for(loopIndex=1; loopIndex <= table_histogram_num(overallGrantSizePkt); loopIndex++)
    {
        /* Print frequency for each bucket */
        fprintf(gspHistFile,"%e-%e %e\n",table_histogram_width(overallGrantSizePkt)*(loopIndex-1),table_histogram_width(overallGrantSizePkt)*loopIndex,(double)((double)((unsigned long)table_histogram_bucket(overallGrantSizePkt,loopIndex))/(double)((unsigned long)table_histogram_total(overallGrantSizePkt))));
    }
    
    /* Report wavelength utilization */
    for(loopIndex=0; loopIndex < numLambdas; loopIndex++)
    {
        fprintf(utFile[loopIndex],"%f %f\n", trafficLoad, util(lambda[loopIndex]));
        fprintf(tpFile[loopIndex],"%f %f\n", trafficLoad, tput(lambda[loopIndex]));
        fprintf(utxlFile[loopIndex],"%f, ", util(lambda[loopIndex]));
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

    /* Record competitive ratio data */
    fprintf(cr1File,"%f %g %g %g %g\n", trafficLoad, table_mean(compRatio1), table_min(compRatio1), table_max(compRatio1), table_var(compRatio1));
    fprintf(cr2File,"%f %g %g %g %g\n", trafficLoad, table_mean(compRatio2), table_min(compRatio2), table_max(compRatio2), table_var(compRatio2));
    fprintf(mcrFile,"%f %g %g %g %g\n", trafficLoad, table_mean(minCompRatio), table_min(minCompRatio), table_max(minCompRatio), table_var(minCompRatio));

    /* Record excess bandwidth data */
    fprintf(ebFile,"%f %g %g %g %g\n", trafficLoad, table_mean(excessBandwidth), table_min(excessBandwidth), table_max(excessBandwidth), table_var(excessBandwidth));
    fprintf(ebpoFile,"%f %g %g %g %g\n", trafficLoad, table_mean(excessBandwidthONU), table_min(excessBandwidthONU), table_max(excessBandwidthONU), table_var(excessBandwidthONU));

    fprintf(statsFile, "od_hist  (num=%lu,total=%lu,low=%e,high=%e)\n",table_histogram_num(overallQueueDelay),table_histogram_total(overallQueueDelay),table_histogram_low(overallQueueDelay),table_histogram_high(overallQueueDelay));
    fprintf(statsFile, "cl_hist  (num=%lu,total=%lu,low=%e,high=%e)\n",table_histogram_num(overallCycleLength),table_histogram_total(overallCycleLength),table_histogram_low(overallCycleLength),table_histogram_high(overallCycleLength));
    fprintf(statsFile, "gsp_hist (num=%lu,total=%lu,low=%e,high=%e)\n",table_histogram_num(overallGrantSizePkt),table_histogram_total(overallGrantSizePkt),table_histogram_low(overallGrantSizePkt),table_histogram_high(overallGrantSizePkt));

    fprintf(statsFile, "\n\n");

    /* Hack */
    //trafficLoad = simParams.DESIRED_LOAD;

    /* Close files for reporting wavelength utilization */
    for(loopIndex=0; loopIndex < numLambdas; loopIndex++)
    {
        fclose(utFile[loopIndex]);
        fclose(tpFile[loopIndex]);
        fclose(utxlFile[loopIndex]);
    }

    /* Close files for reporting ONU throughput */
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        fclose(tpoFile[loopIndex]);
    
    fclose(odoFile);
    fclose(odFile);
    
    if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
    {
	fclose(vodFile);
	fclose(vodsFile);
	fclose(vugFile);
	fclose(vrFile);
	fclose(vrmFile);
    }
		
    fclose(qdFile);
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
    fclose(clHistFile);
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
    fclose(ebFile);
    fclose(ebpoFile);

}

void write_sim_hist_tail_data(double trafficLoad)
{
    int loopIndex;
    char	filename_suffix[100];
	char	filename_str[150];
    FILE *odHistFile, *clHistFile, *gspHistFile;

    filename_str[0]	= '\0';
    sprintf(filename_str, "od_hist_tail_%d_%d_%s", (int)floor(trafficLoad+0.0001), (int)((trafficLoad+0.0001-(int)floor(trafficLoad+0.0001))*10), filename_suffix);
    odHistFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "cl_hist_tail_%d_%d_%s", (int)floor(trafficLoad+0.0001), (int)((trafficLoad+0.0001-(int)floor(trafficLoad+0.0001))*10), filename_suffix);
    clHistFile	= fopen(filename_str,"a");
    filename_str[0]	= '\0';
    sprintf(filename_str, "gsp_hist_tail_%d_%d_%s", (int)floor(trafficLoad+0.0001), (int)((trafficLoad+0.0001-(int)floor(trafficLoad+0.0001))*10), filename_suffix);
    gspHistFile	= fopen(filename_str,"a");

    /* Report Histograms */
    for(loopIndex=1; loopIndex <= table_histogram_num(overallQueueDelay); loopIndex++)
    {
        /* Print frequency for each bucket */
        fprintf(odHistFile,"%e-%e %e\n",table_histogram_width(overallQueueDelay)*(loopIndex-1)+overallQueueDelayEst.minEst,table_histogram_width(overallQueueDelay)*loopIndex+overallQueueDelayEst.minEst,(double)((double)((unsigned long)table_histogram_bucket(overallQueueDelay,loopIndex))/(double)((unsigned long)table_histogram_total(overallQueueDelay))));
    }
    for(loopIndex=1; loopIndex <= table_histogram_num(overallCycleLength); loopIndex++)
    {
        /* Print frequency for each bucket */
        fprintf(clHistFile,"%e-%e %e\n",table_histogram_width(overallCycleLength)*(loopIndex-1)+overallQueueDelayEst.minEst,table_histogram_width(overallCycleLength)*loopIndex+overallQueueDelayEst.minEst,(double)((double)((unsigned long)table_histogram_bucket(overallCycleLength,loopIndex))/(double)((unsigned long)table_histogram_total(overallCycleLength))));
    }
    for(loopIndex=1; loopIndex <= table_histogram_num(overallGrantSizePkt); loopIndex++)
    {
        /* Print frequency for each bucket */
        fprintf(gspHistFile,"%e-%e %e\n",table_histogram_width(overallGrantSizePkt)*(loopIndex-1)+overallGrantSizePktEst.minEst,table_histogram_width(overallGrantSizePkt)*loopIndex+overallGrantSizePktEst.minEst,(double)((double)((unsigned long)table_histogram_bucket(overallGrantSizePkt,loopIndex))/(double)((unsigned long)table_histogram_total(overallGrantSizePkt))));
    }

    fclose(odHistFile);
    fclose(clHistFile);
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
	  

	  int runNum, numLambdas, sim_aborted;
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
        
        for(numLambdas=simParams.START_LAMBDA; numLambdas <= simParams.END_LAMBDA; numLambdas++)
        {
	          // Test Variables main_begin_lambda
	          test_vars.main_begin_lambda[test_vars.runNum]++;
	          test_var_print();
	    	    
            simParams.NUM_LAMBDAS	= numLambdas;				/* Number of wavelengths supported on PON */
            test_vars.loadOrderCounter = 0;
            for(trafficLoad=simParams.START_LOAD; trafficLoad <= (simParams.END_LOAD + 0.0001); trafficLoad += simParams.LOAD_INCR)
            {
	    	        // Test Variables main_begin_load
	    	        test_vars.main_begin_load[test_vars.runNum][test_vars.loadOrderCounter]++;
	    	        test_var_print();
	    	        TSprint("main_begin_load\t\t\trunNum: %d\t\tutilization: %.1f\n", test_vars.runNum + 1, 0.1*(test_vars.loadOrderCounter + 1));
	              //	fprintf(droppedScalPackets, "Load: %.1f\n", 0.1*(test_vars.loadOrderCounter + 1));
	    	    
                sim_aborted = 0;
                if((trafficLoad + 0.01) >= (double)numLambdas)
                {
                    sim_aborted = 1;
                    continue;
                }
                printf("LOAD = %f\n", trafficLoad);
                simParams.DESIRED_LOAD = trafficLoad;
                //simParams.PIPG_LOAD = trafficLoad*1.041;
      
                /* Do a pilot run of the simulation to estimate statistics */
                simType = PILOT_RUN;
      
                //printf("Pilot Run\n");
                //fflush(NULL);
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
      
                // printf("Actual Run\n");
                // fflush(NULL);
                TSprint("Actual Run\n");
      
                sim();
                fflush(NULL);
                
                
                /* if simulation completes produce output */
                if(terminateSim == 0)
                {
                    write_sim_data(runNum,numLambdas,trafficLoad);
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
                
                if(simParams.GET_TAIL == GET_TAIL_ON)
                {
                    setup_hist_tail();
                    /* Setup the model for next run */
                    rerun();
                    sim_cleanup(); /* perform a memory clean up */
      
                    /* Run the simulation again to get data on the tail of distributions */
                    simType = TAIL_RUN;
                    sim();
      
                    /* if simulation completes produce output */
                    if(terminateSim == 0)
                    {
                        write_sim_hist_tail_data(trafficLoad);
                    }
                    else
                    {
                        terminateSim = 0;
                        fatalErrorCount = 0;
                    }
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
      	    // Test Variables main_end_lambda
      	    test_vars.main_end_lambda[test_vars.runNum]++;
      	    test_var_print();
        }
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
