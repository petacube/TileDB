/**
 * @file   tiledb_dense_read_ordered_subarray.c
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017-2018 TileDB, Inc.
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
 * It shows how to read from a dense array, constraining the read
 * to a specific subarray. The cells are copied to the
 * input buffers sorted in row-major order within the selected subarray.
 *
 * You need to run the following to make it work:
 *
 * $ ./tiledb_dense_create_c
 * $ ./tiledb_dense_write_global_1_c
 * $ ./tiledb_dense_read_ordered_subarray_c
 */

#include <stdio.h>
#include <stdlib.h>
#include <tiledb.h>

int main() {
  // Create TileDB context
  tiledb_ctx_t* ctx;
  tiledb_ctx_create(&ctx, NULL);

  // Compute maximum buffer sizes for each attribute
  const char* attributes[] = {"a1", "a2", "a3"};
  uint64_t buffer_sizes[4];
  uint64_t subarray[] = {3, 4, 2, 4};
  tiledb_array_compute_max_read_buffer_sizes(
      ctx, "my_dense_array", subarray, attributes, 3, &buffer_sizes[0]);

  // Prepare cell buffers
  int* buffer_a1 = malloc(buffer_sizes[0]);
  uint64_t* buffer_a2 = malloc(buffer_sizes[1]);
  char* buffer_var_a2 = malloc(buffer_sizes[2]);
  float* buffer_a3 = malloc(buffer_sizes[3]);
  void* buffers[] = {buffer_a1, buffer_a2, buffer_var_a2, buffer_a3};

  // Create query
  tiledb_query_t* query;
  tiledb_query_create(ctx, &query, "my_dense_array", TILEDB_READ);
  tiledb_query_set_layout(ctx, query, TILEDB_ROW_MAJOR);
  tiledb_query_set_subarray(ctx, query, subarray);
  tiledb_query_set_buffers(ctx, query, attributes, 3, buffers, buffer_sizes);

  // Submit query
  tiledb_query_submit(ctx, query);

  // Print cell values (assumes all attributes are read)
  uint64_t result_num = buffer_sizes[0] / sizeof(int);
  printf("Result num: %llu\n\n", (unsigned long long)result_num);
  printf("%5s%10s%10s%10s\n", "a1", "a2", "a3[0]", "a3[1]");
  printf("------------------------------------\n");
  for (uint64_t i = 0; i < result_num; ++i) {
    printf("%5d", buffer_a1[i]);
    uint64_t var_size = (i != result_num - 1) ?
                            buffer_a2[i + 1] - buffer_a2[i] :
                            buffer_sizes[2] - buffer_a2[i];
    printf("%10.*s", (int)var_size, &buffer_var_a2[buffer_a2[i]]);
    printf("%10.1f%10.1f\n", buffer_a3[2 * i], buffer_a3[2 * i + 1]);
  }

  // Clean up
  tiledb_query_free(ctx, &query);
  tiledb_ctx_free(&ctx);
  free(buffer_a1);
  free(buffer_a2);
  free(buffer_var_a2);
  free(buffer_a3);

  return 0;
}
