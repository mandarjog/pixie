/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "src/carnot/funcs/http/http_ops.h"
#include "src/carnot/udf/test_utils.h"
#include "src/common/base/base.h"

namespace px {
namespace carnot {
namespace funcs {
namespace http {

TEST(HTTPOps, basic) {
  auto udf_tester = udf::UDFTester<HTTPRespMessageUDF>();
  udf_tester.ForInput(400).Expect("Bad Request");
  udf_tester.ForInput(0).Expect("Unassigned");
}

}  // namespace http
}  // namespace funcs
}  // namespace carnot
}  // namespace px
