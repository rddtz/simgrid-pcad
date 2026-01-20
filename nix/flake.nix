{
  description = "Projeto PCAD com SimGrid (Master) Reprodutível";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    # SimGrid from source (commit 1ea158ba)
    simgrid-src = {
      url = "git+https://framagit.org/simgrid/simgrid.git?ref=refs/tags/v4.1";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, flake-utils, simgrid-src }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        simgridOverlay = final: prev: {
          simgrid = prev.simgrid.overrideAttrs (oldAttrs: {
            pname = "simgrid-git";
            version = "4.1";
            src = simgrid-src;
#	    outputs = [ "out" ];
            doCheck = false;

            cmakeFlags = oldAttrs.cmakeFlags ++ [ "-Denable_documentation=OFF"];
          });
        };

        pkgs = import nixpkgs {
          inherit system;
          overlays = [ simgridOverlay ];
        };

      in
      {
        # build
        packages.default = pkgs.stdenv.mkDerivation {
          name = "pcad-platform";
          src = ./..; # Usa os arquivos da pasta atual

          # Build dependencies
          nativeBuildInputs = with pkgs; [ cmake pkg-config ];

          # Libraries in the code
          buildInputs = with pkgs; [ simgrid boost ];

          # O Nix runs automatically: cmake configure, make, make install
        };

        # dev env (inherits from above)
        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];

          packages = [ pkgs.gnumake ]; # Extra tools

          shellHook = ''
            echo "PCAD (SimGrid 1ea158ba) enviroment loaded!"
          '';
        };
      }
    );
}