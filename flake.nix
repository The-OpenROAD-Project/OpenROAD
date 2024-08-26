# To get dependencies and build in a Nix environment:
#   1. Install Nix: https://github.com/DeterminateSystems/nix-installer
#   2. Invoke `nix develop` in your shell. First invocation will
#      take around 5 minutes depending on your internet connection and
#      CPU speed.
#   3. `cd build`
#   4. `cmake -G Ninja $cmakeFlags ..`
#   5. `ninja`
#   6. `wrapQtApp ./src/openroad`
{
  inputs = {
    nixpkgs.url = github:nixos/nixpkgs/nixos-24.05;
  };

  outputs = {nixpkgs, ...}: {
    packages = nixpkgs.lib.genAttrs [
      "x86_64-linux"
      "aarch64-linux"
      "x86_64-darwin"
      "aarch64-darwin"
    ] (
      system: let
        pkgs = import nixpkgs {inherit system;};
        self = {
          openroad = (nixpkgs.lib.callPackageWith pkgs) ./default.nix {};
          default = self.openroad;
        };
      in
        self
    );
  };
}
