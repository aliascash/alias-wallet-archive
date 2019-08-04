## SPECTRECOIN V3

`By downloading and using this software, you agree that 1/6 of the staking rewards are contributed to a development fund. The development fund aims to support the long term development and value of Spectrecoin. The fund is managed by the Spectrecoin core team.`

This is a major release and a **MANDATORY** update to the Spectrecoin software! This update involves consensus changes (_details below_) and **you MUST update** your wallet software before:

17/05/2019 (_17th May 2019_) @ 2000 hours (GMT)

If you do not update your wallet software you will no longer be able to connect to the Spectrecoin network and you will no longer be able to conduct transactions on the network and you will no longer be able to deposit or withdraw your funds (XSPEC) from the exchanges.

### Development Contribution Blocks (DCB)
After 21/08/2018 @ 2200 hours (GMT) one in six (1 in 6) block rewards will be designated DCBs and will be sent to the Spectrecoin team development fund wallet. This fund will ensure a future for Spectrecoin and will enable us to pay for certain services and to hire contractors and to pay Spectrecoin core team members in XSPEC to enable them to work full time on the project. We have some long term projects and concepts to implement such as a new proof-of-stake algorithm we call Proof-of-Stealth to enable so called "stealth staking". These developments depend on a source of steady funding. We believe this will give us the opportunity to produce better software and will create value for investors. We currently have some very skilled developers working for us and we want to keep it that way.

### Replay Protection
We have implemented a check for DCBs and we have implemented a replay protection mechanism. This means that after 17/05/2019 @ 2000 hours (GMT) any wallets not updated will not be able to create transactions on the Spectrecoin V3 network.

### Changelog
## 3.0.12 (released 2019-08-04)
- Fix unintentional chain fork possibility in fake stake block spam prevention implementation
- Added block checkpoints every 100000 block and at height 1232433 and 1237500
- Remove due ckeckpoint invalid orphans from memory
- Disconnect from misbehaving nodes with local address (inbound tor connection)
- Improve expected time to staking reward logic. During network staking weight changes, expected time was calculated wrong.
  RPC: add new field 'netstakeweightrecent' for getstakinginfo and getmininginfo which shows network weight of last 72 blocks
- Increase address lookahead from 10 to 100

## 3.0.11 (released 2019-06-30)
- UI: [#185](https://github.com/spectrecoin/spectre/issues/185) Fix shown staking transaction reward after wallet unlock

## 3.0.10 (released 2019-06-10)
- [#184](https://github.com/spectrecoin/spectre/issues/184) Fix fake stake block spam attack vector
- [#173](https://github.com/spectrecoin/spectre/issues/173) Tor process is not cleanly shutting down

## 3.0.9 (released 2019-05-05)
- UI: [#178](https://github.com/spectrecoin/spectre/issues/178) Tooltip for SPECTRE->XSPEC balance transfer
- UI: Add grouping feature of transactions in TRANSACTION view
- UI: Show immature instead mature coins in CHAIN DATA view
- UI & RPC: Fix column least depth in CHAIN DATA view and RPC method anoninfo. (show depth consistently)

## 3.0 (released 2019-04-28)
**V3 blockchain fork consensus changes / Fork time is GMT: Friday, 17. May 2019 20:00:00 (1558123200 unix epoch time)**
- Target block time increased from 64 to 96 seconds
- XSPEC staking reward lowered to fixed 2 XSPEC per block
- Minimum ring size increased from 1 to fix 10
- Minimum maturity for staking and for spending stakes is increased from 288 to 450 blocks (approximately 96 seconds * 450 = 12 hours)
- 8 hours maturity rules for staking is removed (Fixes [#79](https://github.com/spectrecoin/spectre/issues/79))
- Base fee for spending SPECTRE is lowered from 0.01 to 0.0001
- Support for SPECTRE staking (aka Stealth Staking, aka PoAS)
  - Staking reward per block fixed 3 SPECTRE
  - Same maturity rules as for XSPEC (450 blocks) but for all ring signature members
  - Maximal ATXO stake output value of 1'000 (same as max anon output)
  - Consolidation of up to 50 ATXOs in staking transaction
  - If number of unspent anons per denomination is below defined minimum, split staked ATXO to create up to 1 new ATXO of that denomination

Immediate changes:
- Change max anon output from 10'000 to 1'000
- Increase default block size created from 250K to 999K
- ATXOs compromised by an All Spent situation are no longer considered as mixins for new transactions
- New mixin picking algorithm:
  - ATXOs are only read once per transaction
  - handles ATXO_TX_SET problem by adding mixins of same transaction
  - 33% of mixins are picked from last 2700 blocks if possible
  - an ATXO is only used once as mixin per transaction
  - pick all mixins for a vin/ringsignature from different transactions
- Increase levelDB version to 70512 to force reindex:
  - new attribute fCoinStake in CAnonOutput
  - new attribute nBlockHeight in CKeyImageSpent
  - new compromisedanonheights map for detected All Spent
- Anon cache is now updated each block with the new mature ATXOs and with the available mixins for spending and staking
- Fix wallet.dat corruption problem on Windows (On Windows shutdown wallet is safely closed)
- UI: [#149](https://github.com/spectrecoin/spectre/issues/149) Fixed notifications
- UI: Contributions and donations are shown without separate stake entry; show in overview if stakes are contributed or staked
- UI: Show generated but not accepted stakes as 'Orphan' in overview
- UI: Change TransactionRecord sort order to consider nTime first (Fixes UI trx update when more than 200 unrecorded trx exist)
- UI: Show different balance types for SPECTRE and XSPEC separately
- UI: Change CHAIN DATA view columns to Unspent (Mature), Mixins (Mature), System (Compromised)
- UI: Rebased all UI changes since initial commit back to separate UI repository [spectrecoin-ui](https://github.com/spectrecoin/spectrecoin-ui)
- UI & RPC: Optimize getStakeWeight (remove obsolete code; make sure stake weight matches actual staked coins)
- RPC: [#2](https://github.com/spectrecoin/spectre/issues/2) Integrate API method gettxout
- RPC: method anoninfo shows new stats per denomination:
  - No.Mixins: the number of uncompromised ATXOs available as mixins for spending
  - No.MixinsStaking: the number of uncompromised ATXOs available as mixins for staking
  - Compromised Height: the last block height an All Spent situation occured
  - Compromised: does count mixins compromised by ring signature one AND all spent

## 2.x release notes

See [ReleaseNotes for Pre-3.x](./ReleaseNotes_Pre3.0.md)

# Checksums
## Verify MacOS
```
openssl sha -sha256 <archive-name>
```
## Verify Windows
```
certUtil -hashfile "<archive-name>" SHA256
```
## Verify Linux
```
sha256sum <archive-name>
```
## List of sha256 checksums
