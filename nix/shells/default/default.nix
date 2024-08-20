{
  mkShell,
  just,
  pkg-config,
  openssl,
  rustup,
  llvmPackages,
  glibc,
  libclang,
  sgx-sdk,
  automake,
  autoconf,
  gnumake,
  libtool,
  ...
}:
mkShell {
  name = "dawn2";
  packages = [
    just
    rustup

    # to build rusttls
    pkg-config
    openssl.dev

    # to build sgx libs
    libtool
    automake
    autoconf
    gnumake
    sgx-sdk
  ];

  env = {
    LIBCLANG_PATH = "${llvmPackages.libclang.lib}/lib";
    GLIBC_INCLUDE_DIR = "${glibc.dev}/include";
    LIBC_INCLUDE_DIR = "${libclang.lib}/lib/clang/18/include";

    # sgx
    CFLAGS = "-I${sgx-sdk}/include";
    SGX_MODE = "SW"; # use HW for real sgx chips
    SGX_SDK = sgx-sdk;
  };

  shellHook = ''
    rustup override set stable
    rustup override set --path sgx/ nightly-2022-10-22
  '';
}
