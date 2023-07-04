# Calc app slave CMakeList.txt

set(xapp xcalc)

set(ENABLE_MULTI_PRECISION 1)

list(APPEND SOURCE
    calc.h
    winmain.c
    convert.c
    theme.c
    htmlhelp.c
    resource.rc
)

include_directories(res)

if ( NOT ENABLE_MULTI_PRECISION)
#----------------------------------------------
	list(APPEND SOURCE
        fun_ieee.c
        rpn_ieee.c
        utl_ieee.c
    )

	add_executable(${xapp} ${SOURCE})
	target_link_libraries(${xapp} comctl32 winhttp )

else()
#----------------------------------------------	
	# To use MPFR/GMP lib

	list(APPEND SOURCE
        fun_mpfr.c
        rpn_mpfr.c
        utl_mpfr.c
    )

	add_compile_definitions(ENABLE_MULTI_PRECISION)

	find_package(PkgConfig)
	pkg_check_modules(mpfr REQUIRED IMPORTED_TARGET mpfr)

	add_executable(${xapp} ${SOURCE})
	target_link_libraries(${xapp}  PRIVATE PkgConfig::mpfr comctl32 winhttp )

endif()
#----------------------------------------------
