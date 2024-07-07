// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {IERC721} from "forge-std/interfaces/IERC721.sol";
import {IERC20} from "forge-std/interfaces/IERC20.sol";

// common code for Simple- and OvercollateralizedAuctions
abstract contract Auctions {
    // In case of SimpleAuction, revealDeadline = commitDeadline and maxBid = type(uint256).max
    struct Auction {
        IERC721 collection;
        uint256 tokenId;
        IERC20 bidToken;
        address proceedsReceiver;
        uint64 opening; // block after which commits are accepted
        uint64 commitDeadline; // last block where commits can bet included
        uint64 revealDeadline; // last block where reveals can be included
        uint256 maxBid;
        uint256 highestAmount;
        address highestBidder;
        mapping(address => bytes32) commits;
    }

    event AuctionStarted(uint256 auctionId);

    // must be emitted once when a bid is committed/revealed
    // SimpleAuction must emit both
    event Commit(uint256 auctionId);
    event Reveal(uint256 auctionId);

    Auction[] public auctions;

    function startAuction(IERC721 collection, uint256 tokenId, IERC20 bidToken, address proceedsReceiver)
        external
        virtual
        returns (uint256 auctionId);

    function settle(uint256 auctionId) external virtual;
}
