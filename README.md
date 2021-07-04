Uidd integration/staging tree
=====================================

www.uidd.org

What is Uidd?
----------------

Uidd is an experimental digital currency that enables nearly instant, private payments to
anyone, anywhere in the world. Uidd uses peer-to-peer technology to operate
with no central authority: managing transactions and issuing money are carried
out collectively by the network. Uidd Core is the name of the open source
software which enables the use of this currency. A BTC address is also a valid UIDD address but Uidd has replay protection.

For more information, as well as a binary version of the Uidd Core software, see www.uidd.org

## What are the main differences between Bitcoin and Uidd?

Uidd has much lower fees and up to 40 times higher limit of transactions per second than Bitcoin.

Uidd also implements Masternodes, which enable the Swift TX feature along with adding to the speed, connectivity and redundancy of the network. The Swift TX feature allows Masternodes to confirm your transaction almost immediately so that it can be considered confirmed even before it shows in a block. This kind of a transaction is good for Point of Sale situations in the real world. Anybody with 1000 UIDD can run a Masternode.

Uidd uses the latest thoroughly tested Proof of Stake algorithm. Greatly reducing electricity use compared to Bitcoin. This also enables anyone to run the core software and create blocks, without needing a ASIC miner. A laptop is enough.

Unlike Bitcoin, Uidd supports truly anonymous transactions by using the Zerocoin Protocol. What zUIDD provides is a protocol-level coin mixing service using zero knowledge proofs to sever the link between the sender and the receiver with 100% anonymity and untraceability. [Technical paper about zUIDD](https://www.uidd.org/zUIDD-Bitcoin-2-Zerocoin-Protocol).

The Uidd blockchain is currently only about 1.5 GB and yet the balances of Bitcoin as of block 507850 were recorded to it.

Blocks are created around once per minute instead of once per 10 minutes, this means that transactions confirm faster. Uidd's mining/forging difficulty is adjusted every 10 blocks.

License
-------

Uidd is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/bitcoin/bitcoin/tags) are created
regularly to indicate new official, stable release versions of Uidd Core.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.
