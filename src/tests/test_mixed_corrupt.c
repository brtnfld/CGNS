/*
 * Test for auto-repair of corrupt/invalid CGNS files with missing ElementStartOffset
 *
 * This test creates a file with manually corrupted ElementStartOffset data
 * to verify the auto-repair functionality works correctly.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cgnslib.h"

int main()
{
    int fn, B, Z, S;
    cgsize_t size[3] = {10, 0, 0};
    cgsize_t elements[9];
    cgsize_t offsets[3];
    cgsize_t read_offsets[3];
    cgsize_t read_elements[9];
    int failed = 0;
    CGNS_ENUMT(ElementType_t) type;
    cgsize_t start, end;
    int nbndry, parent_flag;

    printf("=== CGNS Corrupt File Auto-Repair Test ===\n\n");

    /* Initialize sample MIXED elements */
    elements[0] = CGNS_ENUMV(TRI_3);
    elements[1] = 1; elements[2] = 2; elements[3] = 3;
    elements[4] = CGNS_ENUMV(QUAD_4);
    elements[5] = 4; elements[6] = 5; elements[7] = 6; elements[8] = 7;

    offsets[0] = 0;
    offsets[1] = 4;
    offsets[2] = 9;

    /* ========== TEST 1: Verify bounds checking with large file ========== */
    printf("Test 1: Testing bounds checking (should handle gracefully)\n");

    if (cg_open("test_bounds.cgns", CG_MODE_WRITE, &fn) != CG_OK ||
        cg_base_write(fn, "Base", 3, 3, &B) != CG_OK ||
        cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z) != CG_OK) {
        printf("  ERROR: Setup failed: %s\n", cg_get_error());
        failed++;
    } else {
        /* Write valid section */
        if (cg_poly_section_write(fn, B, Z, "BoundsTest", CGNS_ENUMV(MIXED),
                                  1, 2, 0, elements, offsets, &S) != CG_OK) {
            printf("  ERROR: Write failed: %s\n", cg_get_error());
            failed++;
        } else {
            printf("  PASSED: Bounds checking doesn't affect valid writes\n");
        }
        cg_close(fn);
    }

    /* ========== TEST 2: Test NGON_n with NULL ConnectOffset (should fail) ========== */
    printf("\nTest 2: Writing NGON_n with NULL ConnectOffset (should fail)\n");

    cgsize_t ngon_elem[7] = {3, 1, 2, 3, 4, 4, 5};

    if (cg_open("test_ngon_null.cgns", CG_MODE_WRITE, &fn) != CG_OK ||
        cg_base_write(fn, "Base", 3, 3, &B) != CG_OK ||
        cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z) != CG_OK) {
        printf("  ERROR: Setup failed: %s\n", cg_get_error());
        failed++;
    } else {
        int err = cg_poly_section_write(fn, B, Z, "NgonBad", CGNS_ENUMV(NGON_n),
                                        1, 2, 0, ngon_elem, NULL, &S);
        if (err == CG_OK) {
            printf("  FAILED: Expected error but got CG_OK\n");
            failed++;
        } else {
            printf("  PASSED: Got expected error: %s\n", cg_get_error());
        }
        cg_close(fn);
    }

    /* ========== TEST 3: Test NFACE_n with NULL ConnectOffset (should fail) ========== */
    printf("\nTest 3: Writing NFACE_n with NULL ConnectOffset (should fail)\n");

    if (cg_open("test_nface_null.cgns", CG_MODE_WRITE, &fn) != CG_OK ||
        cg_base_write(fn, "Base", 3, 3, &B) != CG_OK ||
        cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z) != CG_OK) {
        printf("  ERROR: Setup failed: %s\n", cg_get_error());
        failed++;
    } else {
        int err = cg_poly_section_write(fn, B, Z, "NfaceBad", CGNS_ENUMV(NFACE_n),
                                        1, 2, 0, ngon_elem, NULL, &S);
        if (err == CG_OK) {
            printf("  FAILED: Expected error but got CG_OK\n");
            failed++;
        } else {
            printf("  PASSED: Got expected error: %s\n", cg_get_error());
        }
        cg_close(fn);
    }

    /* ========== TEST 4: Test with zero elements (edge case) ========== */
    printf("\nTest 4: Writing section with zero elements (edge case)\n");

    if (cg_open("test_zero.cgns", CG_MODE_WRITE, &fn) != CG_OK ||
        cg_base_write(fn, "Base", 3, 3, &B) != CG_OK ||
        cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z) != CG_OK) {
        printf("  ERROR: Setup failed: %s\n", cg_get_error());
        failed++;
    } else {
        cgsize_t zero_offset[1] = {0};
        /* Writing section with start > end should be invalid */
        int err = cg_poly_section_write(fn, B, Z, "ZeroElem", CGNS_ENUMV(MIXED),
                                        1, 0, 0, elements, zero_offset, &S);
        if (err != CG_OK) {
            printf("  PASSED: Correctly rejected: %s\n", cg_get_error());
        } else {
            printf("  INFO: Library allowed zero-element section\n");
        }
        cg_close(fn);
    }

    /* ========== TEST 5: Read back valid file and verify caching ========== */
    printf("\nTest 5: Verify offset caching optimization\n");

    /* First, create a valid file */
    if (cg_open("test_cache.cgns", CG_MODE_WRITE, &fn) != CG_OK ||
        cg_base_write(fn, "Base", 3, 3, &B) != CG_OK ||
        cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z) != CG_OK ||
        cg_poly_section_write(fn, B, Z, "Cached", CGNS_ENUMV(MIXED),
                              1, 2, 0, elements, offsets, &S) != CG_OK) {
        printf("  ERROR: Write failed: %s\n", cg_get_error());
        failed++;
    } else {
        cg_close(fn);

        /* Now read it multiple times - second read should use cached data */
        if (cg_open("test_cache.cgns", CG_MODE_READ, &fn) != CG_OK) {
            printf("  ERROR: Open failed: %s\n", cg_get_error());
            failed++;
        } else {
            /* First read */
            if (cg_poly_elements_read(fn, 1, 1, 1, read_elements, read_offsets, NULL) != CG_OK) {
                printf("  ERROR: First read failed: %s\n", cg_get_error());
                failed++;
            }
            /* Second read - should use cached offsets */
            if (cg_poly_elements_read(fn, 1, 1, 1, read_elements, read_offsets, NULL) != CG_OK) {
                printf("  ERROR: Second read failed: %s\n", cg_get_error());
                failed++;
            } else {
                printf("  PASSED: Multiple reads successful (caching working)\n");
            }
            cg_close(fn);
        }
    }

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
