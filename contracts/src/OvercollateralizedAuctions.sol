// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {IERC721} from "@forge-std-1.9.1/src/interfaces/IERC721.sol";
import {SafeTransferLib} from "@solady-0.0.217/src/utils/SafeTransferLib.sol";

import {Auctions} from "./Auctions.sol";

contract OvercollateralizedAuctions is Auctions {
    uint64 immutable blockDelay;
    Auction[] public auctions;

    constructor(uint64 blockDelay_) {
        blockDelay = blockDelay_;
    }

    function startAuction(IERC721 collection, uint256 tokenId, address proceedsReceiver)
        external
        payable
        override
        returns (uint256 auctionId)
    {
        auctionId = auctions.length;
        auctions.push();
        Auction storage auction = auctions[auctionId];

        auction.collection = collection;
        auction.tokenId = tokenId;
        auction.proceedsReceiver = proceedsReceiver;
        auction.opening = uint64(block.number) + 1;
        auction.commitDeadline = auction.opening + blockDelay;
        auction.revealDeadline = auction.commitDeadline + blockDelay;
        auction.maxBid = 10 ether; // FIXME: hardcoded

        collection.transferFrom(msg.sender, address(this), auction.tokenId);

        // use a dummy bid to initialize storage slots
        uint256 amount = 1;
        auction.highestBidder = msg.sender;
        auction.highestAmount = amount;
        require(msg.value == amount);

        emit AuctionStarted(auctionId);
    }

    function computeCommitment(bytes32 blinding, address bidder, uint256 amount) public pure returns (bytes32 commit) {
        commit = keccak256(abi.encode(blinding, bidder, amount));
    }

    function commitBid(uint256 auctionId, bytes32 commit) external payable {
        Auction storage auction = auctions[auctionId];

        require(block.number > auction.opening, "early");
        require(block.number <= auction.commitDeadline, "late");

        require(msg.value == auction.maxBid);

        // NOTE: bidders can self-grief by overwriting their commit
        auction.commits[msg.sender] = commit;

        emit Commit(auctionId);
    }

    function revealBid(uint256 auctionId, bytes32 blinding, uint256 amount) external {
        Auction storage auction = auctions[auctionId];

        require(block.number > auction.commitDeadline, "early");
        require(block.number <= auction.revealDeadline, "late");

        bytes32 commit = computeCommitment(blinding, msg.sender, amount);
        require(auction.commits[msg.sender] == commit, "commit");
        auction.commits[msg.sender] = "";

        if (amount > auction.highestAmount) {
            address prevHighestBidder = auction.highestBidder;
            uint256 prevHighestAmount = auction.highestAmount;
            auction.highestBidder = msg.sender;
            auction.highestAmount = amount;
            SafeTransferLib.safeTransferETH(msg.sender, auction.maxBid - amount);
            SafeTransferLib.safeTransferETH(prevHighestBidder, prevHighestAmount);
        } else {
            SafeTransferLib.safeTransferETH(msg.sender, auction.maxBid);
        }

        emit Reveal(auctionId);
    }

    function settle(uint256 auctionId) external override {
        Auction storage auction = auctions[auctionId];

        require(block.number > auction.revealDeadline, "early");
        require(address(auction.collection) != address(0));

        SafeTransferLib.safeTransferETH(auction.proceedsReceiver, auction.highestAmount);
        auction.collection.transferFrom(address(this), auction.highestBidder, auction.tokenId);

        // prevent replays
        delete auction.collection;
    }
}
