/*
 * Debug test for MIXED sections
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
    int err;

    printf("Testing CGNS MIXED Section - Debug Version\n\n");

    /* Initialize sample MIXED elements */
    elements[0] = CGNS_ENUMV(TRI_3);
    elements[1] = 1; elements[2] = 2; elements[3] = 3;
    elements[4] = CGNS_ENUMV(QUAD_4);
    elements[5] = 4; elements[6] = 5; elements[7] = 6; elements[8] = 7;

    /* Valid offsets array */
    offsets[0] = 0;
    offsets[1] = 4;
    offsets[2] = 9;

    printf("Step 1: Opening file\n");
    err = cg_open("test_debug.cgns", CG_MODE_WRITE, &fn);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        return 1;
    }
    printf("  File opened, fn=%d\n", fn);

    printf("Step 2: Writing base\n");
    err = cg_base_write(fn, "Base", 3, 3, &B);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        return 1;
    }
    printf("  Base written, B=%d\n", B);

    printf("Step 3: Writing zone\n");
    err = cg_zone_write(fn, B, "Zone", size, CGNS_ENUMV(Unstructured), &Z);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        return 1;
    }
    printf("  Zone written, Z=%d\n", Z);

    printf("Step 4: Writing poly section\n");
    err = cg_poly_section_write(fn, B, Z, "MixedGood", CGNS_ENUMV(MIXED),
                                1, 2, 0, elements, offsets, &S);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        return 1;
    }
    printf("  Section written, S=%d\n", S);

    printf("Step 5: Closing file\n");
    err = cg_close(fn);
    if (err != CG_OK) {
        printf("ERROR closing: %s\n", cg_get_error());
        return 1;
    }
    printf("  File closed\n");

    printf("\nAll write operations completed successfully!\n");
    return 0;
}
