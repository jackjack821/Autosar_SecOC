#include "FVM.h"
#include <string.h>
/* This is Stub for Freshness Value Manger that have the global counter */



/* Freshness Counter */
static SecOC_FreshnessArrayType Freshness_Counter[SecOC_FreshnessValue_ID_MAX] = {0};
static uint32 Freshness_Counter_length [SecOC_FreshnessValue_ID_MAX] = {0};

/* Shitf Right by 1 to divide by 2
        untill there is no number
        Can be replaced by --> uint8 len = ceil(log2(counterTemp.counter))*/
uint8 countBits(uint8 n) {
    uint8 count = 0;
    uint8 n_ = n;
    while (n_ > 0)
    {
        count++;
        n_ >>= 1;
    }
    return count;
}


Std_ReturnType FVM_IncreaseCounter(uint16 SecOCFreshnessValueID, uint32* SecOCFreshnessValueLength) {  

    /* increase the counter by 1 */
    uint8 INDEX = 0;
    uint8 maxIndex = BIT_TO_BYTES(SECOC_MAX_FRESHNESS_SIZE);
    for (; INDEX < maxIndex; INDEX ++)
    {
        Freshness_Counter[SecOCFreshnessValueID][INDEX] ++;
        if(Freshness_Counter[SecOCFreshnessValueID][INDEX] != 0)
        {
            break;
        }
    }
    
     /* Calculate the Number of bits in the Counter */
    for (INDEX = maxIndex - 1; INDEX >= 0; INDEX--) {
        if(Freshness_Counter[SecOCFreshnessValueID][INDEX] != 0)
        {
            Freshness_Counter_length[SecOCFreshnessValueID] = countBits(Freshness_Counter[SecOCFreshnessValueID][INDEX]) + (INDEX * 8);
            break;
        }
    }
    if(SecOCFreshnessValueLength != NULL)
    {
        *SecOCFreshnessValueLength = Freshness_Counter_length[SecOCFreshnessValueID];
    }

    return E_OK;
}

/* Set the Counter Value to new Value */
Std_ReturnType FVM_UpdateCounter(uint16 SecOCFreshnessValueID, uint8* SecOCFreshnessValue,
uint32 SecOCFreshnessValueLength)
{
    uint32 freshnessCounterIndex; 
    uint32 SecOCFreshnessValueLengthBytes = BIT_TO_BYTES(SecOCFreshnessValueLength);
    for(freshnessCounterIndex = 0; freshnessCounterIndex < SecOCFreshnessValueLengthBytes;freshnessCounterIndex++)
    {
        Freshness_Counter[SecOCFreshnessValueID][freshnessCounterIndex] = SecOCFreshnessValue[freshnessCounterIndex];
    }
    Freshness_Counter_length[SecOCFreshnessValueID] = SecOCFreshnessValueLength;
}


Std_ReturnType FVM_GetTxFreshness(uint16 SecOCFreshnessValueID, uint8* SecOCFreshnessValue,
uint32* SecOCFreshnessValueLength) {

    Std_ReturnType result = E_OK;
    
    if (SecOCFreshnessValueID > SecOC_FreshnessValue_ID_MAX) {
        result =  E_NOT_OK;
    } else if ( *SecOCFreshnessValueLength > SECOC_MAX_FRESHNESS_SIZE ) {
        result = E_NOT_OK;
    } else {
        
        /*  Acctual Freshness Value length --> for the minimu length of buffer
            Freshnes index and counter Index --> index to copy the value from counter to pointer
            last index t
         */
        uint32 acctualFreshnessVallength = (*SecOCFreshnessValueLength <= Freshness_Counter_length[SecOCFreshnessValueID]) ? (*SecOCFreshnessValueLength ) :  (Freshness_Counter_length[SecOCFreshnessValueID]);
        uint32 acctualFreshnessVallengthBytes = BIT_TO_BYTES(acctualFreshnessVallength);
        uint32 freshnessCounterIndex; 
        for (freshnessCounterIndex = 0; freshnessCounterIndex < acctualFreshnessVallengthBytes;freshnessCounterIndex++) 
        {
            SecOCFreshnessValue[freshnessCounterIndex] = Freshness_Counter[SecOCFreshnessValueID][freshnessCounterIndex];
        }
        /* Update Length */
        *SecOCFreshnessValueLength = acctualFreshnessVallength;
        
    }
    return result;
}

Std_ReturnType FVM_GetRxFreshness(uint16 SecOCFreshnessValueID, const uint8 *SecOCTruncatedFreshnessValue, uint32 SecOCTruncatedFreshnessValueLength,
                                  uint16 SecOCAuthVerifyAttempts, uint8 *SecOCFreshnessValue, uint32 *SecOCFreshnessValueLength)
{
    Std_ReturnType result = E_OK;
    if (SecOCFreshnessValueID > SecOC_FreshnessValue_ID_MAX)
    {
        result = E_NOT_OK;
    }
    else if (*SecOCFreshnessValueLength > SECOC_MAX_FRESHNESS_SIZE)
    {
        result = E_NOT_OK;
    }
    else
    {
        /* The FVM module shall construct Freshness Verify Value (i.e. the Freshness Value to be used for Verification) and
         provide it to SecOC */
        uint32 freshnessVallengthBytes = BIT_TO_BYTES(Freshness_Counter_length[SecOCFreshnessValueID]);
        uint32 truncedFreshnessLengthBytes = BIT_TO_BYTES(SecOCTruncatedFreshnessValueLength);
        uint32 maxTruncedIndex = (truncedFreshnessLengthBytes > 0) ? (truncedFreshnessLengthBytes - 1) : (0);
        uint32 counterIndex;


        if (Freshness_Counter_length[SecOCFreshnessValueID] == SecOCTruncatedFreshnessValueLength)
        {
            (void)memcpy(SecOCFreshnessValue, SecOCTruncatedFreshnessValue, truncedFreshnessLengthBytes);
            *SecOCFreshnessValueLength = Freshness_Counter_length[SecOCFreshnessValueID];


        }
        else
        {
            SecOCAuthVerifyAttempts = 0;
            /* Put the Current Freshness in the FreshnessValue */
            (void)memcpy(SecOCFreshnessValue, Freshness_Counter[SecOCFreshnessValueID], freshnessVallengthBytes);
            /* construction of Freshness Value */
            if(memcmp(SecOCTruncatedFreshnessValue, Freshness_Counter[SecOCFreshnessValueID], maxTruncedIndex) > 0)
            {
                /* most significant bits of FreshnessValue corresponding to FreshnessValueID |
                FreshnessValue parsed from Secured I-PDU */
                for(counterIndex = 0; counterIndex < maxTruncedIndex; counterIndex++)
                {
                    SecOCFreshnessValue[counterIndex] = SecOCTruncatedFreshnessValue[counterIndex];
                }
                uint8 remainingBitsTrunc = 8 - ((truncedFreshnessLengthBytes * 8) - SecOCTruncatedFreshnessValueLength);
                SecOCFreshnessValue[maxTruncedIndex] = (SecOCTruncatedFreshnessValue[maxTruncedIndex] & (~(0xFF << remainingBitsTrunc))) | (Freshness_Counter[SecOCFreshnessValueID][maxTruncedIndex] & (0xFF << remainingBitsTrunc));
            }
            else
            {
                /*  most significant bits of (FreshnessValue corresponding to SecOCFreshnessValueID + 1) |
                FreshnessValue parsed from payload */
                
                for(counterIndex = 0; counterIndex < maxTruncedIndex; counterIndex++)
                {
                    SecOCFreshnessValue[counterIndex] = SecOCTruncatedFreshnessValue[counterIndex];
                }
                uint8 remainingBitsTrunc = 8 - ((truncedFreshnessLengthBytes * 8) - SecOCTruncatedFreshnessValueLength);
                if(remainingBitsTrunc == 0 || SecOCTruncatedFreshnessValueLength == 0)
                {
                    SecOCFreshnessValue[maxTruncedIndex] = Freshness_Counter[SecOCFreshnessValueID][maxTruncedIndex] + 1;
                }
                else if(remainingBitsTrunc == 8)
                {
                    SecOCFreshnessValue[maxTruncedIndex] = SecOCTruncatedFreshnessValue[maxTruncedIndex];
                    SecOCFreshnessValue[maxTruncedIndex + 1] ++;
                }
                else
                {
                    uint8 MSBsCounter = (Freshness_Counter[SecOCFreshnessValueID][maxTruncedIndex] >> remainingBitsTrunc) + 1;
                    MSBsCounter = MSBsCounter << remainingBitsTrunc;
                    SecOCFreshnessValue[maxTruncedIndex] = (SecOCTruncatedFreshnessValue[maxTruncedIndex] & (~(0xFF << remainingBitsTrunc))) | (MSBsCounter);
                }
            }
        }
        //*SecOCFreshnessValueLength = Freshness_Counter_length[SecOCFreshnessValueID];
        //*SecOCFreshnessValueLength = countBits()
        sint8 INDEX = 0;
        uint8 maxIndex = BIT_TO_BYTES(32);
        for (INDEX = maxIndex - 1; INDEX >= 0; INDEX--) {
        if(Freshness_Counter[SecOCFreshnessValueID][INDEX] != 0)
        {
            *SecOCFreshnessValueLength = countBits(SecOCFreshnessValueLength[INDEX]) + (INDEX * 8);
            break;
        }
    }
        /* verified that the constructed FreshnessVerifyValue is larger than the last stored notion of the Freshness Value */
        /* If it is not larger than the last stored notion of the Freshness Value,
         the FVM shall stop the verification and drop the Secured I-PDU */
        if (memcmp(Freshness_Counter[SecOCFreshnessValueID], SecOCFreshnessValue , *SecOCFreshnessValueLength) < 0)
        {
             result = E_OK;
        }
        else
        {
             result= E_NOT_OK;
        }

    }   
    return result;
}

Std_ReturnType FVM_GetTxFreshnessTruncData(uint16 SecOCFreshnessValueID, uint8* SecOCFreshnessValue,
uint32* SecOCFreshnessValueLength, uint8* SecOCTruncatedFreshnessValue, uint32* SecOCTruncatedFreshnessValueLength)
{
    Std_ReturnType result = E_OK; 
    if (SecOCFreshnessValueID > SecOC_FreshnessValue_ID_MAX) 
    {
        result = E_NOT_OK;
    }
    else if (*SecOCTruncatedFreshnessValueLength > SECOC_MAX_FRESHNESS_SIZE) 
    {
        result = E_NOT_OK;
    }
    else
    {
        uint32 acctualFreshnessVallength = (*SecOCFreshnessValueLength <= Freshness_Counter_length[SecOCFreshnessValueID]) ? (*SecOCFreshnessValueLength) :  (Freshness_Counter_length[SecOCFreshnessValueID]);
        uint32 acctualFreshnessVallengthBytes = BIT_TO_BYTES(acctualFreshnessVallength);
        uint32 freshnessCounterIndex;
        uint32 truncFreshnessVallengthBytes = BIT_TO_BYTES(*SecOCTruncatedFreshnessValueLength);
        uint32 acctualFreshnessTruncVallength = (truncFreshnessVallengthBytes <= acctualFreshnessVallengthBytes) ? (truncFreshnessVallengthBytes ) :  (acctualFreshnessVallengthBytes);
        /* get freshness from counter and its length */
        for(freshnessCounterIndex = 0; freshnessCounterIndex < acctualFreshnessVallengthBytes; freshnessCounterIndex++)
        {
            SecOCFreshnessValue[freshnessCounterIndex] = Freshness_Counter[SecOCFreshnessValueID][freshnessCounterIndex];
        }
        *SecOCFreshnessValueLength = acctualFreshnessVallength;
        /* Trunc the LSBs from freshness and store in the Freshness and update it length*/
        if(acctualFreshnessTruncVallength > 0)
        {
            (void)memcpy(SecOCTruncatedFreshnessValue, SecOCFreshnessValue, acctualFreshnessTruncVallength - 1);
            uint8 bitTrunc = 8 - ((acctualFreshnessTruncVallength * 8) - *SecOCTruncatedFreshnessValueLength);
            SecOCTruncatedFreshnessValue[acctualFreshnessTruncVallength - 1] = (SecOCFreshnessValue[acctualFreshnessTruncVallength - 1] & (~(0xFF << bitTrunc)));
        }
        //*SecOCTruncatedFreshnessValueLength = acctualFreshnessTruncVallength;
    }
    return result;
}

