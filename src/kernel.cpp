/* @flow */
// Copyright (c) 2012-2013 The PPCoin developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2017-2019 The Uidd developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>

#include "db.h"
#include "kernel.h"
#include "script/interpreter.h"
#include "timedata.h"
#include "util.h"
#include "spork.h"
#include "chainparams.h"

using namespace std;
extern unsigned int nMaxStakingFutureDrift, nMaxPastTimeSecs, nStakeInterval;
extern unsigned int LastHashedBlockHeight;

bool fTestNet = false; //Params().NetworkID() == CBaseChainParams::TESTNET;

// Modifier interval: time to elapse before new modifier is computed
unsigned int nModifierInterval;
int nStakeTargetSpacing = 60;
unsigned int getIntervalVersion(bool fTestNet)
{
    if (fTestNet)
        return MODIFIER_INTERVAL_TESTNET;
    else
        return MODIFIER_INTERVAL;
}

// Hard checkpoints of stake modifiers to ensure they are deterministic.
//static std::map<int, unsigned int> mapStakeModifierCheckpoints = boost::assign::map_list_of(0, 0xfd11f4e7u);

// Get the last stake modifier and its generation time from a given block
static bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime)
{
    if (!pindex)
        return error("GetLastStakeModifier: null pindex");
    while (pindex && pindex->pprev && !pindex->GeneratedStakeModifier())
        pindex = pindex->pprev;
    if (!pindex->GeneratedStakeModifier())
        return error("GetLastStakeModifier: no generation at genesis block");
    nStakeModifier = pindex->nStakeModifier;
    nModifierTime = pindex->GetBlockTime();
    return true;
}

// Get selection interval section (in seconds)
static int64_t GetStakeModifierSelectionIntervalSection(int nSection)
{
	assert(nSection >= 0 && nSection < 64);
	int64_t a = getIntervalVersion(fTestNet) * 63 / (63 + ((63 - nSection) * (MODIFIER_INTERVAL_RATIO - 1)));
	return a;
}

// Get stake modifier selection interval (in seconds)
static int64_t GetStakeModifierSelectionInterval()
{
    int64_t nSelectionInterval = 0;
    for (int nSection = 0; nSection < 64; nSection++) {
        nSelectionInterval += GetStakeModifierSelectionIntervalSection(nSection);
    }

    return nSelectionInterval;
}

// select a block from the candidate blocks in vSortedByTimestamp, excluding
// already selected blocks in vSelectedBlocks, and with timestamp up to
// nSelectionIntervalStop.
static bool SelectBlockFromCandidates(
    vector<pair<int64_t, uint256> >& vSortedByTimestamp,
    map<uint256, const CBlockIndex*>& mapSelectedBlocks,
    int64_t nSelectionIntervalStop,
    uint64_t nStakeModifierPrev,
    const CBlockIndex** pindexSelected)
{
    bool fSelected = false;
    uint256 hashBest = 0;
    *pindexSelected = (const CBlockIndex*)0;
    BOOST_FOREACH (const PAIRTYPE(int64_t, uint256) & item, vSortedByTimestamp) {
        if (!mapBlockIndex.count(item.second))
            return error("SelectBlockFromCandidates: failed to find block index for candidate block %s", item.second.ToString().c_str());

        const CBlockIndex* pindex = mapBlockIndex[item.second];
        if (fSelected && pindex->GetBlockTime() > nSelectionIntervalStop)
            break;

        if (mapSelectedBlocks.count(pindex->GetBlockHash()) > 0)
            continue;

        // compute the selection hash by hashing an input that is unique to that block
        uint256 hashProof = pindex->GetBlockHash();

        CDataStream ss(SER_GETHASH, 0);
        ss << hashProof << nStakeModifierPrev;
        uint256 hashSelection = Hash(ss.begin(), ss.end());

        // the selection hash is divided by 2**32 so that proof-of-stake block
        // is always favored over proof-of-work block. this is to preserve
        // the energy efficiency property
        if (pindex->IsProofOfStake())
            hashSelection >>= 32;

        if (fSelected && hashSelection < hashBest) {
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*)pindex;
        } else if (!fSelected) {
            fSelected = true;
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*)pindex;
        }
    }
    if (GetBoolArg("-printstakemodifier", false))
        LogPrintf("SelectBlockFromCandidates: selection hash=%s\n", hashBest.ToString().c_str());
    return fSelected;
}

// Stake Modifier (hash modifier of proof-of-stake):
// The purpose of stake modifier is to prevent a txout (coin) owner from
// computing future proof-of-stake generated by this txout at the time
// of transaction confirmation. To meet kernel protocol, the txout
// must hash with a future stake modifier to generate the proof.
// Stake modifier consists of bits each of which is contributed from a
// selected block of a given block group in the past.
// The selection of a block is based on a hash of the block's proof-hash and
// the previous stake modifier.
// Stake modifier is recomputed at a fixed time interval instead of every
// block. This is to make it difficult for an attacker to gain control of
// additional bits in the stake modifier, even after generating a chain of
// blocks.
uint256 ComputeStakeModifierV2(const CBlockIndex* pindexPrev, const uint256& kernel)
{
	if (!pindexPrev)
		return 0;  // genesis block's modifier is 0

	CHashWriter ss(SER_GETHASH, 0);
	ss << kernel << pindexPrev->nStakeModifierV2;
	return ss.GetHash();
}

// V.1:
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier)
{
    nStakeModifier = 0;
    fGeneratedStakeModifier = false;
    if (!pindexPrev) {
        fGeneratedStakeModifier = true;
        return true; // genesis block's modifier is 0
    }
    if (pindexPrev->nHeight == 0) {
        //Give a stake modifier to the first block
        fGeneratedStakeModifier = true;
        nStakeModifier = uint64_t("stakemodifier");
        return true;
    }

    // First find current stake modifier and its generation block time
    // if it's not old enough, return the same stake modifier
    int64_t nModifierTime = 0;
    if (!GetLastStakeModifier(pindexPrev, nStakeModifier, nModifierTime))
        return error("ComputeNextStakeModifier: unable to get last modifier");

    if (GetBoolArg("-printstakemodifier", false))
        LogPrintf("ComputeNextStakeModifier: prev modifier= %s time=%s\n", boost::lexical_cast<std::string>(nStakeModifier).c_str(), DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nModifierTime).c_str());

    if (nModifierTime / getIntervalVersion(fTestNet) >= pindexPrev->GetBlockTime() / getIntervalVersion(fTestNet))
        return true;

    // Sort candidate blocks by timestamp
    vector<pair<int64_t, uint256> > vSortedByTimestamp;
    vSortedByTimestamp.reserve(64 * getIntervalVersion(fTestNet) / nStakeTargetSpacing);
    int64_t nSelectionInterval = GetStakeModifierSelectionInterval();
    int64_t nSelectionIntervalStart = (pindexPrev->GetBlockTime() / getIntervalVersion(fTestNet)) * getIntervalVersion(fTestNet) - nSelectionInterval;
    const CBlockIndex* pindex = pindexPrev;

    while (pindex && pindex->GetBlockTime() >= nSelectionIntervalStart) {
        vSortedByTimestamp.push_back(make_pair(pindex->GetBlockTime(), pindex->GetBlockHash()));
        pindex = pindex->pprev;
    }

    int nHeightFirstCandidate = pindex ? (pindex->nHeight + 1) : 0;
    reverse(vSortedByTimestamp.begin(), vSortedByTimestamp.end());
    sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end());

    // Select 64 blocks from candidate blocks to generate stake modifier
    uint64_t nStakeModifierNew = 0;
    int64_t nSelectionIntervalStop = nSelectionIntervalStart;
    map<uint256, const CBlockIndex*> mapSelectedBlocks;
    for (int nRound = 0; nRound < min(64, (int)vSortedByTimestamp.size()); nRound++) {
        // add an interval section to the current selection round
        nSelectionIntervalStop += GetStakeModifierSelectionIntervalSection(nRound);

        // select a block from the candidates of current round
        if (!SelectBlockFromCandidates(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex))
            return error("ComputeNextStakeModifier: unable to select block at round %d", nRound);

        // write the entropy bit of the selected block
        nStakeModifierNew |= (((uint64_t)pindex->GetStakeEntropyBit()) << nRound);

        // add the selected block from candidates to selected list
        mapSelectedBlocks.insert(make_pair(pindex->GetBlockHash(), pindex));
        if (fDebug && GetBoolArg("-printstakemodifier", false))
            LogPrintf("ComputeNextStakeModifier: selected round %d stop=%s height=%d bit=%d\n",
                nRound, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nSelectionIntervalStop).c_str(), pindex->nHeight, pindex->GetStakeEntropyBit());
    }

    // Print selection map for visualization of the selected blocks
    if (fDebug && GetBoolArg("-printstakemodifier", false)) {
        string strSelectionMap = "";
        // '-' indicates proof-of-work blocks not selected
        strSelectionMap.insert(0, pindexPrev->nHeight - nHeightFirstCandidate + 1, '-');
        pindex = pindexPrev;
        while (pindex && pindex->nHeight >= nHeightFirstCandidate) {
            // '=' indicates proof-of-stake blocks not selected
            if (pindex->IsProofOfStake())
                strSelectionMap.replace(pindex->nHeight - nHeightFirstCandidate, 1, "=");
            pindex = pindex->pprev;
        }
        BOOST_FOREACH (const PAIRTYPE(uint256, const CBlockIndex*) & item, mapSelectedBlocks) {
            // 'S' indicates selected proof-of-stake blocks
            // 'W' indicates selected proof-of-work blocks
            strSelectionMap.replace(item.second->nHeight - nHeightFirstCandidate, 1, item.second->IsProofOfStake() ? "S" : "W");
        }
        LogPrintf("ComputeNextStakeModifier: selection height [%d, %d] map %s\n", nHeightFirstCandidate, pindexPrev->nHeight, strSelectionMap.c_str());
    }
    if (fDebug && GetBoolArg("-printstakemodifier", false)) {
        LogPrintf("ComputeNextStakeModifier: new modifier=%s time=%s\n", boost::lexical_cast<std::string>(nStakeModifierNew).c_str(), DateTimeStrFormat("%Y-%m-%d %H:%M:%S", pindexPrev->GetBlockTime()).c_str());
    }

    nStakeModifier = nStakeModifierNew;
    fGeneratedStakeModifier = true;
    return true;
}

// The stake modifier used to hash for a stake kernel is chosen as the stake
// modifier about a selection interval later than the coin generating the kernel
bool GetKernelStakeModifier(uint256 hashBlockFrom, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime)
{
    nStakeModifier = 0;
    if (!mapBlockIndex.count(hashBlockFrom))
        return error("GetKernelStakeModifier() : block not indexed");
    const CBlockIndex* pindexFrom = mapBlockIndex[hashBlockFrom];
    nStakeModifierHeight = pindexFrom->nHeight;
    nStakeModifierTime = pindexFrom->GetBlockTime();
    int64_t nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();
	//LogPrintf("nStakeModifierSelectionInterval: %d\n", nStakeModifierSelectionInterval);
	//LogPrintf("nStakeModifierTime: %d\n", nStakeModifierTime);
	//LogPrintf("pindexFrom->GetBlockTime(): %d\n", pindexFrom->GetBlockTime());
	

    const CBlockIndex* pindex = pindexFrom;
    CBlockIndex* pindexNext = chainActive[pindexFrom->nHeight + 1];

    // loop to find the stake modifier later by a selection interval
	// so nStakeModifierSelectionInterval must be small enough together with pindexFrom->GetBlockTime() to make nStakeModifierTime bigger than them.
    while (nStakeModifierTime < pindexFrom->GetBlockTime() + nStakeModifierSelectionInterval) {
        if (!pindexNext) {
            // Should never happen.
            return error("Null pindexNext\n");
        }

        pindex = pindexNext;
        pindexNext = chainActive[pindexNext->nHeight + 1];
        if (pindex->GeneratedStakeModifier()) {
			//LogPrintf("nStakeModifierTime: %d\n", nStakeModifierTime);
            nStakeModifierHeight = pindex->nHeight;
            nStakeModifierTime = pindex->GetBlockTime();
        }
    }
    nStakeModifier = pindex->nStakeModifier;
    return true;
}

//test hash vs target
bool stakeTargetHit(const uint256& hashProofOfStake, const int64_t& nValueIn, const uint256& bnTarget)
{
    //get the stake weight - weight is equal to coin amount
    uint256 bnWeight = uint256(nValueIn) / 100;

    // Now check if proof-of-stake hash meets target protocol
    return hashProofOfStake < (bnWeight * bnTarget);
}

//instead of looping outside and reinitializing variables many times, we will give a nTimeTx so that we can do all the hashing here
bool CheckStakeKernelHashV2(unsigned int nBits, CBlockIndex* pindexPrev, unsigned int nTimeBlockFrom, const CTransaction& txPrev, const COutPoint& prevout, unsigned int& nTimeTx, bool fCheck, uint256& hashProofOfStake)
{
	//assign new variables to make it easier to read
	CAmount nValueIn = txPrev.vout[prevout.n].nValue;

	if (nTimeTx < nTimeBlockFrom) // Transaction timestamp violation
		return error("CheckStakeKernelHashV2() : nTime violation");

	if (nTimeBlockFrom + nStakeMinAge > nTimeTx) // Min age requirement
		return error("CheckStakeKernelHashV2() : min age violation - nTimeBlockFrom=%d nStakeMinAge=%d nTimeTx=%d", nTimeBlockFrom, nStakeMinAge, nTimeTx);

	if (nValueIn == 0)
		return error("CheckStakeKernelHashV2() : nValueIn = 0");

	//grab difficulty
	uint256 bnTarget;
	bnTarget.SetCompact(nBits);

	//if wallet is simply checking to make sure a hash is valid
	if (fCheck) {
		CHashWriter ss(SER_GETHASH, 0);
		ss << pindexPrev->nStakeModifierV2 << nTimeBlockFrom << prevout.hash << prevout.n << nTimeTx;
		hashProofOfStake = ss.GetHash();
		bool TargetMet = stakeTargetHit(hashProofOfStake, nValueIn, bnTarget);
		if(!TargetMet) LogPrintf("CheckStakeKernelHashV2() failed: nStakeModifierV2=%s nTimeBlockFrom=%d prevout.hash=%s prevout.n=%d nTimeTx=%u\n", pindexPrev->nStakeModifierV2.ToString().c_str(), nTimeBlockFrom, prevout.hash.ToString().c_str(), prevout.n, nTimeTx);
		return TargetMet;
	}

	bool fSuccess = false;
	// nTimeTx starts as GetAdjustedTime when creating a new block.
	nTimeTx = nTimeTx - (nTimeTx % nStakeInterval); // Round it to the proper staking interval.
	unsigned int nTryTime = 0;
	int nHeightStart = chainActive.Height();
	int HashingStart;

	if(LastHashedBlockHeight != nHeightStart)
	{
		int64_t ActualMedianTimePast = chainActive.Tip()->GetMedianTimePast();
		int64_t TimePastDifference2 = nTimeTx - ActualMedianTimePast; 
		int64_t TimePastDifference = nTimeTx - chainActive.Tip()->GetBlockTime() + nMaxPastTimeSecs - 1;
		if (TimePastDifference > TimePastDifference2) TimePastDifference = TimePastDifference2;

		if (TimePastDifference > 1)
		{
			int nFactor = TimePastDifference / nStakeInterval;
			HashingStart = -nFactor * nStakeInterval; // This makes it try past times too.
		}
		else HashingStart = 0;
	}
	else
	{
		// Set the HashingStart so that we're only trying previously untested times.
		HashingStart = LastHashedBlockTime - nTimeTx + nStakeInterval;
	}

	for (int i = HashingStart; i < nMaxStakingFutureDrift; i += nStakeInterval) //iterate the hashing
	{
		//new block came in, move on
		if (chainActive.Height() != nHeightStart)
			break;

		if (i > nMaxStakingFutureDrift) break;

		//hash this iteration
		nTryTime = nTimeTx + i;

		CHashWriter ss(SER_GETHASH, 0);
		ss << pindexPrev->nStakeModifierV2 << nTimeBlockFrom << prevout.hash << prevout.n << nTryTime;
		hashProofOfStake = ss.GetHash();

		// if stake hash does not meet the target then continue to next iteration
		if (!stakeTargetHit(hashProofOfStake, nValueIn, bnTarget))
			continue;

		fSuccess = true; // if we make it this far then we have successfully created a stake hash
		nTimeTx = nTryTime;
		//LogPrintf("CheckStakeKernelHashV2(): Success. nStakeModifierV2=%s nTimeBlockFrom=%d prevout.hash=%s prevout.n=%d nTimeTx=%u\n", pindexPrev->nStakeModifierV2.ToString().c_str(), nTimeBlockFrom, prevout.hash.ToString().c_str(), prevout.n, nTimeTx);
		break;
	}

	return fSuccess;
}

// v.1 only.
uint256 stakeHash(unsigned int nTimeTx, CDataStream ss, unsigned int prevoutIndex, uint256 prevoutHash, unsigned int nTimeBlockFrom)
{
	//Uidd will hash in the transaction hash and the index number in order to make sure each hash is unique
	ss << nTimeBlockFrom << prevoutIndex << prevoutHash << nTimeTx;
	return Hash(ss.begin(), ss.end());
}

//instead of looping outside and reinitializing variables many times, we will give a nTimeTx so that we can do all the hashing here
bool CheckStakeKernelHash(unsigned int nBits, const CBlock& blockFrom, const CTransaction& txPrev, const COutPoint& prevout, unsigned int& nTimeTx, bool fCheck, uint256& hashProofOfStake)
{
    //assign new variables to make it easier to read
    int64_t nValueIn = txPrev.vout[prevout.n].nValue;
    unsigned int nTimeBlockFrom = blockFrom.GetBlockTime();

    if (nTimeTx < nTimeBlockFrom) // Transaction timestamp violation
        return error("CheckStakeKernelHash() : nTime violation");

    if (nTimeBlockFrom + nStakeMinAge > nTimeTx) // Min age requirement
        return error("CheckStakeKernelHash() : min age violation - nTimeBlockFrom=%d nStakeMinAge=%d nTimeTx=%d", nTimeBlockFrom, nStakeMinAge, nTimeTx);

	if (nValueIn == 0)
		return error("CheckStakeKernelHash() : nValueIn = 0");

    //grab difficulty
    uint256 bnTarget;
	bnTarget.SetCompact(nBits);

	//grab stake modifier
	uint64_t nStakeModifier = 0;
	int nStakeModifierHeight = 0;
	int64_t nStakeModifierTime = 0;
	if (!GetKernelStakeModifier(blockFrom.GetHash(), nStakeModifier, nStakeModifierHeight, nStakeModifierTime)) {
		LogPrintf("CheckStakeKernelHash(): failed to get kernel stake modifier \n");
		return false;
	}

	//create data stream once instead of repeating it in the loop
	CDataStream ss(SER_GETHASH, 0);
	ss << nStakeModifier;

	//if wallet is simply checking to make sure a hash is valid
	if (fCheck) {
		hashProofOfStake = stakeHash(nTimeTx, ss, prevout.n, prevout.hash, nTimeBlockFrom);
		return stakeTargetHit(hashProofOfStake, nValueIn, bnTarget);
	}

    bool fSuccess = false;
    unsigned int nTryTime = 0;
	int64_t ActualMedianTimePast = chainActive.Tip()->GetMedianTimePast();
	int64_t TimePastDifference = nTimeTx - ActualMedianTimePast; // nTimeTx starts as GetAdjustedTime when creating a new block.
	if(nTimeTx % 60 == 0) LogPrintf("CheckStakeKernelHash(): TimePastDifference=%d\n", TimePastDifference);
	if (TimePastDifference > 100) TimePastDifference = 100; // 162 - 230+
    int nHeightStart = chainActive.Height();
	
	unsigned int HashingEnd = nMaxStakingFutureDrift;
	if(TimePastDifference > 1 && LastHashedBlockHeight != chainActive.Height()) HashingEnd += TimePastDifference - 1; // This makes it try past times too.

	for (unsigned int i = 0; i < HashingEnd; ++i) //iterate the hashing
	{
		//new block came in, move on
		if (chainActive.Height() != nHeightStart)
			break;

		//hash this iteration
		nTryTime = nTimeTx + nMaxStakingFutureDrift - i;
		if (i > nMaxStakingFutureDrift && (nTryTime < nTimeBlockFrom || nTimeBlockFrom + nStakeMinAge > nTryTime)) continue; // Don't try with unacceptable times.

		hashProofOfStake = stakeHash(nTryTime, ss, prevout.n, prevout.hash, nTimeBlockFrom);

		// if stake hash does not meet the target then continue to next iteration
		if (!stakeTargetHit(hashProofOfStake, nValueIn, bnTarget))
			continue;

		fSuccess = true; // if we make it this far then we have successfully created a stake hash
		nTimeTx = nTryTime;
		break;
	}

    return fSuccess;
}

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(const CBlock& block, CBlockIndex* pindexPrev, uint256& hashProofOfStake)
{
    const CTransaction tx = block.vtx[1];
    if (!tx.IsCoinStake())
        return error("CheckProofOfStake() : called on non-coinstake %s", tx.GetHash().ToString().c_str());

    // Kernel (input 0) must match the stake hash target (nBits)
    const CTxIn& txin = tx.vin[0];

    // First try finding the previous transaction in database
    uint256 hashBlock;
    CTransaction txPrev;
    if (!GetTransaction(txin.prevout.hash, txPrev, hashBlock, true))
        return error("CheckProofOfStake() : INFO: read txPrev failed");

    //verify signature and script
    if (!VerifyScript(txin.scriptSig, txPrev.vout[txin.prevout.n].scriptPubKey, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&tx, 0)))
        return error("CheckProofOfStake() : VerifySignature failed on coinstake %s", tx.GetHash().ToString().c_str());

	// Find the previous transaction's block
    CBlockIndex* pindex = NULL; 
    BlockMap::iterator it = mapBlockIndex.find(hashBlock);
    if (it != mapBlockIndex.end())
        pindex = it->second;
    else
        return error("CheckProofOfStake() : read block failed");

    // Read block header
    CBlock blockprev;
    if (!ReadBlockFromDisk(blockprev, pindex->GetBlockPos()))
        return error("CheckProofOfStake(): INFO: failed to find block");

	// Verify inputs
	if (txin.prevout.hash != txPrev.GetHash())
		return error("CheckProofOfStake() : coinstake input does not match previous output %s", txin.prevout.hash.GetHex());

    unsigned int nTime = block.nTime;
	if (pindexPrev->nHeight + 1 >= GetSporkValue(SPORK_13_STAKING_PROTOCOL_2))
	{
		if (!CheckStakeKernelHashV2(block.nBits, pindexPrev, blockprev.GetBlockTime(), txPrev, txin.prevout, nTime, true, hashProofOfStake))
			return error("CheckProofOfStake() : INFO: check kernel failed on coinstake v2 %s, pindex->nHeight=%u, hashProof=%s \n", tx.GetHash().ToString().c_str(), pindex->nHeight, hashProofOfStake.ToString().c_str()); // may occur during initial download or if behind on block chain sync
	}
	else
	{
		if (!CheckStakeKernelHash(block.nBits, blockprev, txPrev, txin.prevout, nTime, true, hashProofOfStake))
			return error("CheckProofOfStake() : INFO: check kernel failed on coinstake %s, pindex->nHeight=%u, hashProof=%s \n", tx.GetHash().ToString().c_str(), pindex->nHeight, hashProofOfStake.ToString().c_str()); // may occur during initial download or if behind on block chain sync
	}
    return true;
}

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int64_t nTimeBlock, int64_t nTimeTx)
{
    // v0.3 protocol
    return (nTimeBlock == nTimeTx);
}

// Get stake modifier checksum
/*unsigned int GetStakeModifierChecksum(const CBlockIndex* pindex)
{
    assert(pindex->pprev || pindex->GetBlockHash() == Params().HashGenesisBlock());
    // Hash previous checksum with flags, hashProofOfStake and nStakeModifier
    CDataStream ss(SER_GETHASH, 0);
    if (pindex->pprev)
        ss << pindex->pprev->nStakeModifierChecksum;
    ss << pindex->nFlags << pindex->hashProofOfStake << pindex->nStakeModifier;
    uint256 hashChecksum = Hash(ss.begin(), ss.end());
    hashChecksum >>= (256 - 32);
    return hashChecksum.Get64();
}*/

// Check stake modifier hard checkpoints
/*bool CheckStakeModifierCheckpoints(int nHeight, unsigned int nStakeModifierChecksum)
{
    if (fTestNet) return true; // Testnet has no checkpoints
    if (mapStakeModifierCheckpoints.count(nHeight)) {
        return nStakeModifierChecksum == mapStakeModifierCheckpoints[nHeight];
    }
    return true;
}*/
