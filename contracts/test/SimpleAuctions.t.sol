// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {Test, console} from "@forge-std-1.9.1/src/Test.sol";
import {SimpleAuctions} from "src/SimpleAuctions.sol";
import {Collection} from "src/Collection.sol";
import {IERC721} from "@forge-std-1.9.1/src/interfaces/IERC721.sol";

contract SimpleAuctionsTest is Test {
    SimpleAuctions auctions;
    IERC721 collection;

    address constant operator = address(0x1337);
    address constant bidder1 = address(0x1111111111111111111111111111111111111111);
    address constant bidder2 = address(0x2222222222222222222222222222222222222222);
    address constant proceedsReceiver = address(0x3333333333333333333333333333333333333333);

    function setUp() public {
        auctions = new SimpleAuctions(2);
        vm.prank(operator);
        collection = IERC721(address(new Collection()));
    }

    function startAuction(uint256 tokenId) internal returns (uint256 auctionId) {
        startHoax(operator);
        collection.approve(address(auctions), tokenId);
        auctionId = auctions.startAuction{value: 1 wei}(collection, tokenId, proceedsReceiver);
        vm.stopPrank();
    }

    function testBid() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        vm.roll(block.number + 4);
        hoax(bidder1, 10 ether);
        auctions.bid{value: amount}(auctionId);
        assertEq(address(auctions).balance, amount);
        assertEq(bidder1.balance, 10 ether - amount);
    }

    function testEarlyBid() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        vm.expectRevert(bytes("early"));
        hoax(bidder1);
        auctions.bid{value: amount}(auctionId);
    }

    function testLateBid() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        vm.roll(block.number + 6);
        vm.expectRevert(bytes("late"));
        hoax(bidder1);
        auctions.bid{value: amount}(auctionId);
    }

    function testSettle() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        vm.roll(block.number + 5);
        hoax(bidder1, 10 ether);
        auctions.bid{value: amount}(auctionId);
        vm.roll(block.number + 2);
        auctions.settle(auctionId);
        assertEq(address(auctions).balance, 0);
        assertEq(bidder1.balance, 10 ether - amount);
        assertEq(collection.ownerOf(tokenId), bidder1);
    }

    function testSettleTwoBids() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount1 = 1 ether;
        uint256 amount2 = 2 ether;
        vm.roll(block.number + 5);
        hoax(bidder1, 10 ether);
        auctions.bid{value: amount1}(auctionId);
        hoax(bidder2, 10 ether);
        auctions.bid{value: amount2}(auctionId);
        vm.roll(block.number + 2);
        auctions.settle(auctionId);
        assertEq(address(auctions).balance, 0);
        assertEq(bidder1.balance, 10 ether);
        assertEq(bidder2.balance, 10 ether - amount2);
        assertEq(collection.ownerOf(tokenId), bidder2);
    }
}
