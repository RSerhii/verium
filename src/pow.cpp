// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <math.h>
#include <bignum.h>
#include <util/system.h>
#include <timedata.h>
#include <validation.h>

#include <inttypes.h>

/////////////////
extern arith_uint256 proofOfWorkLimit;
extern arith_uint256 proofOfWorkLimitTestNet;
double GetDifficulty(const CBlockIndex* blockindex = nullptr);

unsigned int calculateBlocktime(const CBlockIndex* pindex)
{
    unsigned int nBlockTime;
    double diff = GetDifficulty(pindex);
    double dBlockTime = -13.03*log(diff)+180;
    if (dBlockTime < 15)
    {
        nBlockTime = 15;
    }
    else
    {
        nBlockTime = dBlockTime;
    }
    return nBlockTime;
}

unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast)
{
    if (pindexLast->nHeight <= 2)
        return proofOfWorkLimit.GetCompact(); // first few blocks
    const CBlockIndex* pindexPrev = pindexLast->pprev;
    const CBlockIndex* pindexPrevPrev = pindexPrev->pprev;
    unsigned int nTargetSpacing = calculateBlocktime(pindexPrev);
    int64_t nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();
    int64_t targetTimespan;

    // Two hour target timespan on launch needed to prevent accelerated blocktime while difficulty first equilibrates
    if (pindexLast->nHeight+1 <= 2394){targetTimespan = 2 * 60 * 60;}
    // 48 hour normal target timespan with more stable difficulty equilibrium
    else{targetTimespan = Params().GetConsensus().nPowTargetTimespan;}

    // ppcoin: retarget with exponential moving toward target spacing (variable in Verium)
    CBigNum bnNew;
    bnNew.SetCompact(pindexPrev->nBits);
    int64_t nInterval = targetTimespan / nTargetSpacing;
    bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((nInterval + 1) * nTargetSpacing);
    if (bnNew > CBigNum(proofOfWorkLimit))
    {
        bnNew = CBigNum(proofOfWorkLimit);
    }
    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    CBigNum bnTarget;
    bnTarget.SetCompact(nBits);

    // Check range
    if (bnTarget <= 0 || bnTarget > CBigNum(ArithToUint256(proofOfWorkLimit)))
        return error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > UintToArith256(bnTarget.getuint256()))
        return error("CheckProofOfWork() : hash doesn't match nBits");

    return true;
}

CAmount calculateMinerReward(const CBlockIndex* pindex)
{
    int64_t nReward;
    unsigned int nBlockTime = calculateBlocktime(pindex);
    int height = pindex->nHeight+1;
    if (height == 1)
    {
        nReward = 564705 * COIN; // Verium purchased in presale ICO
    }
    else if ((pindex->nMoneySupply/COIN) > 2899999)
    {
        double dReward = 0.04*exp(0.0116*nBlockTime); // Reward schedule after 10x VRC supply parity
        nReward = dReward * COIN;
    }
    else
    {
        double dReward = 0.25*exp(0.0116*nBlockTime); // Reward schedule up to 10x VRC supply parity
        nReward = dReward * COIN;
    }
    return nReward;
}

// miner's coin base reward
int64_t GetProofOfWorkReward(int64_t nFees,const CBlockIndex* pindex)
{
    CAmount nSubsidy = calculateMinerReward(pindex);
    return nSubsidy + nFees;
}

// Get the block rate for one hour
int GetBlockRatePerHour()
{
    int nRate = 0;
    CBlockIndex* pindex = ::ChainActive().Tip();
    int64_t nTargetTime = GetAdjustedTime() - 3600;

    while (pindex && pindex->pprev && pindex->nTime > nTargetTime) {
        nRate += 1;
        pindex = pindex->pprev;
    }
    return nRate;
}
