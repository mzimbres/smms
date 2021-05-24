/* Copyright (c) 2018-2021 Marcelo Zimbres Silva (mzimbres at gmail dot com)
 *
 * This file is part of smms.
 *
 * smms is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * smms is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with smms.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <vector>
#include <string>
#include <iostream>

#include "utils.hpp"

using namespace smms;

using ret_type = std::vector<std::string>;

void
check_query(
   std::string const& q,
   ret_type const& res,
   std::string const& info)
{
   auto const r1 = parse_query(q);
   if (r1 != res)
      std::cout << "Error: " << info << std::endl;
   else
      std::cout << "Success: " << info << std::endl;
}

void query_test1()
{
   std::string q1 = "";
   std::string q2 = "f1";
   std::string q3 = "f1=";
   std::string q4 = "f1=v1";
   std::string q5 = "f1=v1&";
   std::string q6 = "f1=v1&f2";
   std::string q7 = "f1=v1&f2=";
   std::string q8 = "f1=v1&f2=v2";
   std::string q9 = "f1=v1&f2=v2&";

   check_query(q1, {}, "r1");
   check_query(q2, {}, "r2");
   check_query(q3, {}, "r3");
   check_query(q4, {"f1", "v1"}, "r4");
   check_query(q5, {"f1", "v1"}, "r5");
   check_query(q6, {}, "r6");
   check_query(q7, {}, "r7");
   check_query(q8, {"f1", "v1", "f2", "v2"}, "r8");
   check_query(q9, {"f1", "v1", "f2", "v2"}, "r9");
}

void query_test2()
{
   std::string q11 = "";
   std::string q12 = "f";
   std::string q13 = "f=";
   std::string q14 = "f=v";
   std::string q15 = "f=v&";
   std::string q16 = "f=v&f";
   std::string q17 = "f=v&f=";
   std::string q18 = "f=v&f=v";
   std::string q19 = "f=v&f=v&";

   check_query(q11, {}, "r11");
   check_query(q12, {}, "r12");
   check_query(q13, {}, "r13");
   check_query(q14, {"f", "v"}, "r14");
   check_query(q15, {"f", "v"}, "r15");
   check_query(q16, {}, "r16");
   check_query(q17, {}, "r17");
   check_query(q18, {"f", "v", "f", "v"}, "r18");
   check_query(q19, {"f", "v", "f", "v"}, "r19");
}

void query_test3()
{
   std::string q21 = "=";
   std::string q22 = "&";
   std::string q23 = "f&v";
   std::string q24 = "=&v";
   std::string q25 = "a&b=v";

   check_query(q21, {}, "r21");
   check_query(q22, {}, "r22");
   check_query(q23, {}, "r23");
   check_query(q24, {}, "r24");
   check_query(q25, {}, "r25");
}

void
check_dir(
   std::string const& target,
   std::string const& expected,
   std::string const& info)
{
   auto const r1 = parse_dir(target);
   if (r1 != expected)
      std::cout << "Error: " << info << std::endl;
   else
      std::cout << "Success: " << info << std::endl;
}

void parse_dir_test1()
{
   std::string t1 = "";
   std::string t2 = "foo";
   std::string t3 = "/foo";
   std::string t4 = "/foo/bar";
   std::string t5 = "/foo/bar/";
   std::string t6 = "/foo/bar.txt";
   std::string t7 = "///bar.txt";

   check_dir(t1, {}, "t1");
   check_dir(t2, {}, "t2");
   check_dir(t3, {}, "t3");
   check_dir(t4, {"/foo"}, "t4");
   check_dir(t5, {"/foo/bar"}, "t5");
   check_dir(t6, {"/foo"}, "t6");
   check_dir(t7, {"//"}, "t7");
}

int main()
{
   query_test1();
   query_test2();
   query_test3();
   parse_dir_test1();
}
