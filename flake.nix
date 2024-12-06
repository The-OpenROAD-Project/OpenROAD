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
    nixpkgs.url = github:nixos/nixpkgs/nixos-24.11;
  };

  outputs = {
    self,
    nixpkgs,
    ...
  }: {
    overlays = {
      default = pkgs': pkgs: {
        or-tools_9_11 = (nixpkgs.lib.callPackageWith pkgs') ./nix/or-tools_9_11.nix {
          inherit (pkgs'.darwin) DarwinTools;
        };
        openroad = (nixpkgs.lib.callPackageWith pkgs') ./nix/default.nix {
          flake = self;
        };
        openroad-release = pkgs'.openroad.overrideAttrs (finalAttrs: previousAttrs: {
          pname = "openroad-release";
          version = "24Q3";
          src = pkgs.fetchFromGitHub {
            owner = "The-OpenROAD-Project";
            repo = "OpenROAD";
            rev = finalAttrs.version;
            fetchSubmodules = true;
            sha256 = "sha256-Ye9XJcoUxtg031eazT4qrexvyN0jZHd8/kmvAr/lPzk=";
          };
        });
      };
    };

    legacyPackages =
      nixpkgs.lib.genAttrs [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ] (
        system:
          import nixpkgs {
            inherit system;
            overlays = [
              self.overlays.default
            ];
          }
      );

    packages =
      nixpkgs.lib.genAttrs [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ] (system: {
        inherit (self.legacyPackages.${system}) openroad openroad-release;
        default = self.legacyPackages.${system}.openroad;
      });
  };
}
