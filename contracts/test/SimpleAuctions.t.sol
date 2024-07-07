// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {Test, console} from "forge-std/Test.sol";
import {SimpleAuctions} from "src/SimpleAuctions.sol";
import {Collection} from "src/Collection.sol";
import {IERC721} from "forge-std/interfaces/IERC721.sol";
import {IERC20} from "forge-std/interfaces/IERC20.sol";

contract SimpleAuctionsTest is Test {
    SimpleAuctions auctions;
    IERC721 collection;
    IERC20 bidToken;

    address payable constant bidder1 = payable(0x1111111111111111111111111111111111111111);
    address payable constant bidder2 = payable(0x2222222222222222222222222222222222222222);
    address payable constant proceedsReceiver = payable(0x3333333333333333333333333333333333333333);

    function setUp() public {
        auctions = new SimpleAuctions(2);
        collection = IERC721(address(new Collection()));
        bidToken = IERC20(address(deployMockERC20("I owe you", "IOU", 18)));

        deal(address(bidToken), bidder1, 10 ether);
        vm.prank(bidder1);
        bidToken.approve(address(auctions), 10 ether);
        deal(address(bidToken), bidder2, 10 ether);
        vm.prank(bidder2);
        bidToken.approve(address(auctions), 10 ether);
    }

    function startAuction(uint256 tokenId) internal returns (uint256 auctionId) {
        collection.approve(address(auctions), tokenId);
        deal(address(bidToken), address(this), 1 wei);
        bidToken.approve(address(auctions), 1 wei);
        return auctions.startAuction(collection, tokenId, bidToken, proceedsReceiver);
    }

    function testBid() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        vm.prank(bidder1);
        vm.roll(block.number + 4);
        auctions.bid(auctionId, amount);
        assertEq(bidToken.balanceOf(address(auctions)), amount);
        assertEq(bidToken.balanceOf(bidder1), 10 ether - amount);
    }

    function testEarlyBid() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        vm.expectRevert(bytes("early"));
        vm.prank(bidder1);
        auctions.bid(auctionId, amount);
    }

    function testLateBid() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        vm.roll(block.number + 6);
        vm.expectRevert(bytes("late"));
        vm.prank(bidder1);
        auctions.bid(auctionId, amount);
    }

    function testSettle() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        vm.prank(bidder1);
        vm.roll(block.number + 5);
        auctions.bid(auctionId, amount);
        vm.roll(block.number + 2);
        auctions.settle(auctionId);
        assertEq(bidToken.balanceOf(address(auctions)), 0);
        assertEq(bidToken.balanceOf(bidder1), 10 ether - amount);
        assertEq(collection.ownerOf(tokenId), bidder1);
    }

    function testSettleTwoBids() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount1 = 1 ether;
        uint256 amount2 = 2 ether;
        vm.roll(block.number + 5);
        vm.prank(bidder1);
        auctions.bid(auctionId, amount1);
        vm.prank(bidder2);
        auctions.bid(auctionId, amount2);
        vm.roll(block.number + 2);
        auctions.settle(auctionId);
        assertEq(bidToken.balanceOf(address(auctions)), 0);
        assertEq(bidToken.balanceOf(bidder1), 10 ether);
        assertEq(bidToken.balanceOf(bidder2), 10 ether - amount2);
        assertEq(collection.ownerOf(tokenId), bidder2);
    }
}
