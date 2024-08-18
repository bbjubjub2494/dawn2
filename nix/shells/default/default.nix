{
  inputs,
  pkgs,
  lib,
  mkShell,
  system,
  ...
}:
mkShell {
  name = "dawn2";
  packages = [
    pkgs.just
    pkgs.pkg-config
    pkgs.openssl.dev
    inputs.nixpkgs-staging-next.legacyPackages.${system}.cargo
    inputs.nixpkgs-staging-next.legacyPackages.${system}.rust-analyzer
  ];

  env = {
    LIBCLANG_PATH = "${pkgs.llvmPackages.libclang.lib}/lib";
    GLIBC_INCLUDE_DIR = "${pkgs.glibc.dev}/include";
    LIBC_INCLUDE_DIR = "${pkgs.libclang.lib}/lib/clang/17/include";
  };
}
