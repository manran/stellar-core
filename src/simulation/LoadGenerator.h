#pragma once

// Copyright 2015 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "main/Application.h"
#include "crypto/SecretKey.h"
#include "transactions/TxTests.h"
#include "generated/Stellar-types.h"
#include <vector>

namespace stellar
{

class VirtualTimer;

class LoadGenerator
{
  public:
    LoadGenerator();
    ~LoadGenerator();

    struct TxInfo;
    struct AccountInfo;
    using AccountInfoPtr = std::shared_ptr<AccountInfo>;

    static std::string pickRandomCurrency();
    static const uint32_t STEP_MSECS;

    std::vector<AccountInfoPtr> mAccounts;
    std::vector<AccountInfoPtr> mGateways;
    std::vector<AccountInfoPtr> mNeedFund;

    std::unique_ptr<VirtualTimer> mLoadTimer;
    int64 mMinBalance;

    // Schedule a callback to generateLoad() STEP_MSECS miliseconds from now.
    void scheduleLoadGeneration(Application& app, uint32_t nAccounts,
                                uint32_t nTxs, uint32_t txRate);

    // Generate one "step" worth of load (assuming 1 step per STEP_MSECS) at a
    // given target number of accounts and txs, and a given target tx/s rate.
    // If work remains after the current step, call scheduleLoadGeneration()
    // with the remainder.
    void generateLoad(Application& app, uint32_t nAccounts, uint32_t nTxs,
                      uint32_t txRate);

    std::vector<TxInfo> accountCreationTransactions(size_t n);
    AccountInfoPtr createAccount(size_t i, uint32_t ledgerNum = 0);
    std::vector<AccountInfoPtr> createAccounts(size_t n);
    bool loadAccount(Application& app, AccountInfo& account);

    TxInfo createTransferNativeTransaction(AccountInfoPtr from,
                                           AccountInfoPtr to, int64_t amount);

    TxInfo createTransferCreditTransaction(AccountInfoPtr from,
                                           AccountInfoPtr to, int64_t amount,
                                           AccountInfoPtr issuer);

    TxInfo createEstablishTrustTransaction(AccountInfoPtr from,
                                           AccountInfoPtr issuer);

    AccountInfoPtr pickRandomAccount(AccountInfoPtr tryToAvoid,
                                     uint32_t ledgerNum);

    AccountInfoPtr pickRandomSharedTrustAccount(AccountInfoPtr from,
                                                uint32_t ledgerNum,
                                                AccountInfoPtr& issuer);

    TxInfo createRandomTransaction(float alpha, uint32_t ledgerNum = 0);
    std::vector<TxInfo> createRandomTransactions(size_t n, float paretoAlpha);
    void updateMinBalance(Application& app);

    struct TrustLineInfo
    {
        AccountInfoPtr mIssuer;
        uint32_t mLedgerEstablished;
        int64_t mBalance;
        int64_t mLimit;
    };

    struct AccountInfo : public std::enable_shared_from_this<AccountInfo>
    {
        AccountInfo(size_t id, SecretKey key, int64_t balance,
                    SequenceNumber seq, LoadGenerator& loadGen);
        size_t mId;
        SecretKey mKey;
        int64_t mBalance;
        SequenceNumber mSeq;

        // Used when this account trusts some other account's credits.
        std::vector<TrustLineInfo> mTrustLines;

        // Reverse map, when other accounts trust this account's credits.
        std::vector<AccountInfoPtr> mTrustingAccounts;
        std::string mIssuedCurrency;

        TxInfo creationTransaction();

      private:
        LoadGenerator& mLoadGen;
    };

    struct TxInfo
    {
        AccountInfoPtr mFrom;
        AccountInfoPtr mTo;
        enum
        {
            TX_CREATE_ACCOUNT,
            TX_ESTABLISH_TRUST,
            TX_TRANSFER_NATIVE,
            TX_TRANSFER_CREDIT
        } mType;
        int64_t mAmount;
        AccountInfoPtr mIssuer;

        bool execute(Application& app);
        void toTransactionFrames(std::vector<TransactionFramePtr>& txs);
        void recordExecution(int64_t baseFee);
    };
};
}
