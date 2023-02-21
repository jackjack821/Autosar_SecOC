/* "Copyright [2022/2023] <Tanta University>" */
#include "SecOC_Lcfg.h"
#include "SecOC_Cfg.h"
#include "SecOC_PBcfg.h"
#include "SecOC_Cbk.h"
#include "ComStack_Types.h"
#include "Rte_SecOC.h"
#include "SecOC.h"
#include "PduR_SecOC.h"
#include "Csm.h"
#include "Rte_SecOC_Type.h"
#include "PduR_Com.h"
#include "PduR_SecOC.h"
#include "Pdur_CanTP.h"
#include "PduR_CanIf.h"

#include <string.h>
#include <stdio.h>

const SecOC_TxPduProcessingType     *SecOCTxPduProcessing;
const SecOC_RxPduProcessingType     *SecOCRxPduProcessing;
const SecOC_GeneralType             *SecOCGeneral;


// Internal functions
static Std_ReturnType constructDataToAuthenticatorTx(const PduIdType TxPduId, const uint8 *DataToAuth, uint32 *DataToAuthLen, const PduInfoType* AuthPdu);
static Std_ReturnType generateMAC(const PduIdType TxPduId, uint8 *DataToAuth, const uint32 *DataToAuthLen, uint8  *authenticatorPtr, uint32  *authenticatorLen);
static Std_ReturnType authenticate(const PduIdType TxPduId, const PduInfoType* AuthPdu, PduInfoType* SecPdu);

static Std_ReturnType parseSecuredPdu(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_RxIntermediateType *SecOCIntermediate);
static Std_ReturnType constructDataToAuthenticatorRx(PduIdType RxPduId, PduInfoType* secPdu, uint8 *DataToAuth, uint32 *DataToAuthLen, uint8 *TruncatedLength_Bytes,uint8* SecOCFreshnessValue,uint32* SecOCFreshnessValueLength );
static Std_ReturnType verify(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_VerificationResultType *verification_result);

/****************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : constructDataToAuthenticatorTx*
 * Function_Index       : SecOC internal                *
 * Parameter in         : TxPduId                       *
 * Parameter in         : DataToAuth                    *
 * Parameter in/out     : DataToAuthLen                 *
 * Parameter in         : AuthPdu                       *
 * Function_Descripton  : This function constructs the  *
 * DataToAuthenticator using Data Identifier, secured   *
 * part of the * Authentic I-PDU, and freshness value   *
 *******************************************************/
static Std_ReturnType constructDataToAuthenticatorTx(const PduIdType TxPduId, uint8 *DataToAuth, uint32 *DataToAuthLen, const PduInfoType* AuthPdu)
{
    Std_ReturnType result;
    *DataToAuthLen = 0;

    // Data Identifier
    memcpy(&DataToAuth[*DataToAuthLen], &TxPduId, sizeof(TxPduId));
    *DataToAuthLen += sizeof(TxPduId);

    // secured part of the Authentic I-PDU
    memcpy(&DataToAuth[*DataToAuthLen], AuthPdu->SduDataPtr, AuthPdu->SduLength);
    *DataToAuthLen += AuthPdu->SduLength;
    
    // Complete Freshness Value
    uint8 FreshnessVal[SECOC_FRESHNESS_MAX_LENGTH/8] = {0};

    uint32 FreshnesslenBits = SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueLength;

    result = SecOC_GetTxFreshness(SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueId, FreshnessVal, &FreshnesslenBits);

    if(result != E_OK)
    {
        return result;
    }

    uint32 FreshnesslenBytes = BIT_TO_BYTES(FreshnesslenBits);

    memcpy(&DataToAuth[*DataToAuthLen], FreshnessVal, FreshnesslenBytes);
    *DataToAuthLen += FreshnesslenBytes;

    return E_OK;
}

/*******************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name            : generateMAC               *
 * Function_Index           : SecOC internal            *
 * Parameter in             : TxPduId                   *
 * Parameter in             : DataToAuth                *
 * Parameter in             : DataToAuthLen             *
 * Parameter in/out         : authenticatorPtr          *
 * Parameter in/out         : authenticatorLen          *
 * Function_Descripton  : This function generates MAC   *
 * based on the DataToAuthenticator                     *
 *******************************************************/
static Std_ReturnType generateMAC(const PduIdType TxPduId, uint8 const *DataToAuth, const uint32 *DataToAuthLen, uint8  *authenticatorPtr, uint32  *authenticatorLen)
{
    Std_ReturnType result;

    result = Csm_MacGenerate(TxPduId, 0, DataToAuth, *DataToAuthLen, authenticatorPtr, authenticatorLen);

    return result;
}

/********************************************************
 *          * Function Info *                           *
 *                                                      *
 * Function_Name        : authenticate                  *
 * Function_Index       : SecOC internal                *
 * Parameter in         : TxPduId                       *
 * Parameter in         : AuthPdu                       *
 * Parameter out        : SecPdu                        * 
 * Function_Descripton  : This function generates the   *
 * secured PDU using authenticator, payload, freshness  * 
 *  value                                               *
 *******************************************************/
static Std_ReturnType authenticate(const PduIdType TxPduId, const PduInfoType* AuthPdu, PduInfoType* SecPdu)
{
    Std_ReturnType result;
    
    // DataToAuthenticator = Data Identifier | secured part of the Authentic I-PDU | Complete Freshness Value
    uint8 DataToAuth[SECOC_TX_DATA_TO_AUTHENTICATOR_LENGTH];
    uint32 DataToAuthLen = 0;
    result = constructDataToAuthenticatorTx(TxPduId, DataToAuth, &DataToAuthLen, AuthPdu);

    // Authenticator generation
    uint8  authenticatorPtr[SECOC_AUTHENTICATOR_MAX_LENGTH];
    uint32  authenticatorLen = BIT_TO_BYTES(SecOCTxPduProcessing[TxPduId].SecOCAuthInfoTruncLength);
    result = generateMAC(TxPduId, DataToAuth, &DataToAuthLen, authenticatorPtr, &authenticatorLen);
    

    // Truncated freshness value
    uint8 FreshnessVal[SECOC_FRESHNESS_MAX_LENGTH/8] = {0};
    uint32 FreshnesslenBits = SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueTruncLength;
    uint32 SecOCFreshnessValueLength = SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueLength;

    result = SecOC_GetTxFreshnessTruncData(SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueId, FreshnessVal, &SecOCFreshnessValueLength, FreshnessVal, &FreshnesslenBits);

    uint32 FreshnesslenBytes = BIT_TO_BYTES(SecOCTxPduProcessing[TxPduId].SecOCFreshnessValueTruncLength);

    PduLengthType SecPduLen = 0;
    // SECURED = HEADER(OPTIONAL) + AuthPdu + TruncatedFreshnessValue(OPTIONAL) + Authenticator

    // HEADER
    uint32 headerLen = SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCAuthPduHeaderLength;
    memcpy(&SecPdu->SduDataPtr[SecPduLen], &AuthPdu->SduLength, headerLen);
    SecPduLen += headerLen;
    

    // AuthPdu
    memcpy(&SecPdu->SduDataPtr[SecPduLen], AuthPdu->SduDataPtr, AuthPdu->SduLength);
    SecPduLen += AuthPdu->SduLength;

    // TruncatedFreshnessValue
    memcpy(&SecPdu->SduDataPtr[SecPduLen], FreshnessVal, FreshnesslenBytes);
    SecPduLen += FreshnesslenBytes;

    // Authenticator
    memcpy(&SecPdu->SduDataPtr[SecPduLen], authenticatorPtr, authenticatorLen);
    SecPduLen += authenticatorLen;


    SecPdu->SduLength = SecPduLen;
    SecPdu->MetaDataPtr = AuthPdu->MetaDataPtr;


    return result;
}


Std_ReturnType SecOC_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) 
{
    Std_ReturnType result = E_OK;

    PduInfoType *authpdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    
    memcpy(authpdu->SduDataPtr, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
    authpdu->MetaDataPtr = PduInfoPtr->MetaDataPtr;
    authpdu->SduLength = PduInfoPtr->SduLength;


    return result;
}


void SecOC_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) 
{
    PduInfoType *authPdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    PduInfoType *securedPdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);

    if (result == E_OK) {
        // clear buffer
        // authPdu->SduLength = 0;
        // securedPdu->SduLength = 0;
    }

    PduR_SecOCIfTxConfirmation(TxPduId, result);
}



Std_ReturnType SecOC_GetTxFreshness(uint16 SecOCFreshnessValueID, uint8* SecOCFreshnessValue,
uint32* SecOCFreshnessValueLength) {
    SecOC_GetTxFreshnessCalloutType PTR = (SecOC_GetTxFreshnessCalloutType)FVM_GetTxFreshness;
Std_ReturnType result = PTR(SecOCFreshnessValueID, SecOCFreshnessValue, SecOCFreshnessValueLength);
    return result;
}







void SecOC_Init(const SecOC_ConfigType *config)
{
    SecOCGeneral = config->General;
    SecOCTxPduProcessing = config->SecOCTxPduProcessings;
    SecOCRxPduProcessing = config->SecOCRxPduProcessings;

    uint8 idx;
    for (idx = 0 ; idx < SECOC_NUM_OF_TX_PDU_PROCESSING ; idx++) 
    {      
        FVM_IncreaseCounter(SecOCTxPduProcessing[idx].SecOCFreshnessValueId);
    }

    
}                   




void SecOCMainFunctionTx(void) {

    PduIdType idx;
    for (idx = 0 ; idx < SECOC_NUM_OF_TX_PDU_PROCESSING ; idx++) 
    {

        PduInfoType *authPdu = &(SecOCTxPduProcessing[idx].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
        PduInfoType *securedPdu = &(SecOCTxPduProcessing[idx].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);

        // check if there is data
        if (authPdu->SduLength > 0) 
        {
            authenticate(idx , authPdu , securedPdu);
            
            FVM_IncreaseCounter(SecOCTxPduProcessing[idx].SecOCFreshnessValueId);
            PduR_SecOCTransmit(idx , securedPdu);

        }
    }
}

void SecOCMainFunctionRx(void)
{
    PduIdType idx = 0;
    SecOC_VerificationResultType result ,macResult;

    
    for (idx = 0 ; idx < SECOC_NUM_OF_RX_PDU_PROCESSING; idx++) 
    {

        PduInfoType *authPdu = &(SecOCRxPduProcessing[idx].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef);
        PduInfoType *securedPdu = &(SecOCRxPduProcessing[idx].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);

        // check if there is data
        if ( securedPdu->SduLength > 0 ) {
            
            result = verify(idx, securedPdu, &macResult);
            if( result == SECOC_VERIFICATIONSUCCESS )
            {
                printf("Verify success for id: %d\n", idx);
                PduR_SecOCIfRxIndication(idx,  authPdu);
            }

        }
    }

}

void SecOC_TpTxConfirmation(PduIdType TxPduId,Std_ReturnType result)
{
    PduInfoType *authPdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    PduInfoType *securedPdu = &(SecOCTxPduProcessing[TxPduId].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);

    if (result == E_OK) {
        // clear buffer
        authPdu->SduLength = 0;
        securedPdu->SduLength = 0;
    }

    if (SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCPduType == SECOC_TPPDU)
    {
        PduR_SecOCTpTxConfirmation(TxPduId, result);
    }
    else if (SecOCTxPduProcessing[TxPduId].SecOCTxAuthenticPduLayer->SecOCPduType == SECOC_IFPDU)
    {
        PduR_SecOCIfTxConfirmation(TxPduId, result);
    }
    else
    {
        // DET Report Error
    }

}






void SecOC_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{   
    /* The SecOC copies the Authentic I-PDU to its own buffer */
    PduInfoType *securedPdu = &(SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);

    memcpy(securedPdu->SduDataPtr, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
    securedPdu->MetaDataPtr = PduInfoPtr->MetaDataPtr;
    securedPdu->SduLength = PduInfoPtr->SduLength;
    
}





Std_ReturnType SecOC_GetTxFreshnessTruncData (uint16 SecOCFreshnessValueID,uint8* SecOCFreshnessValue,
uint32* SecOCFreshnessValueLength,uint8* SecOCTruncatedFreshnessValue,uint32* SecOCTruncatedFreshnessValueLength) 
{
    Std_ReturnType result = FVM_GetTxFreshnessTruncData(SecOCFreshnessValueID, SecOCFreshnessValue , SecOCFreshnessValueLength,
    SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength);
    return result;
}


Std_ReturnType SecOC_GetRxFreshness(uint16 SecOCFreshnessValueID,const uint8* SecOCTruncatedFreshnessValue,uint32 SecOCTruncatedFreshnessValueLength,
    uint16 SecOCAuthVerifyAttempts,uint8* SecOCFreshnessValue,uint32* SecOCFreshnessValueLength)
{
    FVM_GetRxFreshness(SecOCFreshnessValueID,SecOCTruncatedFreshnessValue,SecOCTruncatedFreshnessValueLength,
    SecOCAuthVerifyAttempts,SecOCFreshnessValue,SecOCFreshnessValueLength);
}

static Std_ReturnType parseSecuredPdu(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_RxIntermediateType *SecOCIntermediate)
{
    uint8 SecCursor = 0; // Track the current byte of secured to be parsed

    // Get data length from configuration or header if found
    uint32 headerLen = SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
    SecOCIntermediate->authenticPduLen = 0;
    if(headerLen > 0)
    {
        // [SWS_SecOC_00259]
        memcpy(&SecOCIntermediate->authenticPduLen, &SecPdu->SduDataPtr[SecCursor], headerLen);
        SecCursor += headerLen;
    }
    else
    {
        // [SWS_SecOC_00257]
        SecOCIntermediate->authenticPduLen =  SecOCRxPduProcessing[RxPduId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef.SduLength;
    }

    // Copy authenticPdu to intermediate
    memcpy(SecOCIntermediate->authenticPdu, &SecPdu->SduDataPtr[SecCursor], SecOCIntermediate->authenticPduLen);
    SecCursor += SecOCIntermediate->authenticPduLen;

    uint16 authVeriAttempts = 0;
    // Get Rx freshness from FVM using the truncated freshness in SecPdu 
    const uint8* SecOCTruncatedFreshnessValue = &SecPdu->SduDataPtr[SecCursor];
    uint32 SecOCTruncatedFreshnessValueLength = SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueTruncLength;
    SecOCIntermediate->freshnessLenBits = SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueLength;

    SecOCIntermediate->freshnessResult = SecOC_GetRxFreshness(
            SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueId,
            SecOCTruncatedFreshnessValue, 
            SecOCTruncatedFreshnessValueLength, 
            authVeriAttempts,
            SecOCIntermediate->freshness, 
            &SecOCIntermediate->freshnessLenBits
    );

    SecCursor += BIT_TO_BYTES(SecOCTruncatedFreshnessValueLength);

    // Copy Mac to intermediate
    SecOCIntermediate->macLenBits = SecOCRxPduProcessing[RxPduId].SecOCAuthInfoTruncLength;

    memcpy(SecOCIntermediate->mac, &SecPdu->SduDataPtr[SecCursor], BIT_TO_BYTES(SecOCIntermediate->macLenBits));
    SecCursor += BIT_TO_BYTES(SecOCIntermediate->macLenBits);

    return E_OK;
}

static Std_ReturnType constructDataToAuthenticatorRx(PduIdType RxPduId, PduInfoType* secPdu, uint8 *DataToAuth, uint32 *DataToAuthLen, uint8 *TruncatedLength_Bytes,uint8* SecOCFreshnessValue,uint32* SecOCFreshnessValueLength )
{
    //*DataToAuthLen = 0;
	//copy the Id to buffer Data to Auth
    memcpy(&DataToAuth[*DataToAuthLen], &RxPduId, sizeof(RxPduId));
    *DataToAuthLen += sizeof(RxPduId);	

    // Get data length from configuration or header if found
    uint32 headerLen = SecOCRxPduProcessing[RxPduId].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCAuthPduHeaderLength;
    uint32 dataLen = 0;
    if(headerLen > 0)
    {
        // [SWS_SecOC_00259]
        memcpy(&dataLen, secPdu->SduDataPtr, headerLen);
    }
    else
    {
        // [SWS_SecOC_00257]
        dataLen =  SecOCRxPduProcessing[RxPduId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef.SduLength;
    }

    // copy the data to buffer Data to Auth
    memcpy(&DataToAuth[*DataToAuthLen], (secPdu->SduDataPtr+headerLen), dataLen);
    *DataToAuthLen += dataLen;

    const uint8* SecOCTruncatedFreshnessValue = (secPdu->SduDataPtr+headerLen+dataLen);
    uint32 SecOCTruncatedFreshnessValueLength = SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueTruncLength;
    *TruncatedLength_Bytes = BIT_TO_BYTES(SecOCTruncatedFreshnessValueLength);
    Std_ReturnType Freshness_result;
    uint16 authVeriAttempts = 0;
    // Std_ReturnType Freshness_result = E_OK;

    Freshness_result = SecOC_GetRxFreshness(SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueId,
    SecOCTruncatedFreshnessValue, SecOCTruncatedFreshnessValueLength, authVeriAttempts,
    SecOCFreshnessValue, SecOCFreshnessValueLength);
    // copy the freshness value to buffer Data to Auth
    memcpy(&DataToAuth[*DataToAuthLen], SecOCFreshnessValue, BIT_TO_BYTES(SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueLength));
    *DataToAuthLen += (BIT_TO_BYTES(SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueLength));

    return Freshness_result;
}

// header - auth_data - Freshness - MAC
static Std_ReturnType verify(PduIdType RxPduId, PduInfoType* SecPdu, SecOC_VerificationResultType *verification_result)
{

    SecOC_RxIntermediateType    SecOCIntermediate;
    parseSecuredPdu(RxPduId, SecPdu, &SecOCIntermediate);


    uint8 DataToAuth[SECOC_RX_DATA_TO_AUTHENTICATOR_LENGTH] = {0};  // CAN payload
    uint32 DataToAuthLen = 0;

    uint8 TruncatedLength_Bytes;
    uint8 SecOCFreshnessValue[SECOC_FRESHNESS_MAX_LENGTH / 8] = {0};
    uint32 SecOCFreshnessValueLength = SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueLength;
    constructDataToAuthenticatorRx(RxPduId, SecPdu, DataToAuth, &DataToAuthLen, &TruncatedLength_Bytes, SecOCFreshnessValue, &SecOCFreshnessValueLength);



    SecOC_VerificationResultType result;
    Crypto_VerifyResultType verify_var;

    if(SecOCIntermediate.freshnessResult == E_OK)
    {
        Std_ReturnType Mac_verify = Csm_MacVerify(SecOCRxPduProcessing[RxPduId].SecOCDataId, Crypto_stub, DataToAuth, DataToAuthLen, SecOCIntermediate.mac, SecOCIntermediate.macLenBits, &verify_var);
        if (Mac_verify == E_OK) 
        {
            memcpy((SecOCRxPduProcessing[RxPduId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef.SduDataPtr), SecOCIntermediate.authenticPdu, SecOCIntermediate.authenticPduLen);
            SecOCRxPduProcessing[RxPduId].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef.SduLength = SecOCIntermediate.authenticPduLen;
            FVM_UpdateCounter(SecOCRxPduProcessing[RxPduId].SecOCFreshnessValueId, SecOCFreshnessValue, SecOCFreshnessValueLength);
            *verification_result = CRYPTO_E_VER_OK;
            result = SECOC_VERIFICATIONSUCCESS;
        }
        else 
        {
            // drop message
            SecPdu->SduLength = 0;
            *verification_result = CRYPTO_E_VER_NOT_OK;
            result = SECOC_VERIFICATIONFAILURE;
        }
    }
    else
    {
        // drop message
        SecPdu->SduLength = 0;
        result = SECOC_FRESHNESSFAILURE;
    }
    return result;
}


extern SecOC_ConfigType SecOC_Config;
void SecOC_test()
{
    //FVM_IncreaseCounter(0, 100);
    // uint8_t adata[100] = "hello, world";
    // uint8_t adata[100] = {0, 1, 2, 3, 9, 5, 6, 7, 8, 9, 10, 11, 12};
    // uint8_t sdata[100];
    // PduInfoType AuthPdu, SecPdu;
    // uint8 x = 0;
    // AuthPdu.SduDataPtr = adata;
    // AuthPdu.SduLength = SECOC_AUTHPDU_MAX_LENGTH;
    // AuthPdu.MetaDataPtr = &x;
    uint16 SecOCFreshnessValueID = 10;

    uint8 SecOCFreshnessValue[8]={0};
    uint32 SecOCFreshnessValueLength = 8 * 8;
    
   for(int i = 0; i < 0x32C; i++)
       FVM_IncreaseCounter(10);
    for(int i = 0; i < 0x32C; i++)
       FVM_IncreaseCounter(9);
    
	uint8 buff[16]={10,100,200,250};
    PduLengthType len = 4;
    PduInfoType SPDU;
    uint8 meta = 0;
    SPDU.MetaDataPtr = &meta;
    SPDU.SduDataPtr = buff;
    SPDU.SduLength = len;

    SecOC_Init(&SecOC_Config);
    SecOC_IfTransmit(0, &SPDU);
    
    printf("Data transmit :\n");
    
    for(int i = 0; i < SPDU.SduLength; i++)
        printf("%d ", SPDU.SduDataPtr[i]);
    printf("\n");
 
    PduInfoType *secured_TX = &(SecOCTxPduProcessing[0].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);
    PduInfoType *auth = &(SecOCTxPduProcessing[0].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
    PduInfoType *auth_RX = &(SecOCRxPduProcessing[0].SecOCRxAuthenticPduLayer->SecOCRxAuthenticLayerPduRef);

    SecOCMainFunctionTx();
    
    printf("data after authen: \n");
    for(int i = 0; i < secured_TX->SduLength; i++)
        printf("%d ", secured_TX->SduDataPtr[i]);
    printf("\ndone Tx\n");   

    
    PduR_CanIfRxIndication((uint16)0,secured_TX);

    PduInfoType *secured = &(SecOCRxPduProcessing[0].SecOCRxSecuredPduLayer->SecOCRxSecuredPdu->SecOCRxSecuredLayerPduRef);
    
    printf("in RX data received : \n");
    for(int i = 0; i < secured->SduLength; i++)
        printf("%d ", secured->SduDataPtr[i]);
    printf("\n");  
    // main function
    SecOCMainFunctionRx();
    
    printf("after verification :\n");  
    for(int i = 0; i < auth_RX->SduLength; i++)
        printf("%d ", auth_RX->SduDataPtr[i]);
    printf("\n");
    
    SecOCMainFunctionRx();
    
    printf("after verification :\n");  
    for(int i = 0; i < auth_RX->SduLength; i++)
        printf("%d ", auth_RX->SduDataPtr[i]);
    printf("\n");
    
    // printf("after verification :\n");  
    // for(int i = 0; i < auth_RX->SduLength; i++)
    //     printf("%d ", auth_RX->SduDataPtr[i]);
    // printf("\n");
//      //FVM_IncreaseCounter(0, 100);
//    // uint8_t adata[100] = "hello, world";
//     // uint8_t adata[100] = {0, 1, 2, 3, 9, 5, 6, 7, 8, 9, 10, 11, 12};
//     // uint8_t sdata[100];
//     // PduInfoType AuthPdu, SecPdu;
//     // uint8 x = 0;
//     // AuthPdu.SduDataPtr = adata;
//     // AuthPdu.SduLength = SECOC_AUTHPDU_MAX_LENGTH;
//     // AuthPdu.MetaDataPtr = &x;
//     uint16 SecOCFreshnessValueID = 10;

//     uint8 SecOCFreshnessValue[8]={0};
//     uint32 SecOCFreshnessValueLength = 8 * 8;
    
//    for(int i = 0; i < 0x12C; i++)
//        FVM_IncreaseCounter(SecOCFreshnessValueID, &SecOCFreshnessValueLength);
//     for(int i = 0; i < 0x12C; i++)
//        FVM_IncreaseCounter(9, &SecOCFreshnessValueLength);
		
// 	uint8 buff[16]={1,1,1,1, 9};
//     PduLengthType len = 5;
//     PduInfoType SPDU;
//     SPDU.SduDataPtr = buff;
//     SPDU.SduLength = len;
//      //   FVM_IncreaseCounter(9, NULL);

//     uint8 meta = 0;
//     SPDU.MetaDataPtr = &meta;

//     SecOC_Init(&SecOC_Config);
//     SecOC_IfTransmit(0, &SPDU);
//     // Changing parameters: Authentic I-PDU(constant for now), Freshness Value, Authenticator
//     //SecOC_TxPduProcessing[0].SecOCFreshnessValueTruncLength = 16;
//     //SecOC_TxPduProcessing[0].SecOCAuthInfoTruncLength = 32; // BITS

//     for(int i = 0; i < SPDU.SduLength; i++)
//         printf("%d ", SPDU.SduDataPtr[i]);
//     printf("\n");

//     // main function
    
//     PduInfoType *secured = &(SecOCTxPduProcessing[0].SecOCTxSecuredPduLayer->SecOCTxSecuredPdu->SecOCTxSecuredLayerPduRef);
//     PduInfoType *auth = &(SecOCTxPduProcessing[0].SecOCTxAuthenticPduLayer->SecOCTxAuthenticLayerPduRef);
//     //authenticate(0, auth, secured);
//     //FVM_IncreaseCounter(SecOCTxPduProcessing[0].SecOCFreshnessValueId, &ayhaga);
//     SecOCMainFunctionTx(); // data 44, freshness 45
//     for(int i = 0; i < secured->SduLength; i++)
//         printf("%d ", secured->SduDataPtr[i]);
//     printf("\n");
//     printf("done transmission\n"); 

//     SecOC_RxIndication(0, secured);
//     SecOCMainFunctionRx(); // freshness 44

//     // SecOCMainFunctionTx();
//     for(int i = 0; i < secured->SduLength; i++)
//         printf("%d ", secured->SduDataPtr[i]);
//       printf("\n");
//     printf("done transmission\n"); 


//     SecOC_RxIndication(0, secured);
//     SecOCMainFunctionRx();

//     SecOC_RxIndication(0, secured);
//     // SecOCMainFunctionRx();
}