// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {IERC721} from "forge-std/interfaces/IERC721.sol";
import {IERC20} from "forge-std/interfaces/IERC20.sol";

import {Auctions} from "./Auctions.sol";

contract OvercollateralizedAuctions is Auctions {
    uint64 immutable blockDelay;

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
        auction.opening = uint64(block.number) + 1;
        auction.commitDeadline = auction.opening + blockDelay;
        auction.revealDeadline = auction.commitDeadline + blockDelay;
        auction.maxBid = 10 ether; // FIXME: hardcoded

        collection.transferFrom(msg.sender, address(this), auction.tokenId);

	// use a dummy bid to initialize storage slots
	uint256 amount = 1;
	auction.highestBidder = msg.sender;
	auction.highestAmount = amount;
	auction.bidToken.transferFrom(msg.sender, address(this), amount);

        emit AuctionStarted(auctionId);
    }

    function computeCommitment(bytes32 blinding, address bidder, uint256 amount) public pure returns (bytes32 commit) {
        commit = keccak256(abi.encode(blinding, bidder, amount));
    }

    function commitBid(uint256 auctionId, bytes32 commit) external {
        Auction storage auction = auctions[auctionId];

        require(block.number > auction.opening, "early");
        require(block.number <= auction.commitDeadline, "late");

        require(auction.bidToken.transferFrom(msg.sender, address(this), auction.maxBid));

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
            auction.bidToken.transfer(msg.sender, auction.maxBid - amount);
            auction.bidToken.transfer(prevHighestBidder, prevHighestAmount);
        } else {
            auction.bidToken.transfer(msg.sender, auction.maxBid);
        }

        emit Reveal(auctionId);
    }

    function settle(uint256 auctionId) external override {
        Auction storage auction = auctions[auctionId];

        require(block.number > auction.revealDeadline, "early");
        require(address(auction.collection) != address(0));

        auction.bidToken.transfer(auction.proceedsReceiver, auction.highestAmount);
        auction.collection.transferFrom(address(this), auction.highestBidder, auction.tokenId);

        // prevent replays
        delete auction.collection;
    }
}
