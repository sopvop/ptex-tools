{ smpkgs ? import ~/work/foo/packages.nix {}
}:
with smpkgs;
let
  p = callPackage ./default.nix {
     inherit (nixpkgs.stdenv) mkDerivation;
   };
in nixpkgs.stdenv.lib.overrideDerivation p (attrs: {
   shellHook =
   ''
      export cmakeFlags="$cmakeFlags -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON"
   '';
})
