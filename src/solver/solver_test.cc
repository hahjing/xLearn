//------------------------------------------------------------------------------
// Copyright (c) 2016 by contributors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//------------------------------------------------------------------------------

/*
Author: Chao Ma (mctt90@gmail.com)

This file tests the Solver class.
*/

#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "src/solver/solver.h"
#include "src/base/file_util.h"

namespace xLearn {

typedef std::vector<std::string> StringList;

int argc = 14;
const char* argv[100] = {"./xLearn", "--is_train", "-train_data",
                         "/tmp/solver_test.txt", "-score", "ffm",
                         "-loss", "cross-entropy", "-k", "16",
                         "-lr", "0.5", "-file_format", "libffm"};

std::string filename = "/tmp/solver_test.txt";
int kNumLines = 3 * 1000;

class SolverTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    StringList data;
    for (int i = 0 ; i < 1000; ++i) {
      data.push_back("1 1:1:0.123 2:2:0.123 3:3:0.123\n");
      data.push_back("0 1:1:0.5 10:10:2.3 12:17:1\n");
      data.push_back("0 1:1:0.5 5:5:2.3 6:6:1\n");
    }
    // Create libffm file
    FILE* file_ffm = OpenFileOrDie(filename.c_str(), "w");
    for (index_t i = 0; i < kNumLines; ++i) {
      uint32 write_len = fwrite(data[i].c_str(), 1,
                                data[i].size(),
                                file_ffm);
      EXPECT_NE(write_len, 0);
    }
    Close(file_ffm);
  }
}; // class SolverTest

class TSolver : public Solver {
 public:
  TSolver() {}
  ~TSolver() {}

  HyperParam GetHyperParam() { return hyper_param_; }
  std::vector<Reader*> GetReader() { return reader_; }

 private:
   DISALLOW_COPY_AND_ASSIGN(TSolver);
};

TEST_F(SolverTest, Train_No_CV_Init) {
  TSolver solver;
  solver.Initialize(argc, const_cast<char**>(argv));
  HyperParam hyper_param = solver.GetHyperParam();
  EXPECT_EQ(hyper_param.num_K, 16);
  EXPECT_EQ(hyper_param.learning_rate, 0.5);
  EXPECT_EQ(hyper_param.num_feature, 18);
  EXPECT_EQ(hyper_param.num_field, 13);
  std::vector<Reader*> reader = solver.GetReader();
  EXPECT_EQ(reader.size(), 1);
  int iteration_num = 10;
  DMatrix* matrix = NULL;
  for (int i = 0; i < iteration_num; ++i) {
    int record_num = reader[0]->Samples(matrix);
    if (record_num == 0) {
      --i;
      reader[0]->Reset();
      continue;
    }
    EXPECT_EQ(record_num, hyper_param.batch_size);
  }
}

TEST_F(SolverTest, Train_CV_Init) {
  argc = 18;
  argv[14] = "-cv";
  argv[15] = "true";
  argv[16] = "-fold";
  argv[17] = "4";
  TSolver solver;
  solver.Initialize(argc, const_cast<char**>(argv));
  HyperParam hyper_param = solver.GetHyperParam();
  EXPECT_EQ(hyper_param.num_K, 16);
  EXPECT_EQ(hyper_param.learning_rate, 0.5);
  EXPECT_EQ(hyper_param.num_feature, 18);
  EXPECT_EQ(hyper_param.num_field, 13);
  std::vector<Reader*> reader = solver.GetReader();
  EXPECT_EQ(reader.size(), 4);
}

TEST_F(SolverTest, Infer_Init) {
  // Init hyper_param and odel
  HyperParam hyper_param;
  hyper_param.score_func = "ffm";
  hyper_param.loss_func = "squared";
  hyper_param.num_feature = 10;
  hyper_param.num_K = 8;
  hyper_param.num_field = 10;
  hyper_param.num_param = 10+10*10*8;
  Model model(hyper_param);
  model.SaveModel("./xlearn_model");
  argc = 6;
  argv[1] = {"--is_infer"};
  argv[2] = {"-infer_data"};
  argv[3] = {"/tmp/solver_test.txt"};
  argv[4] = {"-file_format"};
  argv[5] = {"libffm"};
  TSolver solver;
  solver.Initialize(argc, const_cast<char**>(argv));
  HyperParam new_param = solver.GetHyperParam();
  EXPECT_EQ(new_param.num_K, 8);
  EXPECT_EQ(new_param.num_feature, 10);
  EXPECT_EQ(new_param.num_field, 10);
  EXPECT_EQ(new_param.loss_func, "squared");
  EXPECT_EQ(new_param.score_func, "ffm");
}

} // namespace xLearn
