{ stdenv, cmakeCleanSource, cmake
, boost, ptex, zlib
, buildStaticLibs ? false
}:
let
  lib = stdenv.lib;
in stdenv.mkDerivation {
  name="ptextools";
  src= cmakeCleanSource ./.;
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ boost ptex zlib ];
  CXXFLAGS = "-D_GLIBCXX_USE_CXX11_ABI=0";
  cmakeFlags = [
   # "-DBUILD_SHARED_LIBS:BOOL=ON"
    "-DBUILD_PYTHON_MODULE:BOOL=OFF"
    "-DBOOST_ROOT=${boost}"
    "-DPTEX_LOCATION=${ptex}"
    "-DCMAKE_LIBRARY_PATH:PATH=${zlib}/lib"
    "-DCMAKE_INCLUDE_PATH:PATH=${zlib}/include"
    "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
    "-DCMAKE_POLICY_DEFAULT_CMP0063=NEW"
  ] ++ lib.optional buildStaticLibs [ "-DBUILD_SHARED_LIBS:BOOL=OFF"
  ] ++ lib.optional (!buildStaticLibs) [ "-DBUILD_SHARED_LIBS:BOOL=ON" ];
  enableParallelBuilding = true;
}
