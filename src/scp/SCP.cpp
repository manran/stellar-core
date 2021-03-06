// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "SCP.h"

#include <algorithm>

#include "xdrpp/marshal.h"
#include "crypto/SHA.h"
#include "scp/LocalNode.h"
#include "scp/Slot.h"
#include "util/Logging.h"
#include "crypto/Hex.h"

namespace stellar
{

SCP::SCP(SecretKey const& secretKey, SCPQuorumSet const& qSetLocal)
{
    mLocalNode = std::make_shared<LocalNode>(secretKey, qSetLocal, this);
}

std::string
SCP::getValueString(Value const& v) const
{
    uint256 valueHash = sha256(xdr::xdr_to_opaque(v));

    return hexAbbrev(valueHash);
}

// values used to switch hash function between priority and neighborhood checks
static const uint32 hash_N = 1;
static const uint32 hash_P = 2;

uint64
SCP::computeHash(uint64 slotIndex, bool isPriority, int32 roundNumber,
                 uint256 const& nodeID)
{
    auto h = SHA256::create();
    h->add(xdr::xdr_to_opaque(slotIndex));
    // h->add(xdr::xdr_to_opaque(mDependencies)); // TODO: set to previous value
    h->add(xdr::xdr_to_opaque(isPriority ? hash_P : hash_N));
    h->add(xdr::xdr_to_opaque(roundNumber));
    h->add(nodeID);
    uint256 t = h->finish();
    uint64 res = 0;
    for (int i = 0; i < sizeof(res); i++)
    {
        res = res << 8 | t[i];
    }
    return res;
}

SCP::EnvelopeState
SCP::receiveEnvelope(SCPEnvelope const& envelope)
{
    // If the envelope is not correctly signed, we ignore it.
    if (!verifyEnvelope(envelope))
    {
        CLOG(DEBUG, "SCP") << "SCP::receiveEnvelope invalid";
        return SCP::EnvelopeState::INVALID;
    }

    uint64 slotIndex = envelope.statement.slotIndex;
    return getSlot(slotIndex)->processEnvelope(envelope);
}

bool
SCP::abandonBallot(uint64 slotIndex)
{
    assert(!getSecretKey().isZero());
    return getSlot(slotIndex)->abandonBallot();
}

bool
SCP::nominate(uint64 slotIndex, Value const& value, bool timedout)
{
    assert(!getSecretKey().isZero());
    return getSlot(slotIndex)->nominate(value, timedout);
}

void
SCP::updateLocalQuorumSet(SCPQuorumSet const& qSet)
{
    mLocalNode->updateQuorumSet(qSet);
}

SCPQuorumSet const&
SCP::getLocalQuorumSet()
{
    return mLocalNode->getQuorumSet();
}

uint256 const&
SCP::getLocalNodeID()
{
    return mLocalNode->getNodeID();
}

void
SCP::purgeSlots(uint64 maxSlotIndex)
{
    auto it = mKnownSlots.begin();
    while (it != mKnownSlots.end())
    {
        if (it->first < maxSlotIndex)
        {
            it = mKnownSlots.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::shared_ptr<LocalNode>
SCP::getLocalNode()
{
    return mLocalNode;
}

std::shared_ptr<Slot>
SCP::getSlot(uint64 slotIndex)
{
    auto it = mKnownSlots.find(slotIndex);
    if (it == mKnownSlots.end())
    {
        mKnownSlots[slotIndex] = std::make_shared<Slot>(slotIndex, *this);
    }
    return mKnownSlots[slotIndex];
}

void
SCP::signEnvelope(SCPEnvelope& envelope)
{
    assert(envelope.statement.nodeID == getSecretKey().getPublicKey());
    envelope.signature =
        getSecretKey().sign(xdr::xdr_to_opaque(envelope.statement));
    envelopeSigned();
}

bool
SCP::verifyEnvelope(SCPEnvelope const& envelope)
{
    bool b = PublicKey::verifySig(envelope.statement.nodeID, envelope.signature,
                                  xdr::xdr_to_opaque(envelope.statement));
    envelopeVerified(b);
    return b;
}

SecretKey const&
SCP::getSecretKey()
{
    return mLocalNode->getSecretKey();
}

size_t
SCP::getKnownSlotsCount() const
{
    return mKnownSlots.size();
}

size_t
SCP::getCumulativeStatemtCount() const
{
    size_t c = 0;
    for (auto const& s : mKnownSlots)
    {
        c += s.second->getStatementCount();
    }
    return c;
}
}
