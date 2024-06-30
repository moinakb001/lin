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
                pkgs.llvmPackages.clang
                pkgs.llvmPackages.llvm
            ];
            buildPhase = let
                blake3CFiles = [
                    "blake3.c"
                    "blake3_dispatch.c"
                    "blake3_portable.c"
                ];
                blakeCPaths = builtins.concatStringsSep "" (builtins.map (x: " lib/" + x) blake3CFiles);
                blake3SFiles = [
                    "blake3_sse2_x86-64_unix.S"
                    "blake3_sse41_x86-64_unix.S"
                    "blake3_avx2_x86-64_unix.S"
                    "blake3_avx512_x86-64_unix.S"
                ];
                blakeSPaths = builtins.concatStringsSep "" (builtins.map (x: " lib/" + x) blake3SFiles);
                blakePaths = blakeSPaths + blakeCPaths;
            in ''
                clang -static -emit-llvm -c -Iinclude/ ${blakeCPaths}
                clang++ -static -emit-llvm -c -o main.bc -Iinclude ./src/main.cpp
                llvm-link *.bc -o out.bc
                opt --internalize-public-api-list=main -passes=internalize,dce -o opt.bc out.bc
                clang++ -O2 -static -c opt.bc -o out.o
                gcc -O2 -static out.o ${blakeSPaths} -o lin -lc -v
                '';
            installPhase = "mkdir -p $out/bin; install -t $out/bin lin";
        };
    });
}