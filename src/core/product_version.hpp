#pragma once
/**
 * @file product_version.hpp
 * @brief Single source of truth for the product version string.
 *
 * CMake resolves `DABX_VERSION` at configure time (the latest git tag when
 * available, otherwise the project version) and passes it to every target as
 * a compile definition. The fallback below keeps the header self-contained
 * when it is included outside a CMake build (e.g. by an IDE indexer).
 */
#ifndef DABX_VERSION
#define DABX_VERSION "0.0.0-dev"
#endif
