set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# Disable vectorized STL algorithms to avoid linker issues with satellite libraries
set(VCPKG_CXX_FLAGS "/D_DISABLE_VECTOR_ANNOTATION /D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")
set(VCPKG_C_FLAGS "")
