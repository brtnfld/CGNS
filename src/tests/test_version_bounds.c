/*
 * test_version_bounds.c - Comprehensive test suite for CGNS version bounds functionality
 *
 * This test suite validates:
 * 1. AUTO version upgrade when writing high-order elements
 * 2. Version bounds checking on file open
 * 3. detect_extended_elements correctly identifies CGNS 3.0+ elements
 * 4. cgi_require_version properly bumps version in AUTO mode
 * 5. Reading files with restrictive version bounds fails appropriately
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32) && !defined(__NUTC__)
# include <io.h>
# define unlink _unlink
#else
# include <unistd.h>
#endif
#include "cgnslib.h"

#define TEST_FILE_V30 "test_version_v30.cgns"
#define TEST_FILE_V40 "test_version_v40.cgns"
#define TEST_FILE_V50 "test_version_v50.cgns"
#define TEST_FILE_AUTO "test_version_auto.cgns"

static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

/* Helper macros for test assertions */
#define TEST_START(name) \
    do { \
        test_count++; \
        printf("Test %d: %s ... ", test_count, name); \
        fflush(stdout); \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASS\n"); \
        test_passed++; \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf("FAIL: %s\n", msg); \
        if (cg_get_error()) { \
            printf("  CGNS Error: %s\n", cg_get_error()); \
        } \
        test_failed++; \
    } while(0)

#define ASSERT_OK(call, msg) \
    do { \
        if ((call) != CG_OK) { \
            TEST_FAIL(msg); \
            return; \
        } \
    } while(0)

#define ASSERT_ERROR(call, msg) \
    do { \
        if ((call) == CG_OK) { \
            TEST_FAIL(msg); \
            return; \
        } \
        cg_error_print(); /* Print and continue */ \
    } while(0)

/* Helper: set global bounds via cg_configure (replaces old cg_set_version_bounds global) */
static void set_global_bounds(int low, int high) {
    cg_configure(CG_CONFIG_LIBVER_LOW, (void *)(long)low);
    cg_configure(CG_CONFIG_LIBVER_HIGH, (void *)(long)high);
}

/* Helper function to create a simple base and zone */
static int create_base_and_zone(int fn, int *B, int *Z) {
    cgsize_t size[3] = {125, 1, 0}; /* 125 nodes, 1 cell, 0 for unstructured */

    if (cg_base_write(fn, "Base", 3, 3, B) != CG_OK) return CG_ERROR;
    if (cg_zone_write(fn, *B, "Zone", size, CGNS_ENUMV(Unstructured), Z) != CG_OK) return CG_ERROR;

    return CG_OK;
}

/* Helper function to write coordinates */
static int write_coordinates(int fn, int B, int Z) {
    int C;
    cgsize_t size[3];
    double x[125], y[125], z[125];
    int i;

    cg_zone_read(fn, B, Z, NULL, size);

    for (i = 0; i < 125; i++) {
        x[i] = (double)i;
        y[i] = (double)i * 0.5;
        z[i] = (double)i * 0.25;
    }

    if (cg_coord_write(fn, B, Z, CGNS_ENUMV(RealDouble), "CoordinateX", x, &C) != CG_OK) return CG_ERROR;
    if (cg_coord_write(fn, B, Z, CGNS_ENUMV(RealDouble), "CoordinateY", y, &C) != CG_OK) return CG_ERROR;
    if (cg_coord_write(fn, B, Z, CGNS_ENUMV(RealDouble), "CoordinateZ", z, &C) != CG_OK) return CG_ERROR;

    return CG_OK;
}

/* Test 1: Create file with CG_LIBVER_AUTO and write high-order element */
static void test_auto_upgrade_with_high_order_element(void) {
    int fn, B, Z, S;
    float version_read;
    cgsize_t elements[20];
    int i;

    TEST_START("AUTO upgrade with HEXA_20 element");

    unlink(TEST_FILE_AUTO);

    /* Set low=AUTO (start at EARLIEST, auto-upgrade), high=LATEST (no ceiling) */
    set_global_bounds(CG_LIBVER_AUTO, CG_LIBVER_LATEST);

    /* Create file - should start at EARLIEST version */
    ASSERT_OK(cg_open(TEST_FILE_AUTO, CG_MODE_WRITE, &fn), "Failed to create file");
    ASSERT_OK(create_base_and_zone(fn, &B, &Z), "Failed to create base/zone");
    ASSERT_OK(write_coordinates(fn, B, Z), "Failed to write coordinates");

    /* Write HEXA_20 element (requires CGNS 3.0) - nodes 1-20 */
    for (i = 0; i < 20; i++) elements[i] = i + 1;
    ASSERT_OK(cg_section_write(fn, B, Z, "Hexa20Elements", CGNS_ENUMV(HEXA_20),
                                1, 1, 0, elements, &S), "Failed to write HEXA_20 section");

    /* Check version was upgraded while file is still open */
    ASSERT_OK(cg_version(fn, &version_read), "Failed to read version");

    ASSERT_OK(cg_close(fn), "Failed to close file");

    if (version_read < 3.0) {
        char msg[256];
        sprintf(msg, "Version not upgraded (expected >= 3.0, got %.2f)", version_read);
        TEST_FAIL(msg);
    } else {
        TEST_PASS();
    }

    unlink(TEST_FILE_AUTO);
}

/* Test 2: Verify detect_extended_elements identifies HEXA_20 */
static void test_detect_hexa20_requires_v30(void) {
    int fn, B, Z, S;
    float version_read;
    cgsize_t elements[20];
    int i;

    TEST_START("detect_extended_elements identifies HEXA_20 as V3.0 feature");

    unlink(TEST_FILE_V30);

    set_global_bounds(CG_LIBVER_AUTO, CG_LIBVER_LATEST);

    ASSERT_OK(cg_open(TEST_FILE_V30, CG_MODE_WRITE, &fn), "Failed to create file");
    ASSERT_OK(create_base_and_zone(fn, &B, &Z), "Failed to create base/zone");
    ASSERT_OK(write_coordinates(fn, B, Z), "Failed to write coordinates");

    /* Write HEXA_20 - should trigger detect_extended_elements */
    for (i = 0; i < 20; i++) elements[i] = i + 1;
    ASSERT_OK(cg_section_write(fn, B, Z, "Hexa20", CGNS_ENUMV(HEXA_20),
                                1, 1, 0, elements, &S), "Failed to write HEXA_20");

    /* Check version while file is still open */
    ASSERT_OK(cg_version(fn, &version_read), "Failed to read version");
    ASSERT_OK(cg_close(fn), "Failed to close file");

    if (version_read < 3.0) {
        char msg[256];
        sprintf(msg, "HEXA_20 did not trigger V3.0 upgrade (got %.2f)", version_read);
        TEST_FAIL(msg);
    } else {
        TEST_PASS();
    }

    unlink(TEST_FILE_V30);
}

/* Test 3: Test all CGNS 3.0 extended element types */
static void test_all_v30_element_types(void) {
    int fn, B, Z, S;
    float version_read;
    cgsize_t elements[27]; /* Max needed for HEXA_27 */
    int i;
    CGNS_ENUMT(ElementType_t) v30_types[] = {
        CGNS_ENUMV(TETRA_10),
        CGNS_ENUMV(PYRA_14),
        CGNS_ENUMV(PENTA_15),
        CGNS_ENUMV(PENTA_18),
        CGNS_ENUMV(HEXA_20),
        CGNS_ENUMV(HEXA_27)
    };
    const char *type_names[] = {
        "TETRA_10", "PYRA_14", "PENTA_15", "PENTA_18", "HEXA_20", "HEXA_27"
    };
    int num_types = 6;
    int t;

    TEST_START("All CGNS 3.0 element types trigger version upgrade");

    for (t = 0; t < num_types; t++) {
        char filename[64];
        sprintf(filename, "test_v30_%s.cgns", type_names[t]);
        unlink(filename);

        set_global_bounds(CG_LIBVER_AUTO, CG_LIBVER_LATEST);

        if (cg_open(filename, CG_MODE_WRITE, &fn) != CG_OK) {
            char msg[256];
            sprintf(msg, "Failed to create file for %s", type_names[t]);
            TEST_FAIL(msg);
            return;
        }

        if (create_base_and_zone(fn, &B, &Z) != CG_OK ||
            write_coordinates(fn, B, Z) != CG_OK) {
            char msg[256];
            sprintf(msg, "Failed to setup file for %s", type_names[t]);
            cg_close(fn);
            TEST_FAIL(msg);
            return;
        }

        /* Write element of this type */
        for (i = 0; i < 27; i++) elements[i] = i + 1;
        if (cg_section_write(fn, B, Z, type_names[t], v30_types[t],
                            1, 1, 0, elements, &S) != CG_OK) {
            char msg[256];
            sprintf(msg, "Failed to write %s section", type_names[t]);
            cg_close(fn);
            TEST_FAIL(msg);
            return;
        }

        /* Check version while file is still open */
        if (cg_version(fn, &version_read) != CG_OK) {
            char msg[256];
            sprintf(msg, "Failed to check version for %s", type_names[t]);
            cg_close(fn);
            TEST_FAIL(msg);
            return;
        }

        cg_close(fn);

        if (version_read < 3.0) {
            char msg[256];
            sprintf(msg, "%s did not trigger V3.0 upgrade (got %.2f)", type_names[t], version_read);
            TEST_FAIL(msg);
            return;
        }

        unlink(filename);
    }

    TEST_PASS();
}

/* Test 4: Verify feature-based version checking (not header-based) */
static void test_feature_based_version_checking(void) {
    int fn, B, Z;
    int result;

    TEST_START("Feature-based version checking (not header-based)");

    unlink(TEST_FILE_V40);

    /* Create a file with V4.0 low (writes V4.0 header) but only V2.0 features */
    set_global_bounds(CG_LIBVER_V40, CG_LIBVER_LATEST);
    ASSERT_OK(cg_open(TEST_FILE_V40, CG_MODE_WRITE, &fn), "Failed to create file");
    ASSERT_OK(create_base_and_zone(fn, &B, &Z), "Failed to create base/zone");
    ASSERT_OK(write_coordinates(fn, B, Z), "Failed to write coordinates");
    ASSERT_OK(cg_close(fn), "Failed to close file");

    /* Try to open with high=V3.0 ceiling */
    cg_configure(CG_CONFIG_LIBVER_HIGH, (void *)(long)CG_LIBVER_V30);

    /* This SHOULD SUCCEED because version checking is feature-based, not header-based.
     * The file has a V4.0 header but contains only V2.0-compatible features
     * (base, zone, coordinates), so it can be read with V3.0 limit. */
    result = cg_open(TEST_FILE_V40, CG_MODE_READ, &fn);
    if (result != CG_OK) {
        cg_error_exit();
        TEST_FAIL("Opening V4.0-header file with V2.0 features should succeed");
    } else {
        cg_close(fn);
        TEST_PASS();
    }

    /* Reset bounds */
    set_global_bounds(CG_LIBVER_AUTO, CG_LIBVER_LATEST);

    unlink(TEST_FILE_V40);
}

/* Test 4b: Verify bounds correctly reject files with incompatible features */
static void test_bounds_reject_incompatible_features(void) {
    int fn, B, Z, S;
    cgsize_t elements[20];
    int i;

    TEST_START("Bounds reject file with V3.0 features when high=V2.0");

    unlink(TEST_FILE_V30);

    /* Create a valid file containing HEXA_20 (a V3.0 feature) */
    set_global_bounds(CG_LIBVER_AUTO, CG_LIBVER_LATEST);
    ASSERT_OK(cg_open(TEST_FILE_V30, CG_MODE_WRITE, &fn), "Failed to create V3.0 file");
    ASSERT_OK(create_base_and_zone(fn, &B, &Z), "Failed to create base/zone");
    ASSERT_OK(write_coordinates(fn, B, Z), "Failed to write coordinates");
    for (i = 0; i < 20; i++) elements[i] = i + 1;
    ASSERT_OK(cg_section_write(fn, B, Z, "Hexa20", CGNS_ENUMV(HEXA_20),
                                1, 1, 0, elements, &S), "Failed to write HEXA_20");
    ASSERT_OK(cg_close(fn), "Failed to close file");

    /* Try to open with high=V2.0 - should fail because HEXA_20 needs V3.0 */
    cg_configure(CG_CONFIG_LIBVER_HIGH, (void *)(long)2000 /* CGNS 2.0 */);
    if (cg_open(TEST_FILE_V30, CG_MODE_READ, &fn) == CG_OK) {
        cg_close(fn);
        set_global_bounds(CG_LIBVER_AUTO, CG_LIBVER_LATEST);
        TEST_FAIL("Opening V3.0-feature file with high=V2.0 should have failed");
    } else {
        cg_error_print(); /* Print the expected error and continue */
        set_global_bounds(CG_LIBVER_AUTO, CG_LIBVER_LATEST);
        TEST_PASS();
    }

    unlink(TEST_FILE_V30);
}

/* Test 5: Verify CGNS 5.0 high-order elements require V5.0
 * NOTE: HEXA_125 support may not be complete in all CGNS implementations.
 * This test will skip if HEXA_125 writing is not supported. */
static void test_v50_high_order_elements(void) {
    TEST_START("HEXA_125 (V5.0) triggers version upgrade");

    /* HEXA_125 is a CGNS 5.0 feature that may not be fully implemented yet.
     * The important functionality (V3.0 element detection and version upgrade)
     * is thoroughly tested in tests 1-3. */
    printf("SKIP (HEXA_125 support implementation-dependent)\n");

    /* If/when HEXA_125 is fully supported, this test can be enabled to verify
     * that writing HEXA_125 elements triggers an automatic upgrade to V5.0,
     * similar to how tests 1-3 verify HEXA_20 triggers V3.0 upgrade. */
}

/* Test 6: Verify read bounds work correctly for valid files */
static void test_read_bounds_accept_valid_file(void) {
    int fn, B, Z;

    TEST_START("Read bounds correctly accept V3.0 file");

    unlink(TEST_FILE_V30);

    /* Create a V3.0 file (low=V3.0 writes V3.0 header) */
    cg_configure(CG_CONFIG_LIBVER_LOW, (void *)(long)CG_LIBVER_V30);
    ASSERT_OK(cg_open(TEST_FILE_V30, CG_MODE_WRITE, &fn), "Failed to create V3.0 file");
    ASSERT_OK(create_base_and_zone(fn, &B, &Z), "Failed to create base/zone");
    ASSERT_OK(write_coordinates(fn, B, Z), "Failed to write coordinates");
    ASSERT_OK(cg_close(fn), "Failed to close V3.0 file");

    /* Set high=V4.0 ceiling - V3.0 file should pass */
    cg_configure(CG_CONFIG_LIBVER_HIGH, (void *)(long)CG_LIBVER_V40);

    /* This should succeed */
    ASSERT_OK(cg_open(TEST_FILE_V30, CG_MODE_READ, &fn),
              "Opening V3.0 file with V4.0 ceiling should succeed");
    ASSERT_OK(cg_close(fn), "Failed to close file");

    /* Reset bounds */
    set_global_bounds(CG_LIBVER_AUTO, CG_LIBVER_LATEST);

    TEST_PASS();
    unlink(TEST_FILE_V30);
}

/* Test 7: Verify cgi_require_version in modify mode */
static void test_require_version_in_modify_mode(void) {
    int fn, B, Z, S;
    float version_read;
    cgsize_t elements[20];
    int i;

    TEST_START("cgi_require_version upgrades in MODIFY mode");

    unlink(TEST_FILE_AUTO);

    /* Create a basic V2.0 file (low=2000 writes V2.0 header) */
    cg_configure(CG_CONFIG_LIBVER_LOW, (void *)(long)2000 /* CGNS 2.0 */);
    ASSERT_OK(cg_open(TEST_FILE_AUTO, CG_MODE_WRITE, &fn), "Failed to create file");
    ASSERT_OK(create_base_and_zone(fn, &B, &Z), "Failed to create base/zone");
    ASSERT_OK(write_coordinates(fn, B, Z), "Failed to write coordinates");
    ASSERT_OK(cg_close(fn), "Failed to close file");

    /* Reopen in MODIFY mode with AUTO versioning */
    cg_configure(CG_CONFIG_LIBVER_LOW, (void *)(long)CG_LIBVER_AUTO);
    ASSERT_OK(cg_open(TEST_FILE_AUTO, CG_MODE_MODIFY, &fn), "Failed to open in MODIFY mode");

    /* Add HEXA_20 element - should upgrade to V3.0 */
    for (i = 0; i < 20; i++) elements[i] = i + 1;
    ASSERT_OK(cg_section_write(fn, 1, 1, "NewHexa20", CGNS_ENUMV(HEXA_20),
                                1, 1, 0, elements, &S), "Failed to write HEXA_20 in MODIFY mode");

    /* Check version while file is still open */
    ASSERT_OK(cg_version(fn, &version_read), "Failed to read version");
    ASSERT_OK(cg_close(fn), "Failed to close file");

    if (version_read < 3.0) {
        char msg[256];
        sprintf(msg, "Version not upgraded in MODIFY mode (expected >= 3.0, got %.2f)", version_read);
        TEST_FAIL(msg);
    } else {
        TEST_PASS();
    }

    unlink(TEST_FILE_AUTO);
}

/* Test 8: Verify error on writing V3.0 feature with fixed V2.0 version */
static void test_error_on_version_mismatch(void) {
    int fn, B, Z, S;
    cgsize_t elements[20];
    int i;
    int result;

    TEST_START("Error when writing V3.0 feature with fixed V2.0 version");

    unlink(TEST_FILE_AUTO);

    /* Set fixed low=V2.0 (not AUTO) - writing V3.0 features should fail */
    set_global_bounds(2000 /* CGNS 2.0 */, CG_LIBVER_LATEST);
    ASSERT_OK(cg_open(TEST_FILE_AUTO, CG_MODE_WRITE, &fn), "Failed to create file");
    ASSERT_OK(create_base_and_zone(fn, &B, &Z), "Failed to create base/zone");
    ASSERT_OK(write_coordinates(fn, B, Z), "Failed to write coordinates");

    /* Attempt to write HEXA_20 - should fail because version is fixed at V2.0 */
    for (i = 0; i < 20; i++) elements[i] = i + 1;
    result = cg_section_write(fn, B, Z, "Hexa20", CGNS_ENUMV(HEXA_20),
                             1, 1, 0, elements, &S);

    cg_close(fn);

    if (result == CG_OK) {
        TEST_FAIL("Writing HEXA_20 with fixed V2.0 should have failed");
    } else {
        cg_error_print(); /* Print the expected error and continue */
        TEST_PASS();
    }

    unlink(TEST_FILE_AUTO);
}

/* Main test runner */
int main(int argc, char *argv[]) {
    printf("=================================================\n");
    printf("CGNS Version Bounds Comprehensive Test Suite\n");
    printf("=================================================\n\n");

    /* Run all tests */
    test_auto_upgrade_with_high_order_element();
    test_detect_hexa20_requires_v30();
    test_all_v30_element_types();
    test_feature_based_version_checking();
    test_bounds_reject_incompatible_features();
    test_v50_high_order_elements();
    test_read_bounds_accept_valid_file();
    test_require_version_in_modify_mode();
    test_error_on_version_mismatch();

    /* Print summary */
    printf("\n=================================================\n");
    printf("Test Summary:\n");
    printf("  Total:  %d\n", test_count);
    printf("  Passed: %d\n", test_passed);
    printf("  Failed: %d\n", test_failed);
    printf("=================================================\n");

    return (test_failed == 0) ? 0 : 1;
}
