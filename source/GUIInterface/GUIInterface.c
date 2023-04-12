#include "SecOC.h"
#
#include "SecOC_Lcfg.h"
#include <stdio.h>

extern SecOC_ConfigType SecOC_Config;

const SecOC_TxPduProcessingType     *SecOCTxPduProcessing;
const SecOC_RxPduProcessingType     *SecOCRxPduProcessing;
const SecOC_GeneralType             *SecOCGeneral;

static char* errorString(Std_ReturnType error)
{
    switch(error)
    {
        case E_OK:
            return "E_OK";
        break;
        case E_NOT_OK:
            return "E_NOT_OK";
        break;
        case E_BUSY:
            return "E_BUSY";
        break;
        case QUEUE_FULL:
            return "QUEUE_FULL";
        break;
    }
}

void GUIInterface_init()
{
    SecOC_Init(&SecOC_Config);
}
