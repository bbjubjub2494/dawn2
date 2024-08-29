// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {IERC721} from "@forge-std-1.9.1/src/interfaces/IERC721.sol";
import {IERC20} from "@forge-std-1.9.1/src/interfaces/IERC20.sol";

import {Auctions} from "./Auctions.sol";

contract SimpleAuctions is Auctions {
    struct Auction {
        IERC721 collection;
        uint256 tokenId;
        address proceedsReceiver;
        uint64 opening; // block after which bids are accepted
        uint64 deadline; // last block where bids can be included
        IERC20 bidToken;
        uint256 highestAmount;
        address highestBidder;
    }

    uint64 immutable blockDelay;
    Auction[] public auctions;

    constructor(uint64 blockDelay_) {
        blockDelay = blockDelay_;
    }

    function startAuction(IERC721 collection, uint256 tokenId, IERC20 bidToken, address proceedsReceiver)
        external
        override
        returns (uint256 auctionId)
    {
        auctionId = auctions.length;
        auctions.push();
        Auction storage auction = auctions[auctionId];

        auction.collection = collection;
        auction.tokenId = tokenId;
        auction.bidToken = bidToken;
        auction.proceedsReceiver = proceedsReceiver;
        auction.opening = uint64(block.number) + blockDelay + 1;
        auction.deadline = auction.opening + blockDelay;

        collection.transferFrom(msg.sender, address(this), auction.tokenId);

        // use a dummy bid to initialize storage slots
        uint256 amount = 1;
        auction.highestBidder = msg.sender;
        auction.highestAmount = amount;
        auction.bidToken.transferFrom(msg.sender, address(this), amount);

        emit AuctionStarted(
            auctionId,
            address(collection),
            tokenId,
            auction.opening,
            auction.deadline,
            auction.deadline,
            proceedsReceiver
        );
    }

    function bid(uint256 auctionId, uint256 amount) external {
        Auction storage auction = auctions[auctionId];

        require(block.number > auction.opening, "early");
        require(block.number <= auction.deadline, "late");

        if (amount > auction.highestAmount) {
            address prevHighestBidder = auction.highestBidder;
            uint256 prevHighestAmount = auction.highestAmount;
            auction.highestBidder = msg.sender;
            auction.highestAmount = amount;
            auction.bidToken.transferFrom(msg.sender, address(this), amount);
            auction.bidToken.transfer(prevHighestBidder, prevHighestAmount);
        }
        emit Commit(auctionId);
        emit Reveal(auctionId);
    }

    function settle(uint256 auctionId) external override {
        Auction storage auction = auctions[auctionId];

        require(block.number > auction.deadline, "early");
        require(address(auction.collection) != address(0));

        auction.bidToken.transfer(auction.proceedsReceiver, auction.highestAmount);
        auction.collection.transferFrom(address(this), auction.highestBidder, auction.tokenId);

        // prevent replays
        delete auction.collection;
    }
}
