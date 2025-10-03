/*
 * Test for NULL ConnectOffset validation with MIXED sections
 *
 * This test verifies the fix for the issue where cg_poly_section_write()
 * would accept NULL ConnectOffset for MIXED sections, creating invalid files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cgnslib.h"

#define CHECK_ERROR(fn_call, expected_result) \
    do { \
        int err = (fn_call); \
        if (err != expected_result) { \
            printf("  FAILED at line %d: expected %s but got %s\n", \
                   __LINE__, \
                   expected_result == CG_OK ? "CG_OK" : "CG_ERROR", \
                   err == CG_OK ? "CG_OK" : "CG_ERROR"); \
            if (err != CG_OK) printf("  Error: %s\n", cg_get_error()); \
            failed++; \
        } \
    } while(0)

int main()
{
    int fn, B, Z, S;
    cgsize_t size[3] = {10, 0, 0};
    cgsize_t elements[20];
    cgsize_t offsets[3];
    cgsize_t read_elements[20];
    cgsize_t read_offsets[3];
    int failed = 0;

    printf("=== CGNS MIXED Section NULL ConnectOffset Test ===\n\n");

    /* Initialize sample MIXED elements:
     * Element 1: TRI_3 (type + 3 nodes = 4 values)
     * Element 2: QUAD_4 (type + 4 nodes = 5 values)
     * Total: 9 values
     */
    elements[0] = CGNS_ENUMV(TRI_3);   /* Element 1 type */
    elements[1] = 1; elements[2] = 2; elements[3] = 3;  /* Element 1 nodes */
    elements[4] = CGNS_ENUMV(QUAD_4);  /* Element 2 type */
    elements[5] = 4; elements[6] = 5; elements[7] = 6; elements[8] = 7;  /* Element 2 nodes */

    /* Valid offsets array */
    offsets[0] = 0;   /* Start of element 1 */
    offsets[1] = 4;   /* Start of element 2 (after type + 3 nodes) */
    offsets[2] = 9;   /* End (after type + 4 nodes) */

    /* ========== TEST 1: Writing with NULL ConnectOffset should fail ========== */
    printf("Test 1: Writing MIXED section with NULL ConnectOffset (should fail)\n");

    CHECK_ERROR(cg_open("test_mixed_null.cgns", CG_MODE_WRITE, &fn), CG_OK);
    CHECK_ERROR(cg_base_write(fn, "Base", 3, 3, &B), CG_OK);
    CHECK_ERROR(cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z), CG_OK);

    /* This should fail with the fix */
    CHECK_ERROR(cg_poly_section_write(fn, B, Z, "MixedBad", CGNS_ENUMV(MIXED),
                                      1, 2, 0, elements, NULL, &S), CG_ERROR);

    if (cg_poly_section_write(fn, B, Z, "MixedBad", CGNS_ENUMV(MIXED),
                              1, 2, 0, elements, NULL, &S) == CG_OK) {
        printf("  FAILED: Expected CG_ERROR but got CG_OK\n");
        failed++;
    } else {
        printf("  PASSED: Got expected error: %s\n", cg_get_error());
    }

    cg_close(fn);

    /* ========== TEST 2: Writing with valid ConnectOffset should succeed ========== */
    printf("\nTest 2: Writing MIXED section with valid ConnectOffset (should succeed)\n");

    CHECK_ERROR(cg_open("test_mixed_valid.cgns", CG_MODE_WRITE, &fn), CG_OK);
    CHECK_ERROR(cg_base_write(fn, "Base", 3, 3, &B), CG_OK);
    CHECK_ERROR(cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z), CG_OK);

    CHECK_ERROR(cg_poly_section_write(fn, B, Z, "MixedGood", CGNS_ENUMV(MIXED),
                                      1, 2, 0, elements, offsets, &S), CG_OK);

    printf("  PASSED: Section written successfully\n");

    CHECK_ERROR(cg_close(fn), CG_OK);

    /* ========== TEST 3: Reading the valid file should succeed ========== */
    printf("\nTest 3: Reading MIXED section with valid ConnectOffset (should succeed)\n");

    CHECK_ERROR(cg_open("test_mixed_valid.cgns", CG_MODE_READ, &fn), CG_OK);
    CHECK_ERROR(cg_base_read(fn, 1, NULL, NULL, NULL), CG_OK);
    CHECK_ERROR(cg_zone_read(fn, 1, 1, NULL, size), CG_OK);

    /* Read section info */
    CGNS_ENUMT(ElementType_t) type;
    cgsize_t start, end;
    int nbndry, parent_flag;
    CHECK_ERROR(cg_section_read(fn, 1, 1, 1, NULL, &type, &start, &end, &nbndry, &parent_flag), CG_OK);

    /* Verify section type */
    if (type != CGNS_ENUMV(MIXED)) {
        printf("  FAILED: Expected MIXED element type\n");
        failed++;
    } else {
        printf("  Section type: MIXED (correct)\n");
    }

    /* Read elements and offsets */
    CHECK_ERROR(cg_poly_elements_read(fn, 1, 1, 1, read_elements, read_offsets, NULL), CG_OK);

    /* Verify data */
    int data_ok = 1;
    for (int i = 0; i < 9; i++) {
        if (read_elements[i] != elements[i]) {
            printf("  FAILED: Element connectivity mismatch at index %d\n", i);
            data_ok = 0;
            failed++;
            break;
        }
    }
    for (int i = 0; i < 3; i++) {
        if (read_offsets[i] != offsets[i]) {
            printf("  FAILED: Offset mismatch at index %d: expected %ld, got %ld\n",
                   i, (long)offsets[i], (long)read_offsets[i]);
            data_ok = 0;
            failed++;
            break;
        }
    }

    if (data_ok) {
        printf("  PASSED: All data read correctly\n");
    }

    CHECK_ERROR(cg_close(fn), CG_OK);

    /* ========== TEST 4: NGON_n with NULL ConnectOffset should also fail ========== */
    printf("\nTest 4: Writing NGON_n section with NULL ConnectOffset (should fail)\n");

    cgsize_t ngon_elem[7];
    /* Face 1: 3 nodes */
    ngon_elem[0] = 3;
    ngon_elem[1] = 1; ngon_elem[2] = 2; ngon_elem[3] = 3;
    /* Face 2: 4 nodes */
    ngon_elem[4] = 4;
    ngon_elem[5] = 4; ngon_elem[6] = 5;

    CHECK_ERROR(cg_open("test_ngon_null.cgns", CG_MODE_WRITE, &fn), CG_OK);
    CHECK_ERROR(cg_base_write(fn, "Base", 3, 3, &B), CG_OK);
    CHECK_ERROR(cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z), CG_OK);

    CHECK_ERROR(cg_poly_section_write(fn, B, Z, "NgonBad", CGNS_ENUMV(NGON_n),
                                      1, 2, 0, ngon_elem, NULL, &S), CG_ERROR);

    printf("  PASSED: Got expected error for NGON_n: %s\n", cg_get_error());

    cg_close(fn);

    /* ========== TEST 5: Fixed-size elements don't require ConnectOffset ========== */
    printf("\nTest 5: Writing TRI_3 section without ConnectOffset (should succeed)\n");

    cgsize_t tri_elem[6] = {1, 2, 3, 4, 5, 6};  /* 2 triangles */

    CHECK_ERROR(cg_open("test_tri.cgns", CG_MODE_WRITE, &fn), CG_OK);
    CHECK_ERROR(cg_base_write(fn, "Base", 3, 3, &B), CG_OK);
    CHECK_ERROR(cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z), CG_OK);

    /* Fixed-size elements can use cg_section_write (old API) or cg_poly_section_write with NULL */
    CHECK_ERROR(cg_section_write(fn, B, Z, "Triangles", CGNS_ENUMV(TRI_3),
                                 1, 2, 0, tri_elem, &S), CG_OK);

    printf("  PASSED: Fixed-size elements written successfully\n");

    CHECK_ERROR(cg_close(fn), CG_OK);

    /* ========== Summary ========== */
    printf("\n=== Test Summary ===\n");
    if (failed == 0) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("FAILED: %d test(s) failed\n", failed);
        return 1;
    }
}
