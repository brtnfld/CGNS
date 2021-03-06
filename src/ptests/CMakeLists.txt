#########
# Tests #
#########

if (NOT MPIEXEC)
  message(SEND_ERROR "need to define MPIEXEC to run tests")
endif (NOT MPIEXEC)

link_directories(.)
link_libraries(cgns_static)
link_libraries(${HDF5_LIBRARY})
link_libraries(${MPI_LIBS})

if(HDF5_NEED_ZLIB AND ZLIB_LIBRARY)
  link_libraries(${ZLIB_LIBRARY})
endif(HDF5_NEED_ZLIB AND ZLIB_LIBRARY)
if(HDF5_NEED_SZIP AND SZIP_LIBRARY)
  link_libraries(${SZIP_LIBRARY})
endif(HDF5_NEED_SZIP AND SZIP_LIBRARY)

if (NOT WIN32)
  link_libraries(m)
endif (NOT WIN32)

set(pcgns_ctest_FILES pcgns_ctest.c)
add_executable(pcgns_ctest ${pcgns_ctest_FILES})

set(cexample_FILES cexample.c)
add_executable(cexample ${cexample_FILES})

set(benchmark_FILES benchmark.c)
add_executable(benchmark ${benchmark_FILES})

set(benchmark_hdf5_FILES benchmark_hdf5.c)
add_executable(benchmark_hdf5 ${benchmark_hdf5_FILES})

set(Metadata_Stressing_FILES Metadata_Stressing.c)
add_executable(Metadata_Stressing ${Metadata_Stressing_FILES})

set(open_close_FILES open_close.c)
add_executable(open_close ${open_close_FILES})

set(comm_test_FILES comm_test.c)
add_executable(comm_test ${comm_test_FILES})

set(test_base_FILES test_base.c)
add_executable(test_base ${test_base_FILES})

set(test_unstructured_FILES test_unstructured.c)
add_executable(test_unstructured ${test_unstructured_FILES})

set(test_unstruc_quad_FILES test_unstruc_quad.c)
add_executable(test_unstruc_quad ${test_unstruc_quad_FILES})

set(test_zone_FILES test_zone.c)
add_executable(test_zone ${test_zone_FILES})

set(test_general_readwrite_FILES test_general_readwrite.c)
add_executable(test_general_readwrite ${test_general_readwrite_FILES})

set(test_poly_unstructured_FILES test_poly_unstructured.c)
add_executable(test_poly_unstructured ${test_poly_unstructured_FILES})

set(thesis_benchmark_FILES thesis_benchmark.c)
add_executable(thesis_benchmark ${thesis_benchmark_FILES})

if (CGNS_ENABLE_FORTRAN AND HAVE_FORTRAN_2003)
  set(pcgns_ftest_FILES pcgns_ftest.F90)
  add_executable(pcgns_ftest ${pcgns_ftest_FILES})
  set(fexample_FILES fexample.F90)
  add_executable(fexample ${fexample_FILES})
  set(benchmark_hdf5_f90_FILES benchmark_hdf5_f90.F90)
  add_executable(benchmark_hdf5_f90 ${benchmark_hdf5_f90_FILES})
  set(test_mixed_par_ser_FILES test_mixed_par_ser.F90)
  add_executable(test_mixed_par_ser ${test_mixed_par_ser_FILES})
endif (CGNS_ENABLE_FORTRAN AND HAVE_FORTRAN_2003)

# Add the tests so that cmake can find them
if (CGNS_ENABLE_TESTS)
  add_test(pcgns_ctest ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./pcgns_ctest ${MPIEXEC_POSTFLAGS})
  add_test(cexample ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./cexample ${MPIEXEC_POSTFLAGS})
  add_test(benchmark ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./benchmark ${MPIEXEC_POSTFLAGS})
  add_test(benchmark_hdf5 ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./benchmark_hdf5 ${MPIEXEC_POSTFLAGS})
  add_test(Metadata_Stressing ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./Metadata_Stressing ${MPIEXEC_POSTFLAGS})
  add_test(open_close ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./open_close ${MPIEXEC_POSTFLAGS})
  add_test(comm_test ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./comm_test ${MPIEXEC_POSTFLAGS})
  add_test(test_base ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./test_base ${MPIEXEC_POSTFLAGS})
  add_test(test_unstructured ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./test_unstructured ${MPIEXEC_POSTFLAGS})
  add_test(test_unstruc_quad ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./test_unstruc_quad ${MPIEXEC_POSTFLAGS})
  add_test(test_zone ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./test_zone ${MPIEXEC_POSTFLAGS})
  add_test(test_general_readwrite ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./test_general_readwrite ${MPIEXEC_POSTFLAGS})
  add_test(test_poly_unstructured ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./test_poly_unstructured ${MPIEXEC_POSTFLAGS})
  add_test(thesis_benchmark ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
      ${MPIEXEC_PREFLAGS} ./thesis_benchmark ${MPIEXEC_POSTFLAGS})
  if (CGNS_ENABLE_FORTRAN  AND HAVE_FORTRAN_2003)
    add_test(pcgns_ftest ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
        ${MPIEXEC_PREFLAGS} ./pcgns_ftest ${MPIEXEC_POSTFLAGS})
    add_test(fexample ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
        ${MPIEXEC_PREFLAGS} ./fexample ${MPIEXEC_POSTFLAGS})
    add_test(benchmark_hdf5_f90 ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
        ${MPIEXEC_PREFLAGS} ./benchmark_hdf5_f90 ${MPIEXEC_POSTFLAGS})
    add_test(test_mixed_par_ser ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} "2"
        ${MPIEXEC_PREFLAGS} ./test_mixed_par_ser ${MPIEXEC_POSTFLAGS})
  endif (CGNS_ENABLE_FORTRAN  AND HAVE_FORTRAN_2003)
endif (CGNS_ENABLE_TESTS)
