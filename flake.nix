{
    inputs = {
        nixpkgs.url = "nixpkgs/nixpkgs-unstable";

        snowfall-lib = {
            url = "github:snowfallorg/lib";
            inputs.nixpkgs.follows = "nixpkgs";
        };
    };

    outputs = inputs:
        inputs.snowfall-lib.mkFlake {
            inherit inputs;
            src = ./.;
            snowfall.root = ./nix;
        };
}
