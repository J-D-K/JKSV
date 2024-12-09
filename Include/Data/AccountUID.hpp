#pragma once
#include <switch.h>

static inline bool operator==(AccountUid AccountID1, AccountUid AccountID2)
{
    return (AccountID1.uid[0] == AccountID2.uid[0]) && (AccountID1.uid[1] == AccountID2.uid[1]);
}

static inline bool operator==(AccountUid AccountID, u128 u128ID)
{
    return AccountID.uid[0] == (u128ID >> 64 & 0xFFFFFFFFFFFFFFFF) && AccountID.uid[1] == (u128ID & 0xFFFFFFFFFFFFFFFF);
}
