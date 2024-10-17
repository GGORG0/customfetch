{
  description = "Description for the project";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [ "x86_64-linux" "aarch64-linux" ];
      perSystem = { config, self', inputs', pkgs, system, ... }: {
        packages.customfetch = pkgs.stdenv.mkDerivation {
          pname = "customfetch";
          version = "0.8.6";
          src = ./.;

          nativeBuildInputs = [ pkgs.makeWrapper ];

          buildInputs = [ ];

          buildPhase = ''
          make DEBUG=0 GUI_MODE=0 -j$(nproc)
          '';

          installPhase = ''
          make install DESTDIR=$out PREFIX=/usr DEBUG=0 GUI_MODE=0
          install -Dm644 LICENSE $out/usr/share/licenses/customfetch/LICENSE
          '';

          meta = with pkgs.lib; {
            description = "Highly customizable and fast system information fetch program";
            license = licenses.gpl3;
            platforms = platforms.linux;
          };
        };

        packages.default = self'.packages.customfetch;

        apps.default = {
          type = "app";
          program = "${self'.packages.customfetch}/usr/bin/cufetch";
        };
      };
    };
}
