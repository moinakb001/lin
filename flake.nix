{
    inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    inputs.flake-utils.url = "github:numtide/flake-utils";
    outputs = inputs@{self, nixpkgs, flake-utils}:
    flake-utils.lib.eachDefaultSystem (system: let 
        pkgs = import nixpkgs { inherit system; };
    in {
        packages.default = pkgs.pkgsMusl.stdenv.mkDerivation {
            name = "lin";
            src = self;
            nativeBuildInputs = [
                pkgs.pkgsMusl.gcc
                pkgs.llvmPackages.clangUseLLVM
            ];
            buildPhase = let
                blake3Files = [
                    "blake3.c"
                    "blake3_dispatch.c"
                    "blake3_portable.c"
                    "blake3_sse2_x86-64_unix.S"
                    "blake3_sse41_x86-64_unix.S"
                    "blake3_avx2_x86-64_unix.S"
                    "blake3_avx512_x86-64_unix.S"
                ];
                blakePaths = builtins.concatStringsSep "" (builtins.map (x: " lib/" + x) blake3Files);
            in ''
                clang -static -emit-llvm -c -Iinclude/ ${blakePaths}
                clang++ -static -emit-llvm -c -o main.bc -Iinclude ./src/main.cpp
                llvm-link *.bc -o out.bc
                clang++ -O2 -static -c out.bc -o out.o
                gcc -O2 -static out.o -o lin
                '';
            installPhase = "mkdir -p $out/bin; install -t $out/bin lin";
        };
    });
}