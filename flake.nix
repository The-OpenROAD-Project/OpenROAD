# To use Nix just to get an environment with dependencies:
#   1. Install Nix: https://github.com/DeterminateSystems/nix-installer
#   2. Invoke `nix develop` in your shell. First invocation will
#      take around 5 minutes depending on your internet connection and
#      CPU speed.
#   3. cd build
#   4. cmake -G Ninja $cmakeFlags ..
#   5. ninja

# To build OpenROAD with Nix:
#   1. Install Nix: https://github.com/DeterminateSystems/nix-installer
#   2. nix build '.?submodules=1#'

{
  inputs = {
    nixpkgs.url = github:nixos/nixpkgs/nixos-24.05;
  };

  outputs = {self, nixpkgs, ...}: {
    packages = nixpkgs.lib.genAttrs [
      "x86_64-linux"
      "aarch64-linux"
      "x86_64-darwin"
      "aarch64-darwin"
    ] (
      system: let
        pkgs = import nixpkgs {inherit system;};
        all = {
          openroad = (nixpkgs.lib.callPackageWith pkgs) ./default.nix {
            flake = self;
          };
          default = all.openroad;
        };
      in
        all
    );
  };
}
