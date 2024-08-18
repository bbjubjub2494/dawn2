// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {IERC721} from "@forge-std-1.9.1/src/interfaces/IERC721.sol";
import {SafeTransferLib} from "@solady-0.0.217/src/utils/SafeTransferLib.sol";

import {Auctions} from "./Auctions.sol";

contract SimpleAuctions is Auctions {
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
        auction.opening = uint64(block.number) + blockDelay + 1;
        auction.commitDeadline = auction.opening + blockDelay;
        auction.revealDeadline = auction.commitDeadline;
        auction.maxBid = type(uint256).max;

        collection.transferFrom(msg.sender, address(this), auction.tokenId);

        // use a dummy bid to initialize storage slots
        uint256 amount = 1;
        auction.highestBidder = msg.sender;
        auction.highestAmount = amount;
        require(msg.value == amount);

        emit AuctionStarted(auctionId);
    }

    function bid(uint256 auctionId) external payable {
        Auction storage auction = auctions[auctionId];
        uint256 amount = msg.value;

        require(block.number > auction.opening, "early");
        require(block.number <= auction.commitDeadline, "late");

        if (amount > auction.highestAmount) {
            address prevHighestBidder = auction.highestBidder;
            uint256 prevHighestAmount = auction.highestAmount;
            auction.highestBidder = msg.sender;
            auction.highestAmount = amount;
            SafeTransferLib.safeTransferETH(prevHighestBidder, prevHighestAmount);
        }
        emit Commit(auctionId);
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
