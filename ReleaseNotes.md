## SPECTRECOIN V2

```By downloading and using this software, you agree that 1/6 of the staking rewards are contributed to a development fund. The development fund aims to support the long term development and value of Spectrecoin. The fund is managed by the Spectrecoin core team.```

This is a major release and a **MANDATORY** update to the Spectrecoin software! This update involves consensus changes (_details below_) and **you MUST update** your wallet software before:

21/08/2018 (_21th August 2018_) @ 2200 hours (GMT)

If you do not update your wallet software you will no longer be able to connect to the Spectrecoin network and you will no longer be able to conduct transactions on the network and you will no longer be able to deposit or withdraw your funds (XSPEC) from the exchanges.

### Development Contribution Blocks (DCB)
After 21/08/2018 @ 2200 hours (GMT) one in six (1 in 6) block rewards will be designated DCBs and will be sent to the Spectrecoin team development fund wallet. This fund will ensure a future for Spectrecoin and will enable us to pay for certain services and to hire contractors and to pay Spectrecoin core team members in XSPEC to enable them to work full time on the project. We have some long term projects and concepts to implement such as a new proof-of-stake algorithm we call Proof-of-Stealth to enable so called "stealth staking". These developments depend on a source of steady funding. We believe this will give us the opportunity to produce better software and will create value for investors. We currently have some very skilled developers working for us and we want to keep it that way.

### Replay Protection
We have implemented a check for DCBs and we have implemented a replay protection mechanism. This means that after 21/08/2018 @ 2200 hours (GMT) any wallets not updated will not be able to create transactions on the Spectrecoin V2 network.

### Changelog
## 2.2.1
- Fix: Lookup for possible stealth addresses in addressbook bloated logfile and decreased performance
- Fix: Bug in fee calculation could prevent spending of SPECTRE

## 2.2.0
**Please note that to make the various changes and fixes regarding SPECTRE effective, a complete rescan of the Blockchain is required. The wallet will automatically initiate a rescan on startup.**
- Minimum ring size increased to 10 (enforced)
- Allow SPECTRE <> XSPEC transfers only within account (destination address must be owned)
- Disallow sending XSPEC to a stealth address
- Chat functionality removed
- RPC method `listransactions`:
  - Consolidate ATXOs under the stealth address
  - New field currency with value 'XSPEC' or 'SPECTRE'
  - New field narration
  - Dont list SPECTRE change
- RPC method `reloadanondata` removed and logic integrated in `scanforalltxns`
- Improved transactions list in UI:
  - If available show stealth address or addressbook entry for SPECTRE transactions
  - Transaction type now always includes currency (XSPEC or SPECTRE)
  - Transfers between XSPEC and SPECTRE are shown with a distinguished type
  - Show corresponding narration of multiple recipients if available
- Improved Make Payment and Balance Transfer form in UI:
  - Revised 'From/To Account' input fields to reflect new transaction restrictions
  - Show curreny SPECTRE or XSPEC depending on transaction type
  - Remove advance mode, integrate 'Add Recipient' in basic mode
- Addressbook fixes and improvements
- CoinControl dialog now groups stealth derived addresses under the stealth address
- New 3D application icon for macOS
- Progress indicator for load block index, clear cache and rescanning. Fixes the "disconnected UI" problem after startup.
- Fix: Rescanning of ATXO (Also fixes [#45](https://github.com/spectrecoin/spectre/issues/45))
- Fix: Tor was not started for windows if wallet path contained a space
- Update to Qt 5.12 for Windows and MacOS (Fixes Mojave dark theme issues)
- Automatic Build improvements:
  - All wallets show now their build commit hash in the about dialog and on the main window title. ([#117](https://github.com/spectrecoin/spectre/issues/117))
  - Build from develop branch also have the build number and the commit hash in their archive name.
- Changed language level to C++17

## 2.1.0
- Tor is now integrated as a separate process. This provides the same level of privacy but enables Spectrecoin to always use the latest version of TOR and to use the TOR plugins / bridges more effectively. **Note:** Linux users must install **tor** and obfs4proxy (if required) separately using their package manager.
- [#7](https://github.com/spectrecoin/spectre/issues/7) For MacOS and Windows, a separate OBFS4 release is now available with preconfigured OBFS4 bridges. Note that the only difference between the OBFS4 release and the standard release is the file  **torrc-defaults** in the tor subfolder which configures OBFS4.

## 2.0.7
- Change BIP44 ID from 35 (shadowcash) to 213 (spectrecoin). See https://github.com/satoshilabs/slips/blob/master/slip-0044.md.
Attention: Mnemonic seed words used for sub-wallet creation pre 2.0.7 will not work post 2.0.7.
- remove faulty RPC method 'scanforstealthtxns'; scanforalltxns also scans for stealth trxs
- Fix: Add all required libraries for Mac wallet
- Fix: already processed anon transactions were not added to wallet after key change/import

## 2.0.6
- UI sidebar behavior improved. Automatically select appropiate mode depending on viewport.
- External blockexplorer address URL updated.
- Prevent open of default context menu with browser actions.
- [#26](https://github.com/spectrecoin/spectre/issues/26) Fix status icon tooltips offscreen when scrolled down past header text
- [#31](https://github.com/spectrecoin/spectre/issues/31) Fix Connectivity bar not visible on high DPI settings
- [#65](https://github.com/spectrecoin/spectre/issues/65) Fix wallet immediately closed after walletpassphrase via console Win 8.1
- [#69](https://github.com/spectrecoin/spectre/issues/69) Fix Transaction ID in transaction detail dialog: link behavior error
- [#74](https://github.com/spectrecoin/spectre/issues/74) Change 'Spectre' to 'Spectrecoin'; update logo images
- [#75](https://github.com/spectrecoin/spectre/issues/75) Fix Wrong fee calculation when transfer from private to public

## 2.0.5
- [#40](https://github.com/spectrecoin/spectre/issues/40) / [#53](https://github.com/spectrecoin/spectre/issues/53) support cyrillic usernames by using the unicode function of windows to fetch the pathname (Windows)
- [#42](https://github.com/spectrecoin/spectre/issues/42) Remove additional UI id chars from transaction ID when copy/paste
- [#50](https://github.com/spectrecoin/spectre/issues/50) Change text 'No combination of coins matches amount and ring size' to 'No combination of (mature) coins matches amount and ring size.'
- [#64](https://github.com/spectrecoin/spectre/issues/64) DCB staking rewards are labeled 'Contributed'
- Change text of donation setting

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
