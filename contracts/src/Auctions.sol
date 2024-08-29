// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {IERC721} from "@forge-std-1.9.1/src/interfaces/IERC721.sol";
import {IERC20} from "@forge-std-1.9.1/src/interfaces/IERC20.sol";

// common code for Simple- and OvercollateralizedAuctions
interface Auctions {
    // In case of SimpleAuction, revealDeadline = commitDeadline
    event AuctionStarted(
        uint256 auctionId,
        address indexed collection,
        uint256 tokenId,
        uint64 opening,
        uint64 commitDeadline,
        uint64 revealDeadline,
        address proceedsReceiver
    );

    // must be emitted once when a bid is committed/revealed
    // SimpleAuction must emit both
    event Commit(uint256 auctionId);
    event Reveal(uint256 auctionId);

    function startAuction(IERC721 collection, uint256 tokenId, IERC20 bidToken, address proceedsReceiver)
        external
        returns (uint256 auctionId);

    function settle(uint256 auctionId) external;
}
