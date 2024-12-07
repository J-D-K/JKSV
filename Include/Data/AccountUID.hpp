#pragma once
#include <switch.h>

static inline bool operator==(AccountUid AccountID1, AccountUid AccountID2)
{
    return (AccountID1.uid[0] == AccountID2.uid[0]) && (AccountID1.uid[1] == AccountID2.uid[1]);
}

static inline u128 AccountUIDToU128(AccountUid AccountID)
{
    return (static_cast<u128>(AccountID.uid[0]) << 64 | AccountID.uid[1]);
}
