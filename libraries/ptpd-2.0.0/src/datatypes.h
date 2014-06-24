#ifndef DATATYPES_H_
#define DATATYPES_H_

/**
 *\file
 * \brief 5.3 Derived data type specifications
 *
 * This header file defines structures defined by the spec,
 * main program data structure, and all messages structures
 */


/**
 * \brief 5.3.2 The TimeInterval type represents time intervals
 * in scaled nanoseconds where scaledNanoseconds = time[ns] * 2^16
 */

typedef struct
{
		int64_t scaledNanoseconds;
} TimeInterval;

/**
 * \brief 5.3.3 The Timestamp type represents a positive time with respect to the epoch
 */

typedef struct
{
		uint48bit_t secondsField;
		uint32_t nanosecondsField;
} Timestamp;

/**
 * \brief 5.3.4 The ClockIdentity type identifies a clock
 */
typedef octet_t ClockIdentity[CLOCK_IDENTITY_LENGTH];

/**
 * \brief 5.3.5 The PortIdentity identifies a PTP port.
 */

typedef struct
{
		ClockIdentity clockIdentity;
		int16_t portNumber;
} PortIdentity;

/**
 * \brief 5.3.6 The PortAdress type represents the protocol address of a PTP port
 */

 typedef struct
{
		enum16bit_t networkProtocol;
		int16_t adressLength;
		octet_t* adressField;
} PortAdress;

/**
* \brief 5.3.7 The ClockQuality represents the quality of a clock
 */

typedef struct
{
		uint8_t clockClass;
		enum8bit_t clockAccuracy;
		int16_t offsetScaledLogVariance;
} ClockQuality;

/**
 * \brief 5.3.8 The TLV type represents TLV extension fields
 */

typedef struct
{
		enum16bit_t tlvType;
		int16_t lengthField;
		octet_t* valueField;
} TLV;

/**
 * \brief 5.3.9 The PTPText data type is used to represent textual material in PTP messages
 * textField - UTF-8 encoding
 */

typedef struct
{
		uint8_t lengthField;
		octet_t* textField;
} PTPText;

/**
* \brief 5.3.10 The FaultRecord type is used to construct fault logs
 */

typedef struct
{
		int16_t faultRecordLength;
		Timestamp faultTime;
		enum8bit_t severityCode;
		PTPText faultName;
		PTPText faultValue;
		PTPText faultDescription;
} FaultRecord;


/**
 * \brief The common header for all PTP messages (Table 18 of the spec)
 */

typedef struct
{
		nibble_t transportSpecific;
		enum4bit_t messageType;
		uint4bit_t  versionPTP;
		int16_t messageLength;
		uint8_t domainNumber;
		octet_t flagField[2];
		int64_t correctionfield;
		PortIdentity sourcePortIdentity;
		int16_t sequenceId;
		uint8_t controlField;
		int8_t logMessageInterval;
} MsgHeader;


/**
 * \brief Announce message fields (Table 25 of the spec)
 */

typedef struct
{
		Timestamp originTimestamp;
		int16_t currentUtcOffset;
		uint8_t grandmasterPriority1;
		ClockQuality grandmasterClockQuality;
		uint8_t grandmasterPriority2;
		ClockIdentity grandmasterIdentity;
		int16_t stepsRemoved;
		enum8bit_t timeSource;
}MsgAnnounce;


/**
 * \brief Sync message fields (Table 26 of the spec)
 */

typedef struct
{
		Timestamp originTimestamp;
}MsgSync;

/**
 * \brief DelayReq message fields (Table 26 of the spec)
 */

typedef struct
{
		Timestamp originTimestamp;
}MsgDelayReq;

/**
 * \brief DelayResp message fields (Table 30 of the spec)
 */

typedef struct
{
		Timestamp receiveTimestamp;
		PortIdentity requestingPortIdentity;
}MsgDelayResp;

/**
 * \brief FollowUp message fields (Table 27 of the spec)
 */

typedef struct
{
		Timestamp preciseOriginTimestamp;
}MsgFollowUp;

/**
 * \brief PDelayReq message fields (Table 29 of the spec)
 */

typedef struct
{
		Timestamp originTimestamp;

}MsgPDelayReq;

/**
 * \brief PDelayResp message fields (Table 30 of the spec)
 */

typedef struct
{
		Timestamp requestReceiptTimestamp;
		PortIdentity requestingPortIdentity;
}MsgPDelayResp;

/**
 * \brief PDelayRespFollowUp message fields (Table 31 of the spec)
 */

typedef struct
{
		Timestamp responseOriginTimestamp;
		PortIdentity requestingPortIdentity;
}MsgPDelayRespFollowUp;

/**
* \brief Signaling message fields (Table 33 of the spec)
 */

typedef struct
{
		PortIdentity targetPortIdentity;
		char* tlv;
}MsgSignaling;

/**
* \brief Management message fields (Table 37 of the spec)
 */

typedef struct
{
		PortIdentity targetPortIdentity;
		uint8_t startingBoundaryHops;
		uint8_t boundaryHops;
		enum4bit_t actionField;
		char* tlv;
}MsgManagement;


/**
* \brief Time structure to handle Linux time information
 */

typedef struct
{
		int32_t seconds;
		int32_t nanoseconds;
} TimeInternal;

/**
* \brief ForeignMasterRecord is used to manage foreign masters
 */

typedef struct
{
		PortIdentity foreignMasterPortIdentity;
		int16_t foreignMasterAnnounceMessages;

		/* This one is not in the spec */
		MsgAnnounce  announce;
		MsgHeader    header;

} ForeignMasterRecord;

/**
 * \struct DefaultDS
 * \brief spec 8.2.1 default data set
 * spec 7.6.2, spec 7.6.3 PTP device attributes
 */

typedef struct
{
		bool  twoStepFlag;
		ClockIdentity clockIdentity; /**< spec 7.6.2.1 */
		int16_t numberPorts;  /**< spec 7.6.2.7 */
		ClockQuality clockQuality; /**< spec 7.6.2.4, 7.6.3.4 and 7.6.3 */
		uint8_t priority1; /**< spec 7.6.2.2 */
		uint8_t priority2; /**< spec 7.6.2.3 */
		uint8_t domainNumber;
		bool  slaveOnly;
} DefaultDS;


/**
 * \struct CurrentDS
 * \brief spec 8.2.2 current data set
 */

typedef struct
{
		int16_t stepsRemoved;
		TimeInternal offsetFromMaster;
		TimeInternal meanPathDelay;
} CurrentDS;


/**
 * \struct ParentDS
 * \brief spec 8.2.3 parent data set
 */

typedef struct
{
		PortIdentity parentPortIdentity;
		/* 7.6.4 Parent clock statistics - parentDS */
		bool  parentStats; /**< spec 7.6.4.2 */
		int16_t observedParentOffsetScaledLogVariance; /**< spec 7.6.4.3 */
		int32_t observedParentClockPhaseChangeRate; /**< spec 7.6.4.4 */

		ClockIdentity grandmasterIdentity;
		ClockQuality grandmasterClockQuality;
		uint8_t grandmasterPriority1;
		uint8_t grandmasterPriority2;
} ParentDS;

/**
 * \struct TimePropertiesDS
 * \brief spec 8.2.4 time properties data set
 */

 typedef struct
{
		int16_t currentUtcOffset;
		bool  currentUtcOffsetValid;
		bool  leap59;
		bool  leap61;
		bool  timeTraceable;
		bool  frequencyTraceable;
		bool  ptpTimescale;
		enum8bit_t timeSource; /**< spec 7.6.2.6 */
} TimePropertiesDS;


/**
 * \struct PortDS
 * \brief spec 8.2.5 port data set
 */

 typedef struct
{
		PortIdentity portIdentity;
		enum8bit_t portState;
		int8_t logMinDelayReqInterval; /**< spec 7.7.2.4 */
		TimeInternal peerMeanPathDelay;
		int8_t logAnnounceInterval; /**< spec 7.7.2.2 */
		uint8_t announceReceiptTimeout; /**< spec 7.7.3.1 */
		int8_t logSyncInterval; /**< spec 7.7.2.3 */
		enum8bit_t delayMechanism;
		int8_t logMinPdelayReqInterval; /**< spec 7.7.2.5 */
		uint4bit_t  versionNumber;
} PortDS;


/**
 * \struct ForeignMasterDS
 * \brief Foreign master data set
 */

typedef struct
{
		ForeignMasterRecord *records;

		/* Other things we need for the protocol */
		int16_t count;
		int16_t  capacity;
		int16_t  i;
		int16_t  best;
} ForeignMasterDS;

/**
 * \struct Servo
 * \brief Clock servo filters and PI regulator values
 */

typedef struct
{
		bool  noResetClock;
		bool  noAdjust;
		int16_t ap, ai;
		int16_t sDelay;
		int16_t sOffset;
} Servo;

/**
 * \struct RunTimeOpts
 * \brief Program options set at run-time
 */

typedef struct
{
		int8_t  announceInterval;
		int8_t  syncInterval;
		ClockQuality clockQuality;
		uint8_t  priority1;
		uint8_t  priority2;
		uint8_t  domainNumber;
		bool   slaveOnly;
		int16_t  currentUtcOffset;
		octet_t   ifaceName[IFACE_NAME_LENGTH];
		enum8bit_t stats;
		octet_t   unicastAddress[NET_ADDRESS_LENGTH];
		TimeInternal  inboundLatency, outboundLatency;
		int16_t   maxForeignRecords;
		enum8bit_t  delayMechanism;
	Servo servo;
} RunTimeOpts;

/**
 * \struct PtpClock
 * \brief Main program data structure
 */
/* main program data structure */

typedef struct
{

	DefaultDS defaultDS; /**< default data set */
	CurrentDS currentDS; /**< current data set */
	ParentDS parentDS; /**< parent data set */
	TimePropertiesDS timePropertiesDS; /**< time properties data set */
	PortDS portDS; /**< port data set */
	ForeignMasterDS foreignMasterDS; /**< foreign master data set */

		MsgHeader msgTmpHeader; /**< buffer for incomming message header */

		union
		{
				MsgSync  sync;
				MsgFollowUp  follow;
				MsgDelayReq  req;
				MsgDelayResp resp;
				MsgPDelayReq  preq;
				MsgPDelayResp  presp;
				MsgPDelayRespFollowUp  prespfollow;
				MsgManagement  manage;
				MsgAnnounce  announce;
				MsgSignaling signaling;
		} msgTmp; /**< buffer for incomming message body */


		octet_t msgObuf[PACKET_SIZE]; /**< buffer for outgoing message */
		octet_t msgIbuf[PACKET_SIZE]; /** <buffer for incomming message */
		ssize_t msgIbufLength; /**< length of incomming message */

		TimeInternal Tms; /**< Time Master -> Slave */
		TimeInternal Tsm; /**< Time Slave -> Master */

	TimeInternal pdelay_t1; /**< peer delay time t1 */
	TimeInternal pdelay_t2; /**< peer delay time t2 */
	TimeInternal pdelay_t3; /**< peer delay time t3 */
	TimeInternal pdelay_t4; /**< peer delay time t4 */

		TimeInternal timestamp_syncRecieve; /**< timestamp of Sync message */
		TimeInternal timestamp_delayReqSend; /**< timestamp of delay request message */
		TimeInternal timestamp_delayReqRecieve; /**< timestamp of delay request message */

		TimeInternal correctionField_sync; /**< correction field of Sync and FollowUp messages */
		TimeInternal correctionField_pDelayResp; /**< correction fieald of peedr delay response */

		/* MsgHeader  PdelayReqHeader; */ /**< last recieved peer delay reques header */

		int16_t sentPDelayReqSequenceId;
		int16_t sentDelayReqSequenceId;
		int16_t sentSyncSequenceId;
		int16_t sentAnnounceSequenceId;

		int16_t recvPDelayReqSequenceId;
		int16_t recvSyncSequenceId;

		bool   waitingForFollowUp; /**< true if sync message was recieved and 2step flag is set */
	bool   waitingForPDelayRespFollowUp; /**< true if PDelayResp message was recieved and 2step flag is set */

		Filter  ofm_filt; /**< filter offset from master */
		Filter  owd_filt; /**< filter one way delay */
	Filter  slv_filt; /**< filter scaled log variance */
	int16_t offsetHistory[2];
		int32_t  observedDrift;

		bool  messageActivity;

		NetPath netPath;

		enum8bit_t recommendedState;

		octet_t portUuidField[PTP_UUID_LENGTH]; /**< Usefull to init network stuff */

		TimeInternal  inboundLatency, outboundLatency;

	Servo servo;

		int32_t  events;

		enum8bit_t  stats;

		RunTimeOpts * rtOpts;

} PtpClock;

#endif /* DATATYPES_H_*/
