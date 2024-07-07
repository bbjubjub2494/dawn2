// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {ERC721} from "solady/tokens/ERC721.sol";

contract Collection is ERC721 {
    function name() public pure override returns (string memory) {
        return "Daredevil Iguana Squad";
    }

    function symbol() public pure override returns (string memory) {
        return "DDIS";
    }

    function tokenURI(uint256) public pure override returns (string memory) {
        return "https://dedis.ch";
    }

    constructor() {
        for (uint256 i = 0; i < 10; i++) {
            _mint(msg.sender, i);
        }
    }
}
