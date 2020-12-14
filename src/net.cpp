// SPDX-FileCopyrightText: © 2020 Alias Developers
// SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
// SPDX-FileCopyrightText: © 2009 Bitcoin Developers
// SPDX-FileCopyrightText: © 2009 Satoshi Nakamoto
//
// SPDX-License-Identifier: MIT

#include "db.h"
#include "net.h"
#include "init.h"
#include "strlcpy.h"
#include "addrman.h"
#include "interface.h"
#include <sys/stat.h>
#include <boost/dll.hpp>

#ifdef WIN32
#include <string.h>
#include <Windows.h>
#elif __APPLE__
// Tor separate process via boost::process
#include <boost/process.hpp>
boost::process::group gTor;
#elif __linux__
#include <sys/prctl.h>
#include <signal.h>     // signals
#include <unistd.h>     // fork()
#include <sys/wait.h>   // waitpid()
pid_t tor_process_pid = 0;
bool tor_killed_from_here = false;
#endif

#ifdef USE_UPNP
#include <miniupnpc/miniwget.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#endif

// Dump addresses to peers.dat every 15 minutes (900s)
#define DUMP_ADDRESSES_INTERVAL 900

using namespace std;
using namespace boost;
namespace fs = boost::filesystem;

#if !defined(WIN32) && !defined(__APPLE__) && !defined(__linux__)
// Tor embedded
extern "C" {
    int tor_main(int argc, char *argv[]);
}
#endif

static const int MAX_OUTBOUND_CONNECTIONS = 16;
// Increment this variable when doing change to torrc-defaults on Linux
static const int TORRC_DEFAULTS_REV = 1;

bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant *grantOutbound = NULL, const char *strDest = NULL, bool fOneShot = false);


//
// Global state variables
//
bool fDiscover = true;
bool fUseUPnP = false;

CCriticalSection cs_mapLocalHost;
map<CNetAddr, LocalServiceInfo> mapLocalHost;
static bool vfReachable[NET_MAX] = {};
static bool vfLimited[NET_MAX] = {};
static CNode* pnodeLocalHost = NULL;
static CNode* pnodeSync = NULL;
CAddress addrSeenByPeer(CService("0.0.0.0", 0), nLocalServices);
uint64_t nLocalHostNonce = 0;
static std::vector<SOCKET> vhListenSocket;
CAddrMan addrman;

vector<CNode*> vNodes;
CCriticalSection cs_vNodes;
CCriticalSection cs_connectNode;
map<CInv, CDataStream> mapRelay;
deque<pair<int64_t, CInv> > vRelayExpiration;
CCriticalSection cs_mapRelay;
map<CInv, int64_t> mapAlreadyAskedFor;

static deque<string> vOneShots;
CCriticalSection cs_vOneShots;

set<CNetAddr> setservAddNodeAddresses;
CCriticalSection cs_setservAddNodeAddresses;

vector<std::string> vAddedNodes;
CCriticalSection cs_vAddedNodes;

static CSemaphore *semOutbound = NULL;

NodeId nLastNodeId = 0;
CCriticalSection cs_nLastNodeId;

std::string torPath = "tor";

void AddOneShot(string strDest)
{
    LOCK(cs_vOneShots);
    vOneShots.push_back(strDest);
}

unsigned short GetListenPort()
{
    return (unsigned short)(GetArg("-port", Params().GetDefaultPort()));
}

void CNode::PushGetBlocks(uint256& hashBegin, uint256 hashEnd)
{
    //pindexLastGetBlocksBegin = pindexBegin;
    hashLastGetBlocksEnd = hashEnd;

    CBlockLocator blockLocator;
    blockLocator.SetThin(hashBegin);
    PushMessage("getblocks", blockLocator, hashEnd);
}

void CNode::PushGetBlocks(CBlockIndex* pindexBegin, uint256 hashEnd)
{
    // Filter out duplicate requests
    if (pindexBegin == pindexLastGetBlocksBegin && hashEnd == hashLastGetBlocksEnd)
        return;

    pindexLastGetBlocksBegin = pindexBegin;
    hashLastGetBlocksEnd = hashEnd;

    PushMessage("getblocks", CBlockLocator(pindexBegin), hashEnd);
}

void CNode::PushGetBlocks(CBlockThinIndex* pindexBegin, uint256 hashEnd)
{
    // Filter out duplicate requests
    if (pindexBegin == pindexLastGetBlockThinsBegin && hashEnd == hashLastGetBlocksEnd)
        return;

    pindexLastGetBlockThinsBegin = pindexBegin;
    hashLastGetBlocksEnd = hashEnd;

    PushMessage("getblocks", CBlockThinLocator(pindexBegin), hashEnd);
};

void CNode::PushGetHeaders(CBlockThinIndex* pindexBegin, uint256 hashEnd)
{
    // Filter out duplicate requests
    if (pindexBegin == pindexLastGetBlockThinsBegin && hashEnd == hashLastGetBlocksEnd)
        return;

    pindexLastGetBlockThinsBegin = pindexBegin;
    hashLastGetBlocksEnd = hashEnd;

    PushMessage("getheaders", CBlockThinLocator(pindexBegin), hashEnd);
};



// find 'best' local address for a particular peer
bool GetLocal(CService& addr, const CNetAddr *paddrPeer)
{
    if (fNoListen)
        return false;

    int nBestScore = -1;
    int nBestReachability = -1;
    int nBestTorScore = -1;
    {
        LOCK(cs_mapLocalHost);
        for (map<CNetAddr, LocalServiceInfo>::iterator it = mapLocalHost.begin(); it != mapLocalHost.end(); it++)
        {
            int nScore = (*it).second.nScore;
            int nReachability = (*it).first.GetReachabilityFrom(paddrPeer);
            int nTorScore = (*it).first.IsTorV3() ? 1 : 0;
            if (nReachability > nBestReachability || (nReachability == nBestReachability && (nTorScore > nBestTorScore || (nTorScore == nBestTorScore && nScore > nBestScore))))
            {
                addr = CService((*it).first, (*it).second.nPort);
                nBestReachability = nReachability;
                nBestScore = nScore;
                nBestTorScore = nTorScore;
            }
        }
    }
    return nBestScore >= 0;
}

// get best local address for a particular peer as a CAddress
// Otherwise, return the unroutable 0.0.0.0 but filled in with
// the normal parameters, since the IP may be changed to a useful
// one by discovery.
CAddress GetLocalAddress(const CNetAddr *paddrPeer)
{
    CAddress ret(CService("0.0.0.0",GetListenPort()),0);
    CService addr;
    if (GetLocal(addr, paddrPeer))
    {
        ret = CAddress(addr);
    }
    ret.nServices = nLocalServices;
    ret.nTime = GetAdjustedTime();
    return ret;
}

bool RecvLine(SOCKET hSocket, string& strLine)
{
    strLine = "";
    while (true)
    {
        char c;
        int nBytes = recv(hSocket, &c, 1, 0);
        if (nBytes > 0)
        {
            if (c == '\n')
                continue;
            if (c == '\r')
                return true;
            strLine += c;
            if (strLine.size() >= 9000)
                return true;
        }
        else if (nBytes <= 0)
        {
            boost::this_thread::interruption_point();
            if (nBytes < 0)
            {
                int nErr = WSAGetLastError();
                if (nErr == WSAEMSGSIZE)
                    continue;
                if (nErr == WSAEWOULDBLOCK || nErr == WSAEINTR || nErr == WSAEINPROGRESS)
                {
                    MilliSleep(10);
                    continue;
                }
            }
            if (!strLine.empty())
                return true;
            if (nBytes == 0)
            {
                // socket closed
                LogPrint("net", "socket closed\n");
                return false;
            }
            else
            {
                // socket error
                int nErr = WSAGetLastError();
                LogPrint("net", "recv failed: %d\n", nErr);
                return false;
            }
        }
    }
}

int GetnScore(const CService& addr)
{
    LOCK(cs_mapLocalHost);
    if (mapLocalHost.count(addr) == LOCAL_NONE)
        return 0;
    return mapLocalHost[addr].nScore;
}

// Is our peer's addrLocal potentially useful as an external IP source?
bool IsPeerAddrLocalGood(CNode *pnode)
{
    return fDiscover && pnode->addr.IsRoutable() && pnode->addrLocal.IsRoutable() &&
           !IsLimited(pnode->addrLocal.GetNetwork());
}

// used when scores of local addresses may have changed
// pushes better local address to peers
void static AdvertizeLocal()
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if (pnode->fSuccessfullyConnected)
        {
            CAddress addrLocal = GetLocalAddress(&pnode->addr);
            if (addrLocal.IsRoutable() && (CService)addrLocal != (CService)pnode->addrLocal)
            {
                pnode->PushAddress(addrLocal);
                pnode->addrLocal = addrLocal;
            };
        };
    };
}


void SetReachable(enum Network net, bool fFlag)
{
    LOCK(cs_mapLocalHost);
    vfReachable[net] = fFlag;
    if (net == NET_IPV6 && fFlag)
        vfReachable[NET_IPV4] = true;
}

// learn a new local address
bool AddLocal(const CService& addr, int nScore)
{
    if (!addr.IsRoutable())
        return false;

    if (!fDiscover && nScore < LOCAL_MANUAL)
        return false;

    if (IsLimited(addr))
        return false;

    LogPrintf("AddLocal(%s,%i)\n", addr.ToString(), nScore);

    {
        LOCK(cs_mapLocalHost);
        bool fAlready = mapLocalHost.count(addr) > 0;
        LocalServiceInfo &info = mapLocalHost[addr];
        if (!fAlready || nScore >= info.nScore) {
            info.nScore = nScore + (fAlready ? 1 : 0);
            info.nPort = addr.GetPort();
        }
        SetReachable(addr.GetNetwork());
    }

    AdvertizeLocal();

    return true;
}

bool AddLocal(const CNetAddr &addr, int nScore)
{
    return AddLocal(CService(addr, GetListenPort()), nScore);
}

/** Make a particular network entirely off-limits (no automatic connects to it) */
void SetLimited(enum Network net, bool fLimited)
{
    if (net == NET_UNROUTABLE)
        return;
    LOCK(cs_mapLocalHost);
    vfLimited[net] = fLimited;
}

bool IsLimited(enum Network net)
{
    LOCK(cs_mapLocalHost);
    return vfLimited[net];
}

bool IsLimited(const CNetAddr &addr)
{
    return IsLimited(addr.GetNetwork());
}

/** vote for a local address */
bool SeenLocal(const CService& addr)
{
    {
        LOCK(cs_mapLocalHost);
        if (mapLocalHost.count(addr) == 0)
            return false;
        mapLocalHost[addr].nScore++;
    }

    AdvertizeLocal();

    return true;
}

/** check whether a given address is potentially local */
bool IsLocal(const CService& addr)
{
    LOCK(cs_mapLocalHost);
    return mapLocalHost.count(addr) > 0;
}

/** check whether a given address is in a network we can probably connect to */
bool IsReachable(enum Network net)
{
    LOCK(cs_mapLocalHost);
    return vfReachable[net] && !vfLimited[net];
}

bool IsReachable(const CNetAddr& addr)
{
    LOCK(cs_mapLocalHost);
    enum Network net = addr.GetNetwork();
    return vfReachable[net] && !vfLimited[net];
}

void AddressCurrentlyConnected(const CService& addr)
{
    addrman.Connected(addr);
}


uint64_t CNode::nTotalBytesRecv = 0;
uint64_t CNode::nTotalBytesSent = 0;
CCriticalSection CNode::cs_totalBytesRecv;
CCriticalSection CNode::cs_totalBytesSent;

CNode* FindNode(const CNetAddr& ip)
{
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            if ((CNetAddr)pnode->addr == ip)
                return (pnode);
    }
    return NULL;
}

CNode* FindNode(const std::string& addrName)
{
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
        if (pnode->addrName == addrName)
            return (pnode);
    return NULL;
}

CNode* FindNode(const CService& addr)
{
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            if ((CService)pnode->addr == addr)
                return (pnode);
    }
    return NULL;
}

CNode* ConnectNode(CAddress addrConnect, const char *pszDest)
{
    // ConnectNode is called from multiple threads (ThreadOpenConnections, ThreadOpenAddedConnections)
    // each thread must run  duplicate, connect and push to vNodes together
    // or it's possible to connect to the same peer twice
    // don't want to lock cs_vNodes as ConnectSocket may be slow
    LOCK(cs_connectNode);

    if (pszDest == NULL) {

        if (IsLocal(addrConnect))
            return NULL;

        // Look for an existing connection
        CNode* pnode = FindNode((CService)addrConnect);
        if (pnode)
        {
            pnode->AddRef();
            return pnode;
        }
    }


    /// debug print
    LogPrint("net", "trying connection %s lastseen=%.1fhrs\n",
        pszDest ? pszDest : addrConnect.ToString(),
        pszDest ? 0 : (double)(GetAdjustedTime() - addrConnect.nTime)/3600.0);

    // Connect
    SOCKET hSocket;
    bool proxyConnectionFailed = false;
    if (pszDest ? ConnectSocketByName(addrConnect, hSocket, pszDest, Params().GetDefaultPort(), nConnectTimeout, &proxyConnectionFailed) :
                  ConnectSocket(addrConnect, hSocket, nConnectTimeout, &proxyConnectionFailed))
    {
        addrman.Attempt(addrConnect);

        LogPrint("net", "connected %s\n", pszDest ? pszDest : addrConnect.ToString());

        // Set to non-blocking
#ifdef WIN32
        u_long nOne = 1;
        if (ioctlsocket(hSocket, FIONBIO, &nOne) == SOCKET_ERROR)
            LogPrintf("ConnectSocket() : ioctlsocket non-blocking setting failed, error %d\n", WSAGetLastError());
#else
        if (fcntl(hSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
            LogPrintf("ConnectSocket() : fcntl non-blocking setting failed, error %d\n", errno);
#endif

        // Add node
        CNode* pnode = new CNode(hSocket, addrConnect, pszDest ? pszDest : "", false);
        pnode->AddRef();

        {
            LOCK(cs_vNodes);
            vNodes.push_back(pnode);
        }

        pnode->nTimeConnected = GetTime();
        return pnode;
    } else if (!proxyConnectionFailed) {
        // If connecting to the node failed, and failure is not caused by a problem connecting to
        // the proxy, mark this as an attempt.
        addrman.Attempt(addrConnect);
    }

    return NULL;
}

void CNode::CloseSocketDisconnect()
{
    fDisconnect = true;
    if (hSocket != INVALID_SOCKET)
    {
        LogPrint("net", "disconnecting node %s\n", addrName);
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
    }

    // in case this fails, we'll empty the recv buffer when the CNode is deleted
    TRY_LOCK(cs_vRecvMsg, lockRecv);
    if (lockRecv)
        vRecvMsg.clear();

    // if this was the sync node, we'll need a new one
    if (this == pnodeSync)
        pnodeSync = NULL;
}

void CNode::Cleanup()
{
}

void CNode::TryFlushSend()
{
    for (int i = 0; i < 3; ++i)
    {
        TRY_LOCK(cs_vSend, lockSend);
        if (lockSend)
        {
            SocketSendData(this);
            break;
        };
        MilliSleep(10);
    };
}


void CNode::PushVersion()
{
    /// when NTP implemented, change to just nTime = GetAdjustedTime()
    int64_t nTime = (fInbound ? GetAdjustedTime() : GetTime());
    CAddress addrYou = (addr.IsRoutable() && !IsProxy(addr) ? addr : CAddress(CService("0.0.0.0",0)));
    CAddress addrMe = GetLocalAddress(&addr);
    RAND_bytes((unsigned char*)&nLocalHostNonce, sizeof(nLocalHostNonce));
    LogPrint("net", "send version message: version %d, blocks=%d, us=%s, them=%s, peer=%s\n", PROTOCOL_VERSION, nBestHeight, addrMe.ToString(), addrYou.ToString(), addr.ToString());

    // -- node requirements are packed into the top 32 bits of nServices
    //uint64_t nServices = ((uint64_t) nLocalRequirements) << 32 | nLocalServices;
    uint64_t nServices = nLocalServices;
    unsigned char *p = (unsigned char*)&nServices; // fortify
    memcpy(p+4, &nLocalRequirements, 4);

    PushMessage("version", PROTOCOL_VERSION, nServices, nTime, addrYou, addrMe,
                nLocalHostNonce, FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, std::vector<string>()), nBestHeight, nNodeMode);
}





std::map<CNetAddr, int64_t> CNode::setBanned;
CCriticalSection CNode::cs_setBanned;

void CNode::ClearBanned()
{
    setBanned.clear();
}

bool CNode::IsBanned(CNetAddr ip)
{
    bool fResult = false;
    {
        LOCK(cs_setBanned);
        std::map<CNetAddr, int64_t>::iterator i = setBanned.find(ip);
        if (i != setBanned.end())
        {
            int64_t t = (*i).second;
            if (GetTime() < t)
                fResult = true;
        }
    }
    return fResult;
}

bool CNode::Misbehaving(int howmuch)
{
    nMisbehavior += howmuch;
    if (nMisbehavior >= GetArg("-banscore", 100))
    {
        if (addr.IsLocal())
        {
            LogPrintf("Misbehaving: %s (%d -> %d) DISCONNECTING (Warning: local addr not banned)\n", addr.ToString(), nMisbehavior-howmuch, nMisbehavior);
        }
        else {
            int64_t banTime = GetTime()+GetArg("-bantime", 60*60*24);  // Default 24-hour ban
            LogPrintf("Misbehaving: %s (%d -> %d) DISCONNECTING\n", addr.ToString(), nMisbehavior-howmuch, nMisbehavior);
            {
                LOCK(cs_setBanned);
                if (setBanned[addr] < banTime)
                    setBanned[addr] = banTime;
            }
        }
        CloseSocketDisconnect();
        return true;
    } else
        LogPrintf("Misbehaving: %s (%d -> %d)\n", addr.ToString(), nMisbehavior-howmuch, nMisbehavior);
    return false;
}

bool CNode::SoftBan()
{
    // -- same as Misbehaving, but a shorter ban time
    {
        if (addr.IsLocal())
        {
            LogPrintf("SoftBan: %s DISCONNECTING  (Warning: local addr not banned)\n", addr.ToString().c_str());
        }
        else {
            int64_t banTime = GetTime()+GetArg("-softbantime", 60*60*1);  // Default 1-hour ban
            LogPrintf("SoftBan: %s DISCONNECTING\n", addr.ToString().c_str());
            LOCK(cs_setBanned);
            if (setBanned[addr] < banTime)
                setBanned[addr] = banTime;
        }
    }

    CloseSocketDisconnect();

    return true;
}

#undef X
#define X(name) stats.name = name
void CNode::copyStats(CNodeStats &stats)
{
    stats.nodeid = this->GetId();
    X(nServices);
    X(nLastSend);
    X(nLastRecv);
    X(nTimeConnected);
    X(nTimeOffset);
    X(addrName);
    X(nVersion);
    X(nTypeInd);
    X(strSubVer);
    X(fInbound);
    X(nChainHeight);
    X(nMisbehavior);
    X(nSendBytes);
    X(nRecvBytes);
    stats.fSyncNode = (this == pnodeSync);

    // It is common for nodes with good ping times to suddenly become lagged,
    // due to a new block arriving or other large transfer.
    // Merely reporting pingtime might fool the caller into thinking the node was still responsive,
    // since pingtime does not update until the ping is complete, which might take a while.
    // So, if a ping is taking an unusually long time in flight,
    // the caller can immediately detect that this is happening.
    int64_t nPingUsecWait = 0;
    if ((0 != nPingNonceSent) && (0 != nPingUsecStart)) {
        nPingUsecWait = GetTimeMicros() - nPingUsecStart;
    }

    // Raw ping time is in microseconds, but show it to user as whole seconds (Bitcoin users should be well used to small numbers with many decimal places by now :)
    stats.dPingTime = (((double)nPingUsecTime) / 1e6);
    stats.dPingWait = (((double)nPingUsecWait) / 1e6);

    // Leave string empty if addrLocal invalid (not filled in yet)
    stats.addrLocal = addrLocal.IsValid() ? addrLocal.ToString() : "";
}
#undef X

// requires LOCK(cs_vRecvMsg)
bool CNode::ReceiveMsgBytes(const char *pch, unsigned int nBytes)
{
    while (nBytes > 0) {

        // get current incomplete message, or create a new one
        if (vRecvMsg.empty() ||
            vRecvMsg.back().complete())
            vRecvMsg.push_back(CNetMessage(SER_NETWORK, nRecvVersion));

        CNetMessage& msg = vRecvMsg.back();

        // absorb network data
        int handled;
        if (!msg.in_data)
            handled = msg.readHeader(pch, nBytes);
        else
            handled = msg.readData(pch, nBytes);

        if (handled < 0)
            return false;

        pch += handled;
        nBytes -= handled;

        if (msg.complete())
            msg.nTime = GetTimeMicros();
    }

    return true;
}

int CNetMessage::readHeader(const char *pch, unsigned int nBytes)
{
    // copy data to temporary parsing buffer
    unsigned int nRemaining = 24 - nHdrPos;
    unsigned int nCopy = std::min(nRemaining, nBytes);

    memcpy(&hdrbuf[nHdrPos], pch, nCopy);
    nHdrPos += nCopy;

    // if header incomplete, exit
    if (nHdrPos < 24)
        return nCopy;

    // deserialize to CMessageHeader
    try {
        hdrbuf >> hdr;
    }
    catch (std::exception &e) {
        return -1;
    }

    // reject messages larger than MAX_SIZE
    if (hdr.nMessageSize > MAX_SIZE)
        return -1;

    // switch state to reading message data
    in_data = true;

    return nCopy;
}

int CNetMessage::readData(const char *pch, unsigned int nBytes)
{
    unsigned int nRemaining = hdr.nMessageSize - nDataPos;
    unsigned int nCopy = std::min(nRemaining, nBytes);

    if (vRecv.size() < nDataPos + nCopy) {
        // Allocate up to 256 KiB ahead, but never more than the total message size.
        vRecv.resize(std::min(hdr.nMessageSize, nDataPos + nCopy + 256 * 1024));
    }

    memcpy(&vRecv[nDataPos], pch, nCopy);
    nDataPos += nCopy;

    return nCopy;
}









// requires LOCK(cs_vSend)
void SocketSendData(CNode *pnode)
{
    std::deque<CSerializeData>::iterator it = pnode->vSendMsg.begin();

    while (it != pnode->vSendMsg.end()) {
        const CSerializeData &data = *it;
        assert(data.size() > pnode->nSendOffset);
        int nBytes = send(pnode->hSocket, &data[pnode->nSendOffset], data.size() - pnode->nSendOffset, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (nBytes > 0) {
            pnode->nLastSend = GetTime();
            pnode->nSendBytes += nBytes;
            pnode->nSendOffset += nBytes;
            pnode->RecordBytesSent(nBytes);
            if (pnode->nSendOffset == data.size()) {
                pnode->nSendOffset = 0;
                pnode->nSendSize -= data.size();
                it++;
            } else {
                // could not send full message; stop sending more
                break;
            }
        } else {
            if (nBytes < 0) {
                // error
                int nErr = WSAGetLastError();
                if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
                {
                    LogPrintf("socket send error %d\n", nErr);
                    pnode->CloseSocketDisconnect();
                }
            }
            // couldn't send anything at all
            break;
        }
    }

    if (it == pnode->vSendMsg.end()) {
        assert(pnode->nSendOffset == 0);
        assert(pnode->nSendSize == 0);
    }
    pnode->vSendMsg.erase(pnode->vSendMsg.begin(), it);
}

static list<CNode*> vNodesDisconnected;

void ThreadSocketHandler()
{
    unsigned int nPrevNodeCount = 0;

    while (true)
    {
        //
        // Disconnect nodes
        //
        ThreadSocketHandler_DisconnectNodes();
        if(vNodes.size() != nPrevNodeCount) {
            nPrevNodeCount = vNodes.size();
            uiInterface.NotifyNumConnectionsChanged(nPrevNodeCount);
        }


        //
        // Find which sockets have data to receive
        //
        struct timeval timeout;
        timeout.tv_sec  = 0;
        timeout.tv_usec = 50000; // frequency to poll pnode->vSend

        fd_set fdsetRecv;
        fd_set fdsetSend;
        fd_set fdsetError;
        FD_ZERO(&fdsetRecv);
        FD_ZERO(&fdsetSend);
        FD_ZERO(&fdsetError);
        SOCKET hSocketMax = 0;
        bool have_fds = false;

        BOOST_FOREACH(SOCKET hListenSocket, vhListenSocket) {
            FD_SET(hListenSocket, &fdsetRecv);
            hSocketMax = max(hSocketMax, hListenSocket);
            have_fds = true;
        }
        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
            {
                if (pnode->hSocket == INVALID_SOCKET)
                    continue;
                {
                    TRY_LOCK(pnode->cs_vSend, lockSend);
                    if (lockSend) {
                        // do not read, if draining write queue
                        if (!pnode->vSendMsg.empty())
                            FD_SET(pnode->hSocket, &fdsetSend);
                        else
                            FD_SET(pnode->hSocket, &fdsetRecv);
                        FD_SET(pnode->hSocket, &fdsetError);
                        hSocketMax = max(hSocketMax, pnode->hSocket);
                        have_fds = true;
                    }
                }
            }
        }

        int nSelect = select(have_fds ? hSocketMax + 1 : 0,
                             &fdsetRecv, &fdsetSend, &fdsetError, &timeout);
        boost::this_thread::interruption_point();

        if (nSelect == SOCKET_ERROR)
        {
            if (have_fds)
            {
                int nErr = WSAGetLastError();
                LogPrintf("socket select error %d\n", nErr);
                for (unsigned int i = 0; i <= hSocketMax; i++)
                    FD_SET(i, &fdsetRecv);
            }
            FD_ZERO(&fdsetSend);
            FD_ZERO(&fdsetError);
            MilliSleep(timeout.tv_usec/1000);
        }


        //
        // Accept new connections
        //
        BOOST_FOREACH(SOCKET hListenSocket, vhListenSocket)
        if (hListenSocket != INVALID_SOCKET && FD_ISSET(hListenSocket, &fdsetRecv))
        {
            struct sockaddr_storage sockaddr;
            socklen_t len = sizeof(sockaddr);
            SOCKET hSocket = accept(hListenSocket, (struct sockaddr*)&sockaddr, &len);
            CAddress addr;
            int nInbound = 0;

            if (hSocket != INVALID_SOCKET)
                if (!addr.SetSockAddr((const struct sockaddr*)&sockaddr))
                    LogPrintf("Warning: Unknown socket family\n");

            {
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes)
                    if (pnode->fInbound)
                        nInbound++;
            }

            if (hSocket == INVALID_SOCKET)
            {
                int nErr = WSAGetLastError();
                if (nErr != WSAEWOULDBLOCK)
                    LogPrintf("socket error accept failed: %d\n", nErr);
            }
            else if (nInbound >= GetArg("-maxconnections", 125) - MAX_OUTBOUND_CONNECTIONS)
            {
                closesocket(hSocket);
            }
            else if (CNode::IsBanned(addr))
            {
                LogPrintf("connection from %s dropped (banned)\n", addr.ToString());
                closesocket(hSocket);
            }
            else
            {
                LogPrint("net", "accepted connection %s\n", addr.ToString());
                CNode* pnode = new CNode(hSocket, addr, "", true);
                pnode->AddRef();
                {
                    LOCK(cs_vNodes);
                    vNodes.push_back(pnode);
                }
            }
        }


        //
        // Service each socket
        //
        vector<CNode*> vNodesCopy;
        {
            LOCK(cs_vNodes);
            vNodesCopy = vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->AddRef();
        }
        BOOST_FOREACH(CNode* pnode, vNodesCopy)
        {
            boost::this_thread::interruption_point();

            //
            // Receive
            //
            if (pnode->hSocket == INVALID_SOCKET)
                continue;
            if (FD_ISSET(pnode->hSocket, &fdsetRecv) || FD_ISSET(pnode->hSocket, &fdsetError))
            {
                TRY_LOCK(pnode->cs_vRecvMsg, lockRecv);
                if (lockRecv)
                {
                    if (pnode->GetTotalRecvSize() > ReceiveFloodSize()) {
                        if (!pnode->fDisconnect)
                            LogPrintf("socket recv flood control disconnect (%u bytes)\n", pnode->GetTotalRecvSize());
                        pnode->CloseSocketDisconnect();
                    }
                    else {
                        // typical socket buffer is 8K-64K
                        char pchBuf[0x10000];
                        int nBytes = recv(pnode->hSocket, pchBuf, sizeof(pchBuf), MSG_DONTWAIT);
                        if (nBytes > 0)
                        {
                            if (!pnode->ReceiveMsgBytes(pchBuf, nBytes))
                                pnode->CloseSocketDisconnect();
                            pnode->nLastRecv = GetTime();
                            pnode->nRecvBytes += nBytes;
                            pnode->RecordBytesRecv(nBytes);
                        }
                        else if (nBytes == 0)
                        {
                            // socket closed gracefully
                            if (!pnode->fDisconnect)
                                LogPrint("net", "socket closed\n");
                            pnode->CloseSocketDisconnect();
                        }
                        else if (nBytes < 0)
                        {
                            // error
                            int nErr = WSAGetLastError();
                            if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS)
                            {
                                if (!pnode->fDisconnect)
                                    LogPrintf("socket recv error %d\n", nErr);
                                pnode->CloseSocketDisconnect();
                            }
                        }
                    }
                }
            }

            //
            // Send
            //
            if (pnode->hSocket == INVALID_SOCKET)
                continue;
            if (FD_ISSET(pnode->hSocket, &fdsetSend))
            {
                TRY_LOCK(pnode->cs_vSend, lockSend);
                if (lockSend)
                    SocketSendData(pnode);
            }

            //
            // Inactivity checking
            //
            int64_t nTime = GetTime();
            if (nTime - pnode->nTimeConnected > 60)
            {
                if (pnode->nLastRecv == 0 || pnode->nLastSend == 0)
                {
                    LogPrint("net", "socket no message in first 60 seconds, %d %d\n", pnode->nLastRecv != 0, pnode->nLastSend != 0);
                    pnode->fDisconnect = true;
                }
                else if (nTime - pnode->nLastSend > TIMEOUT_INTERVAL)
                {
                    LogPrintf("socket sending timeout: %ds\n", nTime - pnode->nLastSend);
                    pnode->fDisconnect = true;
                }
				else if (nTime - pnode->nLastRecv > TIMEOUT_INTERVAL)
                {
                    LogPrintf("socket receive timeout: %ds\n", nTime - pnode->nLastRecv);
                    pnode->fDisconnect = true;
                }
                else if (pnode->nPingNonceSent && pnode->nPingUsecStart + TIMEOUT_INTERVAL * 1000000 < GetTimeMicros())
                {
                    LogPrintf("ping timeout: %fs\n", 0.000001 * (GetTimeMicros() - pnode->nPingUsecStart));
                    pnode->fDisconnect = true;
                }
            }
        }
        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->Release();
        }

        MilliSleep(100);
    } // main loop
}

void ThreadSocketHandler_DisconnectNodes()
{
    //
    // Disconnect nodes
    //
    {
        LOCK(cs_vNodes);
        // Disconnect unused nodes
        vector<CNode*> vNodesCopy = vNodes;
        BOOST_FOREACH(CNode* pnode, vNodesCopy)
        {
            if (pnode->fDisconnect ||
                (pnode->GetRefCount() <= 0 && pnode->vRecvMsg.empty() && pnode->nSendSize == 0 && pnode->ssSend.empty()))
            {
                // remove from vNodes
                vNodes.erase(remove(vNodes.begin(), vNodes.end(), pnode), vNodes.end());

                // release outbound grant (if any)
                pnode->grantOutbound.Release();

                // close socket and cleanup
                pnode->CloseSocketDisconnect();
                pnode->Cleanup();

                // hold in disconnected pool until all refs are released
                if (pnode->fNetworkNode || pnode->fInbound)
                    pnode->Release();
                vNodesDisconnected.push_back(pnode);
            }
        }
    }
    {
        // Delete disconnected nodes
        list<CNode*> vNodesDisconnectedCopy = vNodesDisconnected;
        BOOST_FOREACH(CNode* pnode, vNodesDisconnectedCopy)
        {
            // wait until threads are done using it
            if (pnode->GetRefCount() <= 0)
            {
                bool fDelete = false;
                {
                    TRY_LOCK(pnode->cs_vSend, lockSend);
                    if (lockSend)
                    {
                        TRY_LOCK(pnode->cs_vRecvMsg, lockRecv);
                        if (lockRecv)
                        {
                            TRY_LOCK(pnode->cs_inventory, lockInv);
                            if (lockInv)
                                fDelete = true;
                        }
                    }
                }
                if (fDelete)
                {
                    vNodesDisconnected.remove(pnode);
                    delete pnode;
                }
            }
        }
    }
}


/* Tor implementation ---------------------------------*/

// hidden service seeds
static const char *strMainNetOnionSeed[][1] = {
    // project-maintained nodes
    {"gji6yid3u2gozq5hvoufkfyybcvj3ftajvrlhmjjrtzv42o7oeosllqd.onion"},
    {"nounwopyxdcwe72v5xey5jgjf6ypsmyitzolambc7c4vgvheyquekmyd.onion"},
    {"zhn7wjfhk232kkmb.onion"},
	{"qe6swgcfktc5l3l7.onion"},
	{"r5mk35ekwr6j7ccb.onion"},

    // other stable nodes that are monitored by the project
    {NULL}
};

static const char *strTestNetOnionSeed[][1] = {
    // project-maintained nodes
    {"f3kcqb22tae4twfvd6lrsanwp4ikfocltsuo4onsjdapcsgatoyw7oid.onion"},
    {"iw5cbprcpm2md7l2.onion"},
    {"glyxixwz4uk6n7w6.onion"},
    {"almrpkxuhk2r35sw.onion"},
    {"vwxv3bstufb7kfsr.onion"},
    {NULL}
};

void ThreadOnionSeed()
{

    // Make this thread recognisable as the tor thread
    RenameThread("onionseed");

    static const char *(*strOnionSeed)[1] = fTestNet ? strTestNetOnionSeed : strMainNetOnionSeed;

    int found = 0;

    LogPrintf("Loading addresses from .onion seeds\n");

    for (unsigned int seed_idx = 0; strOnionSeed[seed_idx][0] != NULL; seed_idx++) {
        CNetAddr parsed;
        if (
            !parsed.SetSpecial(
                strOnionSeed[seed_idx][0]
            )
        ) {
            throw runtime_error("ThreadOnionSeed() : invalid .onion seed");
        }
        int nOneDay = 24*3600;
        CAddress addr = CAddress(CService(parsed, Params().GetDefaultPort()));
        addr.nTime = GetTime() - 3*nOneDay - GetRand(4*nOneDay); // use a random age between 3 and 7 days old
        found++;
        addrman.Add(addr, parsed);
    }

    LogPrintf("%d addresses found from .onion seeds\n", found);
}


/* void ThreadDNSAddressSeed()
{
    // goal: only query DNS seeds if address need is acute
    if ((addrman.size() > 0) &&
        (!GetBoolArg("-forcednsseed", false))) {
        MilliSleep(11 * 1000);

        LOCK(cs_vNodes);
        if (vNodes.size() >= 2) {
            LogPrintf("P2P peers available. Skipped DNS seeding.\n");
            return;
        }
    }

    const vector<CDNSSeedData> &vSeeds = Params().DNSSeeds();
    int found = 0;

    LogPrintf("Loading addresses from DNS seeds (could take a while)\n");

    BOOST_FOREACH(const CDNSSeedData &seed, vSeeds) {
        if (HaveNameProxy()) {
            AddOneShot(seed.host);
        } else {
            vector<CNetAddr> vIPs;
            vector<CAddress> vAdd;
            if (LookupHost(seed.host.c_str(), vIPs))
            {
                BOOST_FOREACH(CNetAddr& ip, vIPs)
                {
                    int nOneDay = 24*3600;
                    CAddress addr = CAddress(CService(ip, Params().GetDefaultPort()));
                    addr.nTime = GetTime() - 3*nOneDay - GetRand(4*nOneDay); // use a random age between 3 and 7 days old
                    vAdd.push_back(addr);
                    found++;
                }
            }
            addrman.Add(vAdd, CNetAddr(seed.name, true));
        }
    }

    LogPrintf("%d addresses found from DNS seeds\n", found);
} */

void DumpAddresses()
{
    int64_t nStart = GetTimeMillis();

    CAddrDB adb;
    adb.Write(addrman);

    LogPrint("net", "Flushed %d addresses to peers.dat  %dms\n",
           addrman.size(), GetTimeMillis() - nStart);
}

void static ProcessOneShot()
{
    string strDest;
    {
        LOCK(cs_vOneShots);
        if (vOneShots.empty())
            return;
        strDest = vOneShots.front();
        vOneShots.pop_front();
    }
    CAddress addr;
    CSemaphoreGrant grant(*semOutbound, true);
    if (grant) {
        if (!OpenNetworkConnection(addr, &grant, strDest.c_str(), true))
            AddOneShot(strDest);
    }
}

void ThreadOpenConnections()
{
    // Connect to specific addresses
    if (mapArgs.count("-connect") && mapMultiArgs["-connect"].size() > 0)
    {
        for (int64_t nLoop = 0;; nLoop++)
        {
            ProcessOneShot();
            BOOST_FOREACH(string strAddr, mapMultiArgs["-connect"])
            {
                CAddress addr;
                OpenNetworkConnection(addr, NULL, strAddr.c_str());
                for (int i = 0; i < 10 && i < nLoop; i++)
                {
                    MilliSleep(500);
                }
            }
            MilliSleep(500);
        }
    }

    // Initiate network connections
    int64_t nStart = GetTime();
    while (true)
    {
        ProcessOneShot();

        MilliSleep(500);

        CSemaphoreGrant grant(*semOutbound);
        boost::this_thread::interruption_point();

        // Add seed nodes if DNS seeds are all down (an infrastructure attack?).
        if (addrman.size() == 0 && (GetTime() - nStart > 60)) {
            static bool done = false;
            if (!done) {
                LogPrintf("Adding fixed seed nodes as DNS doesn't seem to be available.\n");
                addrman.Add(Params().FixedSeeds(), CNetAddr("127.0.0.1"));
                done = true;
            }
        }

        //
        // Choose an address to connect to based on most recently seen
        //
        CAddress addrConnect;

        // Only connect out to one peer per network group (/16 for IPv4).
        // Do this here so we don't have to critsect vNodes inside mapAddresses critsect.
        int nOutbound = 0;
        set<vector<unsigned char> > setConnected;
        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes) {
                if (!pnode->fInbound) {
                    setConnected.insert(pnode->addr.GetGroup());
                    nOutbound++;
                }
            }
        }

        int64_t nANow = GetAdjustedTime();

        int nTries = 0;
        while (true)
        {
            boost::this_thread::interruption_point();
            // use an nUnkBias between 10 (no outgoing connections) and 90 (8 outgoing connections)
            CAddress addr = addrman.Select(10 + min(nOutbound,8)*10);

            // if we selected an invalid address, restart
            if (!addr.IsValid() || setConnected.count(addr.GetGroup()) || IsLocal(addr))
                break;

            // If we didn't find an appropriate destination after trying 100 addresses fetched from addrman,
            // stop this loop, and let the outer loop run again (which sleeps, adds seed nodes, recalculates
            // already-connected network ranges, ...) before trying new addrman addresses.
            nTries++;
            if (nTries > 100)
                break;

            if (IsLimited(addr))
                continue;

            // only consider very recently tried nodes after 30 failed attempts
            if (nANow - addr.nLastTry < 600 && nTries < 30)
                continue;

            // do not allow non-default ports, unless after 50 invalid addresses selected already
            if (addr.GetPort() != Params().GetDefaultPort() && nTries < 50)
                continue;

            addrConnect = addr;
            break;
        }

        if (addrConnect.IsValid())
            OpenNetworkConnection(addrConnect, &grant);
    }
}

void ThreadOpenAddedConnections()
{
    {
        LOCK(cs_vAddedNodes);
        vAddedNodes = mapMultiArgs["-addnode"];
    }

    if (HaveNameProxy()) {
        while(true) {
            list<string> lAddresses(0);
            {
                LOCK(cs_vAddedNodes);
                BOOST_FOREACH(string& strAddNode, vAddedNodes)
                    lAddresses.push_back(strAddNode);
            }
            BOOST_FOREACH(string& strAddNode, lAddresses) {
                CAddress addr;
                CSemaphoreGrant grant(*semOutbound);
                OpenNetworkConnection(addr, &grant, strAddNode.c_str());
                MilliSleep(500);
            }
            MilliSleep(120000); // Retry every 2 minutes
        }
    }

    for (unsigned int i = 0; true; i++)
    {
        list<string> lAddresses(0);
        {
            LOCK(cs_vAddedNodes);
            BOOST_FOREACH(string& strAddNode, vAddedNodes)
                lAddresses.push_back(strAddNode);
        }

        list<vector<CService> > lservAddressesToAdd(0);
        BOOST_FOREACH(string& strAddNode, lAddresses)
        {
            vector<CService> vservNode(0);
            if(Lookup(strAddNode.c_str(), vservNode, Params().GetDefaultPort(), fNameLookup, 0))
            {
                lservAddressesToAdd.push_back(vservNode);
                {
                    LOCK(cs_setservAddNodeAddresses);
                    BOOST_FOREACH(CService& serv, vservNode)
                        setservAddNodeAddresses.insert(serv);
                }
            }
        }
        // Attempt to connect to each IP for each addnode entry until at least one is successful per addnode entry
        // (keeping in mind that addnode entries can have many IPs if fNameLookup)
        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
                for (list<vector<CService> >::iterator it = lservAddressesToAdd.begin(); it != lservAddressesToAdd.end(); it++)
                    BOOST_FOREACH(CService& addrNode, *(it))
                        if (pnode->addr == addrNode)
                        {
                            it = lservAddressesToAdd.erase(it);
                            it--;
                            break;
                        }
        }
        BOOST_FOREACH(vector<CService>& vserv, lservAddressesToAdd)
        {
            CSemaphoreGrant grant(*semOutbound);
            OpenNetworkConnection(CAddress(vserv[i % vserv.size()]), &grant);
            MilliSleep(500);
        }
        MilliSleep(120000); // Retry every 2 minutes
    }
}

// if successful, this moves the passed grant to the constructed node
bool OpenNetworkConnection(const CAddress& addrConnect, CSemaphoreGrant *grantOutbound, const char *strDest, bool fOneShot)
{
    //
    // Initiate outbound network connection
    //
    boost::this_thread::interruption_point();

    if (!strDest)
	{
        if (IsLocal(addrConnect) ||
            FindNode((CNetAddr)addrConnect) || CNode::IsBanned(addrConnect) ||
            FindNode(addrConnect.ToStringIPPort().c_str()))
            return false;
    }
	else if (FindNode(strDest))
        return false;

    CNode* pnode = ConnectNode(addrConnect, strDest);
    boost::this_thread::interruption_point();

    if (!pnode)
        return false;
    if (grantOutbound)
        grantOutbound->MoveTo(pnode->grantOutbound);
    pnode->fNetworkNode = true;
    if (fOneShot)
        pnode->fOneShot = true;

    return true;
}


void ThreadMessageHandler()
{
    SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
    while (true)
    {
        boost::this_thread::interruption_point();
        vector<CNode*> vNodesCopy;
        {
            LOCK(cs_vNodes);
            vNodesCopy = vNodes;
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->AddRef();
        } // cs_vNodes

        // Poll the connected nodes for messages
        CNode* pnodeTrickle = NULL;
        if (!vNodesCopy.empty())
            pnodeTrickle = vNodesCopy[GetRand(vNodesCopy.size())];

        bool fSleep = true;

        //BOOST_FOREACH(CNode* pnode, vNodesCopy)

        size_t r = GetRandInt(vNodesCopy.size()-1); // randomise the order
        for (size_t i = 0; i < vNodesCopy.size(); ++i)
        {
            CNode *pnode = vNodesCopy[(i + r) % vNodesCopy.size()];

            if (pnode->fDisconnect)
                continue;

            // Receive messages
            {
                TRY_LOCK(pnode->cs_vRecvMsg, lockRecv);
                if (lockRecv)
                {
                    if (!ProcessMessages(pnode))
                        pnode->CloseSocketDisconnect();

                    if (pnode->nSendSize < SendBufferSize() && (!pnode->vRecvGetData.empty() || (!pnode->vRecvMsg.empty() && pnode->vRecvMsg[0].complete())))
                        fSleep = false;
                }
            } // cs_vRecvMsg

            boost::this_thread::interruption_point();

            // Send messages
            {
                TRY_LOCK(pnode->cs_vSend, lockSend);
                if (lockSend)
                    SendMessages(pnode, vNodesCopy, pnode == pnodeTrickle);
            } // cs_vSend
        };

        {
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodesCopy)
                pnode->Release();
        } // cs_vNodes

        if (fSleep)
            MilliSleep(100);
    };
}






bool BindListenPort(const CService &addrBind, string& strError)
{
    strError = "";
    int nOne = 1;

#ifdef WIN32
    // Initialize Windows Sockets
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (ret != NO_ERROR)
    {
        strError = strprintf("Error: TCP/IP socket library failed to start (WSAStartup returned error %d)", ret);
        LogPrintf("%s\n", strError.c_str());
        return false;
    };
#endif

    // Create socket for listening for incoming connections
    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);
    if (!addrBind.GetSockAddr((struct sockaddr*)&sockaddr, &len))
    {
        strError = strprintf("Error: bind address family for %s not supported", addrBind.ToString());
        LogPrintf("%s\n", strError);
        return false;
    }

    SOCKET hListenSocket = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);
    if (hListenSocket == INVALID_SOCKET)
    {
        strError = strprintf("Error: Couldn't open socket for incoming connections (socket returned error %d)", WSAGetLastError());
        LogPrintf("%s\n", strError);
        return false;
    }

#ifdef SO_NOSIGPIPE
    // Different way of disabling SIGPIPE on BSD
    setsockopt(hListenSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&nOne, sizeof(int));
#endif

#ifndef WIN32
    // Allow binding if the port is still in TIME_WAIT state after
    // the program was closed and restarted.  Not an issue on windows.
    setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&nOne, sizeof(int));
#endif


#ifdef WIN32
    // Set to non-blocking, incoming connections will also inherit this
    if (ioctlsocket(hListenSocket, FIONBIO, (u_long*)&nOne) == SOCKET_ERROR)
#else
    if (fcntl(hListenSocket, F_SETFL, O_NONBLOCK) == SOCKET_ERROR)
#endif
    {
        strError = strprintf("Error: Couldn't set properties on socket for incoming connections (error %d)", WSAGetLastError());
        LogPrintf("%s\n", strError);
        return false;
    }

    // some systems don't have IPV6_V6ONLY but are always v6only; others do have the option
    // and enable it by default or not. Try to enable it, if possible.
    if (addrBind.IsIPv6()) {
#ifdef IPV6_V6ONLY
#ifdef WIN32
        setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&nOne, sizeof(int));
#else
        setsockopt(hListenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&nOne, sizeof(int));
#endif
#endif
#ifdef WIN32
        int nProtLevel = 10 /* PROTECTION_LEVEL_UNRESTRICTED */;
        int nParameterId = 23 /* IPV6_PROTECTION_LEVEl */;
        // this call is allowed to fail
        setsockopt(hListenSocket, IPPROTO_IPV6, nParameterId, (const char*)&nProtLevel, sizeof(int));
#endif
    }

    if (::bind(hListenSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
    {
        int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE)
            strError = strprintf(_("Unable to bind to %s on this computer. Alias is probably already running."), addrBind.ToString());
        else
            strError = strprintf(_("Unable to bind to %s on this computer (bind returned error %d, %s)"), addrBind.ToString(), nErr, strerror(nErr));
        LogPrintf("%s\n", strError);
        return false;
    };

    LogPrintf("Bound to %s\n", addrBind.ToString().c_str());

    // Listen for incoming connections
    if (listen(hListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        strError = strprintf("Error: Listening for incoming connections failed (listen returned error %d)", WSAGetLastError());
        LogPrintf("%s\n", strError);
        return false;
    }

    vhListenSocket.push_back(hListenSocket);

    if (addrBind.IsRoutable() && fDiscover)
        AddLocal(addrBind, LOCAL_BIND);

    return true;
}

static char *convert_str(const std::string &s) {
    char *pc = new char[s.size()+1];
    std::strcpy(pc, s.c_str());
    return pc;
}

static std::string quoteArg(const bool& quote, const std::string& arg)
{
    if (quote)
        return "\"" + arg + "\"";
    else
        return arg;
}

/**
 * Run tor as separate process for Windows, macOS and Linux
 * - Windows with windows specific CreateProcess()
 * - macOS with boost::process:child
 * - Linux with fork() and execvp()
 * All other platforms run Tor embedded.
 *
 * @brief run_tor
 */
static void run_tor() {
    fs::path tor_dir = GetDataDir() / "tor";
    fs::create_directory(tor_dir);
    fs::path log_file = tor_dir / "tor.log";

    std::vector<std::string> argv;
    bool bQuoteArg = false;
#if defined(WIN32)
    bQuoteArg = true;
#endif
#ifndef __APPLE__
    // Windows CreateProcess(), exec() and main() when embedding tor need 'tor' as first argument
    // Tor separate process via boost::process does not need 'tor' as first argument
    argv.push_back("tor");
#endif
    argv.push_back("--SocksPort");
        if (TestNet()) {
        argv.push_back(quoteArg(bQuoteArg,"9090 OnionTrafficOnly"));
    } else {
        argv.push_back(quoteArg(bQuoteArg,"9089 OnionTrafficOnly"));
    }
    argv.push_back("--ignore-missing-torrc");
    argv.push_back("-f");
    // Move the location of the torrc to the .spectrecoin folder instead of .spectrecoin/tor
    // Allows to have all config files in one place
    argv.push_back(quoteArg(bQuoteArg, (GetDataDir() / "torrc").string()));
    argv.push_back("--DataDirectory");
    argv.push_back(quoteArg(bQuoteArg, tor_dir.string()));
    argv.push_back("--GeoIPFile");
    argv.push_back(quoteArg(bQuoteArg, (tor_dir / "geoip").string()));
    argv.push_back("--GeoIPv6File");
    argv.push_back(quoteArg(bQuoteArg, (tor_dir / "geoipv6").string()));
    argv.push_back("--HiddenServiceDir");
    argv.push_back(quoteArg(bQuoteArg, (tor_dir / "onion").string()));
    argv.push_back("--HiddenServiceVersion");
    if (GetBoolArg("-onionv2")) {
        argv.push_back("2");
    }
    else {
        argv.push_back("3");
    }
    argv.push_back("--HiddenServicePort");
    if (TestNet()) {
        argv.push_back("37111");
    }
    else {
        argv.push_back("37347");
    }

#if defined(WIN32) || defined(__APPLE__)
    // Tor separate process
    fs::path pathApplicationDir = dll::program_location().parent_path();
    fs::path pathTorDir = pathApplicationDir / "Tor";

    argv.push_back("--defaults-torrc");
    fs::path torrc_defaults_file = pathTorDir / "torrc-defaults";
    argv.push_back(quoteArg(bQuoteArg, torrc_defaults_file.string()));
#elif __linux__

    fs::path torrc_defaults_file = tor_dir / "torrc-defaults";
    bool must_override = false;
    if (fs::exists(torrc_defaults_file) && fs::is_regular_file(torrc_defaults_file))
    {
        FILE* file = fopen(torrc_defaults_file.string().c_str(), "r");
        if (file)
        {
            int check_rev = 0;
            fscanf(file, "# torrc-defaults for Alias rev. %d\n", &check_rev);
            if (check_rev != TORRC_DEFAULTS_REV)
                must_override = true;
            fclose(file);
        }
    }
    if (must_override || !fs::exists(torrc_defaults_file) || !fs::is_regular_file(torrc_defaults_file))
    {
        FILE* file = fopen(torrc_defaults_file.string().c_str(), "w");
        if (file)
        {
            fprintf(file, "# torrc-defaults for Alias rev. %d\n", TORRC_DEFAULTS_REV);
            fprintf(file, "#\n");
            fprintf(file, "# DO NOT EDIT THIS FILE\n");
            fprintf(file, "#\n");
            fprintf(file, "# This file is distributed with Alias within the tor folder and SHOULD NOT be modified\n");
            fprintf(file, "# (it may be overwritten during the next Alias update).\n");
            fprintf(file, "# To customize your Tor configuration, shut down Alias and edit the torrc file.\n");
            fprintf(file, "#\n");
            fprintf(file, "# If non-zero, try to write to disk less frequently than we would otherwise.\n");
            fprintf(file, "AvoidDiskWrites 1\n");
            fprintf(file, "#\n");
            fprintf(file, "# If non-zero, try to use built-in (static) crypto hardware acceleration when available.\n");
            fprintf(file, "HardwareAccel 1\n");
#if defined(ANDROID) || defined(__ANDROID__)
            fprintf(file, "# If it is set to 0, Tor will not send any padding cells. This option should be offered to mobile users for use where bandwidth may be expensive.\n");
            fprintf(file, "ConnectionPadding 0\n");
            fprintf(file, "#\n");
            fprintf(file, "# If set to 1, Tor will not not hold OR connections open for very long. This option should be offered to mobile users for use where bandwidth may be expensive.\n");
            fprintf(file, "ReducedConnectionPadding 1\n");
#endif
            fclose(file);
        }
    }
    argv.push_back("--defaults-torrc");
    argv.push_back(quoteArg(bQuoteArg, torrc_defaults_file.string()));
#endif
#ifdef WIN32
    // Tor separate process via CreateProcess
    argv.push_back("--Log");
    argv.push_back("\"notice file " + log_file.string() + "\"");

    HANDLE                               hJob;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
    PROCESS_INFORMATION                  pi = { 0 };
    STARTUPINFOA                         si = { 0 };

    // Create a job object.
    hJob = CreateJobObject(NULL, NULL);

    // Causes all processes associated with the job to terminate when the last handle to the job is closed.
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));

    // Hide the console window
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    // Build concatenated commandline string for CreateProcess
    std::string strCommandLine;
    for (auto const& s : argv) { strCommandLine += s + " "; }

    // Create the process suspended
    LogPrintf("Start tor 'Tor/tor.exe' as separate process (CreateProcess) with: %s\n", strCommandLine);

    if (!CreateProcessA("Tor/tor.exe", const_cast<char *>(strCommandLine.c_str()), NULL, NULL, FALSE,
        CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB /*Important*/, NULL, NULL, &si, &pi)) {
        LogPrintf("Terminating - Error: CreateProcess for tor failed with error %d\n", GetLastError());
        exit(1); // TODO improved termination
    }

    // Add the process to our job object.
    AssignProcessToJobObject(hJob, pi.hProcess); // Does not work if without CREATE_BREAKAWAY_FROM_JOB

    // Start our suspended process.
    ResumeThread(pi.hThread);

    /*
     * At this point, if we are closed, windows will automatically clean up
     * by closing any handles we have open. When the handle to the job object
     * is closed, any processes belonging to the job will be terminated.
     * Note: Grandchild processes automatically become part of the job and
     * will also be terminated. This behaviour can be avoided by using the
     * JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK limit flag.
     */

     // Block this thread until the process exits (to have same behavior as tor_main call for static tor integration)
    WaitForSingleObject(pi.hProcess, INFINITE);
#elif __APPLE__
    // Tor separate process via boost::process
    argv.push_back("--Log");
    argv.push_back("notice file " + log_file.string());

    std::string strCommandLine;
    for (auto const& s : argv) { strCommandLine += s + " "; }
    LogPrintf("Start tor as separate process (process::child) with: %s\n", strCommandLine);

    process::child pChildTor(process::start_dir = pathTorDir, "./tor.real", argv, gTor);
    pChildTor.detach();
     // Block this thread until the process exits (to have same behavior as tor_main call for static tor integration)
    std::error_code ec;
    gTor.wait(ec);
    if (ec && !ShutdownRequested())
         boost::process::detail::throw_error(ec, "Tor process: waitpid(2) failed in wait");
#elif __linux__
    // Tor separate process via fork,execvp
    argv.push_back("--Log");
    argv.push_back("notice file " + log_file.string());

    std::string strCommandLine;
    for (auto const& s : argv) { strCommandLine += s + " "; }
    LogPrintf("Start tor (path: %s) as separate process (fork,execvp) with: %s\n", torPath, strCommandLine);

    std::string torResult;
    pid_t ppid_before_fork = getpid();
    tor_process_pid = fork();
    if (tor_process_pid == -1) {
        torResult = "Terminating - Error: fork() for tor failed: ";
        torResult += strerror(errno);
        torResult += "\n";
    }
    else if (tor_process_pid == 0) {
        // Continue child execution...
        // Make sure tor gets terminated when parent process dies
        int r = prctl(PR_SET_PDEATHSIG, SIGKILL);
        if (r == -1) {
            perror("Could not install PDEATHSIG.");
            _exit(1);
        }
        if (getppid() != ppid_before_fork) {
            LogPrintf("Original parent exited just before the prctl() call\n");
            _exit(1);
        }

        std::vector<char *> argv_c;
        std::transform(argv.begin(), argv.end(), std::back_inserter(argv_c), convert_str);
        argv_c.push_back(nullptr);

        if (execvp(torPath.c_str(), &argv_c[0]) < 0) {
            perror("execvp(\"tor\", ...) failed");
            _exit(127);
        }
    } else {
        // Continue parent execution...
        // Block this thread until the process exits (to have same behavior as tor_main call for static tor integration)
        int status;
        if (waitpid(tor_process_pid, &status, 0) > 0) {
            if (WIFEXITED(status) && !WEXITSTATUS(status)) {
                torResult = "Tor shutdown successfull\n";
            } else if (tor_killed_from_here) {
                torResult = "Tor shutdown during wallet stop\n";
            } else if (WIFEXITED(status) && WEXITSTATUS(status)) {
                if (WEXITSTATUS(status) == 127) {
                    torResult = "Terminating - Error: Could not start tor. Is tor installed and available in PATH?\n";
                } else {
                    torResult = "Terminating - Error: Tor did exit with status " + std::to_string(WEXITSTATUS(status)) +
                                "\n";
                }
            } else {
                torResult = "Terminating - Error: Tor was terminated\n";
            }
        } else {
            torResult = "Terminating - Error: waitpid() for tor failed\n";
        }
    }

#else
    // Tor embedded
    argv.push_back("--Log");
    argv.push_back("notice file " + log_file.string());

    std::string strCommandLine;
    for (auto const& s : argv) { strCommandLine += s + " "; }
    LogPrintf("Start tor embedded (tor_main) with: %s\n", strCommandLine);

    std::vector<char *> argv_c;
    std::transform(argv.begin(), argv.end(), std::back_inserter(argv_c), convert_str);
    tor_main(argv_c.size(), &argv_c[0]);
#endif
}

void StartTor(void *nothing)
{
    // Make this thread recognisable as the tor thread
    RenameThread("onion");

    try
    {
      run_tor();
    }
    catch (std::exception& e) {
      PrintException(&e, "StartTor()");
    }

    // If tor could not be started or exits for any reason, shutdown the application
    StartShutdown();

    LogPrintf("Onion thread exited.\n");
}

void StartNode(boost::thread_group& threadGroup)
{
    if (semOutbound == NULL) {
        // initialize semaphore
        int nMaxOutbound = min(MAX_OUTBOUND_CONNECTIONS, (int)GetArg("-maxconnections", 125));
        semOutbound = new CSemaphore(nMaxOutbound);
    }

    if (pnodeLocalHost == NULL)
        pnodeLocalHost = new CNode(INVALID_SOCKET, CAddress(CService("127.0.0.1", 0), nLocalServices));

    //
    // Start threads
    //

    // start the onion seeder
    if (!GetBoolArg("-onionseed", true))
        LogPrintf(".onion seeding disabled\n");
    else
        threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "onionseed", &ThreadOnionSeed));

    // Send and receive from sockets, accept connections
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "net", &ThreadSocketHandler));

    // Initiate outbound connections from -addnode
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "addcon", &ThreadOpenAddedConnections));

    // Initiate outbound connections
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "opencon", &ThreadOpenConnections));

    // Process messages
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "msghand", &ThreadMessageHandler));

    // Dump network addresses
    threadGroup.create_thread(boost::bind(&LoopForever<void (*)()>, "dumpaddr", &DumpAddresses, DUMP_ADDRESSES_INTERVAL * 1000));
}

bool StopNode()
{
    LogPrintf("StopNode()\n");
    mempool.AddTransactionsUpdated(1);
    if (semOutbound)
        for (int i=0; i<MAX_OUTBOUND_CONNECTIONS; i++)
            semOutbound->post();
    DumpAddresses();

#ifdef __APPLE__
    // Tor separate process via boost::process
    if (gTor && gTor.valid()) {
        LogPrintf("Terminate tor process group\n");
        gTor.terminate();
    }
#elif __linux__
    if (tor_process_pid > 0) {
        // Prevent confusing error output as SIGTERM is not working from here
        tor_killed_from_here = true;

        kill(tor_process_pid, SIGKILL);
    }
#endif

    return true;
}

class CNetCleanup
{
public:
    CNetCleanup()
    {
    }
    ~CNetCleanup()
    {
        // Close sockets
        BOOST_FOREACH(CNode* pnode, vNodes)
            if (pnode->hSocket != INVALID_SOCKET)
                closesocket(pnode->hSocket);
        BOOST_FOREACH(SOCKET hListenSocket, vhListenSocket)
            if (hListenSocket != INVALID_SOCKET)
                if (closesocket(hListenSocket) == SOCKET_ERROR)
                    LogPrintf("closesocket(hListenSocket) failed with error %d\n", WSAGetLastError());

#ifdef WIN32
        // Shutdown Windows Sockets
        WSACleanup();
#endif
    }
}
instance_of_cnetcleanup;

void RelayTransaction(const CTransaction& tx, const uint256& hash)
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss.reserve(10000);
    ss << tx;
    RelayTransaction(tx, hash, ss);
}

void RelayTransaction(const CTransaction& tx, const uint256& hash, const CDataStream& ss)
{
    CInv inv(MSG_TX, hash);
    {
        LOCK(cs_mapRelay);
        // Expire old relay messages
        while (!vRelayExpiration.empty() && vRelayExpiration.front().first < GetTime())
        {
            mapRelay.erase(vRelayExpiration.front().second);
            vRelayExpiration.pop_front();
        }

        // Save original serialized message so newer versions are preserved
        mapRelay.insert(std::make_pair(inv, ss));
        vRelayExpiration.push_back(std::make_pair(GetTime() + 15 * 60, inv));
    }

    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if(!pnode->fRelayTxes)
            continue;
        LOCK(pnode->cs_filter);
        if (pnode->pfilter)
        {
            if (pnode->pfilter->IsRelevantAndUpdate(tx))
                pnode->PushInventory(inv);
        } else
        {
            pnode->PushInventory(inv);
        };
    };
}

void CNode::RecordBytesRecv(uint64_t bytes)
{
    LOCK(cs_totalBytesRecv);
    nTotalBytesRecv += bytes;
}

void CNode::RecordBytesSent(uint64_t bytes)
{
    LOCK(cs_totalBytesSent);
    nTotalBytesSent += bytes;
}

uint64_t CNode::GetTotalBytesRecv()
{
    LOCK(cs_totalBytesRecv);
    return nTotalBytesRecv;
}

uint64_t CNode::GetTotalBytesSent()
{
    LOCK(cs_totalBytesSent);
    return nTotalBytesSent;
}
