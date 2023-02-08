#include "FVM.h"

/* This is Stub for Freshness Value Manger that have the global counter */

/* Freshness Counter */
static SecOC_FreshnessArrayType Freshness_Counter[ID_MAX] = {0};
static uint32 Freshness_Counter_length[ID_MAX] = {0};

/* Shitf Right by 1 to divide by 2
        untill there is no number
        Can be replaced by --> uint8 len = ceil(log2(counterTemp.counter))*/
uint8 countBits(uint8 n)
{
    uint8 count = 0;
    uint8 n_ = n;
    while (n_ > 0)
    {
        count++;
        n_ >>= 1;
    }
    return count;
}


Std_ReturnType FVM_IncreaseCounter(uint16 SecOCFreshnessValueID, uint32 *SecOCFreshnessValueLength)
{

    /* increase the counter by 1 */
    uint8 INDEX = 0;
    for (; INDEX < SECOC_MAX_FRESHNESS_SIZE; INDEX++)
    {
        Freshness_Counter[SecOCFreshnessValueID][INDEX]++;
        if (Freshness_Counter[SecOCFreshnessValueID][INDEX] != 0)
        {
            break;
        }
    }

    /* Calculate the Number of bits in the Counter */
    for (INDEX = SECOC_MAX_FRESHNESS_SIZE - 1; INDEX >= 0; INDEX--)
    {
        if (Freshness_Counter[SecOCFreshnessValueID][INDEX] != 0)
        {
            Freshness_Counter_length[SecOCFreshnessValueID] = countBits(Freshness_Counter[SecOCFreshnessValueID][INDEX]) + (INDEX * 8);
            break;
        }
    }

    *SecOCFreshnessValueLength = Freshness_Counter_length[SecOCFreshnessValueID];

    return E_OK;
}

Std_ReturnType FVM_GetTxFreshness(uint16 SecOCFreshnessValueID, uint8 *SecOCFreshnessValue,
                                  uint32 *SecOCFreshnessValueLength)
{

    Std_ReturnType result = E_OK;
    uint32 FreshnessValueLengthBytes = (BIT_TO_BYTES(*SecOCFreshnessValueLength));
    if (SecOCFreshnessValueID > ID_MAX)
    {
        result = E_NOT_OK;
    }
    else if (FreshnessValueLengthBytes > SECOC_MAX_FRESHNESS_SIZE)
    {
        result = E_NOT_OK;
    }
    else
    {

        /*  Acctual Freshness Value length --> for the minimu length of buffer
            Freshnes index and counter Index --> index to copy the value from counter to pointer
            last index t
         */
        uint32 AcctualFreshnessVallength = (FreshnessValueLengthBytes <= Freshness_Counter_length[SecOCFreshnessValueID]) ? (FreshnessValueLengthBytes) : (Freshness_Counter_length[SecOCFreshnessValueID]);
        uint32 FreshnessIndex = FreshnessValueLengthBytes - 1;
        uint8 FreshnessCounterIndex;
        for (FreshnessCounterIndex = 0; (FreshnessCounterIndex < AcctualFreshnessVallength); FreshnessCounterIndex++)
        {
            SecOCFreshnessValue[FreshnessIndex] = Freshness_Counter[SecOCFreshnessValueID][FreshnessCounterIndex];
            FreshnessIndex--;
        }
        /* Update Length */
        *SecOCFreshnessValueLength = AcctualFreshnessVallength;
    }
    return result;
}

Std_ReturnType FVM_GetRxFreshness(uint16 SecOCFreshnessValueID, const uint8 *SecOCTruncatedFreshnessValue, uint32 SecOCTruncatedFreshnessValueLength,
                                  uint16 SecOCAuthVerifyAttempts, uint8 *SecOCFreshnessValue, uint32 *SecOCFreshnessValueLength)
{
    Std_ReturnType result = E_OK;
    uint32 FreshnessValueLengthBytes = (BIT_TO_BYTES(*SecOCFreshnessValueLength));
    if (SecOCFreshnessValueID > ID_MAX)
    {
        result = E_NOT_OK;
    }
    else if (FreshnessValueLengthBytes > SECOC_MAX_FRESHNESS_SIZE)
    {
        result = E_NOT_OK;
    }
    else
    {

        /* The FVM module shall construct Freshness Verify Value (i.e. the Freshness Value to be used for Verification) and
         provide it to SecOC */
        uint32 FreshnessVallengthBytes = BIT_TO_BYTES(Freshness_Counter_length[SecOCFreshnessValueID]);
        uint32 truncedFreshnessLengthBytes = BIT_TO_BYTES(SecOCTruncatedFreshnessValueLength);
        uint32 maxTruncedIndex = truncedFreshnessLengthBytes - 1;
        uint32 freshnessIndex = FreshnessVallengthBytes - 1;
        uint32 counterIndex;


        if (FreshnessVallengthBytes == truncedFreshnessLengthBytes)
        {
            memcpy(SecOCFreshnessValue, SecOCTruncatedFreshnessValue, truncedFreshnessLengthBytes);
        }
        else
        {
            SecOC_FreshnessArrayType currentFreshnessVal = {0};
            SecOCAuthVerifyAttempts = 0;
            /* Convert the counter to Big india to Compare */
            for(counterIndex = 0; counterIndex < FreshnessVallengthBytes; counterIndex++)
            {
                currentFreshnessVal[freshnessIndex] = Freshness_Counter[SecOCFreshnessValueID][counterIndex];
                freshnessIndex--;
            }

            /* construction of Freshness Value */
            for(counterIndex = 0; counterIndex < maxTruncedIndex; counterIndex++)
            {
                if (SecOCTruncatedFreshnessValue[counterIndex] == currentFreshnessVal[counterIndex] && SecOCTruncatedFreshnessValue[counterIndex] == 0)
                {
                    continue;
                }
                if (SecOCTruncatedFreshnessValue[counterIndex] > currentFreshnessVal[counterIndex])
                {
                    /* most significant bits of FreshnessValue corresponding to FreshnessValueID |
                    FreshnessValue parsed from Secured I-PDU */
                    for(counterIndex = 0; counterIndex < maxTruncedIndex; counterIndex++)
                    {
                        currentFreshnessVal[counterIndex] = SecOCTruncatedFreshnessValue[counterIndex];
                    }
                    uint8 remainingBitsTrunc = 8 - ((truncedFreshnessLengthBytes * 8) - SecOCTruncatedFreshnessValueLength);
                    currentFreshnessVal[maxTruncedIndex] = (SecOCTruncatedFreshnessValue[maxTruncedIndex] & (~(0xFF << remainingBitsTrunc))) | (currentFreshnessVal[maxTruncedIndex] & (0xFF << remainingBitsTrunc));
                }
                else
                {
                    /*  most significant bits of (FreshnessValue corresponding to SecOCFreshnessValueID + 1) |
                    FreshnessValue parsed from payload */
                    FVM_IncreaseCounter(SecOCFreshnessValueID, SecOCFreshnessValueLength);
                    FreshnessVallengthBytes = (BIT_TO_BYTES(Freshness_Counter_length[SecOCFreshnessValueID]));
                    /* Convert the counter to Big india to Compare */
                    freshnessIndex = FreshnessVallengthBytes - 1;
                    for(counterIndex = 0; counterIndex < FreshnessVallengthBytes; counterIndex++)
                    {
                        currentFreshnessVal[freshnessIndex] = Freshness_Counter[SecOCFreshnessValueID][counterIndex];
                        freshnessIndex--;
                    }
                    for(counterIndex = 0; counterIndex < maxTruncedIndex; counterIndex++)
                    {
                        currentFreshnessVal[counterIndex] = SecOCTruncatedFreshnessValue[counterIndex];
                    }
                    uint8 remainingBitsTrunc = 8 - ((truncedFreshnessLengthBytes * 8) - SecOCTruncatedFreshnessValueLength);
                    currentFreshnessVal[maxTruncedIndex] = (SecOCTruncatedFreshnessValue[maxTruncedIndex] & (~(0xFF << remainingBitsTrunc))) | (currentFreshnessVal[maxTruncedIndex] & (0xFF << remainingBitsTrunc));
                }
                
                memcpy(SecOCFreshnessValue, currentFreshnessVal, FreshnessVallengthBytes);
                break;
            }
        }
        *SecOCFreshnessValueLength = Freshness_Counter_length[SecOCFreshnessValueID];
        /* verified that the constructed FreshnessVerifyValue is larger than the last stored notion of the Freshness Value */
        /* If it is not larger than the last stored notion of the Freshness Value,
         the FVM shall stop the verification and drop the Secured I-PDU */
        freshnessIndex = 0;
        for(counterIndex = FreshnessVallengthBytes - 1; counterIndex >= 0; counterIndex--)
        {
            if (Freshness_Counter[SecOCFreshnessValueID][counterIndex] == SecOCFreshnessValue[freshnessIndex])
            {
               continue;
            }
            else if (Freshness_Counter[SecOCFreshnessValueID][counterIndex] < SecOCFreshnessValue[freshnessIndex])
            {
                result = E_OK;
                break;
            }
            else
            {
                result = E_NOT_OK;
                break;
            }
            freshnessIndex++;
        }
    }   

     

    return result;
}