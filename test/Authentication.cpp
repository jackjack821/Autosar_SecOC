#include <gtest/gtest.h>

extern "C" {    

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
extern SecOC_ConfigType SecOC_Config;
extern const SecOC_TxPduProcessingType     *SecOCTxPduProcessing;
extern const SecOC_RxPduProcessingType     *SecOCRxPduProcessing;
extern const SecOC_GeneralType             *SecOCGeneral;
extern Std_ReturnType authenticate(const PduIdType TxPduId, PduInfoType* AuthPdu, PduInfoType* SecPdu);

}



/* This Function Use to made authenticate 
        1- Get Freshness
        2- Construct Data to authenticate
        3- generate the MAC
        4- Construct the Scure PDU to broadCast
    --- to check its working need to see the Secure PDU is changing by length and add a data FV and MAC
*/
