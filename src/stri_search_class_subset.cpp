/* This file is part of the 'stringi' package for R.
 * Copyright (C) 2013-2016, Marek Gagolewski and Bartek Tartanus
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "stri_stringi.h"
#include "stri_container_utf8.h"
#include "stri_container_charclass.h"


/**
 * Detect if a character class occurs in a string
 *
 * @param str character vector
 * @param pattern character vector
 * @param omit_na single logical value
 * @return logical vector
 *
 * @version 0.3-1 (Bartek Tartanus, 2014-07-25)
 *
 * @version 0.3-1 (Marek Gagolewski, 2014-10-17)
 *                using std::vector<int> to avoid mem-leaks
 *
 * @version 0.3-1 (Marek Gagolewski, 2014-11-04)
 *    Issue #112: str_prepare_arg* retvals were not PROTECTed from gc
 *
 * @version 0.4-1 (Marek Gagolewski, 2014-12-04)
 *    FR #122: omit_na arg added
 */
SEXP stri_subset_charclass(SEXP str, SEXP pattern, SEXP omit_na)
{
   bool omit_na1 = stri__prepare_arg_logical_1_notNA(omit_na, "omit_na");
   PROTECT(str = stri_prepare_arg_string(str, "str"));
   PROTECT(pattern = stri_prepare_arg_string(pattern, "pattern"));
   R_len_t vectorize_length =
      stri__recycling_rule(true, 2, LENGTH(str), LENGTH(pattern));

   STRI__ERROR_HANDLER_BEGIN(2)
   StriContainerUTF8 str_cont(str, vectorize_length);
   StriContainerCharClass pattern_cont(pattern, vectorize_length);

   // BT: this cannot be done with deque, because pattern is reused so i does not
   // go like 0,1,2...n but 0,pat_len,2*pat_len,1,pat_len+1 and so on
   // MG: agreed
   std::vector<int> which(vectorize_length);
   int result_counter = 0;

   for (R_len_t i = pattern_cont.vectorize_init();
         i != pattern_cont.vectorize_end();
         i = pattern_cont.vectorize_next(i))
   {
      if (str_cont.isNA(i) || pattern_cont.isNA(i)) {
         if (omit_na1) which[i] = FALSE;
         else {
            which[i] = NA_LOGICAL;
            result_counter++;
         }
         continue;
      }

      const UnicodeSet* pattern_cur = &pattern_cont.get(i);
      R_len_t     str_cur_n = str_cont.get(i).length();
      const char* str_cur_s = str_cont.get(i).c_str();

      UChar32 chr = 0;
      which[i] = FALSE;
      for (R_len_t j=0; j<str_cur_n; ) {
         U8_NEXT(str_cur_s, j, str_cur_n, chr);
         if (chr < 0) // invalid utf-8 sequence
            throw StriException(MSG__INVALID_UTF8);
         if (pattern_cur->contains(chr)) {
            which[i] = TRUE;
            result_counter++;
            break;
         }
      }
   }

   SEXP ret;
   STRI__PROTECT(ret = stri__subset_by_logical(str_cont, which, result_counter));
   STRI__UNPROTECT_ALL
   return ret;
   STRI__ERROR_HANDLER_END(;/* nothing special to be done on error */)
}
