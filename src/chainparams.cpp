// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2017-2019 The Uidd developers
// Copyright (c) 2017-2019 The Uidd Developers
//Copyright (c) 2020 Uidd Developer

// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libzerocoin/Params.h"
#include "chainparams.h"
#include "random.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

/**
 * Main network
 */

//! Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress>& vSeedsOut, const SeedSpec6* data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7 * 24 * 60 * 60;
    for (unsigned int i = 0; i < count; i++) {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

//   What makes a good checkpoint block?
// + Is surrounded by blocks with reasonable timestamps
//   (no blocks before with a timestamp after, none after with
//    timestamp before)
// + Contains no strange transactions
static Checkpoints::MapCheckpoints mapCheckpoints =
	boost::assign::map_list_of(0, uint256("0a94e812e39377fc8e1ee9d0db4711db693dd1aa90b7cc251f5b587e2a5d5923"))
    (256, uint256("3bb33059afca8f54fff46b24ee7efd18d159c3ffbab3967fb1f434f1eaa0cde0"))
    (1298, uint256("d30861f0fa9e0853e1bb55be9d269a863b0e17fc77170ca9817c9d09a6f9415c"))
    (3560, uint256("cb66fe5ee25ed789f3479e5d6c690197cb688a0a9686ea0a357fad36e92c778e"))
    (11425, uint256("447fbfca9651d497bd2533cff349b7256de2ba8caaff31ff5db097ed81f0b05c"))
    (21255, uint256("47198ca2fcc1159c1693ecbc63fb0c0c2133eecf311a727ff54e9fe99a04ee87"))
    (50000, uint256("b7832326493b550cc1c196aea07f779e58185ed44cde1ded55f82e2185e815d8"))
    (78540, uint256("39cf9dcc4f91ca0afca5ead6cbd2dab388315002ef3e479879ce9b8924ee8a94"));

static const Checkpoints::CCheckpointData data = {
    &mapCheckpoints,
	1625200774, // * UNIX timestamp of last checkpoint block. obtained by: date +%s.
	156971,    // * total number of transactions between genesis and last checkpoint
                //   (the tx=... number in the UpdateTip: debug.log lines)
	1440        // * estimated number of transactions per day after checkpoint. 2 transactions per minute = 2880
};

static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
    boost::assign::map_list_of(0, uint256("0a94e812e39377fc8e1ee9d0db4711db693dd1aa90b7cc251f5b587e2a5d5923"));

static const Checkpoints::CCheckpointData dataTestnet = {
    &mapCheckpointsTestnet,
	1581144591,
    0,
    250};

static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
    boost::assign::map_list_of(0, uint256("0a94e812e39377fc8e1ee9d0db4711db693dd1aa90b7cc251f5b587e2a5d5923"));

static const Checkpoints::CCheckpointData dataRegtest = {
    &mapCheckpointsRegtest,
	1581144591,
    0,
    100};

libzerocoin::ZerocoinParams* CChainParams::Zerocoin_Params() const
{
    assert(this);
    static CBigNum bnTrustedModulus(zerocoinModulus);
    static libzerocoin::ZerocoinParams ZCParams = libzerocoin::ZerocoinParams(bnTrustedModulus);

    return &ZCParams;
}

class CMainParams : public CChainParams
{
public:
    CMainParams()
    {
        networkID = CBaseChainParams::MAIN;
        strNetworkID = "main";
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
		 These values are unique to UIDD.
         */
        pchMessageStart[0] = 231;
        pchMessageStart[1] = 195;
        pchMessageStart[2] = 241;
        pchMessageStart[3] = 247;
        vAlertPubKey = ParseHex("043893c78d2431a162624c11ebe310073c279a7cdff65e817349ba015a95fcd29c8cff661f7ddfcaa4c9aa2c614bd2753eb08be56c21633aa1ee7df7aa1fdfec36"); // UIDD
        nDefaultPort = 7860; 
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        nMaxReorganizationDepth = 100;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 0; // Defaults to max that the hardware can efficiently use.
        nTargetTimespan = 60;
        nTargetSpacing = 60;  // 1 minute
        nMaturity = 100;

        nMaxMoneyOut = 1192000000 * COIN; // This is the maximum supply.

        /** Height or Time Based Activations **/
        nLastPOWBlock = 1238;
        nModifierUpdateBlock = 1239;
        nZerocoinStartHeight = 471910;
        nZerocoinStartTime = 1612154685;

        /**
         * Build the genesis block. 0 output in Uidd.
         */
        const char* pszTimestamp = "Hum dekhenge Lazim hain ke hum bhi dekhenge Wo din ke jis ka wada hain";
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 0;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("0472ccf3f85b994a0cd8233d441bb59f049d0bc02b00c790a707dbb2f9038ec69753a25bd69241f67bd17921fcee20b8000697826bcb981da984f6a45e56f13b53") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.nVersion = 1;
        genesis.nTime = 1596611015;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 2;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();

        hashGenesisBlock = genesis.GetHash();
       /*///////////////////////////////////////
if(genesis.GetHash() != uint256("0x"))
{
      printf("MSearching for genesis block...\n");
      uint256 hashTarget;
      hashTarget.SetCompact(genesis.nBits);
      while(uint256(genesis.GetHash()) > uint256(hashTarget))
      {
          ++genesis.nNonce;
          if (genesis.nNonce == 0)
          {
              printf("Mainnet NONCE WRAPPED, incrementing time");
              std::cout << std::string("Mainnet NONCE WRAPPED, incrementing time:\n");
              ++genesis.nTime;
          }
          if (genesis.nNonce % 10000 == 0)
          {
              printf("Mainnet: nonce %08u: hash = %s \n", genesis.nNonce, genesis.GetHash().ToString().c_str());
          }
      }
      printf("Mainnet block.nTime = %u \n", genesis.nTime);
      printf("Mainnet block.nNonce = %u \n", genesis.nNonce);
      printf("Mainnet block.hashMerkleRoot: %s\n", genesis.hashMerkleRoot.ToString().c_str());
      printf("Mainnet block.GetHash = %s\n", genesis.GetHash().ToString().c_str());
}
/////////////////////////////////////// */
        assert(hashGenesisBlock == uint256("0a94e812e39377fc8e1ee9d0db4711db693dd1aa90b7cc251f5b587e2a5d5923"));
        assert(genesis.hashMerkleRoot == uint256("6d28f8b38c2d784880b32f47c0be4d3c6f8cbef7ee41aabe5b3f32576cd543fd"));

		vSeeds.push_back(CDNSSeedData("seed.uidd.org", "seed.uidd.org"));  // Primary DNS Seeder
		vSeeds.push_back(CDNSSeedData("seeder.uidd.org", "seeder.uidd.org"));  // Secondary DNS Seeder on another server.
		convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main)); // Fixed seed IP addresses as a fallback.

		// bitcoin
		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 50);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 20);
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 128);
		base58Prefixes[EXT_PUBLIC_KEY] = { 0x04, 0x88, 0xB2, 0x1E };
		base58Prefixes[EXT_SECRET_KEY] = { 0x04, 0x88, 0xAD, 0xE4 };

        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fSkipProofOfWorkCheck = false;
        fTestnetToBeDeprecatedFieldRPC = false;
        fHeadersFirstSyncingActive = false;

        nPoolMaxTransactions = 3;
		strSporkKey = "04e39b9d811dd2d1e84ff4be5ca81dfa98411d6cf520b3d0394651f35721cf53ddbfdbbf40c768777aac98641e522536c5a86aacc21509658283877d2c73c9fde0";
        strObfuscationPoolDummyAddress = "MU9uN6dYdyiLtjtzCjiWet3sELBifbnWT5";
        nStartMasternodePayments = 1596697415; 

        /** Zerocoin */
		zerocoinModulus = "2cc9d86f80c2681d41409624f5b20af7bfe9488303798fcad260adfd8b9cb96000941e96eb2f525462526a2a11f6b15189991a89cebfdb98db1405c4f5d08a6e57ea26b73a36596041f4d089b19a4372f07d38cd29cb531c4487155470fb2ac3241416b19c2c40a7dda20b"
        "0febc35820260c50b404361b899596bf3cb19a5f632092250ce6003d3c3bc262b979fee1fff4339a761f43c65b39b5a0915ef30f87a056c891a3afd5be3853dc93f6b249d409e34f77"
        "e63626d0d8d3b929bd0a9f88a06ec94142ff8c316e830507f92fdb91f499ddfe12c784ac8a1f95ebb66137f4cf586d81ca6b5c82a3245bce9a63911f92b4917d49c9dff1ef6faac25fe9f82c2f0c7bbd897c6c594d3fdace556f94a556ab32cd0a32976301bbc31f00129aad3dbde42b384776a0a02d2cf0"
        "1f759b4ee73b25c311f9a4f8d524747ce0efe7a0f0056f38241cacf2ebb41dfa8a19a315363b192d45539b032d2a04dc6dc635a30fd64819cfe1ac95eabbf3f1273cf01d3c96fac06f9577d4bf774283cd89773465e5ecf4089ce7710dbeacad916b9ef2209a13c1078e9985b7a6448e1dfc375bca762b50"
        "598d92d26cdcc370a650a0cf481201e11e524d680b6039c942a7b2320e6bae2f74742efd8df35b8cecd4f90b84e0b234bce64aa363f0f811af4a8f705411b429f54388fd1914c5219d3b6cdca63cf7740bf3d940131d712fabbd469a649a06d147a8bac1dd7f0c2b7bd9901a7b5d1f4e99f2c3eee7470628"
        "d52bd2fb1ebab08679de7e9e3ddfcbfdcbc2de512a0d3c4843f59deb986a0204c8763189a4a6479eff4faf1d6671f11cedae8e91c217efbe301a9d12376d61196f80d912180d81f7ddf29f76a6880d20404648ea310e98936a9f49fe07594cc8e23dd334dfc3752425da9ba6f9377af76772e7e98efd4c82"
        "a97b673df74d893eab54fc1d76bef6f8dfc4500d86ab1b9c07c0b7fd6d6ee8347b28c841a5c14fc3a338d5eafb72092ed57df90043131b25a6574be14b1a48ddc1a76c549915d1644f319a24f8831fb4e98f10b21e5b97e5c4a7a2091533deae80046cd4f261fe35a9ccd94a9bf798eee7d9978a156544c3"
        "ee31555c1c8e238654a8236aa6ce75e614ff94e995da77ec29aa4d534726f6af68826f0989f339aae023ec3da25574d84a795d6155232a54bfdf30d7c8fd225455938fb471ee7e3a0711f527afa65fe2bd1d172f38db54ce9dd1c182360cc74df8604e40b78426f5205a663daeafe6e5b4b6c338a4bab082"
        "10b5217fe117c601ebf19c1fb6daf009efba576924a90d2113d91e9e1b963f97df828a2fa";
        
        nMaxZerocoinSpendsPerTransaction = 17; // Assume about 20kb each
        nMinZerocoinMintFee = 100000; // higher fee required for zerocoin mints. 0.001 UIDD.
        nMintRequiredConfirmations = 20; //the maximum amount of confirmations until accumulated in 19
        nRequiredAccumulation = 1;
        nDefaultSecurityLevel = 100; //full security level for accumulators
        nZerocoinHeaderVersion = 4; //Block headers must be this version once zerocoin is active
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return data;
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams
{
public:
    CTestNetParams()
    {
        networkID = CBaseChainParams::TESTNET;
        strNetworkID = "test";
		pchMessageStart[0] = 33;
		pchMessageStart[1] = 97;
		pchMessageStart[2] = 241;
		pchMessageStart[3] = 146;
        vAlertPubKey = ParseHex("043893c78d2431a162624c11ebe310073c279a7cdff65e817349ba015a95fcd29c8cff661f7ddfcaa4c9aa2c614bd2753eb08be56c21633aa1ee7df7aa1fdfec36"); // UIDD
        nDefaultPort = 17860;
        nEnforceBlockUpgradeMajority = 51;
        nRejectBlockOutdatedMajority = 75;
        nToCheckBlockUpgradeMajority = 100;
        nMinerThreads = 0;
        nTargetTimespan = 60;
        nTargetSpacing = 60;
        nLastPOWBlock = 200;
        nMaturity = 15;
        nModifierUpdateBlock = 1391;
        nMaxMoneyOut = 1192000000 * COIN;
        nZerocoinStartHeight = 450000;
        nZerocoinStartTime = 1612611015;

        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1596611015;
        genesis.nNonce = 2;

        hashGenesisBlock = genesis.GetHash();
        genesis.print();
        assert(hashGenesisBlock == uint256("0a94e812e39377fc8e1ee9d0db4711db693dd1aa90b7cc251f5b587e2a5d5923"));

        vFixedSeeds.clear();
        vSeeds.clear();

		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
		base58Prefixes[EXT_PUBLIC_KEY] = { 0x04, 0x35, 0x87, 0xCF };
		base58Prefixes[EXT_SECRET_KEY] = { 0x04, 0x35, 0x83, 0x94 };
        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 2;
        strSporkKey = "04e39b9d811dd2d1e84ff4be5ca81dfa98411d6cf520b3d0394651f35721cf53ddbfdbbf40c768777aac98641e522536c5a86aacc21509658283877d2c73c9fde0";
        nStartMasternodePayments = 1519330596; 
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataTestnet;
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CTestNetParams
{
public:
    CRegTestParams()
    {
        networkID = CBaseChainParams::REGTEST;
        strNetworkID = "regtest";
        strNetworkID = "regtest";
		pchMessageStart[0] = 133;
		pchMessageStart[1] = 140;
		pchMessageStart[2] = 140;
		pchMessageStart[3] = 220;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 1;
        nTargetTimespan = 24 * 60 * 60; // 1 day
        nTargetSpacing = 60;        // 1 minute
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        genesis.nTime = 1596611015;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 2;
        nDefaultPort = 51476;
		hashGenesisBlock = genesis.GetHash();
		
        genesis.print();
        assert(hashGenesisBlock == uint256("0a94e812e39377fc8e1ee9d0db4711db693dd1aa90b7cc251f5b587e2a5d5923"));

        vFixedSeeds.clear(); //! Testnet mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Testnet mode doesn't have any DNS seeds.

		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
		base58Prefixes[EXT_PUBLIC_KEY] = { 0x04, 0x35, 0x87, 0xCF };
		base58Prefixes[EXT_SECRET_KEY] = { 0x04, 0x35, 0x83, 0x94 };

        fMiningRequiresPeers = false;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataRegtest;
    }
};
static CRegTestParams regTestParams;

/**
 * Unit test
 */
class CUnitTestParams : public CMainParams, public CModifiableParams
{
public:
    CUnitTestParams()
    {
        networkID = CBaseChainParams::UNITTEST;
        strNetworkID = "unittest";
        nDefaultPort = 49866;
        vFixedSeeds.clear(); //! Unit test mode doesn't have any fixed seeds.
        vSeeds.clear();      //! Unit test mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fAllowMinDifficultyBlocks = false;
        fMineBlocksOnDemand = true;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        // UnitTest share the same checkpoints as MAIN
        return data;
    }

    //! Published setters to allow changing values in unit test cases
    virtual void setEnforceBlockUpgradeMajority(int anEnforceBlockUpgradeMajority) { nEnforceBlockUpgradeMajority = anEnforceBlockUpgradeMajority; }
    virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority) { nRejectBlockOutdatedMajority = anRejectBlockOutdatedMajority; }
    virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority) { nToCheckBlockUpgradeMajority = anToCheckBlockUpgradeMajority; }
    virtual void setDefaultConsistencyChecks(bool afDefaultConsistencyChecks) { fDefaultConsistencyChecks = afDefaultConsistencyChecks; }
    virtual void setAllowMinDifficultyBlocks(bool afAllowMinDifficultyBlocks) { fAllowMinDifficultyBlocks = afAllowMinDifficultyBlocks; }
    virtual void setSkipProofOfWorkCheck(bool afSkipProofOfWorkCheck) { fSkipProofOfWorkCheck = afSkipProofOfWorkCheck; }
};
static CUnitTestParams unitTestParams;


static CChainParams* pCurrentParams = 0;

CModifiableParams* ModifiableParams()
{
    assert(pCurrentParams);
    assert(pCurrentParams == &unitTestParams);
    return (CModifiableParams*)&unitTestParams;
}

const CChainParams& Params()
{
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(CBaseChainParams::Network network)
{
    switch (network) {
    case CBaseChainParams::MAIN:
        return mainParams;
    case CBaseChainParams::TESTNET:
        return testNetParams;
    case CBaseChainParams::REGTEST:
        return regTestParams;
    case CBaseChainParams::UNITTEST:
        return unitTestParams;
    default:
        assert(false && "Unimplemented network");
        return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);
    return true;
}
