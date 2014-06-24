#include "../ptpd.h"

void initClock(PtpClock *ptpClock)
{
	DBG("initClock\n");

	/* Clear vars */
	ptpClock->Tms.seconds = ptpClock->Tms.nanoseconds = 0;
	ptpClock->observedDrift = 0;  /* clears clock servo accumulator (the I term) */

	/* One way delay */
	ptpClock->owd_filt.n = 0;
	ptpClock->owd_filt.s = ptpClock->servo.sDelay;

	/* Offset from master */
	ptpClock->ofm_filt.n = 0;
	ptpClock->ofm_filt.s = ptpClock->servo.sOffset;

	/* Scaled log variance */
	if (DEFAULT_PARENTS_STATS)
	{
		ptpClock->slv_filt.n = 0;
		ptpClock->slv_filt.s = 6;
		ptpClock->offsetHistory[0] = 0;
		ptpClock->offsetHistory[1] = 0;
	}

	ptpClock->waitingForFollowUp = FALSE;

	ptpClock->waitingForPDelayRespFollowUp = FALSE;

	ptpClock->pdelay_t1.seconds = ptpClock->pdelay_t1.nanoseconds = 0;
	ptpClock->pdelay_t2.seconds = ptpClock->pdelay_t2.nanoseconds = 0;
	ptpClock->pdelay_t3.seconds = ptpClock->pdelay_t3.nanoseconds = 0;
	ptpClock->pdelay_t4.seconds = ptpClock->pdelay_t4.nanoseconds = 0;

	/* Reset parent statistics */
	ptpClock->parentDS.parentStats = FALSE;
	ptpClock->parentDS.observedParentClockPhaseChangeRate = 0;
	ptpClock->parentDS.observedParentOffsetScaledLogVariance = 0;

	/* Level clock */
	if (!ptpClock->servo.noAdjust)
		adjFreq(0);

	netEmptyEventQ(&ptpClock->netPath);
}

static int32_t order(int32_t n)
{
	if (n < 0) {
		n = -n;
	}
	if (n == 0) {
		return 0;
	}
	return floorLog2(n);
}

/* Exponencial smoothing */
static void filter(int32_t * nsec_current, Filter * filt)
{
	int32_t s, s2;

	/*
			using floatingpoint math
			alpha = 1/2^s
			y[1] = x[0]
			y[n] = alpha * x[n-1] + (1-alpha) * y[n-1]

			or equivalent with integer math
			y[1] = x[0]
			y_sum[1] = y[1] * 2^s
			y_sum[n] = y_sum[n-1] + x[n-1] - y[n-1]
			y[n] = y_sum[n] / 2^s
	*/

	/* Increment number of samples */
	filt->n++;

	/* If it is first time, we are running filter, initialize it */
	if (filt->n == 1)
	{
			filt->y_prev = *nsec_current;
			filt->y_sum = *nsec_current;
			filt->s_prev = 0;
	}

	s = filt->s;

	/* Speedup filter, if not 2^s > n */
	if ((1<<s) > filt->n)
	{
		/* Lower the filter order */
		s = order(filt->n);
	}
	else
	{
		/* Avoid overflowing of n */
		filt->n = 1<<s;
	}

	/* Avoid overflowing of filter. 30 is because using signed 32bit integers */
	s2 = 30 - order(max(filt->y_prev, *nsec_current));

	/* Use the lower filter order, higher will overflow */
	s = min(s, s2);

	/* If the order of the filter changed, change also y_sum value */
	if (filt->s_prev > s) {
		filt->y_sum >>= (filt->s_prev - s);
	} else if (filt->s_prev < s) {
		filt->y_sum <<= (s - filt->s_prev);
	}

	/* Compute the filter itself */
	filt->y_sum += *nsec_current - filt->y_prev;
	filt->y_prev = filt->y_sum >> s;

	/* Save previous order of the filter */
	filt->s_prev = s;

	DBGV("filter: %d -> %d (%d)\n", *nsec_current, filt->y_prev, s);

	/* Actualize target value */
	*nsec_current = filt->y_prev;
}

/* 11.2 */
void updateOffset(PtpClock *ptpClock, const TimeInternal *syncEventIngressTimestamp,
									const TimeInternal *preciseOriginTimestamp, const TimeInternal *correctionField)
{
	DBGV("updateOffset\n");

	/*  <offsetFromMaster> = <syncEventIngressTimestamp> - <preciseOriginTimestamp>
		 - <meanPathDelay>  -  correctionField  of  Sync  message
		 -  correctionField  of  Follow_Up message. */

	/* Compute offsetFromMaster */
	subTime(&ptpClock->Tms, syncEventIngressTimestamp, preciseOriginTimestamp);
	subTime(&ptpClock->Tms, &ptpClock->Tms, correctionField);

	ptpClock->currentDS.offsetFromMaster = ptpClock->Tms;

	switch (ptpClock->portDS.delayMechanism)
	{
		case E2E:
				subTime(&ptpClock->currentDS.offsetFromMaster, &ptpClock->currentDS.offsetFromMaster, &ptpClock->currentDS.meanPathDelay);
				break;

		case P2P:
				subTime(&ptpClock->currentDS.offsetFromMaster, &ptpClock->currentDS.offsetFromMaster, &ptpClock->portDS.peerMeanPathDelay);
				break;

		default:
				break;
	}

	if (ptpClock->currentDS.offsetFromMaster.seconds != 0)
	{
		if (ptpClock->portDS.portState == PTP_SLAVE)
		{
				setFlag(ptpClock->events, SYNCHRONIZATION_FAULT);
		}

		DBGV("updateOffset: cannot filter seconds\n");

		return;
	}

	/* Filter offsetFromMaster */
	filter(&ptpClock->currentDS.offsetFromMaster.nanoseconds, &ptpClock->ofm_filt);

	/* Check results */
	if (abs(ptpClock->currentDS.offsetFromMaster.nanoseconds) < DEFAULT_CALIBRATED_OFFSET_NS)
	{
		if (ptpClock->portDS.portState == PTP_UNCALIBRATED)
		{
				setFlag(ptpClock->events, MASTER_CLOCK_SELECTED);
		}
	}
	else if (abs(ptpClock->currentDS.offsetFromMaster.nanoseconds) > DEFAULT_UNCALIBRATED_OFFSET_NS)
	{
		if (ptpClock->portDS.portState == PTP_SLAVE)
		{
				setFlag(ptpClock->events, SYNCHRONIZATION_FAULT);
		}
	}
}

/* 11.3 */
void updateDelay(PtpClock * ptpClock, const TimeInternal *delayEventEgressTimestamp,
								 const TimeInternal *recieveTimestamp, const TimeInternal *correctionField)
{
	/* Tms valid ? */
	if (0 == ptpClock->ofm_filt.n)
	{
		DBGV("updateDelay: Tms is not valid");
		return;
	}

	subTime(&ptpClock->Tsm, recieveTimestamp, delayEventEgressTimestamp);
	subTime(&ptpClock->Tsm, &ptpClock->Tsm, correctionField);
	addTime(&ptpClock->currentDS.meanPathDelay, &ptpClock->Tms, &ptpClock->Tsm);
	div2Time(&ptpClock->currentDS.meanPathDelay);

	/* Filter delay */
	if (0 != ptpClock->currentDS.meanPathDelay.seconds)
	{
		DBGV("updateDelay: cannot filter with seconds");
	}
	else
	{
		filter(&ptpClock->currentDS.meanPathDelay.nanoseconds, &ptpClock->owd_filt);
	}
}

void updatePeerDelay(PtpClock *ptpClock, const TimeInternal *correctionField, bool  twoStep)
{
	DBGV("updatePeerDelay\n");

	if (twoStep)
	{
		TimeInternal Tab, Tba;
		subTime(&Tab, &ptpClock->pdelay_t2 , &ptpClock->pdelay_t1);
		subTime(&Tba, &ptpClock->pdelay_t4, &ptpClock->pdelay_t3);
		addTime(&ptpClock->portDS.peerMeanPathDelay, &Tab, &Tba);
	}
	else /* One step  clock */
	{
		subTime(&ptpClock->portDS.peerMeanPathDelay, &ptpClock->pdelay_t4, &ptpClock->pdelay_t1);
	}

	subTime(&ptpClock->portDS.peerMeanPathDelay, &ptpClock->portDS.peerMeanPathDelay, correctionField);
	div2Time(&ptpClock->portDS.peerMeanPathDelay);

	/* Filter delay */
	if (ptpClock->portDS.peerMeanPathDelay.seconds != 0)
	{
		DBGV("updatePeerDelay: cannot filter with seconds");
		return;
	}
	else
	{
		filter(&ptpClock->portDS.peerMeanPathDelay.nanoseconds, &ptpClock->owd_filt);
	}
}

void updateClock(PtpClock *ptpClock)
{
	int32_t adj;
	TimeInternal timeTmp;
	int32_t offsetNorm;

	DBGV("updateClock\n");

	if (ptpClock->currentDS.offsetFromMaster.seconds != 0 || abs(ptpClock->currentDS.offsetFromMaster.nanoseconds) > MAX_ADJ_OFFSET_NS)
	{
		/* if secs, reset clock or set freq adjustment to max */
		if (!ptpClock->servo.noAdjust)
		{
			if (!ptpClock->servo.noResetClock)
			{
				getTime(&timeTmp);
				subTime(&timeTmp, &timeTmp, &ptpClock->currentDS.offsetFromMaster);
				setTime(&timeTmp);
				initClock(ptpClock);
			}
			else
			{
				adj = ptpClock->currentDS.offsetFromMaster.nanoseconds > 0 ? ADJ_FREQ_MAX : -ADJ_FREQ_MAX;
				adjFreq(-adj);
			}
		}
	}
	else
	{
		/* the PI controller */

		/* normalize offset to 1s sync interval -> response of the servo will
		 * be same for all sync interval values, but faster/slower
		 * (possible lost of precision/overflow but much more stable) */
		offsetNorm = ptpClock->currentDS.offsetFromMaster.nanoseconds;
		if (ptpClock->portDS.logSyncInterval > 0)
			offsetNorm >>= ptpClock->portDS.logSyncInterval;
		else if (ptpClock->portDS.logSyncInterval < 0)
			offsetNorm <<= -ptpClock->portDS.logSyncInterval;

		/* the accumulator for the I component */
		ptpClock->observedDrift += offsetNorm / ptpClock->servo.ai;

		/* clamp the accumulator to ADJ_FREQ_MAX for sanity */
		if (ptpClock->observedDrift > ADJ_FREQ_MAX)
			ptpClock->observedDrift = ADJ_FREQ_MAX;
		else if (ptpClock->observedDrift < -ADJ_FREQ_MAX)
			ptpClock->observedDrift = -ADJ_FREQ_MAX;

		/* apply controller output as a clock tick rate adjustment */
		if (!ptpClock->servo.noAdjust)
		{
			adj = offsetNorm / ptpClock->servo.ap + ptpClock->observedDrift;
			adjFreq(-adj);
		}

		if (DEFAULT_PARENTS_STATS)
		{
			int a, scaledLogVariance;
			ptpClock->parentDS.parentStats = TRUE;
			ptpClock->parentDS.observedParentClockPhaseChangeRate = 1100 * ptpClock->observedDrift;

			a = (ptpClock->offsetHistory[1] - 2 * ptpClock->offsetHistory[0] + ptpClock->currentDS.offsetFromMaster.nanoseconds);
			ptpClock->offsetHistory[1] = ptpClock->offsetHistory[0];
			ptpClock->offsetHistory[0] = ptpClock->currentDS.offsetFromMaster.nanoseconds;

			scaledLogVariance = order(a * a) << 8;
			filter(&scaledLogVariance, &ptpClock->slv_filt);
			ptpClock->parentDS.observedParentOffsetScaledLogVariance = 17000 + scaledLogVariance;
			DBGV("updateClock: observed scalled log variance: 0x%x\n", ptpClock->parentDS.observedParentOffsetScaledLogVariance);
		}
	}

	switch (ptpClock->portDS.delayMechanism)
	{
		case E2E:
			DBG("updateClock: one-way delay averaged (E2E): %d sec %d nsec\n",
					ptpClock->currentDS.meanPathDelay.seconds, ptpClock->currentDS.meanPathDelay.nanoseconds);
			break;

		case P2P:
			DBG("updateClock: one-way delay averaged (P2P): %d sec %d nsec\n",
					ptpClock->portDS.peerMeanPathDelay.seconds, ptpClock->portDS.peerMeanPathDelay.nanoseconds);
			break;

		default:
			DBG("updateClock: one-way delay not computed\n");
	}

	DBG("updateClock: offset from master: %d sec %d nsec\n",
			ptpClock->currentDS.offsetFromMaster.seconds,
			ptpClock->currentDS.offsetFromMaster.nanoseconds);
	DBG("updateClock: observed drift: %d\n", ptpClock->observedDrift);
}
