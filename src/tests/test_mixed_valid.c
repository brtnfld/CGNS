/*
 * Test for valid ConnectOffset with MIXED sections
 */

#include <stdio.h>
#include <stdlib.h>
#include "cgnslib.h"

int main()
{
    int fn, B, Z, S;
    cgsize_t size[3] = {10, 0, 0};
    cgsize_t elements[9];
    cgsize_t offsets[3];
    cgsize_t read_elements[9];
    cgsize_t read_offsets[3];
    int err, i;
    CGNS_ENUMT(ElementType_t) type;
    cgsize_t start, end;
    int nbndry, parent_flag;

    printf("Testing CGNS MIXED Section with Valid ConnectOffset\n\n");

    /* Initialize sample MIXED elements */
    elements[0] = CGNS_ENUMV(TRI_3);
    elements[1] = 1; elements[2] = 2; elements[3] = 3;
    elements[4] = CGNS_ENUMV(QUAD_4);
    elements[5] = 4; elements[6] = 5; elements[7] = 6; elements[8] = 7;

    /* Valid offsets array */
    offsets[0] = 0;   /* Start of element 1 */
    offsets[1] = 4;   /* Start of element 2 (after type + 3 nodes) */
    offsets[2] = 9;   /* End (after type + 4 nodes) */

    printf("Test 1: Writing MIXED section with valid ConnectOffset (should succeed)\n");

    err = cg_open("test_valid.cgns", CG_MODE_WRITE, &fn);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        return 1;
    }

    err = cg_base_write(fn, "Base", 3, 3, &B);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        cg_close(fn);
        return 1;
    }

    err = cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        cg_close(fn);
        return 1;
    }

    err = cg_poly_section_write(fn, B, Z, "MixedGood", CGNS_ENUMV(MIXED),
                                1, 2, 0, elements, offsets, &S);
    if (err != CG_OK) {
        printf("FAILED: %s\n", cg_get_error());
        cg_close(fn);
        return 1;
    }

    printf("  PASSED: Section written successfully\n");

    err = cg_close(fn);
    if (err != CG_OK) {
        printf("ERROR closing: %s\n", cg_get_error());
        return 1;
    }

    printf("\nTest 2: Reading MIXED section (should succeed)\n");

    err = cg_open("test_valid.cgns", CG_MODE_READ, &fn);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        return 1;
    }

    err = cg_section_read(fn, 1, 1, 1, NULL, &type, &start, &end, &nbndry, &parent_flag);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        cg_close(fn);
        return 1;
    }

    if (type != CGNS_ENUMV(MIXED)) {
        printf("FAILED: Expected MIXED element type\n");
        cg_close(fn);
        return 1;
    }
    printf("  Section type: MIXED (correct)\n");

    err = cg_poly_elements_read(fn, 1, 1, 1, read_elements, read_offsets, NULL);
    if (err != CG_OK) {
        printf("FAILED: %s\n", cg_get_error());
        cg_close(fn);
        return 1;
    }

    /* Verify data */
    int data_ok = 1;
    for (i = 0; i < 9; i++) {
        if (read_elements[i] != elements[i]) {
            printf("FAILED: Element mismatch at index %d\n", i);
            data_ok = 0;
            break;
        }
    }
    for (i = 0; i < 3; i++) {
        if (read_offsets[i] != offsets[i]) {
            printf("FAILED: Offset mismatch at index %d: expected %ld, got %ld\n",
                   i, (long)offsets[i], (long)read_offsets[i]);
            data_ok = 0;
            break;
        }
    }

    if (data_ok) {
        printf("  PASSED: All data read correctly\n");
    }

    cg_close(fn);

    if (data_ok) {
        printf("\nAll tests PASSED!\n");
        return 0;
    } else {
        return 1;
    }
}
