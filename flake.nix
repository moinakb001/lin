{
    inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    inputs.flake-utils.url = "github:numtide/flake-utils";
    outputs = inputs@{self, nixpkgs, flake-utils}:
    flake-utils.lib.eachDefaultSystem (system: let 
        pkgs = import nixpkgs { inherit system; };
    in {
        package.default = pkgs.wget;
    });
}