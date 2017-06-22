/**
 * @file   tiledb_directory_type.cc
 *
 * @section LICENSE
 *
 * The MIT License
 * 
 * @copyright Copyright (c) 2016 MIT and Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * @section DESCRIPTION
 *
 * Checks the type of a given directory.
 */

#include "tiledb.h"
#include <iostream>

void print_dir_type(int type) {
  if(type == TILEDB_WORKSPACE) 
    std::cout << "Workspace\n";
  else if(type == TILEDB_GROUP) 
    std::cout << "Group\n";
  else if(type == TILEDB_ARRAY) 
    std::cout << "Array\n";
  else if(type == TILEDB_METADATA) 
    std::cout << "Metadata\n";
  else if(type == -1) 
    std::cout << "Not a TileDB object\n";
  else 
    std::cout << "Unknown directory type\n";
}

int main() {
  // Initialize context with the default configuration parameters
  TileDB_CTX* tiledb_ctx;
  tiledb_ctx_init(&tiledb_ctx, nullptr);

  // Create a workspace
  tiledb_workspace_create(tiledb_ctx, "my_workspace");

  // Create a group in the worskpace
  tiledb_group_create(tiledb_ctx, "my_workspace/dense_arrays");

  // Create two groups in the worskpace
  tiledb_group_create(tiledb_ctx, "my_workspace/sparse_arrays");

  // Check types
  print_dir_type(tiledb_dir_type(tiledb_ctx, "my_workspace"));
  print_dir_type(tiledb_dir_type(tiledb_ctx, "my_workspace/dense_arrays"));
  print_dir_type(tiledb_dir_type(tiledb_ctx, "my_workspace/array"));

  // Finalize context
  tiledb_ctx_finalize(tiledb_ctx);

  return 0;
}
