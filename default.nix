{ mkDerivation, cmakeCleanSource, cmake
, boost_static, ptex_static, zlib_static }:
mkDerivation {
  name="ptextools";
  src= cmakeCleanSource ./.;
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ boost_static ptex_static zlib_static ];
  cmakeFlags = [
    "-DBUILD_SHARED_LIBS:BOOL=OFF"
    "-DBUILD_PYTHON_MODULE:BOOL=OFF"
    "-DBOOST_ROOT=${boost_static}"
    "-DPTEX_LOCATION=${ptex_static}"
    "-DCMAKE_LIBRARY_PATH:PATH=${zlib_static}/lib"
    "-DCMAKE_INCLUDE_PATH:PATH=${zlib_static}/include"
    "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
    "-DCMAKE_POLICY_DEFAULT_CMP0063=NEW"
  ];
}
