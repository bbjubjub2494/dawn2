{
  mkShell,
  just,
  pkg-config,
  openssl,
  cargo,
  llvmPackages,
  glibc,
  libclang,
  ...
}:
mkShell {
  name = "dawn2";
  packages = [
    just
    pkg-config
    openssl.dev
    cargo
  ];

  env = {
    LIBCLANG_PATH = "${llvmPackages.libclang.lib}/lib";
    GLIBC_INCLUDE_DIR = "${glibc.dev}/include";
    LIBC_INCLUDE_DIR = "${libclang.lib}/lib/clang/18/include";
  };
}
