/*
 * Simple test for NULL ConnectOffset validation with MIXED sections
 */

#include <stdio.h>
#include <stdlib.h>
#include "cgnslib.h"

int main()
{
    int fn, B, Z, S;
    cgsize_t size[3] = {10, 0, 0};
    cgsize_t elements[9];
    int err;

    printf("Testing CGNS MIXED Section NULL ConnectOffset Fix\n\n");

    /* Initialize sample MIXED elements */
    elements[0] = CGNS_ENUMV(TRI_3);
    elements[1] = 1; elements[2] = 2; elements[3] = 3;
    elements[4] = CGNS_ENUMV(QUAD_4);
    elements[5] = 4; elements[6] = 5; elements[7] = 6; elements[8] = 7;

    printf("Test: Writing MIXED section with NULL ConnectOffset (should fail)\n");

    err = cg_open("test_null.cgns", CG_MODE_WRITE, &fn);
    if (err != CG_OK) {
        printf("ERROR opening file: %s\n", cg_get_error());
        return 1;
    }

    err = cg_base_write(fn, "Base", 3, 3, &B);
    if (err != CG_OK) {
        printf("ERROR writing base: %s\n", cg_get_error());
        cg_close(fn);
        return 1;
    }

    err = cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z);
    if (err != CG_OK) {
        printf("ERROR writing zone: %s\n", cg_get_error());
        cg_close(fn);
        return 1;
    }

    /* This should fail with the fix */
    err = cg_poly_section_write(fn, B, Z, "MixedBad", CGNS_ENUMV(MIXED),
                                1, 2, 0, elements, NULL, &S);

    cg_close(fn);

    if (err == CG_OK) {
        printf("FAILED: Expected CG_ERROR but got CG_OK\n");
        return 1;
    } else {
        printf("PASSED: Got expected error: %s\n", cg_get_error());
        return 0;
    }
}
