/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once

/* Macros for handling per-gen compilation.
 *
 * The prefixing macros GENX() and genX() automatically prefix whatever you
 * give them by GENX_ or genX_  where X is the gen number.
 *
 * You can declare a function to be used on some range of gens like this:
 *
 * GENX_FUNC(GEN7, GEN75) void
 * genX(my_function_name)(args...)
 * {
 *    // Do stuff
 * }
 *
 * If the file is compiled for any set of gens containing gen7 and gen75,
 * the function will effectively only get compiled twice as
 * gen7_my_function_nmae and gen75_my_function_name.  The function has to
 * be compilable on all gens, but it will become a static inline that gets
 * discarded by the compiler on all gens not in range.
 *
 * You can do pseudo-runtime checks in your function such as
 *
 * if (GEN_GEN > 8 || GEN_IS_HASWELL) {
 *    // Do something
 * }
 *
 * The contents of the if statement must be valid regardless of gen, but
 * the if will get compiled away on everything except haswell.
 *
 * For places where you really do have a compile-time conflict, you can
 * use preprocessor logic:
 *
 * #if (GEN_GEN > 8 || GEN_IS_HASWELL)
 *    // Do something
 * #endif
 *
 * However, it is strongly recommended that the former be used whenever
 * possible.
 */

/* Base macro defined on the command line.  If we don't have this, we can't
 * do anything.
 */
#ifndef GEN_VERSIONx10
#  error "The GEN_VERSIONx10 macro must be defined"
#endif

#define GEN_GEN ((GEN_VERSIONx10) / 10)
#define GEN_IS_HASWELL ((GEN_VERSIONx10) == 75)

/* Prefixing macros */
#if (GEN_VERSIONx10 == 70)
#  define GENX(X) GEN7_##X
#  define genX(x) gen7_##x
#elif (GEN_VERSIONx10 == 75)
#  define GENX(X) GEN75_##X
#  define genX(x) gen75_##x
#elif (GEN_VERSIONx10 == 80)
#  define GENX(X) GEN8_##X
#  define genX(x) gen8_##x
#elif (GEN_VERSIONx10 == 90)
#  define GENX(X) GEN9_##X
#  define genX(x) gen9_##x
#else
#  error "Need to add prefixing macros for this gen"
#endif
