# Copyright (C) 2008-2024 LEGIONFORGE <https://legionforge.gg/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# An interface library to make the target com available to other targets
add_library(legionforge-compile-option-interface INTERFACE)
# Use -std=c++11 instead of -std=gnu++11
set(CXX_EXTENSIONS OFF)
# An interface library to make the target features available to other targets
add_library(legionforge-feature-interface INTERFACE)
target_compile_features(legionforge-feature-interface
  INTERFACE
    cxx_alias_templates
    cxx_auto_type
    cxx_constexpr
    cxx_decltype
    cxx_decltype_auto
    cxx_final
    cxx_lambdas
    cxx_generic_lambdas
    cxx_variadic_templates
    cxx_defaulted_functions
    cxx_nullptr
    cxx_trailing_return_types
    cxx_return_type_deduction)
# An interface library to make the warnings level available to other targets
# This interface taget is set-up through the platform specific script
add_library(legionforge-warning-interface INTERFACE)
# An interface used for all other interfaces
add_library(legionforge-default-interface INTERFACE)
target_link_libraries(legionforge-default-interface
  INTERFACE
    legionforge-compile-option-interface
    legionforge-feature-interface)
# An interface used for silencing all warnings
add_library(legionforge-no-warning-interface INTERFACE)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(legionforge-no-warning-interface
    INTERFACE
      /W0)
else()
  target_compile_options(legionforge-no-warning-interface
    INTERFACE
      -w)
endif()
# An interface library to change the default behaviour
# to hide symbols automatically.
add_library(legionforge-hidden-symbols-interface INTERFACE)
# An interface amalgamation which provides the flags and definitions
# used by the dependency targets.
add_library(legionforge-dependency-interface INTERFACE)
target_link_libraries(legionforge-dependency-interface
  INTERFACE
    legionforge-default-interface
    legionforge-no-warning-interface
    legionforge-hidden-symbols-interface)
# An interface amalgamation which provides the flags and definitions
# used by the core targets.
add_library(legionforge-core-interface INTERFACE)
target_link_libraries(legionforge-core-interface
  INTERFACE
    legionforge-default-interface
    legionforge-warning-interface)