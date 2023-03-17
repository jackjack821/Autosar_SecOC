/******************************************************************************
 * File Name   : SecOC_Lcfg                                                   *
 * Description : Header file of SecOC configuration                           *
 * Created on  : November 9, 2022                                             *
 ******************************************************************************/
#include "SecOC_Cfg.h"
#include "SecOC_Lcfg.h"

/* [SWS_SecOC_00057] The SecOC module shall provide sufficient buffers to store the Authentic I-PDU and the outgoing Secured I-PDU */ 
/* [SWS_SecOC_00146] The SecOC module shall provide separate buffers for the Authentic I-PDU and the Secured I-PDU. */
/* [SWS_SecOC_00058] */
uint8_t AuthPdu0BufferTx[SECOC_AUTHPDU_MAX_LENGTH];
uint8_t SecPdu0BufferTx[SECOC_SECPDU_MAX_LENGTH];

uint8_t AuthPdu0BufferRx[SECOC_AUTHPDU_MAX_LENGTH];
uint8_t SecPdu0BufferRx[SECOC_SECPDU_MAX_LENGTH];


uint8_t AuthPdu1BufferTx[SECOC_AUTHPDU_MAX_LENGTH];
uint8_t SecPdu1BufferTx[SECOC_SECPDU_MAX_LENGTH];

uint8_t AuthPdu1BufferRx[SECOC_AUTHPDU_MAX_LENGTH];
uint8_t SecPdu1BufferRx[SECOC_SECPDU_MAX_LENGTH];

/*
* Start Of General
*/

SecOC_MainFunctionTxType SecOC_MainFunctionTxConf = 
{
    SECOC_MAIN_FUNCTION_PERIOD_TX,
    SECOC_MAIN_FUNCTION_TX_PARTITION_REF, /* NOT SURE ABOUT THAT TYPE */

};


SecOC_MainFunctionRxType SecOC_MainFunctionRx = 
{
    SECOC_MAIN_FUNCTION_PERIOD_RX,
    SECOC_MAIN_FUNCTION_RX_PARTITION_REF, /* NOT SURE ABOUT THAT TYPE */

};



SecOC_GeneralType SecOC_General = 
{
    SECOC_DEFAULT_AUTHENTICATION_INFORMATION_PATTERN_VALUE,
    SECOC_DEV_ERROR_DETECT,
    SECOC_ENABLE_FORCED_PASS_OVERRIDE,
    SECOC_ENABLE_SECURITY_EVENT_REPORTING,
    SECOC_IGNORE_VERIFICATION_RESULT,
    SECOC_MAX_ALIGN_SCALAR_TYPE,
    SECOC_OVERRIDE_STATUS_WITH_DATA_ID,
    SECOC_PROPAGATE_ONLY_FINAL_VERIFICATION_STATUS,
    SECOC_QUERY_FRESHNESS_VALUE,
    /* SECOC_VERIFICATION_STATUS_CALLOUT,*/
    SECOC_VERSION_INFO_API,
    /*SECOC_SECURITY_EVENT_REFS,*/ /* can't have the container of it */

};



SecOC_RxCryptographicPduType SecOC_RxCryptographicPdu =
{
    SECOC_RX_CRYPTOGRAPHIC_PDUID,
    /*&EcuC_Pdu*/
};

SecOC_RxAuthenticPduType SecOC_RxAuthenticPdu =
{
    SECOC_CRYPTO_AUTHPDU_HEADERLENGTH,
    SECOC_CRYPTO_RXAUTHENTIC_PDUID,
    /*&EcuC_Pdu*/
};



SecOC_UseMessageLinkType SecOC_UseMessageLink=
{
    SECOC_MESSAGE_LINKLEN,
    SECOC_MESSAGE_LINKPOS,
};

SecOC_RxSecuredPduCollectionType SecOC_RxSecuredPduCollection =
{
    SECOC_SECURED_RX_PDU_VERIFICATION,
    &SecOC_RxAuthenticPdu,
    &SecOC_RxCryptographicPdu,
    &SecOC_UseMessageLink
};

SecOC_RxAuthenticPduLayerType SecOC_RxAuthenticPduLayer[] = 
{
    {
        SECOC_RX_PDUTYPE,
        SECOC_RXAUTHENTICLAYER_PDUID,
        {AuthPdu0BufferRx, NULL, 4},
    },
    {
        SECOC_RX_PDUTYPE,
        SECOC_RXAUTHENTICLAYER_PDUID,
        {AuthPdu1BufferRx, NULL, 4},
    }
};

SecOC_RxSecuredPduType SecOC_RxSecuredPdu[] = 
{
    {
        SECOC_AUTHPDU_HEADERLENGTH,
        SECOC_RX_SECUREDLAYER_PDUID,
        SECOC_SECURED_RX_PDUVERIFICATION,
        {SecPdu0BufferRx, NULL, 0},
    },
    {
        SECOC_AUTHPDU_HEADERLENGTH,
        SECOC_RX_SECUREDLAYER_PDUID,
        SECOC_SECURED_RX_PDUVERIFICATION,
        {SecPdu1BufferRx, NULL, 0},
    }
};

SecOC_RxSecuredPduLayerType SecOC_RxSecuredPduLayer[] =
{
    {
        &SecOC_RxSecuredPdu[0],
        &SecOC_RxSecuredPduCollection  
    } ,
    {
        &SecOC_RxSecuredPdu[1],
        &SecOC_RxSecuredPduCollection  
    } 
};



Csm_JobType CsmJob = 
{
    CSM_JOBID
};



SecOC_RxAuthServiceConfigRefType SecOC_RxAuthServiceConfigRef = 
{
    &CsmJob
};

SecOC_SameBufferPduCollectionType SecOC_SameBufferPduRef=
{
    SECOC_BUFFERLENGTH
};








SecOC_TxAuthenticPduLayerType SecOC_TxAuthenticPduLayer[]=
{
    {   
        .SecOCPduType = SECOC_IFPDU,
        SECOC_TX_AUTHENTIC_LAYER_PDUID,

        {AuthPdu0BufferTx, NULL, 0},
    },
    {   
        .SecOCPduType = SECOC_TPPDU,
        SECOC_TX_AUTHENTIC_LAYER_PDUID,
        {AuthPdu1BufferTx, NULL, 0},
    }
};

SecOC_TxSecuredPduType SecOC_TxSecuredPdu[]=
{
    {
        SECOC_AUTH_PDUHEADER_LENGTH,
        SECOC_TX_SECURED_LAYER_PDUID,
        {SecPdu0BufferTx, NULL, 0},
    },
    {
        SECOC_AUTH_PDUHEADER_LENGTH,
        SECOC_TX_SECURED_LAYER_PDUID,
        {SecPdu1BufferTx, NULL, 0},
    }
};



/* Crypto not available now*/
/*
SecOC_TxCryptographicPduType SecOC_TxCryptographicPdu=
{
    SECOC_TX_CRYPTOGRAPHIC_PDUID,
    SECOC_TX_CRYPTOGRAPHIC_PDUREF,
};
*/

/* This is related to crypto(the authentic part)*/
/*
SecOC_TxAuthenticPduType SecOC_TxAuthenticPdu=
{
    SECOC_AUTH_PDUHEADER_LENGTH,
    SECOC_TX_AUTHENTIC_PDUID,
    ,
};
*/

/*
SecOC_TxSecuredPduCollectionType SecOC_TxSecuredPduCollection=
{
    &SecOC_TxAuthenticPdu,
    &SecOC_TxCryptographicPdu,
    &SecOC_UseMessageLink,
};
*/

/* This block is commented as the whole authentic I-Pdu is included in authenticating algorithm*/
/*
SecOC_TxPduSecuredAreaType SecOC_TxPduSecuredArea=
{
    SECOC_SECURED_TX_PDULENGTH,
    SECOC_SECURED_TX_PDUOFFSET,
};


SecOC_RxPduSecuredAreaType SecOC_RxPduSecuredArea=
{
    SECOC_SECURED_RX_PDULENGTH,
    SECOC_SECURED_RX_PDUOFFSET,
};
*/

SecOC_TxSecuredPduLayerType SecOC_TxSecuredPduLayer[]=
{
    {
        &SecOC_TxSecuredPdu[0],
        NULL,
    },
    {
        &SecOC_TxSecuredPdu[1],
        NULL,
    }
};

SecOC_TxPduProcessingType SecOC_TxPduProcessing[] = {
    {
        SECOC_AUTHENTICATION_BUILD_ATTEMPTS,
        SECOC_TX_AUTH_INFO_TRUNC_LENGTH,
        SECOC_TX_DATA_ID,
        .SecOCFreshnessValueId = 10,
        SECOC_TX_FRESHNESS_VALUE_LENGTH,
        .SecOCFreshnessValueTruncLength = 1,
        SECOC_PROVIDE_TX_TRUNCATED_FRESHNESS_VALUE,
        SECOC_RE_AUTHENTICATE_AFTER_TRIGGER_TRANSMIT,
        SECOC_TX_PDU_UNUSED_AREAS_DEFAULT,
        SECOC_USE_TX_CONFIRMATION,
        /*                                                  SecOCSameBufferPduRef;*/
        /*                                                  SecOCTxAuthServiceConfigRef*/
        /*                                                  SecOCTxPduMainFunctionRef;*/
        &SecOC_TxAuthenticPduLayer[0],
        &SecOC_TxSecuredPduLayer[0],
        /*&SecOC_TxPduSecuredArea,*/
        /* &EcuC_Pdu,*/
    },
    {
        SECOC_AUTHENTICATION_BUILD_ATTEMPTS,
        SECOC_TX_AUTH_INFO_TRUNC_LENGTH,
        SECOC_TX_DATA_ID,
        .SecOCFreshnessValueId = 9,
        SECOC_TX_FRESHNESS_VALUE_LENGTH,
        SECOC_TX_FRESHNESS_VALUE_TRUNC_LENGTH,
        SECOC_PROVIDE_TX_TRUNCATED_FRESHNESS_VALUE,
        SECOC_RE_AUTHENTICATE_AFTER_TRIGGER_TRANSMIT,
        SECOC_TX_PDU_UNUSED_AREAS_DEFAULT,
        SECOC_USE_TX_CONFIRMATION,
        /*                                                  SecOCSameBufferPduRef;*/
        /*                                                  SecOCTxAuthServiceConfigRef*/
        /*                                                  SecOCTxPduMainFunctionRef;*/
        &SecOC_TxAuthenticPduLayer[1],
        &SecOC_TxSecuredPduLayer[1],
        /*&SecOC_TxPduSecuredArea,*/
        /* &EcuC_Pdu,*/
    }
};



SecOC_RxPduProcessingType SecOC_RxPduProcessing[] = 
{
    
    {        
        SECOC_AUTHDATA_FRESHNESSLEN,
        SECOC_AUTHDATA_FRESHNESS_STARTPOSITION,
        SECOC_AUTHENTICATION_BUILDATTEMPTS,
        SECOC_AUTHENTICATION_VERIFYATTEMPTS,
        SECOC_RX_AUTH_INFO_TRUNCLENGTH,
        SECOC_CLIENTSERVER_VERIFICATIONSTATUS_PROPAGATIONMODE,
        SECOC_RX_DATA_ID,
        .SecOCFreshnessValueId = 10,
        SECOC_RX_FRESHNESS_VALUE_LENGTH,
        .SecOCFreshnessValueTruncLength = 1,
        SECOC_RECEPTION_OVERFLOW_STRATEGY,
        SECOC_RECEPTION_QUEUESIZE,
        SECOC_USE_AUTHDATA_FRESHNESS,
        SECOC_VERIFICATIONSTATUS_PROPAGATIONMODE,
        &SecOC_RxAuthServiceConfigRef,
        &SecOC_MainFunctionRx,
        &SecOC_SameBufferPduRef,
        &SecOC_RxSecuredPduLayer[0],
        &SecOC_RxAuthenticPduLayer[0],
        /*&SecOC_RxPduSecuredArea*/
    },
    {        
        SECOC_AUTHDATA_FRESHNESSLEN,
        SECOC_AUTHDATA_FRESHNESS_STARTPOSITION,
        SECOC_AUTHENTICATION_BUILDATTEMPTS,
        SECOC_AUTHENTICATION_VERIFYATTEMPTS,
        SECOC_RX_AUTH_INFO_TRUNCLENGTH,
        SECOC_CLIENTSERVER_VERIFICATIONSTATUS_PROPAGATIONMODE,
        SECOC_RX_DATA_ID,
        .SecOCFreshnessValueId = 9,
        SECOC_RX_FRESHNESS_VALUE_LENGTH,
        SECOC_RX_FRESHNESS_VALUE_TRUNCLENGTH,
        SECOC_RECEPTION_OVERFLOW_STRATEGY,
        SECOC_RECEPTION_QUEUESIZE,
        SECOC_USE_AUTHDATA_FRESHNESS,
        SECOC_VERIFICATIONSTATUS_PROPAGATIONMODE,
        &SecOC_RxAuthServiceConfigRef,
        &SecOC_MainFunctionRx,
        &SecOC_SameBufferPduRef,
        &SecOC_RxSecuredPduLayer[1],
        &SecOC_RxAuthenticPduLayer[1],
        /*&SecOC_RxPduSecuredArea*/
    }
    
};


SecOC_ConfigType SecOC_Config=
{
    &SecOC_General,
    SecOC_TxPduProcessing,
    SecOC_RxPduProcessing,
};

SecOC_TxCountersType SecOC_TxCounters[SECOC_NUM_OF_TX_PDU_PROCESSING] = 
{
    {
        /* [SWS_SecOC_00226] */
        .AuthenticationCounter = 0,
    },
    {
        /* [SWS_SecOC_00226] */
        .AuthenticationCounter = 0,
    }
};

SecOC_RxCountersType SecOC_RxCounters[SECOC_NUM_OF_RX_PDU_PROCESSING] = 
{
    {
        /* [SWS_SecOC_00235] */
        .AuthenticationCounter = 0,
        .VerificationCounter = 0
    },
    {
        /* [SWS_SecOC_00235] */
        .AuthenticationCounter = 0,
        .VerificationCounter = 0
    }
};