## Alias V4

`By downloading and using this software, you agree that 1/6 of the staking
rewards are contributed to a development fund. The development fund aims
to support the long term development and value of Alias. The fund is managed
by the Alias core team.`

This is a major release and a **MANDATORY** update to the Alias software!
This update involves consensus changes (_details below_) and **you MUST
update** your wallet software before:

2019-09-27 (_27th September 2019_) @ 2000 hours (GMT)

If you do not update your wallet software you will no longer be able to connect
to the Alias network and you will no longer be able to conduct transactions on
the network and you will no longer be able to deposit or withdraw your funds
(public a/o private ALIAS) from the exchanges.

### Development Contribution Blocks (DCB)
After 21/08/2018 @ 2200 hours (GMT) one in six (1 in 6) block rewards will be
a designated DCB and will be sent to the Alias team development fund wallet.
This fund will ensure a future for Alias and will enable us to pay for certain
services, to hire contractors and to pay Alias core team members in ALIAS
to enable them to work full time on the project. We have some long term projects
and concepts to implement such as a new proof-of-stake algorithm we call
Proof-of-Stealth to enable so called "stealth staking". These developments
depend on a source of steady funding. We believe this will give us the opportunity
to produce better software and will create value for investors. We currently
have some very skilled developers working for us and we want to keep it that way.

### Replay Protection
We have implemented a check for DCBs and we have implemented a replay protection
mechanism. This means that after 17/05/2019 @ 2000 hours (GMT) any wallets not
updated will not be able to create transactions on the Alias V3 network.

### Licensing

- SPDX-FileCopyrightText: © 2020 Alias Developers
- SPDX-FileCopyrightText: © 2016 SpectreCoin Developers
- SPDX-FileCopyrightText: © 2014 ShadowCoin Developers
- SPDX-FileCopyrightText: © 2014 BlackCoin Developers
- SPDX-FileCopyrightText: © 2013 NovaCoin Developers
- SPDX-FileCopyrightText: © 2011 PPCoin Developers
- SPDX-FileCopyrightText: © 2009 Bitcoin Developers

SPDX-License-Identifier: MIT

### Changelog
## 4.3.1 (released 2020-??-??)
- Refactored the whole build to CMake as basement.
  For all supported operating systems, there are corresponding
  helper scripts on `scripts/cmake-build*`.

## 4.3.0 (released 2020-10-01)
- Update to Qt 5.12.9 (Windows / Mac)
- Streamlined licensing headers a.s.o. to make the whole project compliant to [REUSE Software](https://reuse.software/)
- Refactored overall namings according to project rebranding
- Refactored naming of some RPC methods according to project rebranding:
  - `sendspectoanon` > `sendpublictoprivate`
  - `sendanontoanon` > `sendprivate`
  - `sendanontospec` > `sendprivatetopublic`
  - `estimateanonfee` > `estimateprivatefee`
- Renamed some fields on RPC results:
  - `anonbalance` > `balance_private`
  - `spectrestake` > `stake_private`
  - `spectrestakeweight` > `stakeweight_private`
  - `unconfirmedanonbalance` > `unconfirmedbalance_private`
  - `spectrestakeweight` > `stakeweight_private`
  - `anonsupply` > `moneysupply_private`
- Added new fields to RPC results:
  - `balance_public`
  - `stake_public`
  - `stakeweight_public`
  - `unconfirmedbalance_public`
  - `stakeweight_public`
  - `moneysupply_public`
- Complete UI revision based on new style and with many UX improvements
- Fix: private transaction did cost twice as much fee as effectivly required and shown in confirmation dialog
- Updated Fedora build to Fedora 32
- Added build for Ubuntu 20.04
- Wipe out build for Ubuntu 19.x as it's EOL

## 4.2.0 (released 2020-05-02)
- Update to Qt 5.12.8
- Replace QtWebEngine with QtWebView to support mobile platforms.
- Open 'Unlock Wallet' Dialog on incoming anon staking reward with unknown sender.
- Allow to select parts of the transaction detail dialog text.
- Improve staking indicator tooltip:
  - 'Not staking, staking is disabled' was never shown.
  - 'Initializing staking...' is now shown instead 'Not staking because you don't have mature coins' during staker thread initialization
- Improve synchronization tooltip: blockchain synchronization state is now updated every 500ms. (Tooltips in general are now updated when open and underlying data changes)
- Improve splash screen with progress messages to reduce UI freezes during startup.
- [#183](https://github.com/aliascash/alias-wallet/issues/183) Reduce UI freezes during blockchain sync.

### Changelog
## 4.1.0 (released 2019-10-13)
- [#82](https://github.com/aliascash/alias-wallet/issues/82) Wallet.dat creation with mnemonic seed words (BIP39).
  If no `wallet.dat` file was detected during startup, the wallet opens a wizard with these three options:
  - Create new `wallet.dat` file based on mnemonic seed words.
  - Restore `wallet.dat` from mnemonic seed words.
  - Import existing `wallet.dat` file.

  For further details see [here](https://medium.com/coinmonks/mnemonic-generation-bip39-simply-explained-e9ac18db9477).
  Duplicated by [#115](https://github.com/aliascash/alias-wallet/issues/115)

- [#214](https://github.com/aliascash/alias-wallet/issues/214) Migrate Debian/Raspbian build to Buster.
  Binaries for both Stretch and Buster will be provided.

- [#216](https://github.com/aliascash/alias-wallet/issues/216) Tor Hidden Service v3 implementation + minor Tor improvements.
  - Implementation of Tor Hidden Service v3.
  - Creation of launch argument `-onionv2`, allowing the usage of legacy v2 addresses.
  - Creation of an torrc-defaults file on Linux.
  - By default, Tor will now use hardware crypto acceleration if available, only connect to Hidden Services,
    and will write to disk less frequently, preserving the lifespan of SD cards on Raspbian.

- [#218](https://github.com/aliascash/alias-wallet/issues/218) Provide binaries for Ubuntu 19.04.
  Binaries for both Ubuntu 18.04 and 19.04 will be provided.

- Updated packaged Tor for MacOS and Windows to 0.4.1.5

## 4.0.0 (released 2019-09-08)
**V4 blockchain fork consensus changes / Fork time is GMT: Friday, 27. September 2019 20:00:00 (1569614400 unix epoch time)**
- One-time 3'000'000 XSPEC staking reward for foundation address SSGCEMb6xESgmuGXkx7yozGDxhVSXzBP3a

## 3.x release notes

See [ReleaseNotes for Pre-4.x](./ReleaseNotes_Pre4.0.md)

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
