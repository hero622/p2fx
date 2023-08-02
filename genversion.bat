for /f "tokens=* usebackq" %%x in (`git describe --tags`) do (set version=%%x)
echo #define P2FX_VERSION "%version%">Version.hpp
if "%RELEASE_BUILD%"=="" echo #define P2FX_DEV_BUILD 1>>Version.hpp
echo #define P2FX_DEMO_SIGN_PUBKEY { %DEMO_SIGN_PUBKEY% }>>Version.hpp
echo #define P2FX_DEMO_SIGN_PRIVKEY { %DEMO_SIGN_PRIVKEY% }>>Version.hpp
