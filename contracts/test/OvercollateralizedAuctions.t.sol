// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {Test, console} from "@forge-std-1.9.1/src/Test.sol";
import {OvercollateralizedAuctions} from "src/OvercollateralizedAuctions.sol";
import {Collection} from "src/Collection.sol";
import {IERC721} from "@forge-std-1.9.1/src/interfaces/IERC721.sol";

contract OvercollateralizedAuctionsTest is Test {
    OvercollateralizedAuctions auctions;
    IERC721 collection;

    address constant operator = address(0x1337);
    address constant bidder1 = address(0x1111111111111111111111111111111111111111);
    address constant bidder2 = address(0x2222222222222222222222222222222222222222);
    address constant proceedsReceiver = address(0x3333333333333333333333333333333333333333);

    uint256 constant MAX_BID = 10 ether;

    function setUp() public {
        auctions = new OvercollateralizedAuctions(2);
        vm.prank(operator);
        collection = IERC721(address(new Collection()));
    }

    function startAuction(uint256 tokenId) internal returns (uint256 auctionId) {
        startHoax(operator);
        collection.approve(address(auctions), tokenId);
        auctionId = auctions.startAuction{value: 1 wei}(collection, tokenId, proceedsReceiver);
        vm.stopPrank();
    }

    function prepareCommit(address bidder, uint256 amount) internal pure returns (bytes32 blinding, bytes32 commit) {
        blinding = hex"1234";
        commit = keccak256(abi.encode(blinding, bidder, amount));
    }

    function doCommit(uint256 auctionId, address bidder, bytes32 commit) internal {
        hoax(bidder, MAX_BID);
        auctions.commitBid{value: MAX_BID}(auctionId, commit);
    }

    function doReveal(uint256 auctionId, address bidder, bytes32 blinding, uint256 amount) internal {
        vm.prank(bidder);
        auctions.revealBid(auctionId, blinding, amount);
    }

    function testCommitBid() public {
        uint256 tokenId = 1;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        (, bytes32 commit) = prepareCommit(bidder1, amount);
        vm.roll(block.number + 2);
        doCommit(auctionId, bidder1, commit);
        assertEq(address(auctions).balance, 10 ether + 1 wei);
        assertEq(bidder1.balance, 0);
    }

    function testEarlyCommitBid() public {
        uint256 tokenId = 1;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        (, bytes32 commit) = prepareCommit(bidder1, amount);
        vm.expectRevert(bytes("early"));
        doCommit(auctionId, bidder1, commit);
    }

    function testLateCommitBid() public {
        uint256 tokenId = 1;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        (, bytes32 commit) = prepareCommit(bidder1, amount);
        vm.roll(block.number + 4);
        vm.expectRevert(bytes("late"));
        doCommit(auctionId, bidder1, commit);
    }

    function testRevealBid() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        (bytes32 blinding, bytes32 commit) = prepareCommit(bidder1, amount);
        vm.roll(block.number + 2);
        doCommit(auctionId, bidder1, commit);
        vm.roll(block.number + 2);
        doReveal(auctionId, bidder1, blinding, amount);
        assertEq(address(auctions).balance, amount);
        assertEq(bidder1.balance, 10 ether - amount);
    }

    function testEarlyRevealBid() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        (bytes32 blinding, bytes32 commit) = prepareCommit(bidder1, amount);
        vm.roll(block.number + 2);
        doCommit(auctionId, bidder1, commit);
        vm.roll(block.number + 1);
        vm.expectRevert("early");
        doReveal(auctionId, bidder1, blinding, amount);
    }

    function testLateRevealBid() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        (bytes32 blinding, bytes32 commit) = prepareCommit(bidder1, amount);
        vm.roll(block.number + 2);
        doCommit(auctionId, bidder1, commit);
        vm.roll(block.number + 4);
        vm.expectRevert(bytes("late"));
        doReveal(auctionId, bidder1, blinding, amount);
    }

    function testWrongAmountRevealBid() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        (bytes32 blinding, bytes32 commit) = prepareCommit(bidder1, amount);
        vm.roll(block.number + 2);
        doCommit(auctionId, bidder1, commit);
        vm.roll(block.number + 2);
        vm.expectRevert("commit");
        doReveal(auctionId, bidder1, blinding, amount - 1);
    }

    function testBidCopyAttackRevealBid() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        // Scenario: bidder2 sniffs bidder1's commit and wants to copy the bid
        (bytes32 blinding, bytes32 commit) = prepareCommit(bidder1, amount);
        vm.roll(block.number + 2);
        doCommit(auctionId, bidder2, commit);
        vm.roll(block.number + 2);
        vm.expectRevert("commit");
        doReveal(auctionId, bidder2, blinding, amount);
    }

    function testSettle() public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount = 1 ether;
        (bytes32 blinding, bytes32 commit) = prepareCommit(bidder1, amount);
        vm.roll(block.number + 2);
        doCommit(auctionId, bidder1, commit);
        vm.roll(block.number + 2);
        doReveal(auctionId, bidder1, blinding, amount);
        vm.roll(block.number + 2);
        auctions.settle(auctionId);
        assertEq(address(auctions).balance, 0);
        assertEq(bidder1.balance, 10 ether - amount);
        assertEq(collection.ownerOf(tokenId), bidder1);
    }

    function testSettleTwoBids(bool outOfOrderReveal) public {
        uint256 tokenId = 2;
        uint256 auctionId = startAuction(tokenId);
        uint256 amount1 = 1 ether;
        uint256 amount2 = 2 ether;
        vm.roll(block.number + 2);
        (bytes32 blinding1, bytes32 commit1) = prepareCommit(bidder1, amount1);
        doCommit(auctionId, bidder1, commit1);
        (bytes32 blinding2, bytes32 commit2) = prepareCommit(bidder2, amount2);
        doCommit(auctionId, bidder2, commit2);
        vm.roll(block.number + 2);
        if (outOfOrderReveal) {
            doReveal(auctionId, bidder2, blinding2, amount2);
            doReveal(auctionId, bidder1, blinding1, amount1);
        } else {
            doReveal(auctionId, bidder1, blinding1, amount1);
            doReveal(auctionId, bidder2, blinding2, amount2);
        }
        vm.roll(block.number + 2);
        auctions.settle(auctionId);
        assertEq(address(auctions).balance, 0);
        assertEq(bidder1.balance, 10 ether);
        assertEq(bidder2.balance, 10 ether - amount2);
        assertEq(collection.ownerOf(tokenId), bidder2);
    }
}
