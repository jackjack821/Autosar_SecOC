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



PduInfoType SecOC_Buffer[SECOC_BUFFERLENGTH] = { { NULL , 0} };



Std_ReturnType SecOC_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    Std_ReturnType result = E_OK;
    if (TxPduId < SECOC_BUFFERLENGTH) {
        SecOC_Buffer[TxPduId] = *PduInfoPtr;
    } else {
        result = E_NOT_OK;
    }

    return result;
}
// PduIdType SecOC_Buffer[SECOC_BUFFERLENGTH];  Hossam declared it


void SecOC_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    if (result == E_OK) {
        // SecOC_Buffer[TxPduId] = NULL;
        // clear buffer
    }
    PduR_SecOCIfTxConfirmation(TxPduId, result);
}



Std_ReturnType SecOC_GetTxFreshness(uint16 SecOCFreshnessValueID, uint8* SecOCFreshnessValue,
uint32* SecOCFreshnessValueLength) {
    SecOC_GetTxFreshnessCallbackType PTR = (SecOC_GetTxFreshnessCallbackType)FVM_GetTxFreshness;
Std_ReturnType result = PTR(SecOCFreshnessValueID, SecOCFreshnessValue, SecOCFreshnessValueLength);
    return result;
}





SecOC_StateType SecOc_State = SECOC_UNINIT;

const SecOC_GeneralType* tmpSecOCGeneral;
const SecOC_RxPduProcessingType* tmpSecOCRxPduProcessing;
const SecOC_TxPduProcessingType* tmpSecOCTxPduProcessing;

static Std_VersionInfoType _SecOC_VersionInfo ={(uint16)1,(uint16) 1,(uint8)1,(uint8)0,(uint8)0};

#if (SECOC_ERROR_DETECT == STD_ON)
#define VALIDATE_STATE_INIT()\
	if(SECOC_INIT!=SecOc_State){\
		Det_ReportError(); \
		return; \
	}

#else
#define VALIDATE_STATE_INIT(_api)
#endif



uint8 success=1;
void SecOC_Init(const SecOC_ConfigType *config)
{
    tmpSecOCGeneral = config->general;
    tmpSecOCTxPduProcessing = config->secOCTxPduProcessings;
    tmpSecOCRxPduProcessing = config->secOCRxPduProcessings;
    if(!success)
    {
        SecOc_State = SECOC_E_UNINIT;        
    }                                   
    SecOc_State = SECOC_INIT;
}                   


void SecOC_GetVersionInfo (Std_VersionInfoType* versioninfo)
{

    VALIDATE_STATE_INIT();
    memcpy(versioninfo, &_SecOC_VersionInfo, sizeof(Std_VersionInfoType));//copy from source to distination with the length
}


extern void SecOC_MainFunctionTx(void) {
    /*
    // check if initialized or not;
    if (SecOc_State == SECOC_UNINIT) {
        // cppcheck-suppress misra-c2012-15.5
        return;
    }
    PduIdType idx = 0;
    PduInfoType transmitPduInfo;
    for ( ; idx < SECOC_BUFFERLENGTH ; idx++) {
        // check if there is data
        if ( SecOC_Buffer[idx].SduLength > 0 ) {
            // authenticate SecOC_Buffer[idx];
            // send authenticated data
            // authenticate( SecOC_Buffer[idx] , &transmitPduInfo)
            PduR_SecOCTransmit(idx , &transmitPduInfo);

        } else {
        }
    }
    */

    uint8 FreshnessValue[100];
    uint32 length[100] = {0};
    /* here we put in the first byte of freshness 240 */
    FreshnessValue[99] = 240;
    printf("%d\n", FreshnessValue[99]);
    // /* Test case 1 */
    // /* get the value in the counter */
    // SecOC_GetTxFreshness(0, FreshnessValue, length);
    // printf("%d\n", FreshnessValue[99]);
    // /* Get the value from Freashness counter */
    // FVM_IncreaseCounter(0, length);
    // /* Get the value from Freashness counter */
    // SecOC_GetTxFreshness(0, FreshnessValue, length);
    // printf("%d\n", FreshnessValue[99]);
    // /* Output : 240 \n 0 \n 1*/
    

    /* Test case 2 */
    /* go to the counter and change the array from 0 to 255 */
    SecOC_GetTxFreshness(0, FreshnessValue, length);
    printf("%d\t %d \n", FreshnessValue[99],FreshnessValue[98]);
    FVM_IncreaseCounter(0, length);
    SecOC_GetTxFreshness(0, FreshnessValue, length);
    printf("%d\t %d \n", FreshnessValue[99],FreshnessValue[98]);
    /* Output : 255 \t 0\n 0 \t 1*/

}




// #if (SECOC_USE_TX_CONFIRMATION == 1)
//     void SecOc_SPduTxConfirmation(uint16 SecOCFreshnessValueID) {
//         /* Specific User's Code need to be written here*/
//     }
// #endif


#define MAX_COUNTER_FRESHNESS_IDS   10

Std_ReturnType SecOC_GetTxFreshnessTruncData (uint16 SecOCFreshnessValueID,uint8* SecOCFreshnessValue,
uint32* SecOCFreshnessValueLength,uint8* SecOCTruncatedFreshnessValue,uint32* SecOCTruncatedFreshnessValueLength) 
{
    Std_ReturnType result = E_OK; 
    if (SecOCFreshnessValueID > (MAX_COUNTER_FRESHNESS_IDS-1)) 
    {
        result = E_NOT_OK;
    }
    else if (SecOCTruncatedFreshnessValueLength > SECOC_MAX_FRESHNESS_SIZE) 
    {
        result = E_NOT_OK;
    }
    SecOC_FreshnessArrayType counter[MAX_COUNTER_FRESHNESS_IDS] = {0};
    uint32 Datalength = SECOC_MAX_FRESHNESS_SIZE - (*SecOCTruncatedFreshnessValueLength);
    
    int DataIndex = (SECOC_MAX_FRESHNESS_SIZE - 1); 
    uint8 big_End_index =0;
    for ( ; ((DataIndex >= Datalength) && (big_End_index < (*SecOCTruncatedFreshnessValueLength))); DataIndex-- , big_End_index++) 
    {
        SecOCTruncatedFreshnessValue[big_End_index] = counter[SecOCFreshnessValueID][DataIndex];
    }
    return result;
}

