/*
 * Test reading MIXED sections
 */

#include <stdio.h>
#include <stdlib.h>
#include "cgnslib.h"

int main()
{
    int fn;
    int err;
    CGNS_ENUMT(ElementType_t) type;
    cgsize_t start, end;
    int nbndry, parent_flag;
    char name[33];
    cgsize_t read_elements[20];
    cgsize_t read_offsets[10];

    printf("Testing CGNS MIXED Section Read\n\n");

    printf("Step 1: Opening file\n");
    err = cg_open("test_debug.cgns", CG_MODE_READ, &fn);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        return 1;
    }
    printf("  File opened, fn=%d\n", fn);

    printf("Step 2: Reading section info\n");
    err = cg_section_read(fn, 1, 1, 1, name, &type, &start, &end, &nbndry, &parent_flag);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        cg_close(fn);
        return 1;
    }
    printf("  Section name: %s, type: %d (MIXED=%d), start: %ld, end: %ld\n",
           name, type, CGNS_ENUMV(MIXED), (long)start, (long)end);

    if (type != CGNS_ENUMV(MIXED)) {
        printf("ERROR: Expected MIXED type\n");
        cg_close(fn);
        return 1;
    }

    printf("Step 3: Reading poly elements\n");
    err = cg_poly_elements_read(fn, 1, 1, 1, read_elements, read_offsets, NULL);
    if (err != CG_OK) {
        printf("ERROR: %s\n", cg_get_error());
        cg_close(fn);
        return 1;
    }
    printf("  Elements read successfully\n");

    printf("Step 4: Verifying data\n");
    printf("  Elements: ");
    for (int i = 0; i < 9; i++) {
        printf("%ld ", (long)read_elements[i]);
    }
    printf("\n");
    printf("  Offsets: ");
    for (int i = 0; i < 3; i++) {
        printf("%ld ", (long)read_offsets[i]);
    }
    printf("\n");

    printf("Step 5: Closing file\n");
    err = cg_close(fn);
    if (err != CG_OK) {
        printf("ERROR closing: %s\n", cg_get_error());
        return 1;
    }
    printf("  File closed\n");

    printf("\nAll read operations completed successfully!\n");
    return 0;
}
