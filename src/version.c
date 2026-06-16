/**
 * version.c - HolePunchMan.
 * Summary: Build version accessors for library consumers.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#include "hpm.h"

#ifndef KC_HPM_BUILD_VERSION
#define KC_HPM_BUILD_VERSION 0
#endif

/**
 * Returns the build version generated at compile time.
 * @return Unix timestamp for the current build.
 */
uint64_t kc_hpm_version(void) {
    return (uint64_t)KC_HPM_BUILD_VERSION;
}
